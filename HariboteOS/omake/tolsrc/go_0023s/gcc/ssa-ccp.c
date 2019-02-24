/* Conditional constant propagation pass for the GNU compiler.
   Copyright (C) 2000, 2001, 2002 Free Software Foundation, Inc.
   Original framework by Daniel Berlin <dan@cgsoftware.com>
   Fleshed out and major cleanups by Jeff Law <law@redhat.com>
   
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

/* Conditional constant propagation.

   References:

     Constant propagation with conditional branches,
     Wegman and Zadeck, ACM TOPLAS 13(2):181-210.

     Building an Optimizing Compiler,
     Robert Morgan, Butterworth-Heinemann, 1998, Section 8.9.

     Advanced Compiler Design and Implementation,
     Steven Muchnick, Morgan Kaufmann, 1997, Section 12.6

   The overall structure is as follows:

	1. Run a simple SSA based DCE pass to remove any dead code.
	2. Run CCP to compute what registers are known constants
	   and what edges are not executable.  Remove unexecutable
	   edges from the CFG and simplify PHI nodes.
	3. Replace registers with constants where possible.
	4. Remove unreachable blocks computed in step #2.
	5. Another simple SSA DCE pass to remove dead code exposed
	   by CCP.

   When we exit, we are still in SSA form. 


   Potential further enhancements:

    1. Handle SUBREGs, STRICT_LOW_PART, etc in destinations more
       gracefully.

    2. Handle insns with multiple outputs more gracefully.

    3. Handle CONST_DOUBLE and symbolic constants.

    4. Fold expressions after performing constant substitutions.  */


#include "config.h"
#include "system.h"

#include "rtl.h"
#include "hard-reg-set.h"
#include "basic-block.h"
#include "ssa.h"
#include "insn-config.h"
#include "recog.h"
#include "output.h"
#include "errors.h"
#include "ggc.h"
#include "df.h"
#include "function.h"

/* Possible lattice values.  */

typedef enum
{
  UNDEFINED,
  CONSTANT,
  VARYING
} latticevalue;

/* Main structure for CCP. 

   Contains the lattice value and, if it's a constant, the constant
   value.  */
typedef struct
{
  latticevalue lattice_val;
  rtx const_value;
} value;

/* Array of values indexed by register number.  */
static value *values;

/* A bitmap to keep track of executable blocks in the CFG.  */
static sbitmap executable_blocks;

/* A bitmap for all executable edges in the CFG.  */
static sbitmap executable_edges;

/* Array of edges on the work list.  */
static edge *edge_info;

/* We need an edge list to be able to get indexes easily.  */
static struct edge_list *edges;

/* For building/following use-def and def-use chains.  */
static struct df *df_analyzer;

/* Current edge we are operating on, from the worklist */
static edge flow_edges;

/* Bitmap of SSA edges which will need reexamination as their definition
   has changed.  */
static sbitmap ssa_edges;

/* Simple macros to simplify code */
#define SSA_NAME(x) REGNO (SET_DEST (x))
#define PHI_PARMS(x) XVEC (SET_SRC (x), 0)
#define EIE(x,y) EDGE_INDEX (edges, x, y)

static void visit_phi_node             PARAMS ((rtx, basic_block));
static void visit_expression           PARAMS ((rtx, basic_block));
static void defs_to_undefined          PARAMS ((rtx));
static void defs_to_varying            PARAMS ((rtx));
static void examine_flow_edges         PARAMS ((void));
static int mark_references             PARAMS ((rtx *, void *));
static void follow_def_use_chains      PARAMS ((void));
stat