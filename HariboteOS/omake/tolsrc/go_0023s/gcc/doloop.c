/* Perform doloop optimizations
   Copyright (C) 1999, 2000, 2001, 2002 Free Software Foundation, Inc.
   Contributed by Michael P. Hayes (m.hayes@elec.canterbury.ac.nz)

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 2, or (at your option) any later
version.

GCC is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with GCC; see the file COPYING.  If not, write to the Free
Software Foundation, 59 Temple Place - Suite 330, Boston, MA
02111-1307, USA.  */

#include "config.h"
#include "system.h"
#include "rtl.h"
#include "flags.h"
#include "expr.h"
#include "loop.h"
#include "hard-reg-set.h"
#include "basic-block.h"
#include "toplev.h"
#include "tm_p.h"


/* This module is used to modify loops with a determinable number of
   iterations to use special low-overhead looping instructions.

   It first validates whether the loop is well behaved and has a
   determinable number of iterations (either at compile or run-time).
   It then modifies the loop to use a low-overhead looping pattern as
   follows:

   1. A pseudo register is allocated as the loop iteration counter.

   2. The number of loop iterations is calculated and is stored
      in the loop counter.

   3. At the end of the loop, the jump insn is replaced by the
      doloop_end pattern.  The compare must remain because it might be
      used elsewhere.  If the loop-variable or condition register are
      used elsewhere, they will be eliminated by flow.

   4. An optional doloop_begin pattern is inserted at the top of the
      loop.
*/


#ifdef HAVE_doloop_end

static rtx doloop_condition_get
  PARAMS ((rtx));
static unsigned HOST_WIDE_INT doloop_iterations_max
  PARAMS ((const struct loop_info *, enum machine_mode, int));
static int doloop_valid_p
  PARAMS ((const struct loop *, rtx));
static int doloop_modify
  PARAMS ((const struct loop *, rtx, rtx, rtx, rtx, rtx));
static int doloop_modify_runtime
  PARAMS ((const struct loop *, rtx, rtx, rtx, enum machine_mode, rtx));


/* Return the loop termination condition for PATTERN or zero
   if it is not a decrement and branch jump insn.  */
static rtx
doloop_condition_get (pattern)
     rtx pattern;
{
  rtx cmp;
  rtx inc;
  rtx reg;
  rtx condition;

  /* The canonical doloop pattern we expect is:

     (parallel [(set (pc) (if_then_else (condition)
                                        (label_ref (label))
                                        (pc)))
                (set (reg) (plus (reg) (const_int -1)))
                (additional clobbers and uses)])

     Some machines (IA-64) make the decrement conditional on
     the condition as well, so we don't bother verifying the
     actual decrement.  In summary, the branch must be the
     first entry of the parallel (also required by jump.c),
     and the second entry of the parallel must be a set of
     the loop counter register.  */

  if (GET_CODE (pattern) != PARALLEL)
    return 0;

  cmp = XVECEXP (pattern, 0, 0);
  inc = XVECEXP (pattern, 0, 1);

  /* Check for (set (reg) (something)).  */
  if (GET_CODE (inc) != SET || ! REG_P (SET_DEST (inc)))
    return 0;

  /* Extract loop counter register.  */
  reg = SET_DEST (inc);

  /* Check for (set (pc) (if_then_else (condition)
                                       (label_ref (label))
                                       (pc))).  */
  if (GET_CODE (cmp) != SET
      || SET_DEST (cmp) != pc_rtx
      || GET_CODE (SET_SRC (cmp)) != IF_THEN_ELSE
      || GET_CODE (XEXP (SET_SRC (cmp), 1)) != LABEL_REF
      || XEXP (SET_SRC (cmp), 2) != pc_rtx)
    return 0;

  /* Extract loop termination condition.  */
  condition = XEXP (SET_SRC (cmp), 0);

  if ((GET_CODE (condition) != GE && GET_CODE (condition) != NE)
      || GET_CODE (XEXP (condition, 1)) != CONST_INT)
    return 0;

  if (XEXP (condition, 0) == reg)
    return condition;

  if (GET_CODE (XEXP (condition, 0)) == PLUS
      && XEXP (XEXP (condition, 0), 0) == reg)
    return condition;

  /* ??? If a machine uses a funny comparison, we could return a
     canonicalised form here.  */

  return 0;
}


/* Return an estimate of the maximum number of loop iterations for the
   loop specified by LOOP or zero if the loop is not normal.
   MODE is the mode of the iteration count and NONNEG is non-zero if
   the iteration count has been proved to be non-negative.  */
static unsigned HOST_WIDE_INT
doloop_iterations_max (loop_info, mode, nonneg)
     const struct loop_info *loop_info;
     enum machine_mode mode;
     int nonneg;
{
  unsigned HOST_WIDE_INT n_iterations_max;
  enum rtx_code code;
  rtx min_value;
  rtx max_value;
  HOST_WIDE_INT abs_inc;
  int neg_inc;

  neg_inc = 0;
  abs_inc = INTVAL (loop_info->increment);
  if (abs_inc < 0)
    {
      abs_inc = -abs_inc;
      neg_inc = 1;
    }

  if (neg_inc)
    {
      code = swap_condition (loop_info->comparison_code);
      min_value = loop_info->final_equiv_value;
      max_value = loop_info->initial_equiv_value;
    }
  else
    {
      code = loop_info->comparison_code;
      min_value = loop_info->initial_equiv_value;
      max_value = loop_info->final_equiv_value;
    }

  /* Since the loop has a VTOP, we know that the initial test will be
     true and thus the value of max_value should be greater than the
     value of min_value.  Thus the difference should always be positive
     and the code must be LT, LE, LTU, LEU, or NE.  Otherwise the loop is
     not normal, e.g., `for (i = 0; i < 10; i--)'.  */
  switch (code)
    {
    case LTU:
    case LEU:
      {
	unsigned HOST_WIDE_INT umax;
	unsigned HOST_WIDE_INT umin;

	if (GET_CODE (min_value) == CONST_INT)
	  umin = INTVAL (min_value);
	else
	  umin = 0;

	if (GET_CODE (max_value) == CONST_INT)
	  umax = INTVAL (max_value);
	else
	  umax = ((unsigned) 2 << (GET_MODE_BITSIZE (mode) - 1)) - 1;

	n_iterations_max = umax - umin;
	break;
      }

    case LT:
    case LE:
      {
	HOST_WIDE_INT smax;
	HOST_WIDE_INT smin;

	if (GET_CODE (min_value) == CONST_INT)
	  smin = INTVAL (min_value);
	else
	  smin = -((unsigned) 1 << (GET_MODE_BITSIZE (mode) - 1));

	if (GET_CODE (max_value) == CONST_INT)
	  smax = INTVAL (max_value);
	else
	  smax = ((unsigned) 1 << (GET_MODE_BITSIZE (mode) - 1)) - 1;

	n_iterations_max = smax - smin;
	break;
      }

    case NE:
      if (GET_CODE (min_value) == CONST_INT
	  && GET_CODE (max_value) == CONST_INT)
	n_iterations_max = INTVAL (max_value) - INTVAL (min_value);
      else
	/* We need to conservatively assume that we might have the maximum
	   number of iterations without any additional knowledge.  */
	n_iterations_max = ((unsigned) 2 << (GET_MODE_BITSIZE (mode) - 1)) - 1;
      break;

    default:
      return 0;
    }

  n_iterations_max /= abs_inc;

  /* If we know that the iteration count is non-negative then adjust
     n_iterations_max if it is so large that it appears negative.  */
  if (nonneg
      && n_iterations_max > ((unsigned) 1 << (GET_MODE_BITSIZE (mode) - 1)))
    n_iterations_max = ((unsigned) 1 << (GET_MODE_BITSIZE (mode) - 1)) - 1;

  return n_iterations_max;
}


/* Return non-zero if the loop specified by LOOP is suitable for
   the use of special low-overhead looping instructions.  */
static int
doloop_valid_p (loop, jump_insn)
     const struct loop *loop;
     rtx jump_insn;
{
  const struct loop_info *loop_info = LOOP_INFO (loop);

  /* The loop must have a conditional jump at the end.  */
  if (! any_condjump_p (jump_insn)
      || ! onlyjump_p (jump_insn))
    {
      if (loop_dump_stream)
  	fprintf (loop_dump_stream,
		 "Doloop: Invalid jump at loop