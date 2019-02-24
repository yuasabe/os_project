/* Generic partial redundancy elimination with lazy code motion support.
   Copyright (C) 1998, 1999, 2000, 2001 Free Software Foundation, Inc.

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

/* These routines are meant to be used by various optimization
   passes which can be modeled as lazy code motion problems.
   Including, but not limited to:

	* Traditional partial redundancy elimination.

	* Placement of caller/caller register save/restores.

	* Load/store motion.

	* Copy motion.

	* Conversion of flat register files to a stacked register
	model.

	* Dead load/store elimination.

  These routines accept as input:

	* Basic block information (number of blocks, lists of
	predecessors and successors).  Note the granularity
	does not need to be basic block, they could be statements
	or functions.

	* Bitmaps of local properties (computed, transparent and
	anticipatable expressions).

  The output of these routines is bitmap of redundant computations
  and a bitmap of optimal placement points.  */


#include "config.h"
#include "system.h"
#include "rtl.h"
#include "regs.h"
#include "hard-reg-set.h"
#include "flags.h"
#include "real.h"
#include "insn-config.h"
#include "recog.h"
#include "basic-block.h"
#include "tm_p.h"

/* We want target macros for the mode switching code to be able to refer
   to instruction attribute values.  */
#include "insn-attr.h"

/* Edge based LCM routines.  */
static void compute_antinout_edge	PARAMS ((sbitmap *, sbitmap *,
						 sbitmap *, sbitmap *));
static void compute_earliest		PARAMS ((struct edge_list *, int,
						 sbitmap *, sbitmap *,
						 sbitmap *, sbitmap *,
						 sbitmap *));
static void compute_laterin		PARAMS ((struct edge_list *, sbitmap *,
						 sbitmap *, sbitmap *,
						 sbitmap *));
static void compute_insert_delete	PARAMS ((struct edge_list *edge_list,
						 sbitmap *, sbitmap *,
						 sbitmap *, sbitmap *,
						 sbitmap *));

/* Edge based LCM routines on a reverse flowgraph.  */
static void compute_farthest		PARAMS ((struct edge_list *, int,
						 sbitmap *, sbitmap *,
						 sbitmap*, sbitmap *,
						 sbitmap *));
static void compute_nearerout		PARAMS ((struct edge_list *, sbitmap *,
						 sbitmap *, sbitmap *,
						 sbitmap *));
static void compute_rev_insert_delete	PARAMS ((struct edge_list *edge_list,
						 sbitmap *, sbitmap *,
						 sbitmap *, sbitmap *,
						 sbitmap *));

/* Edge based lcm routines.  */

/* Compute expression anticipatability at entrance and exit of each block.
   This is done based on the flow graph, and not on the pred-succ lists.
   Other than that, its pretty much identical to compute_antinout.  */

static void
compute_antinout_edge (antloc, transp, antin, antout)
     sbitmap *antloc;
     sbitmap *transp;
     sbitmap *antin;
     sbitmap *antout;
{
  int bb;
  edge e;
  basic_block *worklist, *qin, *qout, *qend;
  unsigned int qlen;

  /* Allocate a worklist array/queue.  Entries are only added to the
     list if they were not already on the list.  So the size is
     bounded by the number of basic blocks.  */
  qin = qout = worklist
    = (basic_block *) xmalloc (sizeof (basic_block) * n_basic_blocks);

  /* We want a maximal solution, so make an optimistic initialization of
     ANTIN.  */
  sbitmap_vector_ones (antin, n_basic_blocks);

  /* Put every block on the worklist