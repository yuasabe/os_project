/* Global common subexpression elimination/Partial redundancy elimination
   and global constant/copy propagation for GNU compiler.
   Copyright (C) 1997, 1998, 1999, 2000, 2001, 2002
   Free Software Foundation, Inc.

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

/* TODO
   - reordering of memory allocation and freeing to be more space efficient
   - do rough calc of how many regs are needed in each block, and a rough
     calc of how many regs are available in each class and use that to
     throttle back the code in cases where RTX_COST is minimal.
   - a store to the same address as a load does not kill the load if the
     source of the store is also the destination of the load.  Handling this
     allows more load motion, particularly out of loops.
   - ability to realloc sbitmap vectors would allow one initial computation
     of reg_set_in_block with only subsequent additions, rather than
     recomputing it for each pass

*/

/* References searched while implementing this.

   Compilers Principles, Techniques and Tools
   Aho, Sethi, Ullman
   Addison-Wesley, 1988

   Global Optimization by Suppression of Partial Redundancies
   E. Morel, C. Renvoise
   communications of the acm, Vol. 22, Num. 2, Feb. 1979

   A Portable Machine-Independent Global Optimizer - Design and Measurements
   Frederick Chow
   Stanford Ph.D. thesis, Dec. 1983

   A Fast Algorithm for Code Movement Optimization
   D.M. Dhamdhere
   SIGPLAN Notices, Vol. 23, Num. 10, Oct. 1988

   A Solution to a Problem with Morel and Renvoise's
   Global Optimization by Suppression of Partial Redundancies
   K-H Drechsler, M.P. Stadel
   ACM TOPLAS, Vol. 10, Num. 4, Oct. 1988

   Practical Adaptation of the Global Optimization
   Algorithm of Morel and Renvoise
   D.M. Dhamdhere
   ACM TOPLAS, Vol. 13, Num. 2. Apr. 1991

   Efficiently Computing Static Single Assignment Form and the Control
   Dependence Graph
   R. Cytron, J. Ferrante, B.K. Rosen, M.N. Wegman, and F.K. Zadeck
   ACM TOPLAS, Vol. 13, Num. 4, Oct. 1991

   Lazy Code Motion
   J. Knoop, O. Ruthing, B. Steffen
   ACM SIGPLAN Notices Vol. 27, Num. 7, Jul. 1992, '92 Conference on PLDI

   What's In a Region?  Or Computing Control Dependence Regions in Near-Linear
   Time for Reducible Flow Control
   Thomas Ball
   ACM Letters on Programming Languages and Systems,
   Vol. 2, Num. 1-4, Mar-Dec 1993

   An Efficient Representation for Sparse Sets
   Preston Briggs, Linda Torczon
   ACM Letters on Programming Languages and Systems,
   Vol. 2, Num. 1-4, Mar-Dec 1993

   A Variation of Knoop, Ruthing, and Steffen's Lazy Code Motion
   K-H Drechsler, M.P. Stadel
   ACM SIGPLAN Notices, Vol. 28, Num. 5, May 1993

   Partial Dead Code Elimination
   J. Knoop, O. Ruthing, B. Steffen
   ACM SIGPLAN Notices, Vol. 29, Num. 6, Jun. 1994

   Effective Partial Redundancy Elimination
   P. Briggs, K.D. Cooper
   ACM SIGPLAN Notices, Vol. 29, Num. 6, Jun. 1994

   The Program Structure Tree: Computing Control Regions in Linear Time
   R. Johnson, D. Pearson, K. Pingali
   ACM SIGPLAN Notices, Vol. 29, Num. 6, Jun. 1994

   Optimal Code Motion: Theory and Practice
   J. Knoop, O. Ruthing, B. Steffen
   ACM TOPLAS, Vol. 16, Num. 4, Jul. 1994

   The power of assignment motion
   J. Knoop, O. Ruthing, B. Steffen
   ACM SIGPLAN Notices Vol. 30, Num. 6, Jun. 1995, '95 Conference on PLDI

   Global code motion / global value numbering
   C. Click
   ACM SIGPLAN Notices Vol. 30, Num. 6, Jun. 1995, '95 Conference on PLDI

   Value Driven Redundancy Elimination
   L.T. Simpson
   Rice University Ph.D. thesis, Apr. 1996

   Value Numbering
   L.T. Simpson
   Massively Scalar Compiler Project, Rice University, Sep. 1996

   High Performance Compilers for Parallel Computing
   Michael Wolfe
   Addison-Wesley, 1996

   Advanced Compiler Design and Implementation
   Steven Muchnick
   Morgan Kaufmann, 1997

   Building an Optimizing Compiler
   Robert Morgan
   Digital Press, 1998

   People wishing to speed up the code here should read:
     Elimination Algorithms for Data Flow Analysis
     B.G. Ryder, M.C. Paull
     ACM Computing Surveys, Vol. 18, Num. 3, Sep. 1986

     How to Analyze Large Programs Efficiently and Informatively
     D.M. Dhamdhere, B.K. Rosen, F.K. Zadeck
     ACM SIGPLAN Notices Vol. 27, Num. 7, Jul. 1992, '92 Conference on PLDI

   People wishing to do something different can find various possibilities
   in the above papers and elsewhere.
*/

#include "config.h"
#include "system.h"
#include "toplev.h"

#include "rtl.h"
#include "tm_p.h"
#include "regs.h"
#include "hard-reg-set.h"
#include "flags.h"
#include "real.h"
#include "insn-config.h"
#include "recog.h"
#include "basic-block.h"
#include "output.h"
#include "function.h"
#include "expr.h" 
#include "except.h"
#include "ggc.h"
#include "params.h"

/* !kawai! */
#include "../include/obstack.h"
#define obstack_chunk_alloc gmalloc
#define obstack_chunk_free free
/* end of !kawai! */

/* Propagate flow information through back edges and thus enable PRE's
   moving loop invariant calculations out of loops.

   Originally this tended to create worse overall code, but several
   improvements during the development of PRE seem to have made following
   back edges generally a win.

   Note much of the loop invariant code motion done here would normally
   be done by loop.c, which has more heuristics for when to move invariants
   out of loops.  At some point we might need to move some of those
   heuristics into gcse.c.  */
#define FOLLOW_BACK_EDGES 1

/* We support GCSE via Partial Redundancy Elimination.  PRE optimizations
   are a superset of those done by GCSE.

   We perform the following steps:

   1) Compute basic block information.

   2) Compute table of places where registers are set.

   3) Perform copy/constant propagation.

   4) Perform global cse.

   5) Perform another pass of copy/constant propagation.

   Two passes of copy/constant propagation are done because the first one
   enables more GCSE and the second one helps to clean up the copies that
   GCSE creates.  This is needed more for PRE than for Classic because Classic
   GCSE will try to use an existing register containing the common
   subexpression rather than create a new one.  This is harder to do for PRE
   because of the code motion (which Classic GCSE doesn't do).

   Expressions we are interested in GCSE-ing are of the form
   (set (pseudo-reg) (expression)).
   Function want_to_gcse_p says what these are.

   PRE handles moving invariant expressions out of loops (by treating them as
   partially redundant).

   Eventually it would be nice to replace cse.c/gcse.c with SSA (static single
   assignment) based GVN (global value numbering).  L. T. Simpson's paper
   (Rice University) on value numbering is a useful reference for this.

   **********************

   We used to support multiple passes but there are diminishing returns in
   doing so.  The first pass usually makes 90% of the changes that are doable.
   A second pass can make a few more changes made possible by the first pass.
   Experiments show any further passes don't make enough changes to justify
   the expense.

   A study of spec92 using an unlimited number of passes:
   [1 pass] = 1208 substitutions, [2] = 577, [3] = 202, [4] = 192, [5] = 83,
   [6] = 34, [7] = 17, [8] = 9, [9] = 4, [10] = 4, [11] = 2,
   [12] = 2, [13] = 1, [15] = 1, [16] = 2, [41] = 1

   It was found doing copy propagation between each pass enables further
   substitutions.

   PRE is quite expensive in complicated functions because the DFA can take
   awhile to converge.  Hence we only perform one pass.  The parameter max-gcse-passes can
   be modified if one wants to experiment.

   **********************

   The steps for PRE are:

   1) Build the hash table of expressions we wish to GCSE (expr_hash_table).

   2) Perform the data flow analysis for PRE.

   3) Delete the redundant instructions

   4) Insert the required copies [if any] that make the partially
      redundant instructions fully redundant.

   5) For other reaching expressions, insert an instruction to copy the value
      to a newly created pseudo that will reach the redundant instruction.

   The deletion is done first so that when we do insertions we
   know which pseudo reg to use.

   Various papers have argued that PRE DFA is expensive (O(n^2)) and others
   argue it is not.  The number of iterations for the algorithm to converge
   is typically 2-4 so I don't view it as that expensive (relatively speaking).

   PRE GCSE depends heavily on the second CSE pass to clean up the copies
   we create.  To make an expression reach the place where it's redundant,
   the result of the expression is copied to a new register, and the redundant
   expression is deleted by replacing it with this new register.  Classic GCSE
   doesn't have this problem as much as it computes the reaching defs of
   each register in each block and thus can try to use an existing register.

   **********************

   A fair bit of simplicity is created by creating small functions for simple
   tasks, even when the function is only called in one place.  This may
   measurably slow things down [or may not] by creating more function call
   overhead than is necessary.  The source is laid out so that it's trivial
   to make the affected functions inline so that one can measure what speed
   up, if any, can be achieved, and maybe later when things settle things can
   be rearranged.

   Help stamp out big monolithic functions!  */

/* GCSE global vars.  */

/* -dG dump file.  */
static FILE *gcse_file;

/* Note whether or not we should run jump optimization after gcse.  We
   want to do this for two cases.

    * If we changed any jumps via cprop.

    * If we added any labels via edge splitting.  */

static int run_jump_opt_after_gcse;

/* Bitmaps are normally not included in debugging dumps.
   However it's useful to be able to print them from GDB.
   We could create special functions for this, but it's simpler to
   just allow passing stderr to the dump_foo fns.  Since stderr can
   be a macro, we store a copy here.  */
static FILE *debug_stderr;

/* An obstack for our working variables.  */
static struct obstack gcse_obstack;

/* Non-zero for each mode that supports (set (reg) (reg)).
   This is trivially true for integer and floating point values.
   It may or may not be true for condition codes.  */
static char can_copy_p[(int) NUM_MACHINE_MODES];

/* Non-zero if can_copy_p has been initialized.  */
static int can_copy_init_p;

struct reg_use {rtx reg_rtx; };

/* Hash table of expressions.  */

struct expr
{
  /* The expression (SET_SRC for expressions, PATTERN for assignments).  */
  rtx expr;
  /* Index in the available expression bitmaps.  */
  int bitmap_index;
  /* Next entry with the same hash.  */
  struct expr *next_same_hash;
  /* List of anticipatable occurrences in basic blocks in the function.
     An "anticipatable occurrence" is one that is the first occurrence in the
     basic block, the operands are not modified in the basic block prior
     to the occurrence and the output is not used between the start of
     the block and the occurrence.  */
  struct occr *antic_occr;
  /* List of available occurrence in basic blocks in the function.
     An "available occurrence" is one that is the last occurrence in the
     basic block and the operands are not modified by following statements in
     the basic block [including this insn].  */
  struct occr *avail_occr;
  /* Non-null if the computation is PRE redundant.
     The value is the newly created pseudo-reg to record a copy of the
     expression in all the places that reach the redundant copy.  */
  rtx reaching_reg;
};

/* Occurrence of an expression.
   There is one per basic block.  If a pattern appears more than once the
   last appearance is used [or first for anticipatable expressions].  */

struct occr
{
  /* Next occurrence of this expression.  */
  struct occr *next;
  /* The insn that computes the expression.  */
  rtx insn;
  /* Non-zero if this [anticipatable] occurrence has been deleted.  */
  char deleted_p;
  /* Non-zero if this [available] occurrence has been copied to
     reaching_reg.  */
  /* ??? This is mutually exclusive with deleted_p, so they could share
     the same byte.  */
  char copied_p;
};

/* Expression and copy propagation hash tables.
   Each hash table is an array of buckets.
   ??? It is known that if it were an array of entries, structure elements
   `next_same_hash' and `bitmap_index' wouldn't be necessary.  However, it is
   not clear whether in the final analysis a sufficient amount of memory would
   be saved as the size of the available expression bitmaps would be larger
   [one could build a mapping table without holes afterwards though].
   Someday I'll perform the computation and figure it out.  */

/* Total size of the expression hash table, in elements.  */
static unsigned int expr_hash_table_size;

/* The table itself.
   This is an array of `expr_hash_table_size' elements.  */
static struct expr **expr_hash_table;

/* Total size of the copy propagation hash table, in elements.  */
static unsigned int set_hash_table_size;

/* The table itself.
   This is an array of `set_hash_table_size' elements.  */
static struct expr **set_hash_table;

/* Mapping of uids to cuids.
   Only real insns get cuids.  */
static int *uid_cuid;

/* Highest UID in UID_CUID.  */
static int max_uid;

/* Get the cuid of an insn.  */
#ifdef ENABLE_CHECKING
#define INSN_CUID(INSN) (INSN_UID (INSN) > max_uid ? (abort (), 0) : uid_cuid[INSN_UID (INSN)])
#else
#define INSN_CUID(INSN) (uid_cuid[INSN_UID (INSN)])
#endif

/* Number of cuids.  */
static int max_cuid;

/* Mapping of cuids to insns.  */
static rtx *cuid_insn;

/* Get insn from cuid.  */
#define CUID_INSN(CUID) (cuid_insn[CUID])

/* Maximum register number in function prior to doing gcse + 1.
   Registers created during this pass have regno >= max_gcse_regno.
   This is named with "gcse" to not collide with global of same name.  */
static unsigned int max_gcse_regno;

/* Maximum number of cse-able expressions found.  */
static int n_exprs;

/* Maximum number of assignments for copy propagation found.  */
static int n_sets;

/* Table of registers that are modified.

   For each register, each element is a list of places where the pseudo-reg
   is set.

   For simplicity, GCSE is done on sets of pseudo-regs only.  PRE GCSE only
   requires knowledge of which blocks kill which regs [and thus could use
   a bitmap instead of the lists `reg_set_table' uses].

   `reg_set_table' and could be turned into an array of bitmaps (num-bbs x
   num-regs) [however perhaps it may be useful to keep the data as is].  One
   advantage of recording things this way is that `reg_set_table' is fairly
   sparse with respect to pseudo regs but for hard regs could be fairly dense
   [relatively speaking].  And recording sets of pseudo-regs in lists speeds
   up functions like compute_transp since in the case of pseudo-regs we only
   need to iterate over the number of times a pseudo-reg is set, not over the
   number of basic blocks [clearly there is a bit of a slow down in the cases
   where a pseudo is set more than once in a block, however it is believed
   that the net effect is to speed things up].  This isn't done for hard-regs
   because recording call-clobbered hard-regs in `reg_set_table' at each
   function call can consume a fair bit of memory, and iterating over
   hard-regs stored this way in compute_transp will be more expensive.  */

typedef struct reg_set
{
  /* The next setting of this register.  */
  struct reg_set *next;
  /* The insn where it was set.  */
  rtx insn;
} reg_set;

static reg_set **reg_set_table;

/* Size of `reg_set_table'.
   The table starts out at max_gcse_regno + slop, and is enlarged as
   necessary.  */
static int reg_set_table_size;

/* Amount to grow `reg_set_table' by when it's full.  */
#define REG_SET_TABLE_SLOP 100

/* This is a list of expressions which are MEMs and will be used by load
   or store motion. 
   Load motion tracks MEMs which aren't killed by
   anything except itself. (ie, loads and stores to a single location).
   We can then allow movement of these MEM refs with a little special 
   allowance. (all stores copy the same value to the reaching reg used
   for the loads).  This means all values used to store into memory must have
   no side effects so we can re-issue the setter value.  
   Store Motion uses this structure as an expression table to track stores
   which look interesting, and might be moveable towards the exit block.  */

struct ls_expr
{
  struct expr * expr;		/* Gcse expression reference for LM.  */
  rtx pattern;			/* Pattern of this mem.  */
  rtx loads;			/* INSN list of loads seen.  */
  rtx stores;			/* INSN list of stores seen.  */
  struct ls_expr * next;	/* Next in the list.  */
  int invalid;			/* Invalid for some reason.  */
  int index;			/* If it maps to a bitmap index.  */
  int hash_index;		/* Index when in a hash table.  */
  rtx reaching_reg;		/* Register to use when re-writing.  */
};

/* Head of the list of load/store memory refs.  */
static struct ls_expr * pre_ldst_mems = NULL;

/* Bitmap containing one bit for each register in the program.
   Used when performing GCSE to track which registers have been set since
   the start of the basic block.  */
static regset reg_set_bitmap;

/* For each block, a bitmap of registers set in the block.
   This is used by expr_killed_p and compute_transp.
   It is computed during hash table computation and not by compute_sets
   as it includes registers added since the last pass (or between cprop and
   gcse) and it's currently not easy to realloc sbitmap vectors.  */
static sbitmap *reg_set_in_block;

/* Array, indexed by basic block number for a list of insns which modify
   memory within that block.  */
static rtx * modify_mem_list;
bitmap modify_mem_list_set;

/* This array parallels modify_mem_list, but is kept canonicalized.  */
static rtx * canon_modify_mem_list;
bitmap canon_modify_mem_list_set;
/* Various variables for statistics gathering.  */

/* Memory used in a pass.
   This isn't intended to be absolutely precise.  Its intent is only
   to keep an eye on memory usage.  */
static int bytes_used;

/* GCSE substitutions made.  */
static int gcse_subst_count;
/* Number of copy instructions created.  */
static int gcse_create_count;
/* Number of constants propagated.  */
static int const_prop_count;
/* Number of copys propagated.  */
static int copy_prop_count;

/* These variables are used by classic GCSE.
   Normally they'd be defined a bit later, but `rd_gen' needs to
   be declared sooner.  */

/* Each block has a bitmap of each type.
   The length of each blocks bitmap is:

       max_cuid  - for reaching definitions
       n_exprs - for available expressions

   Thus we view the bitmaps as 2 dimensional arrays.  i.e.
   rd_kill[block_num][cuid_num]
   ae_kill[block_num][expr_num]			 */

/* For reaching defs */
static sbitmap *rd_kill, *rd_gen, *reaching_defs, *rd_out;

/* for available exprs */
static sbitmap *ae_kill, *ae_gen, *ae_in, *ae_out;

/* Objects of this type are passed around by the null-pointer check
   removal routines.  *