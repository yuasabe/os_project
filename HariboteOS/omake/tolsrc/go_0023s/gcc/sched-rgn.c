/* Instruction scheduling pass.
   Copyright (C) 1992, 1993, 1994, 1995, 1996, 1997, 1998,
   1999, 2000, 2001, 2002 Free Software Foundation, Inc.
   Contributed by Michael Tiemann (tiemann@cygnus.com) Enhanced by,
   and currently maintained by, Jim Wilson (wilson@cygnus.com)

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

/* This pass implements list scheduling within basic blocks.  It is
   run twice: (1) after flow analysis, but before register allocation,
   and (2) after register allocation.

   The first run performs interblock scheduling, moving insns between
   different blocks in the same "region", and the second runs only
   basic block scheduling.

   Interblock motions performed are useful motions and speculative
   motions, including speculative loads.  Motions requiring code
   duplication are not supported.  The identification of motion type
   and the check for validity of speculative motions requires
   construction and analysis of the function's control flow graph.

   The main entry point for this pass is schedule_insns(), called for
   each function.  The work of the scheduler is organized in three
   levels: (1) function level: insns are subject to splitting,
   control-flow-graph is constructed, regions are computed (after
   reload, each region is of one block), (2) region level: control
   flow graph attributes required for interblock scheduling are
   computed (dominators, reachability, etc.), data dependences and
   priorities are computed, and (3) block level: insns in the block
   are actually scheduled.  */

#include "config.h"
#include "system.h"
#include "toplev.h"
#include "rtl.h"
#include "tm_p.h"
#include "hard-reg-set.h"
#include "basic-block.h"
#include "regs.h"
#include "function.h"
#include "flags.h"
#include "insn-config.h"
#include "insn-attr.h"
#include "except.h"
#include "toplev.h"
#include "recog.h"
#include "cfglayout.h"
#include "sched-int.h"

/* Define when we want to do count REG_DEAD notes before and after scheduling
   for sanity checking.  We can't do that when conditional execution is used,
   as REG_DEAD exist only for unconditional deaths.  */

#if !defined (HAVE_conditional_execution) && defined (ENABLE_CHECKING)
#define CHECK_DEAD_NOTES 1
#else
#define CHECK_DEAD_NOTES 0
#endif


#ifdef INSN_SCHEDULING
/* Some accessor macros for h_i_d members only used within this file.  */
#define INSN_REF_COUNT(INSN)	(h_i_d[INSN_UID (INSN)].ref_count)
#define FED_BY_SPEC_LOAD(insn)	(h_i_d[INSN_UID (insn)].fed_by_spec_load)
#define IS_LOAD_INSN(insn)	(h_i_d[INSN_UID (insn)].is_load_insn)

#define MAX_RGN_BLOCKS 10
#define MAX_RGN_INSNS 100

/* nr_inter/spec counts interblock/speculative motion for the function.  */
static int nr_inter, nr_spec;

/* Control flow graph edges are kept in circular lists.  */
typedef struct
{
  int from_block;
  int to_block;
  int next_in;
  int next_out;
}
haifa_edge;
static haifa_edge *edge_table;

#define NEXT_IN(edge) (edge_table[edge].next_in)
#define NEXT_OUT(edge) (edge_table[edge].next_out)
#define FROM_BLOCK(edge) (edge_table[edge].from_block)
#define TO_BLOCK(edge) (edge_table[edge].to_block)

/* Number of edges in the control flow graph.  (In fact, larger than
   that by 1, since edge 0 is unused.)  */
static int nr_edges;

/* Circular list of incoming/outgoing edges of a block.  */
static int *in_edges;
static int *out_edges;

#define IN_EDGES(block) (in_edges[block])
#define OUT_EDGES(block) (out_edges[block])

static int is_cfg_nonregular PARAMS ((void));
static int build_control_flow PARAMS ((struct edge_list *));
static void new_edge PARAMS ((int, int));

/* A region is the main entity for interblock scheduling: insns
   are allowed to move between blocks in the same region, along
   control flow graph edges, in the 'up' direction.  */
typedef struct
{
  int rgn_nr_blocks;		/* Number of blocks in region.  */
  int rgn_blocks;		/* cblocks in the region (actually index in rgn_bb_table).  */
}
region;

/* Number of regions in the procedure.  */
static int nr_regions;

/* Table of region descriptions.  */
static region *rgn_table;

/* Array of lists of regions' blocks.  */
static int *rgn_bb_table;

/* Topological order of blocks in the region (if b2 is reachable from
   b1, block_to_bb[b2] > block_to_bb[b1]).  Note: A basic block is
   always referred to by either block or b, while its topological
   order name (in the region) is refered to by bb.  */
static int *block_to_bb;

/* The number of the region containing a block.  */
static int *containing_rgn;

#define RGN_NR_BLOCKS(rgn) (rgn_table[rgn].rgn_nr_blocks)
#define RGN_BLOCKS(rgn) (rgn_table[rgn].rgn_blocks)
#define BLOCK_TO_BB(block) (block_to_bb[block])
#define CONTAINING_RGN(block) (containing_rgn[block])

void debug_regions PARAMS ((void));
static void find_single_block_region PARAMS ((void));
static void find_rgns PARAMS ((struct edge_list *, sbitmap *));
static int too_large PARAMS ((int, int *, int *));

extern void debug_live PARAMS ((int, int));

/* Blocks of the current region being scheduled.  */
static int current_nr_blocks;
static int current_blocks;

/* The mapping from bb to block.  */
#define BB_TO_BLOCK(bb) (rgn_bb_table[current_blocks + (bb)])

typedef struct
{
  int *first_member;		/* Pointer to the list start in bitlst_table.  */
  int nr_members;		/* The number of members of the bit list.  */
}
bitlst;

static int bitlst_table_last;
static int bitlst_table_size;
static int *bitlst_table;

static void extract_bitlst PARAMS ((sbitmap, bitlst *));

/* Target info declarations.

   The block currently being scheduled is referred to as the "target" block,
   while other blocks in the region from which insns can be moved to the
   target are called "source" blocks.  The candidate structure holds info
   about such sources: are they valid?  Speculative?  Etc.  */
typedef bitlst bblst;
typedef struct
{
  char is_valid;
  char is_speculative;
  int src_prob;
  bblst split_bbs;
  bblst update_bbs;
}
candidate;

static candidate *candidate_table;

/* A speculative motion requires checking live information on the path
   from 'source' to 'target'.  The split blocks are those to be checked.
   After a speculative motion, live information should be modified in
   the 'update' blocks.

   Lists of split and update blocks for each candidate of the current
   target are in array bblst_table.  */
static int *bblst_table, bblst_size, bblst_last;

#define IS_VALID(src) ( candidate_table[src].is_valid )
#define IS_SPECULATIVE(src) ( candidate_table[src].is_speculative )
#define SRC_PROB(src) ( candidate_table[src].src_prob )

/* The bb being currently scheduled.  */
static int target_bb;

/* List of edges.  */
typedef bitlst edgelst;

/* Target info functions.  */
static void split_edges PARAMS ((int, int, edgelst *));
static void compute_trg_info PARAMS ((int));
void debug_candidate PARAMS ((int));
void debug_candidates PARAMS ((int));

/* Dominators array: dom[i] contains the sbitmap of dominators of
   bb i in the region.  */
static sbitmap *dom;

/* bb 0 is the only region entry.  */
#define IS_RGN_ENTRY(bb) (!bb)

/* Is bb_src dominated by bb_trg.  */
#define IS_DOMINATED(bb_src, bb_trg)                                 ¥
( TEST_BIT (dom[bb_src], bb_trg) )

/* Probability: Prob[i] is a float in [0, 1] which is the probability
   of bb i relative to the region entry.  */
static float *prob;

/* The probability of bb_src, relative to bb_trg.  Note, that while the
   'prob[bb]' is a float in [0, 1], this macro returns an integer
   in [0, 100].  */
#define GET_SRC_PROB(bb_src, bb_trg) ((int) (100.0 * (prob[bb_src] / ¥
						      prob[bb_trg])))

/* Bit-set of edges, where bit i stands for edge i.  */
typedef sbitmap edgeset;

/* Number of edges in the region.  */
static int rgn_nr_edges;

/* Array of size rgn_nr_edges.  */
static int *rgn_edges;


/* Mapping from each edge in the graph to its number in the rgn.  */
static int *edge_to_bit;
#define EDGE_TO_BIT(edge) (edge_to_bit[edge])

/* The split edges of a source bb is different for each target
   bb.  In order to compute this efficiently, the 'potential-split edges'
   are computed for each bb prior to scheduling a region.  This is actually
   the split edges of each bb relative to the region entry.

   pot_split[bb] is the set of potential split edges of bb.  */
static edgeset *pot_split;

/* For every bb, a set of its ancestor edges.  */
static edgeset *ancestor_edges;

static void compute_dom_prob_ps PARAMS ((int));

#define ABS_VALUE(x) (((x)<0)?(-(x)):(x))
#define INSN_PROBABILITY(INSN) (SRC_PROB (BLOCK_TO_BB (BLOCK_NUM (INSN))))
#define IS_SPECULATIVE_INSN(INSN) (IS_SPECULATIVE (BLOCK_TO_BB (BLOCK_NUM (INSN))))
#define INSN_BB(INSN) (BLOCK_TO_BB (BLOCK_NUM (INSN)))

/* Parameters affecting the decision of rank_for_schedule().
   ??? Nope.  But MIN_PROBABILITY is used in copmute_trg_info.  */
#define MIN_DIFF_PRIORITY 2
#define MIN_PROBABILITY 40
#define MIN_PROB_DIFF 10

/* Speculative scheduling functions.  */
static int check_live_1 PARAMS ((int, rtx));
static void update_live_1 PARAMS ((int, rtx));
static int check_live PARAMS ((rtx, int));
static void update_live PARAMS ((rtx, int));
static void set_spec_fed PARAMS ((rtx));
static int is_pfree PARAMS ((rtx, int, int));
static int find_conditional_protection PARAMS ((rtx, int));
static int is_conditionally_protected PARAMS ((rtx, int, int));
static int may_trap_exp PARAMS ((rtx, int));
static int haifa_classify_insn PARAMS ((rtx));
static int is_prisky PARAMS ((rtx, int, int));
static int is_exception_free PARAMS ((rtx, int, int));

static bool sets_likely_spilled PARAMS ((rtx));
static void sets_likely_spilled_1 PARAMS ((rtx, rtx, void *));
static void add_branch_dependences PARAMS ((rtx, rtx));
static void compute_block_backward_dependences PARAMS ((int));
void debug_dependencies PARAMS ((void));

static void init_regions PARAMS ((void));
static void schedule_region PARAMS ((int));
static rtx concat_INSN_LIST PARAMS ((rtx, rtx));
static void concat_insn_mem_list PARAMS ((rtx, rtx, rtx *, rtx *));
static void propagate_deps PARAMS ((int, struct deps *));
static void free_pending_lists PARAMS ((void));

/* Functions for construction of the control flow graph.  */

/* Return 1 if control flow graph should not be constructed, 0 otherwise.

   We decide not to build the control flow graph if there is possibly more
   than one entry to the function, if computed branches exist, of if we
   have nonlocal gotos.  */

static int
is_cfg_nonregular ()
{
  int b;
  rtx insn;
  RTX_CODE code;

  /* If we have a label that could be the target of a nonlocal goto, then
     the cfg is not well structured.  */
  if (nonlocal_goto_handler_labels)
    return 1;

  /* If we have any forced labels, then the cfg is not well structured.  */
  if (forced_labels)
    return 1;

  /* If this function has a computed jump, then we consider the cfg
     not well structured.  */
  if (current_function_has_computed_jump)
    return 1;

  /* If we have exception handlers, then we consider the cfg not well
     structured.  ?!?  We should be able to handle this now that flow.c
     computes an accurate cfg for EH.  */
  if (current_function_has_exception_handlers ())
    return 1;

  /* If we have non-jumping insns which refer to labels, then we consider
     the cfg not well structured.  */
  /* Check for labels referred to other thn by jumps.  */
  for (b = 0; b < n_basic_blocks; b++)
    for (insn = BLOCK_HEAD (b);; insn = NEXT_INSN (insn))
      {
	code = GET_CODE (insn);
	if (GET_RTX_CLASS (code) == 'i' && code != JUMP_INSN)
	  {
	    rtx note = find_reg_note (insn, REG_LABEL, NULL_RTX);

	    if (note
		&& ! (GET_CODE (NEXT_INSN (insn)) == JUMP_INSN
		      && find_reg_note (NEXT_INSN (insn), REG_LABEL,
					XEXP (note, 0))))
	      return 1;
	  }

	if (insn == BLOCK_END (b))
	  break;
      }

  /* All the tests passed.  Consider the cfg well structured.  */
  return 0;
}

/* Build the control flow graph and set nr_edges.

   Instead of trying to build a cfg ourselves, we rely on flow to
   do it for us.  Stamp out useless code (and bug) duplication.

   Return nonzero if an irregularity in the cfg is found which would
   prevent cross block scheduling.  */

static int
build_control_flow (edge_list)
     struct edge_list *edge_list;
{
  int i, unreachable, num_edges;

  /* This already accounts for entry/exit edges.  */
  num_edges = NUM_EDGES (edge_list);

  /* Unreachable loops with more than one basic block are detected
     during the DFS traversal in find_rgns.

     Unreachable loops with a single block are detected here.  This
     test is redundant with the one in find_rgns, but it's much
    cheaper to go ahead and catch the trivial case here.  */
  unreachable = 0;
  for (i = 0; i < n_basic_blocks; i++)
    {
      basic_block b = BASIC_BLOCK (i);

      if (b->pred == NULL
	  || (b->pred->src == b
	      && b->pred->pred_next == NULL))
	unreachable = 1;
    }

  /* ??? We can kill these soon.  */
  in_edges = (int *) xcalloc (n_basic_blocks, sizeof (int));
  out_edges = (int *) xcalloc (n_basic_blocks, sizeof (int));
  edge_table = (haifa_edge *) xcalloc (num_edges, sizeof (haifa_edge));

  nr_edges = 0;
  for (i = 0; i < num_edges; i++)
    {
      edge e = INDEX_EDGE (edge_list, i);

      if (e->dest != EXIT_BLOCK_PTR
	  && e->src != ENTRY_BLOCK_PTR)
	new_edge (e->src->index, e->dest->index);
    }

  /* Increment by 1, since edge 0 is unused.  */
  nr_edges++;

  return unreachable;
}

/* Record an edge in the control flow graph from SOURCE to TARGET.

   In theory, this is redundant with the s_succs computed above, but
   we have not converted all of haifa to use information from the
   integer lists.  */

static void
new_edge (source, target)
     int source, target;
{
  int e, next_edge;
  int curr_edge, fst_edge;

  /* Check for duplicates.  */
  fst_edge = curr_edge = OUT_EDGES (source);
  while (curr_edge)
    {
      if (FROM_BLOCK (curr_edge) == source
	  && TO_BLOCK (curr_edge) == target)
	{
	  return;
	}

      curr_edge = NEXT_OUT (curr_edge);

      if (fst_edge == curr_edge)
	break;
    }

  e = ++nr_edges;

  FROM_BLOCK (e) = source;
  TO_BLOCK (e) = target;

  if (OUT_EDGES (source))
    {
      next_edge = NEXT_OUT (OUT_EDGES (source));
      NEXT_OUT (OUT_EDGES (source)) = e;
      NEXT_OUT (e) = next_edge;
    }
  else
    {
      OUT_EDGES (source) = e;
      NEXT_OUT (e) = e;
    }

  if (IN_EDGES (target))
    {
      next_edge = NEXT_IN (IN_EDGES (target));
      NEXT_IN (IN_EDGES (target)) = e;
      NEXT_IN (e) = next_edge;
    }
  else
    {
      IN_EDGES (target) = e;
      NEXT_IN (e) = e;
    }
}

/* Translate a bit-set SET to a list BL of the bit-set members.  */

static void
extract_bitlst (set, bl)
     sbitmap set;
     bitlst *bl;
{
  int i;

  /* bblst table space is reused in each call to extract_bitlst.  */
  bitlst_table_last = 0;

  bl->first_member = &bitlst_table[bitlst_table_last];
  bl->nr_members = 0;

  /* Iterate over each word in the bitset.  */
  EXECUTE_IF_SET_IN_SBITMAP (set, 0, i,
  {
    bitlst_table[bitlst_table_last++] = i;
    (bl->nr_members)++;
  });

}

/* Functions for the construction of regions.  */

/* Print the regions, for debugging pu