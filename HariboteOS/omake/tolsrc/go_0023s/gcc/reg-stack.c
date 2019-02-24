/* Register to Stack convert for GNU compiler.
   Copyright (C) 1992, 1993, 1994, 1995, 1996, 1997, 1998,
   1999, 2000, 2001, 2002 Free Software Foundation, Inc.

   This file is part of GCC.

   GCC is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   GCC is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License
   along with GCC; see the file COPYING.  If not, write to the Free
   Software Foundation, 59 Temple Place - Suite 330, Boston, MA
   02111-1307, USA.  */

/* This pass converts stack-like registers from the "flat register
   file" model that gcc uses, to a stack convention that the 387 uses.

   * The form of the input:

   On input, the function consists of insn that have had their
   registers fully allocated to a set of "virtual" registers.  Note that
   the word "virtual" is used differently here than elsewhere in gcc: for
   each virtual stack reg, there is a hard reg, but the mapping between
   them is not known until this pass is run.  On output, hard register
   numbers have been substituted, and various pop and exchange insns have
   been emitted.  The hard register numbers and the virtual register
   numbers completely overlap - before this pass, all stack register
   numbers are virtual, and afterward they are all hard.

   The virtual registers can be manipulated normally by gcc, and their
   semantics are the same as for normal registers.  After the hard
   register numbers are substituted, the semantics of an insn containing
   stack-like regs are not the same as for an insn with normal regs: for
   instance, it is not safe to delete an insn that appears to be a no-op
   move.  In general, no insn containing hard regs should be changed
   after this pass is done.

   * The form of the output:

   After this pass, hard register numbers represent the distance from
   the current top of stack to the desired register.  A reference to
   FIRST_STACK_REG references the top of stack, FIRST_STACK_REG + 1,
   represents the register just below that, and so forth.  Also, REG_DEAD
   notes indicate whether or not a stack register should be popped.

   A "swap" insn looks like a parallel of two patterns, where each
   pattern is a SET: one sets A to B, the other B to A.

   A "push" or "load" insn is a SET whose SET_DEST is FIRST_STACK_REG
   and whose SET_DEST is REG or MEM.  Any other SET_DEST, such as PLUS,
   will replace the existing stack top, not push a new value.

   A store insn is a SET whose SET_DEST is FIRST_STACK_REG, and whose
   SET_SRC is REG or MEM.

   The case where the SET_SRC and SET_DEST are both FIRST_STACK_REG
   appears ambiguous.  As a special case, the presence of a REG_DEAD note
   for FIRST_STACK_REG differentiates between a load insn and a pop.

   If a REG_DEAD is present, the insn represents a "pop" that discards
   the top of the register stack.  If there is no REG_DEAD note, then the
   insn represents a "dup" or a push of the current top of stack onto the
   stack.

   * Methodology:

   Existing REG_DEAD and REG_UNUSED notes for stack registers are
   deleted and recreated from scratch.  REG_DEAD is never created for a
   SET_DEST, only REG_UNUSED.

   * asm_operands:

   There are several rules on the usage of stack-like regs in
   asm_operands insns.  These rules apply only to the operands that are
   stack-like regs:

   1. Given a set of input regs that die in an asm_operands, it is
      necessary to know which are implicitly popped by the asm, and
      which must be explicitly popped by gcc.

	An input reg that is implicitly popped by the asm must be
	explicitly clobbered, unless it is constrained to match an
	output operand.

   2. For any input reg that is implicitly popped by an asm, it is
      necessary to know how to adjust the stack to compensate for the pop.
      If any non-popped input is closer to the top of the reg-stack than
      the implicitly popped reg, it would not be possible to know what the
      stack looked like - it's not clear how the rest of the stack "slides
      up".

	All implicitly popped input regs must be closer to the top of
	the reg-stack than any input that is not implicitly popped.

   3. It is possible that if an input dies in an insn, reload might
      use the input reg for an output reload.  Consider this example:

		asm ("foo" : "=t" (a) : "f" (b));

      This asm says that input B is not popped by the asm, and that
      the asm pushes a result onto the reg-stack, ie, the stack is one
      deeper after the asm than it was before.  But, it is possible that
      reload will think that it can use the same reg for both the input and
      the output, if input B dies in this insn.

	If any input operand uses the "f" constraint, all output reg
	constraints must use the "&" earlyclobber.

      The asm above would be written as

		asm ("foo" : "=&t" (a) : "f" (b));

   4. Some operands need to be in particular places on the stack.  All
      output operands fall in this category - there is no other way to
      know which regs the outputs appear in unless the user indicates
      this in the constraints.

	Output operands must specifically indicate which reg an output
	appears in after an asm.  "=f" is not allowed: the operand
	constraints must select a class with a single reg.

   5. Output operands may not be "inserted" between existing stack regs.
      Since no 387 opcode uses a read/write operand, all output operands
      are dead before the asm_operands, and are pushed by the asm_operands.
      It makes no sense to push anywhere but the top of the reg-stack.

	Output operands must start at the top of the reg-stack: output
	operands may not "skip" a reg.

   6. Some asm statements may need extra stack space for internal
      calculations.  This can be guaranteed by clobbering stack registers
      unrelated to the inputs and outputs.

   Here are a couple of reasonable asms to want to write.  This asm
   takes one input, which is internally popped, and produces two outputs.

	asm ("fsincos" : "=t" (cos), "=u" (sin) : "0" (inp));

   This asm takes two inputs, which are popped by the fyl2xp1 opcode,
   and replaces them with one output.  The user must code the "st(1)"
   clobber for reg-stack.c to know that fyl2xp1 pops both inputs.

	asm ("fyl2xp1" : "=t" (result) : "0" (x), "u" (y) : "st(1)");

*/

#include "config.h"
#include "system.h"
#include "tree.h"
#include "rtl.h"
#include "tm_p.h"
#include "function.h"
#include "insn-config.h"
#include "regs.h"
#include "hard-reg-set.h"
#include "flags.h"
#include "toplev.h"
#include "recog.h"
#include "output.h"
#include "basic-block.h"
#include "varray.h"
#include "reload.h"

#ifdef STACK_REGS

#define REG_STACK_SIZE (LAST_STACK_REG - FIRST_STACK_REG + 1)

/* This is the basic stack record.  TOP is an index into REG[] such
   that REG[TOP] is the top of stack.  If TOP is -1 the stack is empty.

   If TOP is -2, REG[] is not yet initialized.  Stack initialization
   consists of placing each live reg in array `reg' and setting `top'
   appropriately.

   REG_SET indicates which registers are live.  */

typedef struct stack_def
{
  int top;			/* index to top stack element */
  HARD_REG_SET reg_set;		/* set of live registers */
  unsigned char reg[REG_STACK_SIZE];/* register - stack mapping */
} *stack;

/* This is used to carry information about basic blocks.  It is 
   attached to the AUX field of the standard CFG block.  */

typedef struct block_info_def
{
  struct stack_def stack_in;	/* Input stack configuration.  */
  struct stack_def stack_out;	/* Output stack configuration.  */
  HARD_REG_SET out_reg_set;	/* Stack regs live on output.  */
  int done;			/* True if block already converted.  */
  int predecessors;		/* Number of predecessors that needs
				   to be visited.  */
} *block_info;

#define BLOCK_INFO(B)	((block_info) (B)->aux)

/* Passed to change_stack to indicate where to emit insns.  */
enum emit_where
{
  EMIT_AFTER,
  EMIT_BEFORE
};

/* We use this array to cache info about insns, because otherwise we
   spend too much time in stack_regs_mentioned_p. 

   Indexed by insn UIDs.  A value of zero is uninitialized, one indicates
   the insn uses stack registers, two indicates the insn does not use
   stack registers.  */
static varray_type stack_regs_mentioned_data;

/* The block we're currently working on.  */
static basic_block current_block;

/* This is the register file for all register after conversion */
static rtx
  FP_mode_reg[LAST_STACK_REG+1-FIRST_STACK_REG][(int) MAX_MACHINE_MODE];

#define FP_MODE_REG(regno,mode)	¥
  (FP_mode_reg[(regno)-FIRST_STACK_REG][(int) (mode)])

/* Used to initialize uninitialized registers.  */
static rtx nan;

/* Forward declarations */

static int stack_regs_mentioned_p	PARAMS ((rtx pat));
static void straighten_stack		PARAMS ((rtx, stack));
static void pop_stack			PARAMS ((stack, int));
static rtx *get_true_reg		PARAMS ((rtx *));

static int check_asm_stack_operands	PARAMS ((rtx));
static int get_asm_operand_n_inputs	PARAMS ((rtx));
static rtx stack_result			PARAMS ((tree));
static void replace_reg			PARAMS ((rtx *, int));
static void remove_regno_note		PARAMS ((rtx, enum reg_note,
						 unsigned int));
static int get_hard_regnum		PARAMS ((stack, rtx));
static rtx emit_pop_insn		PARAMS ((rtx, stack, rtx,
					       enum emit_where));
static void emit_swap_insn		PARAMS ((rtx, stack, rtx));
static void move_for_stack_reg		PARAMS ((rtx, stack, rtx));
static int swap_rtx_condition_1		PARAMS ((rtx));
static int swap_rtx_condition		PARAMS ((rtx));
static void compare_for_stack_reg	PARAMS ((rtx, stack, rtx));
static void subst_stack_regs_pat	PARAMS ((rtx, stack, rtx));
static void subst_asm_stack_regs	PARAMS ((rtx, stack));
static void subst_stack_regs		PARAMS ((rtx, stack));
static void change_stack		PARAMS ((rtx, stack, stack,
					       enum emit_where));
static int convert_regs_entry		PARAMS ((void));
static void convert_regs_exit		PARAMS ((void));
static int convert_regs_1		PARAMS ((FILE *, basic_block));
static int convert_regs_2		PARAMS ((FILE *, basic_block));
static int convert_regs			PARAMS ((FILE *));
static void print_stack 		PARAMS ((FILE *, stack));
static rtx next_flags_user 		PARAMS ((rtx));
static void record_label_references	PARAMS ((rtx, rtx));
static bool compensate_edge		PARAMS ((edge, FILE *));

/* Return non-zero if any stack register is mentioned somewhere within PAT.  */

static int
stack_regs_mentioned_p (pat)
     rtx pat;
{
  const char *fmt;
  int i;

  if (STACK_REG_P (pat))
    return 1;

  fmt = GET_RTX_FORMAT (GET_CODE (pat));
  for (i = GET_RTX_LENGTH (GET_CODE (pat)) - 1; i >= 0; i--)
    {
      if (fmt[i] == 'E')
	{
	  int j;

	  for (j = XVECLEN (pat, i) - 1; j >= 0; j--)
	    if (stack_regs_mentioned_p (XVECEXP (pat, i, j)))
	      return 1;
	}
      else if (fmt[i] == 'e' && stack_regs_mentioned_p (XEXP (pat, i)))
	return 1;
    }

  return 0;
}

/* Return nonzero if INSN mentions stacked registers, else return zero.  */

int
stack_regs_mentioned (insn)
     rtx insn;
{
  unsigned int uid, max;
  int test;

  if (! INSN_P (insn) || !stack_regs_mentioned_data)
    return 0;

  uid = INSN_UID (insn);
  max = VARRAY_SIZE (stack_regs_mentioned_data);
  if (uid >= max)
    {
      /* Allocate some extra size to avoid too many reallocs, but
	 do not grow too quickly.  */
      max = uid + uid / 20;
      VARRAY_GROW (stack_regs_mentioned_data, max);
    }

  test = VARRAY_CHAR (stack_regs_mentioned_data, uid);
  if (test == 0)
    {
      /* This insn has yet to be examined.  Do so now.  */
      te