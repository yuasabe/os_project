/* Perform instruction reorganizations for delay slot filling.
   Copyright (C) 1992, 1993, 1994, 1995, 1996, 1997, 1998,
   1999, 2000, 2001, 2002 Free Software Foundation, Inc.
   Contributed by Richard Kenner (kenner@vlsi1.ultra.nyu.edu).
   Hacked by Michael Tiemann (tiemann@cygnus.com).

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

/* Instruction reorganization pass.

   This pass runs after register allocation and final jump
   optimization.  It should be the last pass to run before peephole.
   It serves primarily to fill delay slots of insns, typically branch
   and call insns.  Other insns typically involve more complicated
   interactions of data dependencies and resource constraints, and
   are better handled by scheduling before register allocation (by the
   function `schedule_insns').

   The Branch Penalty is the number of extra cycles that are needed to
   execute a branch insn.  On an ideal machine, branches take a single
   cycle, and the Branch Penalty is 0.  Several RISC machines approach
   branch delays differently:

   The MIPS and AMD 29000 have a single branch delay slot.  Most insns
   (except other branches) can be used to fill this slot.  When the
   slot is filled, two insns execute in two cycles, reducing the
   branch penalty to zero.

   The Motorola 88000 conditionally exposes its branch delay slot,
   so code is shorter when it is turned off, but will run faster
   when useful insns are scheduled there.

   The IBM ROMP has two forms of branch and call insns, both with and
   without a delay slot.  Much like the 88k, insns not using the delay
   slot can be shorted (2 bytes vs. 4 bytes), but will run slowed.

   The SPARC always has a branch delay slot, but its effects can be
   annulled when the branch is not taken.  This means that failing to
   find other sources of insns, we can hoist an insn from the branch
   target that would only be safe to execute knowing that the branch
   is taken.

   The HP-PA always has a branch delay slot.  For unconditional branches
   its effects can be annulled when the branch is taken.  The effects
   of the delay slot in a conditional branch can be nullified for forward
   taken branches, or for untaken backward branches.  This means
   we can hoist insns from the fall-through path for forward branches or
   steal insns from the target of backward branches.

   The TMS320C3x and C4x have three branch delay slots.  When the three
   slots are filled, the branch penalty is zero.  Most insns can fill the
   delay slots except jump insns.

   Three techniques for filling delay slots have been implemented so far:

   (1) `fill_simple_delay_slots' is the simplest, most efficient way
   to fill delay slots.  This pass first looks for insns which come
   from before the branch and which are safe to execute after the
   branch.  Then it searches after the insn requiring delay slots or,
   in the case of a branch, for insns that are after the point at
   which the branch merges into the fallthrough code, if such a point
   exists.  When such insns are found, the branch penalty decreases
   and no code expansion takes place.

   (2) `fill_eager_delay_slots' is more complicated: it is used for
   scheduling conditional jumps, or for scheduling jumps which cannot
   be filled using (1).  A machine need not have annulled jumps to use
   this strategy, but it helps (by keeping more options open).
   `fill_eager_delay_slots' tries to guess the direction the branch
   will go; if it guesses right 100% of the time, it can reduce the
   branch penalty as much as `fill_simple_delay_slots' does.  If it
   guesses wrong 100% of the time, it might as well schedule nops (or
   on the m88k, unexpose the branch slot).  When
   `fill_eager_delay_slots' takes insns from the fall-through path of
   the jump, usually there is no code expansion; when it takes insns
   from the branch target, there is code expansion if it is not the
   only way to reach that target.

   (3) `relax_delay_slots' uses a set of rules to simplify code that
   has been reorganized by (1) and (2).  It finds cases where
   conditional test can be eliminated, jumps can be threaded, extra
   insns can be eliminated, etc.  It is the job of (1) and (2) to do a
   good job of scheduling locally; `relax_delay_slots' takes care of
   making the various individual schedules work well together.  It is
   especially tuned to handle the control flow interactions of branch
   insns.  It does nothing for insns with delay slots that do not
   branch.

   On machines that use CC0, we are very conservative.  We will not make
   a copy of an insn involving CC0 since we want to maintain a 1-1
   correspondence between the insn that sets and uses CC0.  The insns are
   allowed to be separated by placing an insn that sets CC0 (but not an insn
   that uses CC0; we could do this, but it doesn't seem worthwhile) in a
   delay slot.  In that case, we point each insn at the other with REG_CC_USER
   and REG_CC_SETTER notes.  Note that these restrictions affect very few
   machines because most RISC machines with delay slots will not use CC0
   (the RT is the only known exception at this point).

   Not yet implemented:

   The Acorn Risc Machine can conditionally execute most insns, so
   it is profitable to move single insns into a position to execute
   based on the condition code of the previous insn.

   The HP-PA can conditionally nullify insns, providing a similar
   effect to the ARM, differing mostly in which insn is "in charge".  */

/* !kawai! */
#include "config.h"
#include "system.h"
#include "toplev.h"
#include "rtl.h"
#include "tm_p.h"
#include "expr.h"
#include "function.h"
#include "insn-config.h"
#include "conditions.h"
#include "hard-reg-set.h"
#include "basic-block.h"
#include "regs.h"
#include "recog.h"
#include "flags.h"
#include "output.h"
#include "../include/obstack.h"
#include "insn-attr.h"
#include "resource.h"
#include "except.h"
#include "params.h"
/* end of !kawai! */

#ifdef DELAY_SLOTS

#define obstack_chunk_alloc xmalloc
#define obstack_chunk_free free

#ifndef ANNUL_IFTRUE_SLOTS
#define eligible_for_annul_true(INSN, SLOTS, TRIAL, FLAGS) 0
#endif
#ifndef ANNUL_IFFALSE_SLOTS
#define eligible_for_annul_false(INSN, SLOTS, TRIAL, FLAGS) 0
#endif

/* Insns which have delay slots that have not yet been filled.  */

static struct obstack unfilled_slots_obstack;
static rtx *unfilled_firstobj;

/* Define macros to refer to the first and last slot containing unfilled
   insns.  These are used because the list may move and its address
   should be recomputed at each use.  */

#define unfilled_slots_base	¥
  ((rtx *) obstack_base (&unfilled_slots_obstack))

#define unfilled_slots_next	¥
  ((rtx *) obstack_next_free (&unfilled_slots_obstack))

/* Points to the label before the end of the function.  */
static rtx end_of_function_label;

/* Mapping between INSN_UID's and position in the code since INSN_UID's do
   not always monotonically increase.  */
static int *uid_to_ruid;

/* Highest valid index in `uid_to_ruid'.  */
static int max_uid;

static int stop_search_p		PARAMS ((rtx, int));
static int resource_conflicts_p		PARAMS ((struct resources *,
					       struct resources *));
static int insn_references_resource_p	PARAMS ((rtx, struct resources *, int));
static int insn_sets_resource_p		PARAMS ((rtx, struct resources *, int));
static rtx find_end_label		PARAMS ((void));
static rtx emit_delay_sequence		PARAMS ((rtx, rtx, int));
static rtx add_to_delay_list		PARAMS ((rtx, rtx));
static rtx delete_from_delay_slot	PARAMS ((rtx));
static void delete_scheduled_jump	PARAMS ((rtx));
static void note_delay_statistics	PARAMS ((int, int));
#if defined(ANNUL_IFFALSE_SLOTS) || defined(ANNUL_IFTRUE_SLOTS)
static rtx optimize_skip		PARAMS ((rtx));
#endif
static int get_jump_flags		PARAMS ((rtx, rtx));
static int rare_destination		PARAMS ((rtx));
static int mostly_true_jump		PARAMS ((rtx, rtx));
static rtx get_branch_condition		PARAMS ((rtx, rtx));
static int condition_dominates_p	PARAMS ((rtx, rtx));
static int redirect_with_delay_slots_safe_p PARAMS ((rtx, rtx, rtx));
static int redirect_with_delay_list_safe_p PARAMS ((rtx, rtx, rtx));
static int check_annul_list_true_false	PARAMS ((int, rtx));
static rtx steal_delay_list_from_target PARAMS ((rtx, rtx, rtx, rtx,
					       struct resources *,
					       struct resources *,
					       struct resources *,
					       int, int *, int *, rtx *));
static rtx steal_delay_list_from_fallthrough PARAMS ((rtx, rtx, rtx, rtx,
						    struct resources *,
						    struct resources *,
						    struct resources *,
						    int, int *, int *));
static void try_merge_delay_insns	PARAMS ((rtx, rtx));
static rtx redundant_insn		PARAMS ((rtx, rtx, rtx));
static int own_thread_p			PARAMS ((rtx, rtx, int));
static void update_block		PARAMS ((rtx, rtx));
static int reorg_redirect_jump		PARAMS ((rtx, rtx));
static void update_reg_dead_notes	PARAMS ((rtx, rtx));
static void fix_reg_dead_note		PARAMS ((rtx, rtx));
static void update_reg_unused_notes	PARAMS ((rtx, rtx));
static void fill_simple_delay_slots	PARAMS ((int));
static rtx fill_slots_from_thread	PARAMS ((rtx, rtx, rtx, rtx, int, int,
					       int, int, int *, rtx));
static void fill_eager_delay_slots	PARAMS ((void));
static void relax_delay_slots		PARAMS ((rtx));
#ifdef HAVE_return
static void make_return_insns		PARAMS ((rtx));
#endif

/* Return TRUE if this insn should stop the search for insn to fill delay
   slots.  LABELS_P indicates that labels should terminate the search.
   In all cases, jumps terminate the search.  */

static int
stop_search_p (insn, labels_p)
     rtx insn;
     int labels_p;
{
  if (insn == 0)
    return 1;

  switch (GET_CODE (insn))
    {
    case NOTE:
    case CALL_INSN:
      return 0;

    case CODE_LABEL:
      return labels_p;

    case JUMP_INSN:
    case BARRIER:
      return 1;

    case INSN:
      /* OK unless it contains a delay slot or is an `asm' insn of some type.
	 We don't know anything about these.  */
      return (GET_CODE (PATTERN (insn)) == SEQUENCE
	      || GET_CODE (PATTERN (insn)) == ASM_INPUT
	      || asm_noperands (PATTERN (insn)) >= 0);

    default:
      abort ();
    }
}

/* Return TRUE if any resources are marked in both RES1 and RES2 or if either
   resource set contains a volatile memory reference.  Otherwise, return FALSE.  */

static int
resource_conflicts_p (res1, res2)
     struct resources *res1, *res2;
{
  if ((res1->cc && res2->cc) || (res1->memory && res2->memory)
      || (res1->unch_memory && res2->unch_memory)
      || res1->volatil || res2->volatil)
    return 1;

#ifdef HARD_REG_SET
  return (res1->regs & res2->regs) != HARD_CONST (0);
#else
  {
    int i;

    for (i = 0; i < HARD_REG_SET_LONGS; i++)
      if ((res1->regs[i] & res2->regs[i]) != 0)
	return 1;
    return 0;
  }
#endif
}

/* Return TRUE if any resource marked in RES, a `struct resources', is
   referenced by INSN.  If INCLUDE_DELAYED_EFFECTS is set, return if the called
   routine is using those resources.

   We compute this by computing all the resources referenced by INSN and
   seeing if this conflicts with RES.  It might be faster to directly check
   ourselves, and this is the way it used to work, but it means duplicating
   a large block of complex code.  */

static int
insn_references_resource_p (insn, res, include_delayed_effects)
     rtx insn;
     struct resources *res;
     int include_delayed_effects;
{
  struct resources insn_res;

  CLEAR_RESOURCE (&insn_res);
  mark_referenced_resources (insn, &insn_res, include_delayed_effects);
  return resource_conflicts_p (&insn_res, res);
}

/* Return TRUE if INSN modifies resources that are marked in RES.
   INCLUDE_DELAYED_EFFECTS is set if the actions of that routine should be
   included.   CC0 is only modified if it is explicitly set; see comments
   in front of mark_set_resources for details.  */

static int
insn_sets_resource_p (insn, res, include_delayed_effects)
     rtx insn;
     struct resources *res;
     int include_delayed_effects;
{
  struct resources insn_sets;

  CLEAR_RESOURCE (&insn_sets);
  mark_set_resources (insn, &insn_sets, 0, include_delayed_effects);
  return resource_conflicts_p (&insn_sets, res);
}

/* Find a label at the end of the function or before a RETURN.  If there is
   none, make one.  */

static rtx
find_end_label ()
{
  rtx insn;

  /* If we found one previously, return it.  */
  if (end_of_function_label)
    return end_of_function_label;

  /* Otherwise, see if there is a label at the end of the function.  If there
     is, it must be that RETURN insns aren't needed, so that is our return
     label and we don't have to do anything else.  */

  insn = get_last_insn ();
  while (GET_CODE (insn) == NOTE
	 || (GET_CODE (insn) == INSN
	     && (GET_CODE (PATTERN (insn)) == USE
		 || GET_CODE (PATTERN (insn)) == CLOBBER)))
    insn = PREV_INSN (insn);

  /* When a target threads its epilogue we might already have a
     suitable return insn.  If so put a label before it for the
     end_of_function_label.  */
  if (GET_CODE (insn) == BARRIER
      && GET_CODE (PREV_INSN (insn)) == JUMP_INSN
      && GET_CODE (PATTERN (PREV_INSN (insn))) == RETURN)
    {
      rtx temp = PREV_INSN (PREV_INSN (insn));
      end_of_function_label = gen_label_rtx ();
      LABEL_NUSES (end_of_function_label) = 0;

      /* Put the label before an USE insns that may proceed the RETURN insn.  */
      while (GET_CODE (temp) == USE)
	temp = PREV_INSN (temp);

      emit_label_after (end_of_function_label, temp);
    }

  else if (GET_CODE (insn) == CODE_LABEL)
    end_of_function_label = insn;
  else
    {
      end_of_function_label = gen_label_rtx ();
      LABEL_NUSES (end_of_function_label) = 0;
      /* If the basic block reorder pass moves the return insn to
	 some other place try to locate it again and put our
	 end_of_function_label there.  */
      while (insn && ! (GET_CODE (insn) == JUMP_INSN
		        && (GET_CODE (PATTERN (insn)) == RETURN)))
	insn = PREV_INSN (insn);
      if (insn)
	{
	  insn = PREV_INSN (insn);

	  /* Put the label before an USE insns that may proceed the
	     RETURN insn.  */
	  while (GET_CODE (insn) == USE)
	    insn = PREV_INSN (insn);

	  emit_label_after (end_of_function_label, insn);
	}
      else
	{
	  /* Otherwise, make a new label and emit a RETURN and BARRIER,
	     if needed.  */
	  emit_label (end_of_function_label);
#ifdef HAVE_return
	  if (HAVE_return)
	    {
	      /* The return we make may have delay slots too.  */
	      rtx insn = gen_return ();
	      insn = emit_jump_insn (insn);
	      emit_barrier ();
	      if (num_delay_slots (insn) > 0)
		obstack_ptr_grow (&unfilled_slots_obstack, insn);
	    }
#endif
	}
    }

  /* Show one additional use for this label so it won't go away until
     we are done.  */
  ++LABEL_NUSES (end_of_function_label);

  return end_of_function_label;
}

/* Put INSN and LIST together in a SEQUENCE rtx of LENGTH, and replace
   the pattern of INSN with the SEQUENCE.

   Chain the insns so that NEXT_INSN of each insn in the sequence points to
   the next and NEXT_INSN of the last insn in the sequence points to
   the first insn after the sequence.  Similarly for PREV_INSN.  This makes
   it easier to scan all insns.

   Returns the SEQUENCE that replaces INSN.  */

static rtx
emit_delay_sequence (insn, list, length)
     rtx insn;
     rtx list;
     int length;
{
  int i = 1;
  rtx li;
  int had_barrier = 0;

  /* Allocate the rtvec to hold the insns and the SEQUENCE.  */
  rtvec seqv = rtvec_alloc (length + 1);
  rtx seq = gen_rtx_SEQUENCE (VOIDmode, seqv);
  rtx seq_insn = make_insn_raw (seq);
  rtx first = get_insns ();
  rtx last = get_last_insn ();

  /* Make a copy of the insn having delay slots.  */
  rtx delay_insn = copy_rtx (insn);

  /* If INSN is followed by a BARRIER, delete the BARRIER since it will only
     confuse further processing.  Update LAST in case it was the last insn.
     We will put the BARRIER back in later.  */
  if (NEXT_INSN (insn) && GET_CODE (NEXT_INSN (insn)) == BARRIER)
    {
      delete_related_insns (NEXT_INSN (insn));
      last = get_last_insn ();
      had_barrier = 1;
    }

  /* Splice our SEQUENCE into the insn stream where INSN used to be.  */
  NEXT_INSN (seq_insn) = NEXT_INSN (insn);
  PREV_INSN (seq_insn) = PREV_INSN (insn);

  if (insn != last)
    PREV_INSN (NEXT_INSN (seq_insn)) = seq_insn;

  if (insn != first)
    NEXT_INSN (PREV_INSN (seq_insn)) = seq_insn;

  /* Note the calls to set_new_first_and_last_insn must occur after
     SEQ_INSN has been completely spliced into the insn stream.

     Otherwise CUR_INSN_UID will get set to an incorrect value because
     set_new_first_and_last_insn will not find SEQ_INSN in the chain.  */
  if (insn == last)
    set_new_first_and_last_insn (first, seq_insn);

  if (insn == first)
    set_new_first_and_last_insn (seq_insn, last);

  /* Build our SEQUENCE and rebuild the insn chain.  */
  XVECEXP (seq, 0, 0) = delay_insn;
  INSN_DELETED_P (delay_insn) = 0;
  PREV_INSN (delay_insn) = PREV_INSN (seq_insn);

  for (li = list; li; li = XEXP (li, 1), i++)
    {
      rtx tem = XEXP (li, 0);
      rtx note, next;

      /* Show that this copy of the insn isn't deleted.  */
      INSN_DELETED_P (tem) = 0;

      XVECEXP (seq, 0, i) = tem;
      PREV_INSN (tem) = XVECEXP (seq, 0, i - 1);
      NEXT_INSN (XVECEXP (seq, 0, i - 1)) = tem;

      for (note = REG_NOTES (tem); note; note = next)
	{
	  next = XEXP (note, 1);
	  switch (REG_NOTE_KIND (note))
	    {
	    case REG_DEAD:
	      /* Remove any REG_DEAD notes because we can't rely on them now
		 that the insn has been moved.  */
	      remove_note (tem, note);
	      break;

	    case REG_LABEL:
	      /* Keep the label reference count up to date.  */
	      if (GET_CODE (XEXP (note, 0)) == CODE_LABEL)
		LABEL_NUSES (XEXP (note, 0)) ++;
	      break;

	    default:
	      break;
	    }
	}
    }

  NEXT_INSN (XVECEXP (seq, 0, length)) = NEXT_INSN (seq_insn);

  /* If the previous insn is a SEQUENCE, update the NEXT_INSN pointer on the
     last insn in that SEQUENCE to point to us.  Similarly for the first
     insn in the following insn if it is a SEQUENCE.  */

  if (PREV_INSN (seq_insn) && GET_CODE (PREV_INSN (seq_insn)) == INSN
      && GET_CODE (PATTERN (PREV_INSN (seq_insn))) == SEQUENCE)
    NEXT_INSN (XVECEXP (PATTERN (PREV_INSN (seq_insn)), 0,
			XVECLEN (PATTERN (PREV_INSN (seq_insn)), 0) - 1))
      = seq_insn;

  if (NEXT_INSN (seq_insn) && GET_CODE (NEXT_INSN (seq_insn)) == INSN
      && GET_CODE (PATTERN (NEXT_INSN (seq_insn))) == SEQUENCE)
    PREV_INSN (XVECEXP (PATTERN (NEXT_INSN (seq_insn)), 0, 0)) = seq_insn;

  /* If there used to be a BARRIER, put it back.  */
  if (had_barrier)
    emit_barrier_after (seq_insn);

  if (i != length + 1)
    abort ();

  return seq_insn;
}

/* Add INSN to DELAY_LIST and return the head of the new list.  The list must
   be in the order in which the insns are to be executed.  */

static rtx
add_to_delay_list (insn, delay_list)
     rtx insn;
     rtx delay_list;
{
  /* If we have an empty list, just make a new list element.  If
     INSN has its block number recorded, clear it since we may
     be moving the insn to a new block.  */

  if (delay_list == 0)
    {
      clear_hashed_info_for_insn (insn);
      return gen_rtx_INSN_LIST (VOIDmode, insn, NULL_RTX);
    }

  /* Otherwise this must be an INSN_LIST.  Add INSN to the end of the
     list.  */
  XEXP (delay_list, 1) = add_to_delay_list (insn, XEXP (delay_list, 1));

  return delay_list;
}

/* Delete INSN from the delay slot of the insn that it is in, which may
   produce an insn with no delay slots.  Return the new insn.  */

static rtx
delete_from_delay_slot (insn)
     rtx insn;
{
  rtx trial, seq_insn, seq, prev;
  rtx delay_list = 0;
  int i;

  /* We first must find the insn containing the SEQUENCE with INSN in its
     delay slot.  Do this by finding an insn, TRIAL, where
     PREV_INSN (NEXT_INSN (TRIAL)) != TRIAL.  */

  for (trial = insn;
       PREV_INSN (NEXT_INSN (trial)) == trial;
       trial = NEXT_INSN (trial))
    ;

  seq_insn = PREV_INSN (NEXT_INSN (trial));
  seq = PATTERN (seq_insn);

  /* Create a delay list consisting of all the insns other than the one
     we are deleting (unless we were the only one).  */
  if (XVECLEN (seq, 0) > 2)
    for (i = 1; i < XVECLEN (seq, 0); i++)
      if (XVECEXP (seq, 0, i) != insn)
	delay_list = add_to_delay_list (XVECEXP (seq, 0, i), delay_list);

  /* Delete the old SEQUENCE, re-emit the insn that used to have the delay
     list, and rebuild the delay list if non-empty.  */
  prev = PREV_INSN (seq_insn);
  trial = XVECEXP (seq, 0, 0);
  delete_related_insns (seq_insn);
  add_insn_after (trial, prev);

  if (GET_CODE (trial) == JUMP_INSN
      && (simplejump_p (trial) || GET_CODE (PATTERN (trial)) == RETURN))
    emit_barrier_after (trial);

  /* If there are any delay insns, remit them.  Otherwise clear the
     annul flag.  */
  if (delay_list)
    trial = emit_delay_sequence (trial, delay_list, XVECLEN (seq, 0) - 2);
  else
    INSN_ANNULLED_BRANCH_P (trial) = 0;

  INSN_FROM_TARGET_P (insn) = 0;

  /* Show we need to fill this insn again.  */
  obstack_ptr_grow (&unfilled_slots_obstack, trial);

  return trial;
}

/* Delete INSN, a JUMP_INSN.  If it is a conditional jump, we must track down
   the insn that sets CC0 for it and delete it too.  */

static void
delete_scheduled_jump (insn)
     rtx insn;
{
  /* Delete the insn that sets cc0 for us.  On machines without cc0, we could
     delete the insn that sets the condition code, but it is hard to find it.
     Since this case is rare anyway, don't bother trying; there would likely
     be other insns that became dead anyway, which we wouldn't know to
     delete.  */

#ifdef HAVE_cc0
  if (reg_mentioned_p (cc0_rtx, insn))
    {
      rtx note = find_reg_note (insn, REG_CC_SETTER, NULL_RTX);

      /* If a reg-note was found, it points to an insn to set CC0.  This
	 insn is in the delay list of some other insn.  So delete it from
	 the delay list it was in.  */
      if (note)
	{
	  if (! FIND_REG_INC_NOTE (XEXP (note, 0), NULL_RTX)
	      && sets_cc0_p (PATTERN (XEXP (note, 0))) == 1)
	    delete_from_delay_slot (XEXP (note, 0));
	}
      else
	{
	  /* The insn setting CC0 is our previous insn, but it may be in
	     a delay slot.  It will be the last insn in the delay slot, if
	     it is.  */
	  rtx trial = previous_insn (insn);
	  if (GET_CODE (trial) == NOTE)
	    trial = prev_nonnote_insn (trial);
	  if (sets_cc0_p (PATTERN (trial)) != 1
	      || FIND_REG_INC_NOTE (trial, NULL_RTX))
	    return;
	  if (PREV_INSN (NEXT_INSN (trial)) == trial)
	    delete_related_insns (trial);
	  else
	    delete_from_delay_slot (trial);
	}
    }
#endif

  delete_related_insns (insn);
}

/* Counters for delay-slot filling.  */

#define NUM_REORG_FUNCTIONS 2
#define MAX_DELAY_HISTOGRAM 3
#define MAX_REORG_PASSES 2

static int num_insns_needing_delays[NUM_REORG_FUNCTIONS][MAX_REORG_PASSES];

static int num_filled_delays[NUM_REORG_FUNCTIONS][MAX_DELAY_HISTOGRAM+1][MAX_REORG_PASSES];

static int reorg_pass_number;

static void
note_delay_statistics (slots_filled, index)
     int slots_filled, index;
{
  num_insns_needing_delays[index][reorg_pass_number]++;
  if (slots_filled > MAX_DELAY_HISTOGRAM)
    slots_filled = MAX_DELAY_HISTOGRAM;
  num_filled_delays[index][slots_filled][reorg_pass_number]++;
}

#if defined(ANNUL_IFFALSE_SLOTS) || defined(ANNUL_IFTRUE_SLOTS)

/* Optimize the following cases:

   1.  When a conditional branch skips over only one instruction,
       use an annulling branch and put that insn in the delay slot.
       Use either a branch that annuls when the condition if true or
       invert the test with a branch that annuls when the condition is
       false.  This saves insns, since otherwise we must copy an insn
       from the L1 target.

        (orig)		 (skip)		(otherwise)
	Bcc.n L1	Bcc',a L1	Bcc,a L1'
	insn		insn		insn2
      L1:	      L1:	      L1:
	insn2		insn2		insn2
	insn3		insn3	      L1':
					insn3

   2.  When a conditional branch skips over only one instruction,
       and after that, it unconditionally branches somewhere else,
       perform the similar optimization. This saves executing the
       second branch in the case where the inverted condition is true.

	Bcc.n L1	Bcc',a L2
	insn		insn
      L1:	      L1:
	Bra L2		Bra L2

   INSN is a JUMP_INSN.

   This should be expanded to skip over N insns, where N is the number
   of delay slots required.  */

static rtx
optimize_skip (insn)
     rtx insn;
{
  rtx trial = next_nonnote_insn (insn);
  rtx next_trial = next_active_insn (trial);
  rtx delay_list = 0;
  rtx target_label;
  int flags;

  flags = get_jump_flags (insn, JUMP_LABEL (insn));

  if (trial == 0
      || GET_CODE (trial) != INSN
      || GET_CODE (PATTERN (trial)) == SEQUENCE
      || recog_memoized (trial) < 0
      || (! eligible_for_annul_false (insn, 0, trial, flags)
	  && ! eligible_for_annul_true (insn, 0, trial, flags))
      || can_throw_internal (trial))
    return 0;

  /* There are two cases where we are just executing one insn (we assume
     here that a branch requires only one insn; this should be generalized
     at some point):  Where the branch goes around a single insn or where
     we have one insn followed by a branch to the same label we branch to.
     In both of these cases, inverting the jump and annulling the delay
     slot give the same effect in fewer insns.  */
  if ((next_trial == next_active_insn (JUMP_LABEL (insn))
       && ! (next_trial == 0 && current_function_epilogue_delay_list != 0))
      || (next_trial != 0
	  && GET_CODE (next_trial) == JUMP_INSN
	  && JUMP_LABEL (insn) == JUMP_LABEL (next_trial)
	  && (simplejump_p (next_trial)
	      || GET_CODE (PATTERN (next_trial)) == RETURN)))
    {
      if (eligible_for_annul_false (insn, 0, trial, flags))
	{
	  if (invert_jump (insn, JUMP_LABEL (insn), 1))
	    INSN_FROM_TARGET_P (trial) = 1;
	  else if (! eligible_for_annul_true (insn, 0, trial, flags))
	    return 0;
	}

      delay_list = add_to_delay_list (trial, NULL_RTX);
      next_trial = next_active_insn (trial);
      update_block (trial, trial);
      delete_related_insns (trial);

      /* Also, if we are targeting an unconditional
	 branch, thread our jump to the target of that branch.  Don't
	 change this into a RETURN here, because it may not accept what
	 we have in the delay slot.  We'll fix this up later.  */
      if (next_trial && GET_CODE (next_trial) == JUMP_INSN
	  && (simplejump_p (next_trial)
	      || GET_CODE (PATTERN (next_trial)) == RETURN))
	{
	  target_label = JUMP_LABEL (next_trial);
	  if (target_label == 0)
	    target_label = find_end_label ();

	  /* Recompute the flags based on TARGET_LABEL since threading
	     the jump to TARGET_LABEL may change the direction of the
	     jump (which may change the circumstances in which the
	     delay slot is nullified).  */
	  flags = get_jump_flags (insn, target_label);
	  if (eligible_for_annul_true (insn, 0, trial, flags))
	    reorg_redirect_jump (insn, target_label);
	}

      INSN_ANNULLED_BRANCH_P (insn) = 1;
    }

  return delay_list;
}
#endif

/*  Encode and return branch direction and prediction information for
    INSN assuming it will jump to LABEL.

    Non conditional branches return no direction information and
    are predicted as very likely taken.  */

static int
get_jump_flags (insn, label)
     rtx insn, label;
{
  int flags;

  /* get_jump_flags can be passed any insn with delay slots, these may
     be INSNs, CALL_INSNs, or JUMP_INSNs.  Only JUMP_INSNs have branch
     direction information, and only if they are conditional jumps.

     If LABEL is zero, then there is no way to determine the branch
     direction.  */
  if (GET_CODE (insn) == JUMP_INSN
      && (condjump_p (insn) || condjump_in_parallel_p (insn))
      && INSN_UID (insn) <= max_uid
      && label != 0
      && INSN_UID (label) <= max_uid)
    flags
      = (uid_to_ruid[INSN_UID (label)] > uid_to_ruid[INSN_UID (insn)])
	 ? ATTR_FLAG_forward : ATTR_FLAG_backward;
  /* No valid direction information.  */
  else
    flags = 0;

  /* If insn is a conditional branch call mostly_true_jump to get
     determine the branch prediction.

     Non conditional branches are predicted as very likely taken.  */
  if (GET_CODE (insn) == JUMP_INSN
      && (condjump_p (insn) || condjump_in_parallel_p (insn)))
    {
      int prediction;

      prediction = mostly_true_jump (insn, get_branch_condition (insn, label));
      switch (prediction)
	{
	case 2:
	  flags |= (ATTR_FLAG_very_likely | ATTR_FLAG_likely);
	  break;
	case 1:
	  flags |= ATTR_FLAG_likely;
	  break;
	case 0:
	  flags |= ATTR_FLAG_unlikely;
	  break;
	case -1:
	  flags |= (ATTR_FLAG_very_unlikely | ATTR_FLAG_unlikely);
	  break;

	default:
	  abort ();
	}
    }
  else
    flags |= (ATTR_FLAG_very_likely | ATTR_FLAG_likely);

  return flags;
}

/* Return 1 if INSN is a destination that will be branched to rarely (the
   return point of a function); return 2 if DEST will be branched to very
   rarely (a call to a function that doesn't return).  Otherwise,
   return 0.  */

static int
rare_destination (insn)
     rtx insn;
{
  int jump_count = 0;
  rtx next;

  for (; insn; insn = next)
    {
      if (GET_CODE (insn) == INSN && GET_CODE (PATTERN (insn)) == SEQUENCE)
	insn = XVECEXP (PATTERN (insn), 0, 0);

      next = NEXT_INSN (insn);

      switch (GET_CODE (insn))
	{
	case CODE_LABEL:
	  return 0;
	case BARRIER:
	  /* A BARRIER can either be after a JUMP_INSN or a CALL_INSN.  We
	     don't scan past JUMP_INSNs, so any barrier we find here must
	     have been after a CALL_INSN and hence mean the call doesn't
	     return.  */
	  return 2;
	case JUMP_INSN:
	  if (GET_CODE (PATTERN (insn)) == RETURN)
	    return 1;
	  else if (simplejump_p (insn)
		   && jump_count++ < 10)
	    next = JUMP_LABEL (insn);
	  else
	    return 0;

	default:
	  break;
	}
    }

  /* If we got here it means we hit the end of the function.  So this
     is an unlikely destination.  */

  return 1;
}

/* Return truth value of the statement that this branch
   is mostly taken.  If we think that the branch is extremely likely
   to be taken, we return 2.  If the branch is slightly more likely to be
   taken, return 1.  If the branch is slightly less likely to be taken,
   return 0 and if the branch is highly unlikely to be taken, return -1.

   CONDITION, if non-zero, is the condition that JUMP_INSN is testing.  */

static int
mostly_true_jump (jump_insn, condition)
     rtx jump_insn, condition;
{
  rtx target_label = JUMP_LABEL (jump_insn);
  rtx insn, note;
  int rare_dest = rare_destination (target_label);
  int rare_fallthrough = rare_destination (NEXT_INSN (jump_insn));

  /* If branch probabilities are available, then use that number since it
     always gives a correct answer.  */
  note = find_reg_note (jump_insn, REG_BR_PROB, 0);
  if (note)
    {
      int prob = INTVAL (XEXP (note, 0));

      if (prob >= REG_BR_PROB_BASE * 9 / 10)
	return 2;
      else if (pro