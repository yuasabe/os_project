/* Calculate branch probabilities, and basic block execution counts.
   Copyright (C) 1990, 1991, 1992, 1993, 1994, 1996, 1997, 1998, 1999,
   2000, 2001  Free Software Foundation, Inc.
   Contributed by James E. Wilson, UC Berkeley/Cygnus Support;
   based on some ideas from Dain Samples of UC Berkeley.
   Further mangling by Bob Manson, Cygnus Support.

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

/* ??? Register allocation should use basic block execution counts to
   give preference to the most commonly executed blocks.  */

/* ??? The .da files are not safe.  Changing the program after creating .da
   files or using different options when compiling with -fbranch-probabilities
   can result the arc data not matching the program.  Maybe add instrumented
   arc count to .bbg file?  Maybe check whether PFG matches the .bbg file?  */

/* ??? Should calculate branch probabilities before instrumenting code, since
   then we can use arc counts to help decide which arcs to instrument.  */

#include "config.h"
#include "system.h"
#include "rtl.h"
#include "tree.h"
#include "flags.h"
#include "insn-config.h"
#include "output.h"
#include "regs.h"
#include "expr.h"
#include "function.h"
#include "toplev.h"
#include "ggc.h"
#include "hard-reg-set.h"
#include "basic-block.h"
#include "gcov-io.h"
#include "target.h"

/* Additional information about the edges we need.  */
struct edge_info
  {
    unsigned int count_valid : 1;
    unsigned int on_tree : 1;
    unsigned int ignore : 1;
  };
struct bb_info
  {
    unsigned int count_valid : 1;
    gcov_type succ_count;
    gcov_type pred_count;
  };

#define EDGE_INFO(e)  ((struct edge_info *) (e)->aux)
#define BB_INFO(b)  ((struct bb_info *) (b)->aux)

/* Keep all basic block indexes nonnegative in the gcov output.  Index 0
   is used for entry block, last block exit block.  */
#define GCOV_INDEX_TO_BB(i)  ((i) == 0 ? ENTRY_BLOCK_PTR		¥
			      : (((i) == n_basic_blocks + 1)		¥
			         ? EXIT_BLOCK_PTR : BASIC_BLOCK ((i)-1)))
#define BB_TO_GCOV_INDEX(bb)  ((bb) == ENTRY_BLOCK_PTR ? 0		¥
			       : ((bb) == EXIT_BLOCK_PTR		¥
				  ? n_basic_blocks + 1 : (bb)->index + 1))

/* Name and file pointer of the output file for the basic block graph.  */

static FILE *bbg_file;

/* Name and file pointer of the input file for the arc count data.  */

static FILE *da_file;

/* Pointer of the output file for the basic block/line number map.  */
static FILE *bb_file;

/* Last source file name written to bb_file.  */

static char *last_bb_file_name;

/* Used by final, for allocating the proper amount of storage for the
   instrumented arc execution counts.  */

int count_instrumented_edges;

/* Collect statistics on the performance of this pass for the entire source
   file.  */

static int total_num_blocks;
static int total_num_edges;
static int total_num_edges_ignored;
static int total_num_edges_instrumented;
static int total_num_blocks_created;
static int total_num_passes;
static int total_num_times_called;
static int total_hist_br_prob[20];
static int total_num_never_executed;
static int total_num_branches;

/* Forward declarations.  */
static void find_spanning_tree PARAMS ((struct edge_list *));
static void init_edge_profiler PARAMS ((void));
static rtx gen_edge_profiler PARAMS ((int));
static void instrument_edges PARAMS ((struct edge_list *));
static void output_gcov_string PARAMS ((const char *, long));
static void compute_branch_probabilities PARAMS ((void));
static basic_block find_group PARAMS ((basic_block));
static void union_groups PARAMS ((basic_block, basic_block));

/* If non-zero, we need to output a constructor to set up the
   per-object-file data.  */
static int need_func_profiler = 0;

/* Add edge instrumentation code to the entire insn chain.

   F is the first insn of the chain.
   NUM_BLOCKS is the number of basic blocks found in F.  */

static void
instrument_edges (el)
     struct edge_list *el;
{
  int i;
  int num_instr_edges = 0;
  int num_edges = NUM_EDGES (el);
  remove_fake_edges ();

  for (i = 0; i < n_basic_blocks + 2; i++)
    {
      basic_block bb = GCOV_INDEX_TO_BB (i);
      edge e = bb->succ;
      while (e)
	{
	  struct edge_info *inf = EDGE_INFO (e);
	  if (!inf->ignore && !inf->on_tree)
	    {
	      if (e->flags & EDGE_ABNORMAL)
		abort ();
	      if (rtl_dump_file)
		fprintf (rtl_dump_file, "Edge %d to %d instrumented%s¥n",
			 e->src->index, e->dest->index,
			 EDGE_CRITICAL_P (e) ? " (and split)" : "");
	      need_func_profiler = 1;
	      insert_insn_on_edge (
			 gen_edge_profiler (total_num_edges_instrumented
					    + num_instr_edges++), e);
	    }
	  e = e->succ_next;
	}
    }

  total_num_edges_instrumented += num_instr_edges;
  count_instrumented_edges = total_num_edges_instrumented;

  total_num_blocks_created += num_edges;
  if (rtl_dump_file)
    fprintf (rtl_dump_file, "%d edges instrumented¥n", num_instr_edges);

  commit_edge_insertions ();
}

/* Output STRING to bb_file, surrounded by DELIMITER.  */

static void
output_gcov_string (string, delimiter)
     const char *string;
     long delimiter;
{
  long temp;

  /* Write a delimiter to indicate that a file name follows.  */
  __write_long (delimiter, bb_file, 4);

  /* Write the string.  */
  temp = strlen (string) + 1;
  fwrite (string, temp, 1, bb_file);

  /* Append a few zeros, to align the output to a 4 byte boundary.  */
  temp = temp & 0x3;
  if (temp)
    {
      char c[4];

      c[0] = c[1] = c[2] = c[3] = 0;
      fwrite (c, sizeof (char), 4 - temp, bb_file);
    }

  /* Store another delimiter in the .bb file, just to make it easy to find
     the end of the file name.  */
  __write_long (delimiter, bb_file, 4);
}


/* Compute the branch probabilities for the various branches.
   Annotate them accordingly.  */

static void
compute_branch_probabilities ()
{
  int i;
  int num_edges = 0;
  int changes;
  int passes;
  int hist_br_prob[20];
  int num_never_executed;
  int num_branches;

  /* Attach extra info block to each bb.  */

  alloc_aux_for_blocks (sizeof (struct bb_info));
  for (i = 0; i < n_basic_blocks + 2; i++)
    {
      basic_block bb = GCOV_INDEX_TO_BB (i);
      edge e;

      for (e = bb->succ; e; e = e->succ_next)
	if (!EDGE_INFO (e)->ignore)
	  BB_INFO (bb)->succ_count++;
      for (e = bb->pred; e; e = e->pred_next)
	if (!EDGE_INFO (e)->ignore)
	  BB_INFO (bb)->pred_count++;
    }

  /* Avoid predicting entry on exit nodes.  */
  BB_INFO (EXIT_BLOCK_PTR)->succ_count = 2;
  BB_INFO (ENTRY_BLOCK_PTR)->pred_count = 2;

  /* For each edge not on the spanning tree, set its execution count from
     the .da file.  */

  /* The first count in the .da file is the number of times that the function
     was entered.  This is the exec_count for block zero.  */

  for (i = 0; i < n_basic_blocks + 2; i++)
    {
      basic_block bb = GCOV_INDEX_TO_BB (i);
      edge e;
      for (e = bb->succ; e; e = e->succ_next)
	if (!EDGE_INFO (e)->ignore && !EDGE_INFO (e)->on_tree)
	  {
	    num_edges++;
	    if (da_file)
	      {
		gcov_type value;
		__read_gcov_type (&value, da_file, 8);
		e->count = value;
	      }
	    else
	      e->count = 0;
	    EDGE_INFO (e)->count_valid = 1;
	    BB_INFO (bb)->succ_count--;
	    BB_INFO (e->dest)->pred_count--;
	    if (rtl_dump_file)
	      {
		fprintf (rtl_dump_file, "¥nRead edge from %i to %i, count:",
			 bb->index, e->dest->index);
		fprintf (rtl_dump_file, HOST_WIDEST_INT_PRINT_DEC,
			 (HOST_WIDEST_INT) e->count);
	      }
	  }
    }

  if (rtl_dump_file)
    fprintf (rtl_dump_file, "¥n%d edge counts read¥n", num_edges);

  /* For every block in the file,
     - if every exit/entrance edge has a known count, then set the block count
     - if the block count is known, and every exit/entrance edge but one has
     a known execution count, then set the count of the remaining edge

     As edge counts are set, decrement the succ/pred count, but don't delete
     the edge, that way we can easily tell when all edges are known, or only
     one edge is unknown.  */

  /* The order that the basic blocks are iterated through is important.
     Since the code that finds spanning trees starts with block 0, low numbered
     edges are put on the spanning tree in preference to high numbered edges.
     Hence, most instrumented edges are at the end.  Graph solving works much
     faster if we propagate numbers from the end to the start.

     This takes an average of slightly more than 3 passes.  */

  changes = 1;
  passes = 0;
  while (changes)
    {
      passes++;
      changes = 0;
      for (i = n_basic_blocks + 1; i >= 0; i--)
	{
	  basic_block bb = GCOV_INDEX_TO_BB (i);
	  struct bb_info *bi = BB_INFO (bb);
	  if (! bi->count_valid)
	    {
	      if (bi->succ_count == 0)
		{
		  edge e;
		  gcov_type total = 0;

		  for (e = bb->succ; e; e = e->succ_next)
		    total += e->count;
		  bb->count = total;
		  bi->count_valid = 1;
		  changes = 1;
		}
	      else if (bi->pred_count == 0)
		{
		  edge e;
		  gcov_type total = 0;

		  for (e = bb->pred; e; e = e->pred_next)
		    total += e->count;
		  bb->count = total;
		  bi->count_valid = 1;
		  changes = 1;
		}
	    }
	  if (bi->count_valid)
	    {
	      if (bi->succ_count == 1)
		{
		  edge e;
		  gcov_type total = 0;

		  /* One of the counts will be invalid, but it is zero,
		     so adding it in also doesn't hurt.  */
		  for (e = bb->succ; e; e = e->succ_next)
		    total += e->count;

		  /* Seedgeh for the invalid edge, and set its count.  */
		  for (e = bb->succ; e; e = e->succ_next)
		    if (! EDGE_INFO (e)->count_valid && ! EDGE_INFO (e)->ignore)
		      break;

		  /* Calculate count for remaining edge by conservation.  */
		  total = bb->count - total;

		  if (! e)
		    abort ();
		  EDGE_INFO (e)->count_valid = 1;
		  e->count = total;
		  bi->succ_count--;

		  BB_INFO (e->dest)->pred_count--;
		  changes = 1;
		}
	      if (bi->pred_count == 1)
		{
		  edge e;
		  gcov_type total = 0;

		  /* One of the counts will be invalid, but it is zero,
		     so adding it in also doesn't hurt.  */
		  for (e = bb->pred; e; e = e->pred_next)
		    total += e->count;

		  /* Seedgeh for the invalid edge, and set its count.  */
		  for (e = bb->pred; e; e = e->pred_next)
		    if (! EDGE_INFO (e)->count_valid && ! EDGE_INFO (e)->ignore)
		      break;

		  /* Calculate count for remaining edge by conservation.  */
		  total = bb->count - total + e->count;

		  if (! e)
		    abort ();
		  EDGE_INFO (e)->count_valid = 1;
		  e->count = total;
		  bi->pred_count--;

		  BB_INFO (e->src)->succ_count--;
		  changes = 1;
		}
	    }
	}
    }
  if (rtl_dump_file)
    dump_flow_info (rtl_dump_file);

  total_num_passes += passes;
  if (rtl_dump_file)
    fprintf (rtl_dump_file, "Graph solving took %d passes.¥n¥n", passes);

  /* If the graph has been correctly solved, every block will have a
     succ and pred count of zero.  */
  for (i = 0; i < n_basic_blocks; i++)
    {
      basic_block bb = BASIC_BLOCK (i);
      if (BB_INFO (bb)->succ_count || BB_INFO (bb)->pred_count)
	abort ();
    }

  /* For every edge, calculate its branch probability and add a reg_note
     to the branch insn to indicate this.  */

  for (i = 0; i < 20; i++)
    hist_br_prob[i] = 0;
  num_never_executed =