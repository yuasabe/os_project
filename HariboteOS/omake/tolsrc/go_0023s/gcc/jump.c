/* Optimize jump instructions, for GNU compiler.
   Copyright (C) 1987, 1988, 1989, 1991, 1992, 1993, 1994, 1995, 1996, 1997
   1998, 1999, 2000, 2001, 2002 Free Software Foundation, Inc.

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

/* This is the pathetic reminder of old fame of the jump-optimization pass
   of the compiler.  Now it contains basically set of utility function to
   operate with jumps.

   Each CODE_LABEL has a count of the times it is used
   stored in the LABEL_NUSES internal field, and each JUMP_INSN
   has one label that it refers to stored in the
   JUMP_LABEL internal field.  With this we can detect labels that
   become unused because of the deletion of all the jumps that
   formerly used them.  The JUMP_LABEL info is sometimes looked
   at by later passes.

   The subroutines delete_insn, redirect_jump, and invert_jump are used
   from other passes as well.  */

#include "config.h"
#include "system.h"
#include "rtl.h"
#include "tm_p.h"
#include "flags.h"
#include "hard-reg-set.h"
#include "regs.h"
#include "insn-config.h"
#include "insn-attr.h"
#include "recog.h"
#include "function.h"
#include "expr.h"
#include "real.h"
#include "except.h"
#include "toplev.h"
#include "reload.h"
#include "predict.h"

/* Optimize jump y; x: ... y: jumpif... x?
   Don't know if it is worth bothering with.  */
/* Optimize two cases of conditional jump to conditional jump?
   This can never delete any instruction or make anything dead,
   or even change what is live at any point.
   So perhaps let combiner do it.  */

static int init_label_info		PARAMS ((rtx));
static void mark_all_labels		PARAMS ((rtx));
static int duplicate_loop_exit_test	PARAMS ((rtx));
static void delete_computation		PARAMS ((rtx));
static void redirect_exp_1		PARAMS ((rtx *, rtx, rtx, rtx));
static int redirect_exp			PARAMS ((rtx, rtx, rtx));
static void invert_exp_1		PARAMS ((rtx));
static int invert_exp			PARAMS ((rtx));
static int returnjump_p_1	        PARAMS ((rtx *, void *));
static void delete_prior_computation    PARAMS ((rtx, rtx));

/* Alternate entry into the jump optimizer.  This entry point only rebuilds
   the JUMP_LABEL field in jumping insns and REG_LABEL notes in non-jumping
   instructions.  */
void
rebuild_jump_labels (f)
     rtx f;
{
  rtx insn;
  int max_uid = 0;

  max_uid = init_label_info (f) + 1;

  mark_all_labels (f);

  /* Keep track of labels used from static data; we don't track them
     closely enough to delete them here, so make sure their reference
     count doesn't drop to zero.  */

  for (insn = forced_labels; insn; insn = XEXP (insn, 1))
    if (GET_CODE (XEXP (insn, 0)) == CODE_LABEL)
      LABEL_NUSES (XEXP (insn, 0))++;
}

/* Some old code expects exactly one BARRIER as the NEXT_INSN of a
   non-fallthru insn.  This is not generally true, as multiple barriers
   may have crept in, or the BARRIER may be separated from the last
   real insn by one or more NOTEs.

   This simple pass moves barriers and removes duplicates so that the
   old code is happy.
 */
void
cleanup_barriers ()
{
  rtx insn, next, prev;
  for (insn = get_insns (); insn; insn = next)
    {
      next = NEXT_INSN (insn);
      if (GET_CODE (insn) == BARRIER)
	{
	  prev = prev_nonnote_insn (insn);
	  if (GET_CODE (prev) == BARRIER)
	    delete_barrier (insn);
	  else if (prev != PREV_INSN (insn))
	    reorder_insns (insn, insn, prev);
	}
    }
}

void
copy_loop_headers (f)
     rtx f;
{
  rtx insn, next;
  /* Now iterate optimizing jumps until nothing changes over one pass.  */
  for (insn = f; insn; insn = next)
    {
      rtx temp, temp1;

      next = NEXT_INSN (insn);

      /* See if this is a NOTE_INSN_LOOP_BEG followed by an unconditional
	 jump.  Try to optimize by duplicating the loop exit test if so.
	 This is only safe immediately after regscan, because it uses
	 the values of regno_first_uid and regno_last_uid.  */
      if (GET_CODE (insn) == NOTE
	  && NOTE_LINE_NUMBER (insn) == NOTE_INSN_LOOP_BEG
	  && (temp1 = next_nonnote_insn (insn)) != 0
	  && any_uncondjump_p (temp1) && onlyjump_p (temp1))
	{
	  temp = PREV_INSN (insn);
	  if (duplicate_loop_exit_test (insn))
	    {
	      next = NEXT_INSN (temp);
	    }
	}
    }
}

void
purge_line_number_notes (f)
     rtx f;
{
  rtx last_note = 0;
  rtx insn;
  /* Delete extraneous line number notes.
     Note that two consecutive notes for different lines are not really
     extraneous.  There should be some indication where that line belonged,
     even if it became empty.  */

  for (insn = f; insn; insn = NEXT_INSN (insn))
    if (GET_CODE (insn) == NOTE)
      {
	if (NOTE_LINE_NUMBER (insn) == NOTE_INSN_FUNCTION_BEG)
	  /* Any previous line note was for the prologue; gdb wants a new
	     note after the prologue even if it is for the same line.  */
	  last_note = NULL_RTX;
	else if (NOTE_LINE_NUMBER (insn) >= 0)
	  {
	    /* Delete this note if it is identical to previous note.  */
	    if (last_note
		&& NOTE_SOURCE_FILE (insn) == NOTE_SOURCE_FILE (last_note)
		&& NOTE_LINE_NUMBER (insn) == NOTE_LINE_NUMBER (last_note))
	      {
		delete_related_insns (insn);
		continue;
	      }

	    last_note = insn;
	  }
      }
}

/* Initialize LABEL_NUSES and JUMP_LABEL fields.  Delete any REG_LABEL
   notes whose labels don't occur in the insn any more.  Returns the
   largest INSN_UID found.  */
static int
init_label_info (f)
     rtx f;
{
  int largest_uid = 0;
  rtx insn;

  for (insn = f; insn; insn = NEXT_INSN (insn))
    {
      if (GET_CODE (insn) == CODE_LABEL)
	LABEL_NUSES (insn) = (LABEL_PRESERVE_P (insn) != 0);
      else if (GET_CODE (insn) == JUMP_INSN)
	JUMP_LABEL (insn) = 0;
      else if (GET_CODE (insn) == INSN || GET_CODE (insn) == CALL_INSN)
	{
	  rtx note, next;

	  for (note = REG_NOTES (insn); note; note = next)
	    {
	      next = XEXP (note, 1);
	      if (REG_NOTE_KIND (note) == REG_LABEL
		  && ! reg_mentioned_p (XEXP (note, 0), PATTERN (insn)))
		remove_note (insn, note);
	    }
	}
      if (INSN_UID (insn) > largest_uid)
	largest_uid = INSN_UID (insn);
    }

  return largest_uid;
}

/* Mark the label each jump jumps to.
   Combine consecutive labels, and count uses of labels.  */

static void
mark_all_labels (f)
     rtx f;
{
  rtx insn;

  for (insn = f; insn; insn = NEXT_INSN (insn))
    if (INSN_P (insn))
      {
	if (GET_CODE (insn) == CALL_INSN
	    && GET_CODE (PATTERN (insn)) == CALL_PLACEHOLDER)
	  {
	    mark_all_labels (XEXP (PATTERN (insn), 0));
	    mark_all_labels (XEXP (PATTERN (insn), 1));
	    mark_all_labels (XEXP (PATTERN (insn), 2));

	    /* Canonicalize the tail recursion label attached to the
	       CALL_PLACEHOLDER insn.  */
	    if (XEXP (PATTERN (insn), 3))
	      {
		rtx label_ref = gen_rtx_LABEL_REF (VOIDmode,
						   XEXP (PATTERN (insn), 3));
		mark_jump_label (label_ref, insn, 0);
		XEXP (PATTERN (insn), 3) = XEXP (label_ref, 0);
	      }

	    continue;
	  }

	mark_jump_label (PATTERN (insn), insn, 0);
	if (! INSN_DELETED_P (insn) && GET_CODE (insn) == JUMP_INSN)
	  {
	    /* When we know the LABEL_REF contained in a REG used in
	       an indirect jump, we'll have a REG_LABEL note so that
	       flow can tell where it's going.  */
	    if (JUMP_LABEL (insn) == 0)
	      {
		rtx label_note = find_reg_note (insn, REG_LABEL, NULL_RTX);
		if (label_note)
		  {
		    /* But a LABEL_REF around the REG_LABEL note, so
		       that we can canonicalize it.  */
		    rtx label_ref = gen_rtx_LABEL_REF (VOIDmode,
						       XEXP (label_note, 0));

		    mark_jump_label (label_ref, insn, 0);
		    XEXP (label_note, 0) = XEXP (label_ref, 0);
		    JUMP_LABEL (insn) = XEXP (label_note, 0);
		  }
	      }
	  }
      }
}

/* LOOP_START is a NOTE_INSN_LOOP_BEG note that is followed by an unconditional
   jump.  Assume that this unconditional jump is to the exit test code.  If
   the code is sufficiently simple, make a copy of it before INSN,
   followed by a jump to the exit of the loop.  Then delete the unconditional
   jump after INSN.

   Return 1 if we made the change, else 0.

   This is only safe immediately after a regscan pass because it uses the
   values of regno_first_uid and regno_last_uid.  */

static int
duplicate_loop_exit_test (loop_start)
     rtx loop_start;
{
  rtx insn, set, reg, p, link;
  rtx copy = 0, first_copy = 0;
  int num_insns = 0;
  rtx exitcode = NEXT_INSN (JUMP_LABEL (next_nonnote_insn (loop_start)));
  rtx lastexit;
  int max_reg = max_reg_num ();
  rtx *reg_map = 0;
  rtx loop_pre_header_label;

  /* Scan the exit code.  We do not perform this optimization if any insn:

         is a CALL_INSN
	 is a CODE_LABEL
	 has a REG_RETVAL or REG_LIBCALL note (hard to adjust)
	 is a NOTE_INSN_LOOP_BEG because this means we have a nested loop
	 is a NOTE_INSN_BLOCK_{BEG,END} because duplicating these notes
	      is not valid.

     We also do not do this if we find an insn with ASM_OPERANDS.  While
     this restriction should not be necessary, copying an insn with
     ASM_OPERANDS can confuse asm_noperands in some cases.

     Also, don't do this if the exit code is more than 20 insns.  */

  for (insn = exitcode;
       insn
       && ! (GET_CODE (insn) == NOTE
	     && NOTE_LINE_NUMBER (insn) == NOTE_INSN_LOOP_END);
       insn = NEXT_INSN (insn))
    {
      switch (GET_CODE (insn))
	{
	case CODE_LABEL:
	case CALL_INSN:
	  return 0;
	case NOTE:
	  /* We could be in front of the wrong NOTE_INSN_LOOP_END if there is
	     a jump immediately after the loop start that branches outside
	     the loop but within an outer loop, near the exit test.
	     If we copied this exit test and created a phony
	     NOTE_INSN_LOOP_VTOP, this could make instructions immediately
	     before the exit test look like these could be safely moved
	     out of the loop even if they actually may be never executed.
	     This can be avoided by checking here for NOTE_INSN_LOOP_CONT.  */

	  if (NOTE_LINE_NUMBER (insn) == NOTE_INSN_LOOP_BEG
	      || NOTE_LINE_NUMBER (insn) == NOTE_INSN_LOOP_CONT)
	    return 0;

	  if (optimize < 2
	      && (NOTE_LINE_NUMBER (insn) == NOTE_INSN_BLOCK_BEG
		  || NOTE_LINE_NUMBER (insn) == NOTE_INSN_BLOCK_END))
	    /* If we were to duplicate this code, we would not move
	       the BLOCK notes, and so debugging the moved code would
	       be difficult.  Thus, we only move the code with -O2 or
	       higher.  */
	    return 0;

	  break;
	case JUMP_INSN:
	case INSN:
	  /* The code below would grossly mishandle REG_WAS_0 notes,
	     so get rid of them here.  */
	  while ((p = find_reg_note (insn, REG_WAS_0, NULL_RTX)) != 0)
	    remove_note (insn, p);
	  if (++num_insns > 20
	      || find_reg_note (insn, REG_RETVAL, NULL_RTX)
	      || find_reg_note (insn, REG_LIBCALL, NULL_RTX))
	    return 0;
	  break;
	default:
	  break;
	}
    }

  /* Unless INSN is zero, we can do the optimization.  */
  if (insn == 0)
    return 0;

  lastexit = insn;

  /* See if any insn sets a register only used in the loop exit code and
     not a user variable.  If so, replace it with a new register.  */
  for (insn = exitcode; insn != lastexit; insn = NEXT_INSN (insn))
    if (GET_CODE (insn) == INSN
	&& (set = single_set (insn)) != 0
	&& ((reg = SET_DEST (set), GET_CODE (reg) == REG)
	    || (GET_CODE (reg) == SUBREG
		&& (reg = SUBREG_REG (reg), GET_CO