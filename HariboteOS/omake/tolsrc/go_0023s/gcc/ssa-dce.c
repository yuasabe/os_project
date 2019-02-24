/* Dead-code elimination pass for the GNU compiler.
   Copyright (C) 2000, 2001, 2002 Free Software Foundation, Inc.
   Written by Jeffrey D. Oldham <oldham@codesourcery.com>.

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

/* Dead-code elimination is the removal of instructions which have no
   impact on the program's output.  "Dead instructions" have no impact
   on the program's output, while "necessary instructions" may have
   impact on the output.

   The algorithm consists of three phases:
   1) marking as necessary all instructions known to be necessary,
      e.g., writing a value to memory,
   2) propagating necessary instructions, e.g., the instructions
      giving values to operands in necessary instructions, and
   3) removing dead instructions (except replacing dead conditionals
      with unconditional jumps).

   Side Effects:
   The last step can require adding labels, deleting insns, and
   modifying basic block structures.  Some conditional jumps may be
   converted to unconditional jumps so the control-flow graph may be
   out-of-date.

   Edges from some infinite loops to the exit block can be added to
   the control-flow graph, but will be removed after this pass is
   complete.

   It Does Not Perform:
   We decided to not simultaneously perform jump optimization and dead
   loop removal during dead-code elimination.  Thus, all jump
   instructions originally present remain after dead-code elimination
   but 1) unnecessary conditional jump instructions are changed to
   unconditional jump instructions and 2) all unconditional jump
   instructions remain.

   Assumptions:
   1) SSA has been performed.
   2) The basic block and control-flow graph structures are accurate.
   3) The flow graph permits constructing an edge_list.
   4) note rtxes should be saved.

   Unfinished:
   When replacing unnecessary conditional jumps with unconditional
   jumps, the control-flow graph is not updated.  It should be.

   References:
   Building an Optimizing Compiler
   Robert Morgan
   Butterworth-Heinemann, 1998
   Section 8.9
*/

#include "config.h"
#include "system.h"

#include "rtl.h"
#include "hard-reg-set.h"
#include "basic-block.h"
#include "ssa.h"
#include "insn-config.h"
#include "recog.h"
#include "output.h"


/* A map from blocks to the edges on which they are control dependent.  */
typedef struct {
  /* An dynamically allocated array.  The Nth element corresponds to
     the block with index N + 2.  The Ith bit in the bitmap is set if
     that block is dependent on the Ith edge.  */
  bitmap *data;
  /* The number of elements in the array.  */
  int length;
} control_dependent_block_to_edge_map_s, *control_dependent_block_to_edge_map;

/* Local function prototypes.  */
static control_dependent_block_to_edge_map control_dependent_block_to_edge_map_create
  PARAMS((size_t num_basic_blocks));
static void set_control_dependent_block_to_edge_map_bit
  PARAMS ((control_dependent_block_to_edge_map c, basic_block bb,
	   int edge_index));
static void control_dependent_block_to_edge_map_free
  PARAMS ((control_dependent_block_to_edge_map c));
static void find_all_control_dependences
  PARAMS ((struct edge_list *el, int *pdom,
	   control_dependent_block_to_edge_map cdbte));
static void find_control_dependence
  PARAMS ((struct edge_list *el, int edge_index, int *pdom,
	   control_dependent_block_to_edge_map cdbte));
static basic_block find_pdom
  PARAMS ((int *pdom, basic_block block));
static int inherently_necessary_register_1
  PARAMS ((rtx *current_rtx, void *data));
static int inherently_necessary_register
  PARAMS ((rtx current_rtx));
static int find_inherently_necessary
  PARAMS ((rtx current_rtx));
static int propagate_necessity_through_operand
  PARAMS ((rtx *current_rtx, void *data));
static void note_inherently_necessary_set
  PARAMS ((rtx, rtx, void *));

/* Unnecessary insns are indicated using insns' in_struct bit.  */

/* Indicate INSN is dead-code; returns nothing.  */
#define KILL_INSN(INSN)		INSN_DEAD_CODE_P(INSN) = 1
/* Indicate INSN is necessary, i.e., not dead-code; returns nothing.  */
#define RESURRECT_INSN(INSN)	INSN_DEAD_CODE_P(INSN) = 0
/* Return nonzero if INSN is unnecessary.  */
#define UNNECESSARY_P(INSN)	INSN_DEAD_CODE_P(INSN)
static void mark_all_insn_unnecessary
  PARAMS ((void));
/* Execute CODE with free variable INSN for all unnecessary insns in
   an unspecified order, producing no output.  */
#define EXECUTE_IF_UNNECESSARY(INSN, CODE)	¥
{								¥
  rtx INSN;							¥
								¥
  for (INSN = get_insns (); INSN != NULL_RTX; INSN = NEXT_INSN (INSN))	¥
    if (INSN_DEAD_CODE_P (INSN)) {				¥
      CODE;							¥
    }								¥
}
/* Find the label beginning block BB.  */
static rtx find_block_label
  PARAMS ((basic_block bb));
/* Remove INSN, updating its basic block structure.  */
static void delete_insn_bb
  PARAMS ((rtx insn));

/* Recording which blocks are control dependent on which edges.  We
   expect each block to be control dependent on very few edges so we
   use a bitmap for each block recording its edges.  An array holds
   the bitmap.  Its position 0 entry holds the bitmap for block
   INVALID_BLOCK+1 so that all blocks, including the entry and exit
   blocks can participate in the data structure.  */

/* Create a control_dependent_block_to_edge_map, given the number
   NUM_BASIC_BLOCKS of non-entry, non-exit basic blocks, e.g.,
   n_basic_blocks.  This memory must be released using
   control_dependent_block_to_edge_map_free ().  */

static control_dependent_block_to_edge_map
control_dependent_block_to_edge_map_create (num_basic_blocks)
     size_t num_basic_blocks;
{
  int i;
  control_dependent_block_to_edge_map c
    = xmalloc (sizeof (control_dependent_block_to_edge_map_s));
  c->length = num_basic_blocks - (INVALID_BLOCK+1);
  c->data = xmalloc ((size_t) c->length*sizeof (bitmap));
  for (i = 0; i < c->length; ++i)
    c->data[i] = BITMAP_XMALLOC ();

  return c;
}

/* Indicate block BB is control dependent on an edge with index
   EDGE_INDEX in the mapping C of blocks to edges on which they are
   control-dependent.  */

static void
set_control_dependent_block_to_edge_map_bit (c, bb, edge_index)
     control_dependent_block_to_edge_map c;
     basic_block bb;
     int edge_index;
{
  if (bb->index - (INVALID_BLOCK+1) >= c->length)
    abort ();

  bitmap_set_bit (c->data[bb->index - (INVALID_BLOCK+1)],
		  edge_index);
}

/* Execute CODE for each edge (given number EDGE_NUMBER within the
   CODE) for which the block containing INSN is control dependent,
   returning no output.  CDBTE is the mapping of blocks to edges on
   which they are control-dependent.  */

#define EXECUTE_IF_CONTROL_DEPENDENT(CDBTE, INSN, EDGE_NUMBER, CODE) ¥
	EXECUTE_IF_SET_IN_BITMAP ¥
	  (CDBTE->data[BLOCK_NUM (INSN) - (INVALID_BLOCK+1)], 0, ¥
	  EDGE_NUMBER, CODE)

/* Destroy a control_dependent_block_to_edge_map C.  */

static void
control_dependent_block_to_edge_map_free (c)
     control_dependent_block_to_edge_map c;
{
  int i;
  for (i = 0; i < c->length; ++i)
    BITMAP_XFREE (c->data[i]);
  free ((PTR) c);
}

/* Record all blocks' control dependences on all edges in the edge
   list EL, ala Morgan, Section 3.6.  The mapping PDOM of blocks to
   their postdominators are used, and results are stored in CDBTE,
   which should be empty.  */

static void
find