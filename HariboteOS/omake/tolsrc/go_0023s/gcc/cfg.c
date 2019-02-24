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

/* This file contains low level functions to manipulate the CFG and
   analyze it.  All other modules should not transform the datastructure
   directly and use abstraction instead.  The file is supposed to be
   ordered bottom-up and should not contain any code dependent on a
   particular intermediate language (RTL or trees).

   Available functionality:
     - Initialization/deallocation
	 init_flow, clear_edges
     - Low level basic block manipulation
	 alloc_block, expunge_block
     - Edge manipulation
	 make_edge, make_single_succ_edge, cached_make_edge, remove_edge
	 - Low level edge redirection (without updating instruction chain)
	     redirect_edge_succ, redirect_edge_succ_nodup, redirect_edge_pred
     - Dumping and debugging
	 dump_flow_info, debug_flow_info, dump_edge_info
     - Allocation of AUX fields for basic blocks
	 alloc_aux_for_blocks, free_aux_for_blocks, alloc_aux_for_block
 */

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

/* The obstack on which the flow graph components are allocated.  */

struct obstack flow_obstack;
static char *flow_firstobj;

/* Number of basic blocks in the current function.  */

int n_basic_blocks;

/* Number of edges in the current function.  */

int n_edges;

/* First edge in the deleted edges chain.  */

edge first_deleted_edge;
static basic_block first_deleted_block;

/* The basic block array.  */

varray_type basic_block_info;

/* The special entry and exit blocks.  */

struct basic_block_def entry_exit_blocks[2]
= {{NULL,			/* head */
    NULL,			/* end */
    NULL,			/* head_tree */
    NULL,			/* end_tree */
    NULL,			/* pred */
    NULL,			/* succ */
    NULL,			/* local_set */
    NULL,			/* cond_local_set */
    NULL,			/* global_live_at_start */
    NULL,			/* global_live_at_end */
    NULL,			/* aux */
    ENTRY_BLOCK,		/* index */
    0,				/* loop_depth */
    0,				/* count */
    0,				/* frequency */
    0				/* flags */
  },
  {
    NULL,			/* head */
    NULL,			/* end */
    NULL,			/* head_tree */
    NULL,			/* end_tree */
    NULL,			/* pred */
    NULL,			/* succ */
    NULL,			/* local_set */
    NULL,			/* cond_local_set */
    NULL,			/* global_live_at_start */
    NULL,			/* global_live_at_end */
    NULL,			/* aux */
    EXIT_BLOCK,			/* index */
    0,				/* loop_depth */
    0,				/* count */
    0,				/* frequency */
    0				/* flags */
  }
};

void debug_flow_info			PARAMS ((void));
static void free_edge			PARAMS ((edge));

/* Called once at initialization time.  */

void
init_flow ()
{
  static int initialized;

  first_deleted_edge = 0;
  first_deleted_block = 0;
  n_edges = 0;

  if (!initialized)
    {
      gcc_obstack_init (&flow_obstack);
      flow_firstobj = (char *) obstack_alloc (&flow_obstack, 0);
      initialized = 1;
    }
  else
    {
      obs