/* Subroutines used by or related to instruction recognition.
   Copyright (C) 1987, 1988, 1991, 1992, 1993, 1994, 1995, 1996, 1997, 1998
   1999, 2000, 2001, 2002 Free Software Foundation, Inc.

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
#include "tm_p.h"
#include "insn-config.h"
#include "insn-attr.h"
#include "hard-reg-set.h"
#include "recog.h"
#include "regs.h"
#include "expr.h"
#include "function.h"
#include "flags.h"
#include "real.h"
#include "toplev.h"
#include "basic-block.h"
#include "output.h"
#include "reload.h"

#ifndef STACK_PUSH_CODE
#ifdef STACK_GROWS_DOWNWARD
#define STACK_PUSH_CODE PRE_DEC
#else
#define STACK_PUSH_CODE PRE_INC
#endif
#endif

#ifndef STACK_POP_CODE
#ifdef STACK_GROWS_DOWNWARD
#define STACK_POP_CODE POST_INC
#else
#define STACK_POP_CODE POST_DEC
#endif
#endif

static void validate_replace_rtx_1	PARAMS ((rtx *, rtx, rtx, rtx));
static rtx *find_single_use_1		PARAMS ((rtx, rtx *));
static void validate_replace_src_1 	PARAMS ((rtx *, void *));
static rtx split_insn			PARAMS ((rtx));

/* Nonzero means allow operands to be volatile.
   This should be 0 if you are generating rtl, such as if you are calling
   the functions in optabs.c and expmed.c (most of the time).
   This should be 1 if all valid insns need to be recognized,
   such as in regclass.c and final.c and reload.c.

   init_recog and init_recog_no_volatile are responsible for setting this.  */

int volatile_ok;

struct recog_data recog_data;

/* Contains a vector of operand_alternative structures for every operand.
   Set up by preprocess_constraints.  */
struct operand_alternative recog_op_alt[MAX_RECOG_OPERANDS][MAX_RECOG_ALTERNATIVES];

/* On return from `constrain_operands', indicate which alternative
   was satisfied.  */

int which_alternative;

/* Nonzero after end of reload pass.
   Set to 1 or 0 by toplev.c.
   Controls the significance of (SUBREG (MEM)).  */

int reload_completed;

/* Initialize data used by the function `recog'.
   This must be called once in the compilation of a function
   before any insn recognition may be done in the function.  */

void
init_recog_no_volatile ()
{
  volatile_ok = 0;
}

void
init_recog ()
{
  volatile_ok = 1;
}

/* Try recognizing the instruction INSN,
   and return the code number that results.
   Remember the code so that repeated calls do not
   need to spend the time for actual rerecognition.

   This function is the normal interface to instruction recognition.
   The automatically-generated function `recog' is normally called
   through this one.  (The only exception is in combine.c.)  */

int
recog_memoized_1 (insn)
     rtx insn;
{
  if (INSN_CODE (insn) < 0)
    INSN_CODE (insn) = recog (PATTERN (insn), insn, 0);
  return INSN_CODE (insn);
}

/* Check that X is an insn-body for an `asm' with operands
   and that the operands mentioned in it are legitimate.  */

int
check_asm_operands (x)
     rtx x;
{
  int noperands;
  rtx *operands;
  const char **constraints;
  int i;

  /* Post-reload, be more strict with things.  */
  if (reload_completed)
    {
      /* ??? Doh!  We've not got the wrapping insn.  Cook one up.  */
      extract_insn (make_insn_raw (x));
      constrain_operands (1);
      return which_alternative >= 0;
    }

  noperands = asm_no