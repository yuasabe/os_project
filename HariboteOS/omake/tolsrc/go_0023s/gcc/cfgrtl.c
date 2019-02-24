/* Control flow graph manipulation code for GNU compiler.
   Copyright (C) 1987, 1988, 1992, 1993, 1994, 1995, 1996, 1997, 1998,
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

/* This file contains low level functions to manipulate the CFG and analyze it
   that are aware of the RTL intermediate language.

   Available functionality:
     - CFG-aware instruction chain manipulation
	 delete_insn, delete_insn_chain
     - Basic block manipulation
	 create_basic_block, flow_delete_block, split_block,
	 merge_blocks_nomove
     - Infrastructure to determine quickly basic block for insn
	 compute_bb_for_insn, update_bb_for_insn, set_block_for_insn,
     - Edge redirection with updating and optimizing of insn chain
	 block_label, redirect_edge_and_branch,
	 redirect_edge_and_branch_force, tidy_fallthru_edge, force_nonfallthru
     - Edge splitting and commiting to edges
	 split_edge, insert_insn_on_edge, commit_edge_insertions
     - Dumping and debugging
	 print_rtl_with_bb, dump_bb, debug_bb, debug_bb_n
     - Consistency checking
	 verify_flow_info
     - CFG updating after constant propagation
	 purge_dead_edges, purge_all_dead_edges   */

/* !kawai! */
#include "config.h"
#include "system.h"
#include "tree.h"
#include "rtl.h"
#include "hard-reg-set.h"
#include "basic-block.h"
#include "regs.h"
#include "flags.h"
#include "output.h"
#include "function.h"
#include "except.h"
#include "toplev.h"
#include "tm_p.h"
#include "../include/obstack.h"
/* end of !kawai! */

/* Stubs in case we don't have a return insn.  */
#ifndef HAVE_return
#define HAVE_return 0
#define gen_return() NULL_RTX
#endif

/* The basic block structure for every insn, indexed by uid.  */
varray_type basic_block_for_insn;

/* The labels mentioned in non-jump rtl.  Valid during find_basic_blocks.  */
/* ??? Should probably be using LABEL_NUSES instead.  It would take a
   bit of surgery to be able to use or co-opt the routines in jump.  */
rtx label_value_list;
rtx tail_recursion_label_list;

static int can_delete_note_p		PARAMS ((rtx));
static int can_delete_label_p		PARAMS ((rtx));
static void commit_one_edge_insertion	PARAMS ((edge));
static bool try_redirect_by_replacing_jump PARAMS ((edge, basic_block));
static rtx last_loop_beg_note		PARAMS ((rtx));
static bool back_edge_of_syntactic_loop_p PARAMS ((basic_block, basic_block));
static basic_block force_nonfallthru_and_redirect PARAMS ((edge, basic_block));

/* Return true if NOTE is not one of the ones that must be kept paired,
   so that we may simply delete it.  */

static int
can_delete_note_p (note)
     rtx note;
{
  return (NOTE_LINE_NUMBER (note) == NOTE_INSN_DELETED
	  || NOTE_LINE_NUMBER (note) == NOTE_INSN_BASIC_BLOCK);
}

/* True if a given label can be deleted.  */

static int
can_delete_label_p (label)
     rtx label;
{
  return (!LABEL_PRESERVE_P (label)
	  /* User declared labels must be preserved.  */
	  && LABEL_NAME (label) == 0
	  && !in_expr_list_p (forced_labels, label)
	  && !in_expr_list_p (label_value_list, label));
}

/* Delete INSN by patching it out.  Return the next insn.  */

rtx
delete_insn (insn)
     rtx insn;
{
  rtx next = NEXT_INSN (insn);
  rtx note;
  bool really_delete = true;

  if (GET_CODE (insn) == CODE_LABEL)
    {
      /* Some labels can't be directly removed from the INSN chain, as they
         might be references via variables, constant pool etc. 
         Convert them to the special NOTE_INSN_DELETED_LABEL note.  */
      if (! can_delete_label_p (insn))
	{
	  const char *name = LABEL_NAME (insn);

	  really_delete = false;
	  PUT_CODE (insn, NOTE);
	  NOTE_LINE_NUMBER (insn) = NOTE_INSN_DELETED_LABEL;
	  NOTE_SOURCE_FILE (insn) = name;
	}

      remove_node_from_expr_list (insn, &nonlocal_goto_handler_labels);
    }

  if (really_delete)
    {
      /* If this insn has already been deleted, something is very wrong.  */
      if (INSN_DELETED_P (insn))
	abort ();
      remove_insn (insn);
      INSN_DELETED_P (insn) = 1;
    }

  /* If deleting a jump, decrement the use count of the label.  Deleting
     the label itself should happen in the normal course of block merging.  */
  if (GET_CODE (insn) == JUMP_INSN
      && JUMP_LABEL (insn)
      && GET_CODE (JUMP_LABEL (insn)) == CODE_LABEL)
    LABEL_NUSES (JUMP_LABEL (insn))--;

  /* Also if deleting an insn that references a label.  */
  else if ((note = find_reg_note (insn, REG_LABEL, NULL_RTX)) != NULL_RTX
	   && GET_CODE (XEXP (note, 0)) == CODE_LABEL)
    LABEL_NUSES (XEXP (note, 0))--;

  if (GET_CODE (insn) == JUMP_INSN
      && (GET_CODE (PATTERN (insn)) == ADDR_VEC
	  || GET_CODE (PATTERN (insn)) == ADDR_DIFF_VEC))
    {
      rtx pat = PATTERN (insn);
      int diff_vec_p = GET_CODE (PATTERN (insn)) == ADDR_DIFF_VEC;
      int len = XVECLEN (pat, diff_vec_p);
      int i;

      for (i = 0; i < len; i++)
	{
	  rtx label = XEXP (XVECEXP (pat, diff_vec_p, i), 0);

	  /* When deleting code in bulk (e.g. removing many unreachable
	     blocks) we can delete a label that's a target of the vector
	     before deleting the vector itself.  */
	  if (GET_CODE (label) != NOTE)
	    LABEL_NUSES (label)--;
	}
    }

  return next;
}

/* Unlink a chain of insns between START and FINISH, leaving notes
   that must be paired.  */

void
delete_insn_chain (start, finish)
     rtx start, finish;
{
  rtx next;

  /* Unchain the insns one by one.  It would be quicker to delete all of these
     with a single unchaining, rather than one at a time, but we need to keep
     the NOTE's.  */
  while (1)
    {
      next = NEXT_INSN (start);
      if (GET_CODE (start) == NOTE && !can_delete_note_p (start))
	;
      else
	next = delete_insn (start);

      if (start == finish)
	break;
      start = next;
    }
}

/* Create a new basic block consisting of the instructions between HEAD and END
   inclusive.  This function is designed to allow fast BB construction - reuses
   the note and basic block struct in BB_NOTE, if any and do not grow
   BASIC_BLOCK chain and should be used directly only by CFG construction code.
   END can be NULL in to create new empty basic block before HEAD.  Both END
   and HEAD can be NULL to create basic block at the end of INSN chain.  */

basic_block
create_basic_block_structure (index, head, end, bb_note)
     int index;
     rtx head, end, bb_note;
{
  basic_block bb;

  if (bb_note
      && ! RTX_INTEGRATED_P (bb_note)
      && (bb = NOTE_BASIC_BLOCK (bb_note)) != NULL
      && bb->aux == NULL)
    {
      /* If we found an existing note, thread it back onto the chain.  */

      rtx after;

      if (GET_CODE (head) == CODE_LABEL)
	after = head;
      else
	{
	  after = PREV_INSN (head);
	  head = bb_note;
	}

      if (after != bb_note && NEXT_INSN (after) != bb_note)
	reorder_insns (bb_note, bb_note, after);
    }
  else
    {
      /* Otherwise we must create a note and a basic block structure.  */

      bb = alloc_block ();

      if (!head && !end)
	head = end = bb_note
	  = emit_note_after (NOTE_INSN_BASIC_BLOCK, get_last_insn ());
      else if (GET_CODE (head) == CODE_LABEL && end)
	{
	  bb_note = emit_note_after (NOTE_INSN_BASIC_BLOCK, head);
	  if (head == end)
	    end = bb_note;
	}
      else
	{
	  bb_note = emit_note_before (NOTE_INSN_BASIC_BLOCK, head);
	  head = bb_note;
	  if (!end)
	    end = head;
	}

      NOTE_BASIC_BLOCK (bb_note) = bb;
    }

  /* Always include the bb note in the block.  */
  if (NEXT_INSN (end) == bb_note)
    end = bb_note;

  bb->head = head;
  bb->end = end;
  bb->index = index;
  BASIC_BLOCK (index) = bb;
  if (basic_block_for_insn)
    update_bb_for_insn (bb);

  /* Tag the block so that we know it has been used when considering
     other basic block notes.  */
  bb->aux = bb;

  return bb;
}

/* Create new basic block consisting of instructions in between HEAD and END
   and place it to the BB chain at position INDEX.  END can be NULL in to
   create new empty basic block before HEAD.  Both END and HEAD can be NULL to
   create basic block at the end of INSN chain.  */

basic_block
create_basic_block (index, head, end)
     int index;
     rtx head, end;
{
  basic_block bb;
  int i;

  /* Place the new block just after the block being split.  */
  VARRAY_GROW (basic_block_info, ++n_basic_blocks);

  /* Some parts of the compiler expect blocks to be number in
     sequential order so insert the new block immediately after the
     block being split..  */
  for (i = n_basic_blocks - 1; i > index; --i)
    {
      basic_block tmp = BASIC_BLOCK (i - 1);

      BASIC_BLOCK (i) = tmp;
      tmp->index = i;
    }

  bb = create_basic_block_structure (index, head, end, NULL);
  bb->aux = NULL;
  return bb;
}

/* Delete the insns in a (non-live) block.  We physically delete every
   non-deleted-note insn, and update the flow graph appropriately.

   Return nonzero if we deleted an exception handler.  */

/* ??? Preserving all such notes strikes me as wrong.  It would be nice
   to post-process the stream to remove empty blocks, loops, ranges, etc.  */

int
flow_delete_block_noexpunge (b)
     basic_block b;
{
  int deleted_handler = 0;
  rtx insn, end, tmp;

  /* If the head of this block is a CODE_LABEL, then it might be the
     label for an exception handler which can't be reached.

     We need to remove the label from the exception_handler_label list
     and remove the associated NOTE_INSN_EH_REGION_BEG and
     NOTE_INSN_EH_REGION_END notes.  */

  insn = b->head;

  never_reached_warning (insn, b->end);

  if (GET_CODE (insn) == CODE_LABEL)
    maybe_remove_eh_handler (insn);

  /* Include any jump table following the basic block.  */
  end = b->end;
  if (GET_CODE (end) == JUMP_INSN
      && (tmp = JUMP_LABEL (end)) != NULL_RTX
      && (tmp = NEXT_INSN (tmp)) != NULL_RTX
      && GET_CODE (tmp) == JUMP_INSN
      && (GET_CODE (PATTERN (tmp)) == ADDR_VEC
	  || GET_CODE (PATTERN (tmp)) == ADDR_DIFF_VEC))
    end = tmp;

  /* Include any barrier that may follow the basic block.  */
  tmp = next_nonnote_insn (end);
  if (tmp && GET_CODE (tmp) == BARRIER)
    end = tmp;

  /* Selectively delete the entire chain.  */
  b->head = NULL;
  delete_insn_chain (insn, end);

  /* Remove the edges into and out of this block.  Note that there may
     indeed be edges in, if we are removing an unreachable loop.  */
  while (b->pred != NULL)
    remove_edge (b->pred);
  while (b->succ != NULL)
    remove_edge (b->succ);

  b->pred = NULL;
  b->succ = NULL;

  return deleted_handler;
}

int
flow_delete_block (b)
     basic_block b;
{
  int deleted_handler = flow_delete_block_noexpunge (b);
  
  /* Remove the basic block from the array, and compact behind it.  */
  expunge_block (b);

  return deleted_handler;
}

/* Records the basic block struct in BB_FOR_INSN, for every instruction
   indexed by INSN_UID.  MAX is the size of the array.  */

void
compute_bb_for_insn (max)
     int max;
{
  int i;

  if (basic_block_for_insn)
    VARRAY_FREE (basic_block_for_insn);

  VARRAY_BB_INIT (basic_block_for_insn, max, "basic_block_for_insn");

  for (i = 0; i < n_basic_blocks; ++i)
    {
      basic_block bb = BASIC_BLOCK (i);
      rtx end = bb->end;
      rtx insn;

      for (insn = bb->head; ; insn = NEXT_INSN (insn))
	{
	  if (INSN_UID (insn) < max)
	    VARRAY_BB (basic_block_for_insn, INSN_UID (insn)) = bb;

	  if (insn == end)
	    break;
	}
    }
}

/* Release the basic_block_for_insn array.  */

void
free_bb_for_insn ()
{
  if (basic_block_for_insn)
    VARRAY_FREE (basic_block_for_insn);

  basic_block_for_insn = 0;
}

/* Update insns block within BB.  */

void
update_bb_for_insn (bb)
     basic_block bb;
{
  rtx insn;

  if (! basic_block_for_insn)
    return;

  for (insn = bb->head; ; insn = NEXT_INSN (insn))
    {
      set_block_for_insn (insn, bb);
      if (insn == bb->end)
	break;
    }
}

/* Record INSN's block as BB.  */

void
set_block_for_insn (insn, bb)
     rtx insn;
     basic_block bb;
{
  size_t uid = INSN_UID (insn);

  if (uid >= basic_block_for_insn->num_elements)
    {
      /* Add one-eighth the size so we don't keep calling xrealloc.  */
      size_t new_size = uid + (uid + 7) / 8;

      VARRAY_GROW (basic_block_for_insn, new_size);
    }

  VARRAY_BB (basic_block_for_insn, uid) = bb;
}

/* Split a block BB after insn INSN creating a new fallthru edge.
   Return the new edge.  Note that to keep other parts of the compiler happy,
   this function renumbers all the basic blocks so that the new
   one has a number one greater than the block split.  */

edge
split_block (bb, insn)
     basic_block bb;
     rtx insn;
{
  basic_block new_bb;
  edge new_edge;
  edge e;

  /* There is no point splitting the block after its end.  */
  if (bb->end == insn)
    return 0;

  /* Create the new basic block.  */
  new_bb = create_basic_block (bb->index + 1, NEXT_INSN (insn), bb->end);
  new_bb->count = bb->count;
  new_bb->frequency = bb->frequency;
  new_bb->loop_depth = bb->loop_depth;
  bb->end = insn;

  /* Redirect the outgoing edges.  */
  new_bb->succ = bb->succ;
  bb->succ = NULL;
  for (e = new_bb->succ; e; e = e->succ_next)
    e->src = new_bb;

  new_edge = make_single_succ_edge (bb, new_bb, EDGE_FALLTHRU);

  if (bb->global_live_at_start)
    {
      new_bb->global_live_at_start = OBSTACK_ALLOC_REG_SET (&flow_obstack);
      new_bb->global_live_at_end = OBSTACK_ALLOC_REG_SET (&flow_obstack);
      COPY_REG_SET (new_bb->global_live_at_end, bb->global_live_at_end);

      /* We now have to calculate which registers are live at the end
	 of the split basic block and at the start of the new basic
	 block.  Start with those registers that are known to be live
	 at the end of the original basic block and get
	 propagate_block to determine which registers are live.  */
      COPY_REG_SET (new_bb->global_live_at_start, bb->global_live_at_end);
      propagate_block (new_bb, new_bb->global_live_at_start, NULL, NULL, 0);
      COPY_REG_SET (bb->global_live_at_end,
		    new_bb->global_live_at_start);
    }

  return new_edge;
}

/* Blocks A and B are to be merged into a single block A.  The insns
   are already contiguous, hence `nomove'.  */

void
merge_blocks_nomove (a, b)
     basic_block a, b;
{
  rtx b_head = b->head, b_end = b->end, a_end = a->end;
  rtx del_first = NULL_RTX, del_last = NULL_RTX;
  int b_empty = 0;
  edge e;

  /* If there was a CODE_LABEL beginning B, delete it.  */
  if (GET_CODE (b_head) == CODE_LABEL)
    {
      /* Detect basic blocks with nothing but a label.  This can happen
	 in particular at the end of a function.  */
      if (b_head == b_end)
	b_empty = 1;

      del_first = del_last = b_head;
      b_head = NEXT_INSN (b_head);
    }

  /* Delete the basic block note and handle blocks containing just that
     note.  */
  if (NOTE_INSN_BASIC_BLOCK_P (b_head))
    {
      if (b_head == b_end)
	b_empty = 1;
      if (! del_last)
	del_first = b_head;

      del_last = b_head;
      b_head = NEXT_INSN (b_head);
    }

  /* If there was a jump out of A, delete it.  */
  if (GET_CODE (a_end) == JUMP_INSN)
    {
      rtx prev;

      for (prev = PREV_INSN (a_end); ; prev = PREV_INSN (prev))
	if (GET_CODE (prev) != NOTE
	    || NOTE_LINE_NUMBER (prev) == NOTE_INSN_BASIC_BLOCK
	    || prev == a->head)
	  break;

      del_first = a_end;

#ifdef HAVE_cc0
      /* If this was a conditional jump, we need to also delete
	 the insn that set cc0.  */
      if (only_sets_cc0_p (prev))
	{
	  rtx tmp = prev;

	  prev = prev_nonnote_insn (prev);
	  if (!prev)
	    prev = a->head;
	  del_first = tmp;
	}
#endif

      a_end = PREV_INSN (del_first);
    }
  else if (GET_CODE (NEXT_INSN (a_end)) == BARRIER)
    del_first = NEXT_INSN (a_end);

  /* Normally there should only be one successor of A and that is B, but
     partway though the merge of blocks for conditional_execution we'll
     be merging a TEST block with THEN and ELSE successors.  Free the
     whole lot of them and hope the caller knows what they're doing.  */
  while (a->succ)
    remove_edge (a->succ);

  /* Adjust the edges out of B for the new owner.  */
  for (e = b->succ; e; e = e->succ_next)
    e->src = a;
  a->succ = b->succ;

  /* B hasn't quite yet ceased to exist.  Attempt to prevent mishap.  */
  b->pred = b->succ = NULL;
  a->global_live_at_end = b->global_live_at_end;

  expunge_block (b);

  /* Delete everything marked above as well as crap that might be
     hanging out between the two blocks.  */
  delete_insn_chain (del_first, del_last);

  /* Reassociate the insns of B with A.  */
  if (!b_empty)
    {
      if (basic_block_for_insn)
	{
	  rtx x;

	  for (x = a_end; x != b_end; x = NEXT_INSN (x))
	    set_block_for_insn (x, a);

	  set_block_for_insn (b_end, a);
	}

      a_end = b_end;
    }

  a->end = a_end;
}

/* Return the label in the head of basic block BLOCK.  Create one if it doesn't
   exist.  */

rtx
block_label (block)
     basic_block block;
{
  if (block == EXIT_BLOCK_PTR)
    return NULL_RTX;

  if (GET_CODE (block->head) != CODE_LABEL)
    {
      block->head = emit_label_before (gen_label_rtx (), block->head);
      if (basic_block_for_insn)
	set_block_for_insn (block->head, block);
    }

  return block->head;
}

/* Attempt to perform edge redirection by replacing possibly complex jump
   instruction by unconditional jump or removing jump completely.  This can
   apply only if all edges now point to the same block.  The parameters and
   return values are equivalent to redirect_edge_and_branch.  */

static bool
try_redirect_by_replacing_jump (e, target)
     edge e;
     basic_block target;
{
  basic_block src = e->src;
  rtx insn = src->end, kill_from;
  edge tmp;
  rtx set;
  int fallthru = 0;

  /* Verify that all targets will be TARGET.  */
  for (tmp = src->succ; tmp; tmp = tmp->succ_next)
    if (tmp->dest != target && tmp != e)
      break;

  if (tmp || !onlyjump_p (insn))
    return false;

  /* Avoid removing branch with side effects.  */
  set = single_set (insn);
  if (!set || side_effects_p (set))
    return false;

  /* In case we zap a conditional jump, we'll need to kill
     the cc0 setter too.  */
  kill_from = insn;
#ifdef HAVE_cc0
  if (reg_mentioned_p (cc0_rtx, PATTERN (insn)))
    kill_from = PREV_INSN (insn);
#endif

  /* See if we can create the fallthru edge.  */
  if (can_fallthru (src, target))
    {
      if (rtl_dump_file)
	fprintf (rtl_dump_file, "Removing jump %i.¥n", INSN_UID (insn));
      fallthru = 1;

      /* Selectively unlink whole insn chain.  */
      delete_insn_chain (kill_from, PREV_INSN (target->head));
    }

  /* If this already is simplejump, redirect it.  */
  else if (simplejump_p (insn))
    {
      if (e->dest == target)
	return false;
      if (rtl_dump_file)
	fprintf (rtl_dump_file, "Redirecting jump %i from %i to %i.¥n",
		 INSN_UID (insn), e->dest->index, target->index);
      if (!redirect_jump (insn, block_label (target), 0))
	{
	  if (target == EXIT_BLOCK_PTR)
	    return false;
	  abort ();
	}
    }

  /* Cannot do anything for target exit block.  */
  else if (target == EXIT_BLOCK_PTR)
    return false;

  /* Or replace possibly complicated jump insn by simple jump insn.  */
  else
    {
      rtx target_label = block_label (target);
      rtx barrier, tmp;

      emit_jump_insn_after (gen_jump (target_label), insn);
      JUMP_LABEL (src->end) = target_label;
      LABEL_NUSES (target_label)++;
      if (rtl_dump_file)
	fprintf (rtl_dump_file, "Replacing insn %i by jump %i¥n",
		 INSN_UID (insn), INSN_UID (src->end));


      delete_insn_chain (kill_from, insn);

      /* Recognize a tablejump that we are converting to a
	 simple jump and remove its associated CODE_LABEL
	 and ADDR_VEC or ADDR_DIFF_VEC.  */
      if ((tmp = JUMP_LABEL (insn)) != NULL_RTX
	  && (tmp = NEXT_INSN (tmp)) != NULL_RTX
	  && GET_CODE (tmp) == JUMP_INSN
	  && (GET_CODE (PATTERN (tmp)) == ADDR_VEC
	      || GET_CODE (PATTERN (tmp)) == ADDR_DIFF_VEC))
	{
	  delete_insn_chain (JUMP_LABEL (insn), tmp);
	}

      barrier = next_nonnote_insn (src->end);
      if (!barrier || GET_CODE (barrier) != BARRIER)
	emit_barrier_after (src->end);
    }

  /* Keep only one edge out and set proper flags.  */
  while (src->succ->succ_next)
    remove_edge (src->succ);
  e = src->succ;
  if (fallthru)
    e->flags = EDGE_FALLTHRU;
  else
    e->flags = 0;

  e->probability = REG_BR_PROB_BASE;
  e->count = src->count;

  /* We don't want a block to end on a line-number note since that has
     the potential of changing the code between -g and not -g.  */
  while (GET_CODE (e->src->end) == NOTE
	 && NOTE_LINE_NUMBER (e->src->end) >= 0)
    delete_insn (e->src->end);

  if (e->dest != target)
    redirect_edge_succ (e, target);

  return true;
}

/* Return last loop_beg note appearing after INSN, before start of next
   basic block.  Return INSN if there are no such notes.

   When emitting jump to redirect an fallthru edge, it should always appear
   after the LOOP_BEG notes, as loop optimizer expect loop to either start by
   fallthru edge or jump following the LOOP_BEG note jumping to the loop exit
   test.  */

static rtx
last_loop_beg_note (insn)
     rtx insn;
{
  rtx last = insn;

  for (insn = NEXT_INSN (insn); insn && GET_CODE (insn) == NOTE
       && NOTE_LINE_NUMBER (insn) != NOTE_INSN_BASIC_BLOCK;
       insn = NEXT_INSN (insn))
    if (NOTE_LINE_NUMBER (insn) == NOTE_INSN_LOOP_BEG)
      last = insn;

  return last;
}

/* Attempt to change code to redirect edge E to TARGET.  Don't do that on
   expense of adding new instructions or reordering basic blocks.

   Function can be also called with edge destination equivalent to the TARGET.
   Then it should try the simplifications and do nothing if none is possible.

   Return true if transformation succeeded.  We still return false in case E
   already destinated TARGET and we didn't managed to simplify instruction
   stream.  */

bool
redirect_edge_and_branch (e, target)
     edge e;
     basic_block target;
{
  rtx tmp;
  rtx old_label = e->dest->head;
  basic_block src = e->src;
  rtx insn = src->end;

  if (e->flags & (EDGE_ABNORMAL_CALL | EDGE_EH))
    return false;

  if (try_redirect_by_replacing_jump (e, target))
    return true;

  /* Do this fast path late, as we want above code to simplify for cases
     where called on single edge leaving basic block containing nontrivial
     jump insn.  */
  else if (e->dest == target)
    return false;

  /* We can only redirect non-fallthru edges of jump insn.  */
  if (e->flags & EDGE_FALLTHRU)
    return false;
  else if (GET_CODE (insn) != JUMP_INSN)
    return false;

  /* Recognize a tablejump and adjust all matching cases.  */
  if ((tmp = JUMP_LABEL (insn)) != NULL_RTX
      && (tmp = NEXT_INSN (tmp)) != NULL_RTX
      && GET_CODE (tmp) == JUMP_INSN
      && (GET_CODE (PATTERN (tmp)) == ADDR_VEC
	  || GET_CODE (PATTERN (tmp)) == ADDR_DIFF_VEC))
    {
      rtvec vec;
      int j;
      rtx new_label = block_label (target);

      if (target == EXIT_BLOCK_PTR)
	return false;
      if (GET_CODE (PATTERN (tmp)) == ADDR_VEC)
	vec = XVEC (PATTERN (tmp), 0);
      else
	vec = XVEC (PATTERN (tmp), 1);

      for (j = GET_NUM_ELEM (vec) - 1; j >= 0; --j)
	if (XEXP (RTVEC_ELT (vec, j), 0) == old_label)
	  {
	    RTVEC_ELT (vec, j) = gen_rtx_LABEL_REF (Pmode, new_label);
	    --LABEL_NUSES (old_label);
	    ++LABEL_NUSES (new_label);
	  }

      /* Handle casesi dispatch insns */
      if ((tmp = single_set (insn)) != NULL
	  && SET_DEST (tmp) == pc_rtx
	  && GET_CODE (SET_SRC (tmp)) == IF_THEN_ELSE
	  && GET_CODE (XEXP (SET_SRC (tmp), 2)) == LABEL_REF
	  && XEXP (XEXP (SET_SRC (tmp), 2), 0) == old_label)
	{
	  XEXP (SET_SRC (tmp), 2) = gen_rtx_LABEL_REF (VOIDmode,
						       new_label);
	  --LABEL_NUSES (old_label);
	  ++LABEL_NUSES (new_label);
	}
    }
  else
    {
      /* ?? We may play the games with moving the named labels from
	 one basic block to the other in case only one computed_jump is
	 available.  */
      if (computed_jump_p (insn)
	  /* A return instruction can't be redirected.  */
	  || returnjump_p (insn))
	return false;

      /* If the insn doesn't go where we think, we're confused.  */
      if (JUMP_LABEL (insn) != old_label)
	abort ();

      /* If the substitution doesn't succeed, die.  This can happen
	 if the back end emitted unrecognizable instructions or if
	 target is exit block on some arches.  */
      if (!redirect_jump (insn, block_label (target), 0))
	{
	  if (target == EXIT_BLOCK_PTR)
	    return false;
	  abort ();
	}
    }

  if (rtl_dump_file)
    fprintf (rtl_dump_file, "Edge %i->%i redirected to %i¥n",
	     e->src->index, e->dest->index, target->index);

  if (e->dest != target)
    redirect_edge_succ_nodup (e, target);

  return true;
}

/* Like force_nonfallthru below, but additionally performs redirection
   Used by redirect_edge_and_branch_force.  */

static basic_block
force_nonfallthru_and_redirect (e, target)
     edge e;
     basic_block target;
{
  basic_block jump_block, new_bb = NULL;
  rtx note;
  edge new_edge;

  if (e->flags & EDGE_ABNORMAL)
    abort ();
  else if (!(e->flags & EDGE_FALLTHRU))
    abort ();
  else if (e->src == ENTRY_BLOCK_PTR)
    {
      /* We can't redirect the entry block.  Create an empty block at the
         start of the function which we use to add the new jump.  */
      edge *pe1;
      basic_block bb = create_basic_block (0, e->dest->head, NULL);

      /* Change the existing edge's source to be the new block, and add
	 a new edge from the entry block to the new block.  */
      e->src = bb;
      bb->count = e->count;
      bb->frequency = EDGE_FREQUENCY (e);
      bb->loop_depth = 0;
      for (pe1 = &ENTRY_BLOCK_PTR->succ; *pe1; pe1 = &(*pe1)->succ_next)
	if (*pe1 == e)
	  {
	    *pe1 = e->succ_next;
	    break;
	  }
      e->succ_next = 0;
      bb->succ = e;
      make_single_succ_edge (ENTRY_BLOCK_PTR, bb, EDGE_FALLTHRU);
    }

  if (e->src->succ->succ_next)
    {
      /* Create the new structures.  */

      /* Position the new block correctly relative to loop notes.  */
      note = last_loop_beg_note (e->src->end);
      note = NEXT_INSN (note);

      /* ... and ADDR_VECs.  */
      if (note != NULL
	  && GET_CODE (note) == CODE_LABEL
	  && NEXT_INSN (note)
	  && GET_CODE (NEXT_INSN (note)) == JUMP_INSN
	  && (GET_CODE (PATTERN (NEXT_INSN (note))) == ADDR_DIFF_VEC
	      || GET_CODE (PATTERN (NEXT_INSN (note))) == ADDR_VEC))
	note = NEXT_INSN (NEXT_INSN (note));

      jump_block = create_basic_block (e->src->index + 1, note, NULL);
      jump_block->count = e->count;
      jump_block->frequency = EDGE_FREQUENCY (e);
      jump_block->loop_depth = target->loop_depth;

      if (target->global_live_at_start)
	{
	  jump_block->global_live_at_start
	    = OBSTACK_ALLOC_REG_SET (&flow_obstack);
	  jump_block->global_live_at_end
	    = OBSTACK_ALLOC_REG_SET (&flow_obstack);
	  COPY_REG_SET (jump_block->global_live_at_start,
			target->global_live_at_start);
	  COPY_REG_SET (jump_block->global_live_at_end,
			target->global_live_at_start);
	}

      /* Wire edge 