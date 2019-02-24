/* Control flow graph analysis code for GNU compiler.
   Copyright (C) 1987, 1988, 1992, 1993, 1994, 1995, 1996, 1997, 1998,
   1999, 2000, 2001 Free Software Foundation, Inc.

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

/* !kawai! */
/* This file contains various simple utilities to analyze the CFG.  */
#include "config.h"
#include "system.h"
#include "rtl.h"
#include "hard-reg-set.h"
#include "basic-block.h"
#include "insn-config.h"
#include "recog.h"
#include "toplev.h"
#include "../include/obstack.h"
#include "tm_p.h"
/* end of !kawai! */

/* Store the data structures necessary for depth-first search.  */
struct depth_first_search_dsS {
  /* stack for backtracking during the algorithm */
  basic_block *stack;

  /* number of edges in the stack.  That is, positions 0, ..., sp-1
     have edges.  */
  unsigned int sp;

  /* record of basic blocks already seen by depth-first search */
  sbitmap visited_blocks;
};
typedef struct depth_first_search_dsS *depth_first_search_ds;

static void flow_dfs_compute_reverse_init
  PARAMS ((depth_first_search_ds));
static void flow_dfs_compute_reverse_add_bb
  PARAMS ((depth_first_search_ds, basic_block));
static basic_block flow_dfs_compute_reverse_execute
  PARAMS ((depth_first_search_ds));
static void flow_dfs_compute_reverse_finish
  PARAMS ((depth_first_search_ds));
static void remove_fake_successors	PARAMS ((basic_block));
static bool need_fake_edge_p		PARAMS ((rtx));
static bool keep_with_call_p		PARAMS ((rtx));

/* Return true if the block has no effect and only forwards control flow to
   its single destination.  */

bool
forwarder_block_p (bb)
     basic_block bb;
{
  rtx insn;

  if (bb == EXIT_BLOCK_PTR || bb == ENTRY_BLOCK_PTR
      || !bb->succ || bb->succ->succ_next)
    return false;

  for (insn = bb->head; insn != bb->end; insn = NEXT_INSN (insn))
    if (INSN_P (insn) && active_insn_p (insn))
      return false;

  return (!INSN_P (insn)
	  || (GET_CODE (insn) == JUMP_INSN && simplejump_p (insn))
	  || !active_insn_p (insn));
}

/* Return nonzero if we can reach target from src by falling through.  */

bool
can_fallthru (src, target)
     basic_block src, target;
{
  rtx insn = src->end;
  rtx insn2 = target->head;

  if (src->index + 1 == target->index && !active_insn_p (insn2))
    insn2 = next_active_insn (insn2);

  /* ??? Later we may add code to move jump tables offline.  */
  return next_active_insn (insn) == insn2;
}

/* Mark the back edges in DFS traversal.
   Return non-zero if a loop (natural or otherwise) is present.
   Inspired by Depth_First_Search_PP described in:

     Advanced Compiler Design and Implementation
     Steven Muchnick
     Morgan Kaufmann, 1997

   and heavily borrowed from flow_depth_first_order_compute.  */

bool
mark_dfs_back_edges ()
{
  edge *stack;
  int *pre;
  int *post;
  int sp;
  int prenum = 1;
  int postnum = 1;
  sbitmap visited;
  bool found = false;

  /* Allocate the preorder and postorder number arrays.  */
  pre = (int *) xcalloc (n_basic_blocks, sizeof (int));
  post = (int *) xcalloc (n_basic_blocks, sizeof (int));

  /* Allocate stack for back-tracking up CFG.  */
  stack = (edge *) xmalloc ((n_basic_blocks + 1) * sizeof (edge));
  sp = 0;

  /* Allocate bitmap to track nodes that have been visited.  */
  visited = sbitmap_alloc (n_basic_blocks);

  /* None of the nodes in the CFG have been visited yet.  */
  sbitmap_zero (visited);

  /* Push the first edge on to the stack.  */
  stack[sp++] = ENTRY_BLOCK_PTR->succ;

  while (sp)
    {
      edge e;
      basic_block src;
      basic_block dest;

      /* Look at the edge on the top of the stack.  */
      e = stack[sp - 1];
      src = e->src;
      dest = e->dest;
      e->flags &= ‾EDGE_DFS_BACK;

      /* Check if the edge destination has been visited yet.  */
      if (dest != EXIT_BLOCK_PTR && ! TEST_BIT (visited, dest->index))
	{
	  /* Mark that we have visited the destination.  */
	  SET_BIT (visited, dest->index);

	  pre[dest->index] = prenum++;
	  if (dest->succ)
	    {
	      /* Since the DEST node has been visited for the first
		 time, check its successors.  */
	      stack[sp++] = dest->succ;
	    }
	  else
	    post[dest->index] = postnum++;
	}
      else
	{
	  if (dest != EXIT_BLOCK_PTR && src != ENTRY_BLOCK_PTR
	      && pre[src->index] >= pre[dest->index]
	      && post[dest->index] == 0)
	    e->flags |= EDGE_DFS_BACK, found = true;

	  if (! e->succ_next && src != ENTRY_BLOCK_PTR)
	    post[src->index] = postnum++;

	  if (e->succ_next)
	    stack[sp - 1] = e->succ_next;
	  else
	    sp--;
	}
    }

  free (pre);
  free (post);
  free (stack);
  sbitmap_free (visited);

  return found;
}

/* Return true if we need to add fake edge to exit.
   Helper function for the flow_call_edges_add.  */

static bool
need_fake_edge_p (insn)
     rtx insn;
{
  if (!INSN_P (insn))
    return false;

  if ((GET_CODE (insn) == CALL_INSN
       && !SIBLING_CALL_P (insn)
       && !find_reg_note (insn, REG_NORETURN, NULL)
       && !find_reg_note (insn, REG_ALWAYS_RETURN, NULL)
       && !CONST_OR_PURE_CALL_P (insn)))
    return true;

  return ((GET_CODE (PATTERN (insn)) == ASM_OPERANDS
	   && MEM_VOLATILE_P (PATTERN (insn)))
	  || (GET_CODE (PATTERN (insn)) == PARALLEL
	      && asm_noperands (insn) != -1
	      && MEM_VOLATILE_P (XVECEXP (PATTERN (insn), 0, 0)))
	  || GET_CODE (PATTERN (insn)) == ASM_INPUT);
}

/* Return true if INSN should be kept in the same block as a preceding call.
   This is done for a single-set whose destination is a fixed register or
   whose source is the function return value.  This is a helper function for
   flow_call_edges_add.  */

static bool
keep_with_call_p (insn)
     rtx insn;
{
  rtx set;

  if (INSN_P (insn) && (set = single_set (insn)) != NULL)
    {
      if (GET_CODE (SET_DEST (set)) == REG
	  && fixed_regs[REGNO (SET_DEST (set))]
	  && general_operand (SET_SRC (set), VOIDmode))
	return true;
      if (GET_CODE (SET_SRC (set)) == REG
	  && FUNCTION_VALUE_REGNO_P (REGNO (SET_SRC (set)))
	  && GET_CODE (SET_DEST (set)) == REG
	  && REGNO (SET_DEST (set)) >= FIRST_PSEUDO_REGISTER)
	return true;
    }
  return false;
}

/* Add fake edges to the function exit for any non constant and non noreturn
   calls, volatile inline assembly in the bitmap of blocks specified by
   BLOCKS or to the whole CFG if BLOCKS is zero.  Return the number of blocks
   that were split.

   The goal is to expose cases in which entering a basic block does not imply
   that all subsequent instructions must be executed.  */

int
flow_call_edges_add (blocks)
     sbitmap blocks;
{
  int i;
  int blocks_split = 0;
  int bb_num = 0;
  basic_block *bbs;
  bool check_last_block = false;

  /* Map bb indices into basic block pointers since split_block
     will renumber the basic blocks.  */

  bbs = xmalloc (n_basic_blocks * sizeof (*bbs));

  if (! blocks)
    {
      for (i = 0; i < n_basic_blocks; i++)
	bbs[bb_num++] = BASIC_BLOCK (i);

      check_last_block = true;
    }
  else
    EXECUTE_IF_SET_IN_SBITMAP (blocks, 0, i,
			       {
				 bbs[bb_num++] = BASIC_BLOCK (i);
				 if (i == n_basic_blocks - 1)
				   check_last_block = true;
			       });

  /* In the last basic block, before epilogue generation, there will be
     a fallthru edge to EXIT.  Special care is required if the last insn
     of the last basic block is a call because make_edge folds duplicate
     edges, which would result in the fallthru edge also being marked
     fake, which would result in the fallthru edge being removed by
     remove_fake_edges, which would result in an invalid CFG.

     Moreover, we can't elide the outgoing fake edge, since the block
     profiler needs to take this into account in order to solve the minimal
     spanning tree in the case that the call doesn't return.

     Handle this by adding a dummy instruction in a new last basic block.  */
  if (check_last_block)
    {
      basic_block bb = BASIC_BLOCK (n_basic_blocks - 1);
      rtx insn = bb->end;

      /* Back up past insns that must be kept in the same block as a call.  */
      while (insn != bb->head
	     && keep_with_call_p (insn))
	insn = PREV_INSN (insn);

      if (need_fake_edge_p (insn))
	{
	  edge e;

	  for (e = bb->succ; e; e = e->succ_next)
	    if (e->dest == EXIT_BLOCK_PTR)
	      break;

	  insert_insn_on_edge (gen_rtx_USE (VOIDmode, const0_rtx), e);
	  commit_edge_insertions ();
	}
    }

  /* Now add fake edges to the function exit for any non constant
     calls since there is no way that we can determine if they will
     return or not...  */

  for (i = 0; i < bb_num; i++)
    {
      basic_block bb = bbs[i];
      rtx insn;
      rtx prev_insn;

      for (insn = bb->end; ; insn = prev_insn)
	{
	  prev_insn = PREV_INSN (insn);
	  if (need_fake_edge_p (insn))
	    {
	      edge e;
	      rtx split_at_insn = insn;

	      /* Don't split the block between a call and an insn that should
	         remain in the same block as the call.  */
	      if (GET_CODE (insn) == CALL_INSN)
		while (split_at_insn != bb->end
		       && keep_with_call_p (NEXT_INSN (split_at_insn)))
		  split_at_insn = NEXT_INSN (split_at_insn);

	      /* The handling above of the final block before the epilogue
	         should be enough to verify that there is no edge to the exit
		 block in CFG already.  Calling make_edge in such case would
		 cause us to mark that edge as fake and remove it later.  */

#ifdef ENABLE_CHECKING
	      if (split_at_insn == bb->end)
		for (e = bb->succ; e; e = e->succ_next)
		  if (e->dest == EXIT_BLOCK_PTR)
		    abort ();
#endif

	      /* Note that the following may create a new basic block
		 and renumber the existing basic blocks.  */
	      e = split_block (bb, split_at_insn);
	      if (e)
		blocks_split++;

	      make_edge (bb, EXIT_BLOCK_PTR, EDGE_FAKE);
	    }

	  if (insn == bb->head)
	    break;
	}
    }

  if (blocks_split)
    verify_flow_info ();

  free (bbs);
  return blocks_split;
}

/* Find unreachable blocks.  An unreachable block will have 0 in
   the reachable bit in block->flags.  A non-zero value indicates the
   block is reachable.  */

void
find_unreachable_blocks ()
{
  edge e;
  int i, n;
  basic_block *tos, *worklist;

  n = n_basic_blocks;
  tos = worklist = (basic_block *) xmalloc (sizeof (basic_block) * n);

  /* Clear all the reachability flags.  */

  for (i = 0; i < n; ++i)
    BASIC_BLOCK (i)->flags &= ‾BB_REACHABLE;

  /* Add our starting points to the worklist.  Almost always there will
     be only one.  It isn't inconceivable that we might one day directly
     support Fortran alternate entry points.  */

  for (e = ENTRY_BLOCK_PTR->succ; e; e = e->succ_next)
    {
      *tos++ = e->dest;

      /* Mark the block reachable.  */
      e->dest->flags |= BB_REACHABLE;
    }

  /* Iterate: find everything reachable from what we've already seen.  */

  while (tos != worklist)
    {
      basic_block b = *--tos;

      for (e = b->succ; e; e = e->succ_next)
	if (!(e->dest->flags & BB_REACHABLE))
	  {
	    *tos++ = e->dest;
	    e->dest->flags |= BB_REACHABLE;
	  }
    }

  free (worklist);
}

/* Functions to access an edge list with a vector representation.
   Enough data is kept such that given an index number, the
   pred a