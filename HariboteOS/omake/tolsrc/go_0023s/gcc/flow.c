/* Data flow analysis for GNU compiler.
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

/* This file contains the data flow analysis pass of the compiler.  It
   computes data flow information which tells combine_instructions
   which insns to consider combining and controls register allocation.

   Additional data flow information that is too bulky to record is
   generated during the analysis, and is used at that time to create
   autoincrement and autodecrement addressing.

   The first step is dividing the function into basic blocks.
   find_basic_blocks does this.  Then life_analysis determines
   where each register is live and where it is dead.

   ** find_basic_blocks **

   find_basic_blocks divides the current function's rtl into basic
   blocks and constructs the CFG.  The blocks are recorded in the
   basic_block_info array; the CFG exists in the edge structures
   referenced by the blocks.

   find_basic_blocks also finds any unreachable loops and deletes them.

   ** life_analysis **

   life_analysis is called immediately after find_basic_blocks.
   It uses the basic block information to determine where each
   hard or pseudo register is live.

   ** live-register info **

   The information about where each register is live is in two parts:
   the REG_NOTES of insns, and the vector basic_block->global_live_at_start.

   basic_block->global_live_at_start has an element for each basic
   block, and the element is a bit-vector with a bit for each hard or
   pseudo register.  The bit is 1 if the register is live at the
   beginning of the basic block.

   Two types of elements can be added to an insn's REG_NOTES.
   A REG_DEAD note is added to an insn's REG_NOTES for any register
   that meets both of two conditions:  The value in the register is not
   needed in subsequent insns and the insn does not replace the value in
   the register (in the case of multi-word hard registers, the value in
   each register must be replaced by the insn to avoid a REG_DEAD note).

   In the vast majority of cases, an object in a REG_DEAD note will be
   used somewhere in the insn.  The (rare) exception to this is if an
   insn uses a multi-word hard register and only some of the registers are
   needed in subsequent insns.  In that case, REG_DEAD notes will be
   provided for those hard registers that are not subsequently needed.
   Partial REG_DEAD notes of this type do not occur when an insn sets
   only some of the hard registers used in such a multi-word operand;
   omitting REG_DEAD notes for objects stored in an insn is optional and
   the desire to do so does not justify the complexity of the partial
   REG_DEAD notes.

   REG_UNUSED notes are added for each register that is set by the insn
   but is unused subsequently (if every register set by the insn is unused
   and the insn does not reference memory or have some other side-effect,
   the insn is deleted instead).  If only part of a multi-word hard
   register is used in a subsequent insn, REG_UNUSED notes are made for
   the parts that will not be used.

   To determine which registers are live after any insn, one can
   start from the beginning of the basic block and scan insns, noting
   which registers are set by each insn and which die there.

   ** Other actions of life_analysis **

   life_analysis sets up the LOG_LINKS fields of insns because the
   information needed to do so is readily available.

   life_analysis deletes insns whose only effect is to store a value
   that is never used.

   life_analysis notices cases where a reference to a register as
   a memory address can be combined with a preceding or following
   incrementation or decrementation of the register.  The separate
   instruction to increment or decrement is deleted and the address
   is changed to a POST_INC or similar rtx.

   Each time an incrementing or decrementing address is created,
   a REG_INC element is added to the insn's REG_NOTES list.

   life_analysis fills in certain vectors containing information about
   register usage: REG_N_REFS, REG_N_DEATHS, REG_N_SETS, REG_LIVE_LENGTH,
   REG_N_CALLS_CROSSED and REG_BASIC_BLOCK.

   life_analysis sets current_function_sp_is_unchanging if the function
   doesn't modify the stack pointer.  */

/* TODO:

   Split out from life_analysis:
	- local property discovery (bb->local_live, bb->local_set)
	- global property computation
	- log links creation
	- pre/post modify transformation
*/

#include "config.h"
#include "system.h"
#include "tree.h"
#include "rtl.h"
#include "tm_p.h"
#include "hard-reg-set.h"
#include "basic-block.h"
#include "insn-config.h"
#include "regs.h"
#include "flags.h"
#include "output.h"
#include "function.h"
#include "except.h"
#include "toplev.h"
#include "recog.h"
#include "expr.h"
#include "ssa.h"
#include "timevar.h"

/* !kawai! */
#include "../include/obstack.h"
#include "../include/splay-tree.h"
/* end of !kawai! */

#define obstack_chunk_alloc xmalloc
#define obstack_chunk_free free

/* EXIT_IGNORE_STACK should be nonzero if, when returning from a function,
   the stack pointer does not matter.  The value is tested only in
   functions that have frame pointers.
   No definition is equivalent to always zero.  */
#ifndef EXIT_IGNORE_STACK
#define EXIT_IGNORE_STACK 0
#endif

#ifndef HAVE_epilogue
#define HAVE_epilogue 0
#endif
#ifndef HAVE_prologue
#define HAVE_prologue 0
#endif
#ifndef HAVE_sibcall_epilogue
#define HAVE_sibcall_epilogue 0
#endif

#ifndef LOCAL_REGNO
#define LOCAL_REGNO(REGNO)  0
#endif
#ifndef EPILOGUE_USES
#define EPILOGUE_USES(REGNO)  0
#endif
#ifndef EH_USES
#define EH_USES(REGNO)  0
#endif

#ifdef HAVE_conditional_execution
#ifndef REVERSE_CONDEXEC_PREDICATES_P
#define REVERSE_CONDEXEC_PREDICATES_P(x, y) ((x) == reverse_condition (y))
#endif
#endif

/* Nonzero if the second flow pass has completed.  */
int flow2_completed;

/* Maximum register number used in this function, plus one.  */

int max_regno;

/* Indexed by n, giving various register information */

varray_type reg_n_info;

/* Size of a regset for the current function,
   in (1) bytes and (2) elements.  */

int regset_bytes;
int regset_size;

/* Regset of regs live when calls to `setjmp'-like functions happen.  */
/* ??? Does this exist only for the setjmp-clobbered warning message?  */

regset regs_live_at_setjmp;

/* List made of EXPR_LIST rtx's which gives pairs of pseudo registers
   that have to go in the same hard reg.
   The first two regs in the list are a pair, and the next two
   are another pair, etc.  */
rtx regs_may_share;

/* Callback that determines if it's ok for a function to have no
   noreturn attribute.  */
int (*lang_missing_noreturn_ok_p) PARAMS ((tree));

/* Set of registers that may be eliminable.  These are handled specially
   in updating regs_ever_live.  */

static HARD_REG_SET elim_reg_set;

/* Holds information for tracking conditional register life information.  */
struct reg_cond_life_info
{
  /* A boolean expression of conditions under which a register is dead.  */
  rtx condition;
  /* Conditions under which a register is dead at the basic block end.  */
  rtx orig_condition;

  /* A boolean expression of conditions under which a register has been
     stored into.  */
  rtx stores;

  /* ??? Could store mask of bytes that are dead, so that we could finally
     track lifetimes of multi-word registers accessed via subregs.  */
};

/* For use in communicating between propagate_block and its subroutines.
   Holds all information needed to compute life and def-use information.  */

struct propagate_block_info
{
  /* The basic block we're considering.  */
  basic_block bb;

  /* Bit N is set if register N is conditionally or unconditionally live.  */
  regset reg_live;

  /* Bit N is set if register N is set this insn.  */
  regset new_set;

  /* Element N is the next insn that uses (hard or pseudo) register N
     within the current basic block; or zero, if there is no such insn.  */
  rtx *reg_next_use;

  /* Contains a list of all the MEMs we are tracking for dead store
     elimination.  */
  rtx mem_set_list;

  /* If non-null, record the set of registers set unconditionally in the
     basic block.  */
  regset local_set;

  /* If non-null, record the set of registers set conditionally in the
     basic block.  */
  regset cond_local_set;

#ifdef HAVE_conditional_execution
  /* Indexed by register number, holds a reg_cond_life_info for each
     register that is not unconditionally live or dead.  */
  splay_tree reg_cond_dead;

  /* Bit N is set if register N is in an expression in reg_cond_dead.  */
  regset reg_cond_reg;
#endif

  /* The length of mem_set_list.  */
  int mem_set_list_len;

  /* Non-zero if the value of CC0 is live.  */
  int cc0_live;

  /* Flags controling the set of information propagate_block collects.  */
  int flags;
};

/* Maximum length of pbi->mem_set_list before we start dropping
   new elements on the floor.  */
#define MAX_MEM_SET_LIST_LEN	100

/* Forward declarations */
static int verify_wide_reg_1		PARAMS ((rtx *, void *));
static void verify_wide_reg		PARAMS ((int, basic_block));
static void verify_local_live_at_start	PARAMS ((regset, basic_block));
static void notice_stack_pointer_modification_1 PARAMS ((rtx, rtx, void *));
static void notice_stack_pointer_modification PARAMS ((rtx));
static void mark_reg			PARAMS ((rtx, void *));
static void mark_regs_live_at_end	PARAMS ((regset));
static int set_phi_alternative_reg      PARAMS ((rtx, int, int, void *));
static void calculate_global_regs_live	PARAMS ((sbitmap, sbitmap, int));
static void propagate_block_delete_insn PARAMS ((basic_block, rtx));
static rtx propagate_block_delete_libcall PARAMS ((rtx, rtx));
static int insn_dead_p			PARAMS ((struct propagate_block_info *,
						 rtx, int, rtx));
static int libcall_dead_p		PARAMS ((struct propagate_block_info *,
						 rtx, rtx));
static void mark_set_regs		PARAMS ((struct propagate_block_info *,
						 rtx, rtx));
static void mark_set_1			PARAMS ((struct propagate_block_info *,
						 enum rtx_code, rtx, rtx,
						 rtx, int));
static int find_regno_partial		PARAMS ((rtx *, void *));

#ifdef HAVE_conditional_execution
static int mark_regno_cond_dead		PARAMS ((struct propagate_block_info *,
						 int, rtx));
static void free_reg_cond_life_info	PARAMS ((splay_tree_value));
static int flush_reg_cond_reg_1		PARAMS ((splay_tree_node, void *));
static void flush_reg_cond_reg		PARAMS ((struct propagate_block_info *,
						 int));
static rtx elim_reg_cond		PARAMS ((rtx, unsigned int));
static rtx ior_reg_cond			PARAMS ((rtx, rtx, int));
static rtx not_reg_cond			PARAMS ((rtx));
static rtx and_reg_cond			PARAMS ((rtx, rtx, int));
#endif
#ifdef AUTO_INC_DEC
static void attempt_auto_inc		PARAMS ((struct propagate_block_info *,
						 rtx, rtx, rtx, rtx, rtx));
static void find_auto_inc		PARAMS ((struct propagate_block_info *,
						 rtx, rtx));
static int try_pre_increment_1		PARAMS ((struct propagate_block_info *,
						 rtx));
static int try_pre_increment		PARAMS ((rtx, rtx, HOST_WIDE_INT));
#endif
static void mark_used_reg		PARAMS ((struct propagate_block_info *,
						 rtx, rtx, rtx));
static void mark_used_regs		PARAMS ((struc