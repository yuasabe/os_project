/* Try to unroll loops, and split induction variables.
   Copyright (C) 1992, 1993, 1994, 1995, 1997, 1998, 1999, 2000, 2001, 2002
   Free Software Foundation, Inc.
   Contributed by James E. Wilson, Cygnus Support/UC Berkeley.

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

/* Try to unroll a loop, and split induction variables.

   Loops for which the number of iterations can be calculated exactly are
   handled specially.  If the number of iterations times the insn_count is
   less than MAX_UNROLLED_INSNS, then the loop is unrolled completely.
   Otherwise, we try to unroll the loop a number of times modulo the number
   of iterations, so that only one exit test will be needed.  It is unrolled
   a number of times approximately equal to MAX_UNROLLED_INSNS divided by
   the insn count.

   Otherwise, if the number of iterations can be calculated exactly at
   run time, and the loop is always entered at the top, then we try to
   precondition the loop.  That is, at run time, calculate how many times
   the loop will execute, and then execute the loop body a few times so
   that the remaining iterations will be some multiple of 4 (or 2 if the
   loop is large).  Then fall through to a loop unrolled 4 (or 2) times,
   with only one exit test needed at the end of the loop.

   Otherwise, if the number of iterations can not be calculated exactly,
   not even at run time, then we still unroll the loop a number of times
   approximately equal to MAX_UNROLLED_INSNS divided by the insn count,
   but there must be an exit test after each copy of the loop body.

   For each induction variable, which is dead outside the loop (replaceable)
   or for which we can easily calculate the final value, if we can easily
   calculate its value at each place where it is set as a function of the
   current loop unroll count and the variable's value at loop entry, then
   the induction variable is split into `N' different variables, one for
   each copy of the loop body.  One variable is live across the backward
   branch, and the others are all calculated as a function of this variable.
   This helps eliminate data dependencies, and leads to further opportunities
   for cse.  */

/* Possible improvements follow:  */

/* ??? Add an extra pass somewhere to determine whether unrolling will
   give any benefit.  E.g. after generating all unrolled insns, compute the
   cost of all insns and compare against cost of insns in rolled loop.

   - On traditional architectures, unrolling a non-constant bound loop
     is a win if there is a giv whose only use is in memory addresses, the
     memory addresses can be split, and hence giv increments can be
     eliminated.
   - It is also a win if the loop is executed many times, and preconditioning
     can be performed for the loop.
   Add code to check for these and similar cases.  */

/* ??? Improve control of which loops get unrolled.  Could use profiling
   info to only unroll the most commonly executed loops.  Perhaps have
   a user specifyable option to control the amount of code expansion,
   or the percent of loops to consider for unrolling.  Etc.  */

/* ??? Look at the register copies inside the loop to see if they form a
   simple permutation.  If so, iterate the permutation until it gets back to
   the start state.  This is how many times we should unroll the loop, for
   best results, because then all register copies can be eliminated.
   For example, the lisp nreverse function should be unrolled 3 times
   while (this)
     {
       next = this->cdr;
       this->cdr = prev;
       prev = this;
       this = next;
     }

   ??? The number of times to unroll the loop may also be based on data
   references in the loop.  For example, if we have a loop that references
   x[i-1], x[i], and x[i+1], we should unroll it a multiple of 3 times.  */

/* ??? Add some simple linear equation solving capability so that we can
   determine the number of loop iterations for more complex loops.
   For example, consider this loop from gdb
   #define SWAP_TARGET_AND_HOST(buffer,len)
     {
       char tmp;
       char *p = (char *) buffer;
       char *q = ((char *) buffer) + len - 1;
       int iterations = (len + 1) >> 1;
       int i;
       for (p; p < q; p++, q--;)
	 {
	   tmp = *q;
	   *q = *p;
	   *p = tmp;
	 }
     }
   Note that:
     start value = p = &buffer + current_iteration
     end value   = q = &buffer + len - 1 - current_iteration
   Given the loop exit test of "p < q", then there must be "q - p" iterations,
   set equal to zero and solve for number of iterations:
     q - p = len - 1 - 2*current_iteration = 0
     current_iteration = (len - 1) / 2
   Hence, there are (len - 1) / 2 (rounded up to the nearest integer)
   iterations of this loop.  */

/* ??? Currently, no labels are marked as loop invariant when doing loop
   unrolling.  This is because an insn inside the loop, that loads the address
   of a label inside the loop into a register, could be moved outside the loop
   by the invariant code motion pass if labels were invariant.  If the loop
   is subsequently unrolled, the code will be wrong because each unrolled
   body of the loop will use the same address, whereas each actually needs a
   different address.  A case where this happens is when a loop containing
   a switch statement is unrolled.

   It would be better to let labels be considered invariant.  When we
   unroll loops here, check to see if any insns using a label local to the
   loop were moved before the loop.  If so, then correct the problem, by
   moving the insn back into the loop, or perhaps replicate the insn before
   the loop, one copy for each time the loop is unrolled.  */

#include "config.h"
#include "system.h"
#include "rtl.h"
#include "tm_p.h"
#include "insn-config.h"
#include "integrate.h"
#include "regs.h"
#include "recog.h"
#include "flags.h"
#include "function.h"
#include "expr.h"
#include "loop.h"
#include "toplev.h"
#include "hard-reg-set.h"
#include "basic-block.h"
#include "predict.h"

/* The prime factors looked for when trying to unroll a loop by some
   number which is modulo the total number of iterations.  Just checking
   for these 4 prime factors will find at least one factor for 75% of
   all numbers theoretically.  Practically speaking, this will succeed
   almost all of the time since loops are generally a multiple of 2
   and/or 5.  */

#define NUM_FACTORS 4

static struct _factor { const int factor; int count; }
factors[NUM_FACTORS] = { {2, 0}, {3, 0}, {5, 0}, {7, 0}};

/* Describes the different types of loop unrolling performed.  */

enum unroll_types
{
  UNROLL_COMPLETELY,
  UNROLL_MODULO,
  UNROLL_NAIVE
};

/* This controls which loops are unrolled, and by how much we unroll
   them.  */

#ifndef MAX_UNROLLED_INSNS
#define MAX_UNROLLED_INSNS 100
#endif

/* Indexed by register number, if non-zero, then it contains a pointer
   to a struct induction for a DEST_REG giv which has been combined with
   one of more address givs.  This is needed because whenever such a DEST_REG
   giv is modified, we must modify the value of all split address givs
   that were combined with this DEST_REG giv.  */

static struct induction **addr_combined_regs;

/* Indexed by register number, if this is a splittable induction variable,
   then this will hold the current value of the register, which depends on the
   iteration number.  */

static rtx *splittable_regs;

/* Indexed by register number, if this is a splittable induction variable,
   then this will hold the number of instructions in the loop that modify
   the induction variable.  Used to ensure that only the last insn modifying
   a split iv will update the original iv of the dest.  */

static int *splittable_regs_updates;

/* Forward declarations.  */

static void init_reg_map PARAMS ((struct inline_remap *, int));
static rtx calculate_giv_inc PARAMS ((rtx, rtx, unsigned int));
static rtx initial_reg_note_copy PARAMS ((rtx, struct inline_remap *));
static void final_reg_note_copy PARAMS ((rtx *, struct inline_remap *));
static void copy_loop_body PARAMS ((struct loop *, rtx, rtx,
				    struct inline_remap *, rtx, int,
				    enum unroll_types, rtx, rtx, rtx, rtx));
static int find_splittable_regs PARAMS ((const struct loop *,
					 enum unroll_types, int));
static int find_splittable_givs PARAMS ((const struct loop *,
					 struct iv_class *, enum unroll_types,
					 rtx, int));
static int reg_dead_after_loop PARAMS ((const struct loop *, rtx));
static rtx fold_rtx_mult_add PARAMS ((rtx, rtx, rtx, enum machine_mode));
static int verify_addresses PARAMS ((struct induction *, rtx, int));
static rtx remap_split_bivs PARAMS ((struct loop *, rtx));
static rtx find_common_reg_term PARAMS ((rtx, rtx));
static rtx subtract_reg_term PARAMS ((rtx, rtx));
static rtx loop_find_equiv_value PARAMS ((const struct loop *, rtx));
static rtx ujump_to_loop_cont PARAMS ((rtx, rtx));

/* Try to unroll one loop and split induction variables in the loop.

   The loop is described by the arguments LOOP and INSN_COUNT.
   STRENGTH_REDUCTION_P indicates whether information generated in the
   strength reduction pass is available.

   This function is intended to be called from within `strength_reduce'
   in loop.c.  */

void
unroll_loop (loop, insn_count, strength_reduce_p)
     struct loop *loop;
     int insn_count;
     int strength_reduce_p;
{
  struct loop_info *loop_info = LOOP_INFO (loop);
  struct loop_ivs *ivs = LOOP_IVS (loop);
  int i, j;
  unsigned int r;
  unsigned HOST_WIDE_INT temp;
  int unroll_number = 1;
  rtx copy_start, copy_end;
  rtx insn, sequence, pattern, tem;
  int max_labelno, max_insnno;
  rtx insert_before;
  struct inline_remap *map;
  char *local_label = NULL;
  char *local_regno;
  unsigned int max_local_regnum;
  unsigned int maxregnum;
  rtx exit_label = 0;
  rtx start_label;
  struct iv_class *bl;
  int splitting_not_safe = 0;
  enum unroll_types unroll_type = UNROLL_NAIVE;
  int loop_preconditioned = 0;
  rtx safety_label;
  /* This points to the last real insn in the loop, which should be either
     a JUMP_INSN (for conditional jumps) or a BARRIER (for unconditional
     jumps).  */
  rtx last_loop_insn;
  rtx loop_start = loop->start;
  rtx loop_end = loop->end;

  /* Don't bother unrolling huge loops.  Since the minimum factor is
     two, loops greater than one half of MAX_UNROLLED_INSNS will never
     be unrolled.  */
  if (insn_count > MAX_UNROLLED_INSNS / 2)
    {
      if (loop_dump_stream)
	fprintf (loop_dump_stream, "Unrolling failure: Loop too big.¥n");
      return;
    }

  /* When emitting debugger info, we can't unroll loops with unequal numbers
     of block_beg and block_end notes, because that would unbalance the block
     structure of the function.  This can happen as a result of the
     "if (foo) bar; else break;" optimization in jump.c.  */
  /* ??? Gcc has a general policy that -g is never supposed to change the code
     that the compiler emits, so we must disable this optimization always,
     even if debug info is not being output.  This is rare, so this should
     not be a significant performance problem.  */

  if (1 /* write_symbols != NO_DEBUG */)
    {
      int block_begins = 0;
      int block_ends = 0;

      for (insn = loop_start; insn != loop_end; insn = NEXT_INSN (insn))
	{
	  if (GET_CODE (insn) == NOTE)
	    {
	      if (NOTE_LINE_NUMBER (insn) == NOTE_INSN_BLOCK_BEG)
		block_begins++;
	      else if (NOTE_LINE_NUMBER (insn) == NOTE_INSN_BLOCK_END)
		block_ends++;
	      if (NOTE_LINE_NUMBER (insn) == NOTE_INSN_EH_REGION_BEG
		  || NOTE_LINE_NUMBER (insn) == NOTE_INSN_EH_REGION_END)
		{
		  /* Note, would be nice to add code to unroll EH
		     regions, but until that time, we punt (don't
		     unroll).  For the proper way of doing it, see
		     expand_inline_function.  */

		  if (loop_dump_stream)
		    fprintf (loop_dump_stream,
			     "Unrolling failure: cannot unroll EH regions.¥n");
		  return;
		}
	    }
	}

      if (block_begins != block_ends)
	{
	  if (loop_dump_stream)
	    fprintf (loop_dump_stream,
		     "Unrolling failure: Unbalanced block notes.¥n");
	  return;
	}
    }

  /* Determine type of unroll to perform.  Depends on the number of iterations
     and the size of the loop.  */

  /* If there is no strength reduce info, then set
     loop_info->n_iterations to zero.  This can happen if
     strength_reduce can't find any bivs in the loop.  A value of zero
     indicates that the number of iterations could not be calculated.  */

  if (! strength_reduce_p)
    loop_info->n_iterations = 0;

  if (loop_dump_stream && loop_info->n_iterations > 0)
    {
      fputs ("Loop unrolling: ", loop_dump_stream);
      fprintf (loop_dump_stream, HOST_WIDE_INT_PRINT_DEC,
	       loop_info->n_iterations);
      fputs (" iterations.¥n", loop_dump_stream);
    }

  /* Find and save a pointer to the last nonnote insn in the loop.  */

  last_loop_insn = prev_nonnote_insn (loop_end);

  /* Calculate how many times to unroll the loop.  Indicate whether or
     not the loop is being completely unrolled.  */

  if (loop_info->n_iterations == 1)
    {
      /* Handle the case where the loop begins with an unconditional
	 jump to the loop condition.  Make sure to delete the jump
	 insn, otherwise the loop body will never execute.  */

      rtx ujump = ujump_to_loop_cont (loop->start, loop->cont);
      if (ujump)
	delete_related_insns (ujump);

      /* If number of iterations is exactly 1, then eliminate the compare and
	 branch at the end of the loop since they will never be taken.
	 Then return, since no other action is needed here.  */

      /* If the last instruction is not a BARRIER or a JUMP_INSN, then
	 don't do anything.  */

      if (GET_CODE (last_loop_insn) == BARRIER)
	{
	  /* Delete the jump insn.  This will delete the barrier also.  */
	  delete_related_insns (PREV_INSN (last_loop_insn));
	}
      else if (GET_CODE (last_loop_insn) == JUMP_INSN)
	{
#ifdef HAVE_cc0
	  rtx prev = PREV_INSN (last_loop_insn);
#endif
	  delete_related_insns (last_loop_insn);
#ifdef HAVE_cc0
	  /* The immediately preceding insn may be a compare which must be
	     deleted.  */
	  if (only_sets_cc0_p (prev))
	    delete_related_insns (prev);
#endif
	}

      /* Remove the loop notes since this is no longer a loop.  */
      if (loop->vtop)
	delete_related_insns (loop->vtop);
      if (loop->cont)
	delete_related_insns (loop->cont);
      if (loop_start)
	delete_related_insns (loop_start);
      if (loop_end)
	delete_related_insns (loop_end);

      return;
    }
  else if (loop_info->n_iterations > 0
	   /* Avoid overflow in the next expression.  */
	   && loop_info->n_iterations < MAX_UNROLLED_INSNS
	   && loop_info->n_iterations * insn_count < MAX_UNROLLED_INSNS)
    {
      unroll_number = loop_info->n_iterations;
      unroll_type = UNROLL_COMPLETELY;
    }
  else if (loop_info->n_iterations > 0)
    {
      /* Try to factor the number of iterations.  Don't bother with the
	 general case, only using 2, 3, 5, and 7 will get 75% of all
	 numbers theoretically, and almost all in practice.  */

      for (i = 0; i < NUM_FACTORS; i++)
	factors[i].count = 0;

      temp = loop_info->n_iterations;
      for (i = NUM_FACTORS - 1; i >= 0; i--)
	while (temp % factors[i].factor == 0)
	  {
	    factors[i].count++;