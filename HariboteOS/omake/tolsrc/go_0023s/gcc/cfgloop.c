/* Natural loop discovery code for GNU compiler.
   Copyright (C) 2000, 2001 Free Software Foundation, Inc.

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
#include "hard-reg-set.h"
#include "basic-block.h"

static void flow_loops_cfg_dump		PARAMS ((const struct loops *,
						 FILE *));
static int flow_loop_nested_p		PARAMS ((struct loop *,
						 struct loop *));
static int flow_loop_entry_edges_find	PARAMS ((basic_block, const sbitmap,
						 edge **));
static int flow_loop_exit_edges_find	PARAMS ((const sbitmap, edge **));
static int flow_loop_nodes_find		PARAMS ((basic_block, basic_block,
						 sbitmap));
static void flow_loop_pre_header_scan	PARAMS ((struct loop *));
static basic_block flow_loop_pre_header_find PARAMS ((basic_block,
						      const sbitmap *));
static void flow_loop_tree_node_add	PARAMS ((struct loop *,
						 struct loop *));
static void flow_loops_tree_build	PARAMS ((struct loops *));
static int flow_loop_level_compute	PARAMS ((struct loop *, int));
static int flow_loops_level_compute	PARAMS ((struct loops *));

/* Dump loop related CFG information.  */

static void
flow_loops_cfg_dump (loops, file)
     const struct loops *loops;
     FILE *file;
{
  int i;

  if (! loops->num || ! file || ! loops->cfg.dom)
    return;

  for (i = 0; i < n_basic_blocks; i++)
    {
      edge succ;

      fprintf (file, ";; %d succs { ", i);
      for (succ = BASIC_BLOCK (i)->succ; succ; succ = succ->succ_next)
	fprintf (file, "%d ", succ->dest->index);
      flow_nodes_print ("} dom", loops->cfg.dom[i], file);
    }

  /* Dump the DFS node order.  */
  if (loops->cfg.dfs_order)
    {
      fputs (";; DFS order: ", file);
      for (i = 0; i < n_basic_blocks; i++)
	fprintf (file, "%d ", loops->cfg.dfs_order[i]);

      fputs ("¥n", file);
    }

  /* Dump the reverse completion node order.  */
  if (loops->cfg.rc_order)
    {
      fputs (";; RC order: ", file);
      for (i = 0; i < n_basic_blocks; i++)
	fprintf (file, "%d ", loops->cfg.rc_order[i]);

      fputs ("¥n", file);
    }
}

/* Return non-zero if the nodes of LOOP are a subset of OUTER.  */

static int
flow_loop_nested_p (outer, loop)
     struct loop *outer;
     struct loop *loop;
{
  return sbitmap_a_subset_b_p (loop->nodes, outer->nodes);
}

/* Dump the loop information specified by LOOP to the stream FILE
   using auxiliary dump callback function LOOP_DUMP_AUX if non null.  */

void
flow_loop_dump (loop, file, loop_dump_aux, verbose)
     const struct loop *loop;
     FILE *file;
     void (*loop_dump_aux) PARAMS((const struct loop *, FILE *, int));
     int verbose;
{
  if (! loop || ! loop->header)
    return;

  if (loop->first->head && loop->last->end)
    fprintf (file, ";;¥n;; Loop %d (%d to %d):%s%s¥n",
	    loop->num, INSN_UID (loop->first->head),
	    INSN_UID (loop->last->end),
	    loop->shared ? " shared" : "", loop->invalid ? " invalid" : "");
  else
    fprintf (file, ";;¥n;; Loop %d:%s%s¥n", loop->num,
	     loop->shared ? " shared" : "", loop->invalid ? " invalid" : "");

  fprintf (file, ";;  header %d, latch %d, pre-header %d, first %d, last %d¥n",
	   loop->header->index, loop->latch->index,
	   loop->pre_header ? loop->pre_header->index : -1,
	   loop->first->index, loop->last->index);
  fprintf (file, ";;  depth %d, level %d, outer %ld¥n",
	   loop->depth, loop->level,
	   (long) (loop->outer ? loop->outer->num : -1));

  if (loop->pre_header_edges)
    flow_edge_list_print (";;  pre-header edges", loop->pre_header_edges,
			  loop->num_pre_header_edges, file);

  flow_edge_list_print (";;  entry edges", loop->entry_edges,
			loop->num_entries, file);
  fprintf (file, ";;  %d", loop->num_nodes);
  flow_nodes_print (" nodes", loop->nodes, file);
  flow_edge_list_print (";;  exit edges", loop->exit_edges,
			loop->num_exits, file);

  if (loop->exits_doms)
    flow_nodes_print (";;  exit doms", loop->exits_doms, file);

  if (loop_dump_aux)
    loop_dump_aux (loop, file, verbose);
}

/* Dump the loop information specified by LOOPS to the stream FILE,
   using auxiliary dump callback function LOOP_DUMP_AUX if non null.  */

void
flow_loops_dump (loops, file, loop_dump_aux, verbose)
     const struct loops *loops;
     FILE *file;
     void (*loop_dump_aux) PARAMS((const struct loop *, FILE *, int));
     int verbose;
{
  int i, j;
  int num_loops;

  num_loops = loops->num;
  if (! num_loops || ! file)
    return;

  fprintf (file, ";; %d loops found, %d levels¥n", num_loops, loops->levels);
  for (i = 0; i < num_loops; i++)
    {
      struct loop *loop = &loops->array[i];

      flow_loop_dump (loop, file, loop_dump_aux, verbose);
      if (loop->shared)
	for (j = 0; j < i; j++)
	  {
	    struct loop *oloop = &loops->array[j];

	    if (loop->header == oloop->header)
	      {
		int disjoint;
		int smaller;

		smaller = loop->num_nodes < oloop->num_nodes;

		/* If the union of LOOP and OLOOP is different than
		   the larger of LOOP and OLOOP then LOOP and OLOOP
		   must be disjoint.  */
		disjoint = ! flow_loop_nested_p (smaller ? loop : oloop,
						 smaller ? oloop : loop);
		fprintf (file,
			 ";; loop header %d shared by loops %d, %d %s¥n",
			 loop->header->index, i, j,
			 disjoint ? "disjoint" : "nested");
	      }
	  }
    }

  if (verbose)
    flow_loops_cfg_dump (loops, file);
}

/* Free all the memory allocated for LOOPS.  */

void
flow_loops_free (loops)
     struct loops *loops;
{
  if (loops->array)
    {
      int i;

      if (! loops->num)
	abort ();

      /* Free the loop descriptors.  */
      for (i = 0; i < loops->num; i++)
	{
	  struct loop *loop = &loops->array[i];

	  if (loop->pre_header_edges)
	    free (loop->pre_header_edges);
	  if (loop->nodes)
	    sbitmap_free (loop->nodes);
	  if (loop->entry_edges)
	    free (loop->entry_edges);
	  if (loop->exit_edges)
	    free (loop->exit_edges);
	  if (loop->exits_doms)
	    sbitmap_free (loop->exits_doms);
	}

      free (loops->array);
      loops->array = NULL;

      if (loops->cfg.dom)
	sbitmap_vector_free (loops->cfg.dom);

      if (loops->cfg.dfs_order)
	free (loops->cfg.dfs_order);

      if (loops->shared_headers)
	sbitmap_free (loops->shared_headers);
    }
}

/* Find the entry edges into the loop with header HEADER and nodes
   NODES and store in ENTRY_EDGES array.  Return the number of entry
   edges from the loop.  */

static int
flow_loop_entry_edges_find (header, nodes, entry_edges)
     basic_block header;
     const sbitmap nodes;
     edge **entry_edges;
{
  edge e;
  int num_entries;

  *entry_edges = NULL;

  num_entries = 0;
  for (e = header->pred; e; e = e->pred_next)
    {
      basic_block src = e->src;

      if (src == ENTRY_BLOCK_PTR || ! TEST_BIT (nodes, src->index))
	num_entries++;
    }

  if (! num_entries)
    abort ();

  *entry_edges = (edge *) xmalloc (num_entries * sizeof (edge));

  num_entries = 0;
  for (e = header->pred; e; e = e->pred_next)
    {
      basic_block src = e->src;

      if (src == ENTRY_BLOCK_PTR || ! TEST_BIT (nodes, src->index))
	(*entry_edges)[num_entries++] = e;
    }

  return num_entries;
}

/* Find the exit edges from the loop using the bitmap of loop nodes
   NODES and store in EXIT_EDGES array.  Return the number of
   exit edges from the loo