/* Convert function calls to rtl insns, for GNU C compiler.
   Copyright (C) 1989, 1992, 1993, 1994, 1995, 1996, 1997, 1998
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

#include "config.h"
#include "system.h"
#include "rtl.h"
#include "tree.h"
#include "flags.h"
#include "expr.h"
#include "libfuncs.h"
#include "function.h"
#include "regs.h"
#include "toplev.h"
#include "output.h"
#include "tm_p.h"
#include "timevar.h"
#include "sbitmap.h"

#if !defined FUNCTION_OK_FOR_SIBCALL
#define FUNCTION_OK_FOR_SIBCALL(DECL) 1
#endif

/* Decide whether a function's arguments should be processed
   from first to last or from last to first.

   They should if the stack and args grow in opposite directions, but
   only if we have push insns.  */

#ifdef PUSH_ROUNDING

#if defined (STACK_GROWS_DOWNWARD) != defined (ARGS_GROW_DOWNWARD)
#define PUSH_ARGS_REVERSED  PUSH_ARGS
#endif

#endif

#ifndef PUSH_ARGS_REVERSED
#define PUSH_ARGS_REVERSED 0
#endif

#ifndef STACK_POINTER_OFFSET
#define STACK_POINTER_OFFSET    0
#endif

/* Like PREFERRED_STACK_BOUNDARY but in units of bytes, not bits.  */
#define STACK_BYTES (PREFERRED_STACK_BOUNDARY / BITS_PER_UNIT)

/* Data structure and subroutines used within expand_call.  */

struct arg_data
{
  /* Tree node for this argument.  */
  tree tree_value;
  /* Mode for value; TYPE_MODE unless promoted.  */
  enum machine_mode mode;
  /* Current RTL value for argument, or 0 if it isn't precomputed.  */
  rtx value;
  /* Initially-compute RTL value for argument; only for const functions.  */
  rtx initial_value;
  /* Register to pass this argument in, 0 if passed on stack, or an
     PARALLEL if the arg is to be copied into multiple non-contiguous
     registers.  */
  rtx reg;
  /* Register to pass this argument in when generating tail call sequence.
     This is not the same register as for normal calls on machines with
     register windows.  */
  rtx tail_call_reg;
  /* If REG was promoted from the actual mode of the argument expression,
     indicates whether the promotion is sign- or zero-extended.  */
  int unsignedp;
  /* Number of registers to use.  0 means put the whole arg in registers.
     Also 0 if not passed in registers.  */
  int partial;
  /* Non-zero if argument must be passed on stack.
     Note that some arguments may be passed on the stack
     even though pass_on_stack is zero, just because FUNCTION_ARG says so.
     pass_on_stack identifies arguments that *cannot* go in registers.  */
  int pass_on_stack;
  /* Offset of this argument from beginning of stack-args.  */
  struct args_size offset;
  /* Similar, but offset to the start of the stack slot.  Different from
     OFFSET if this arg pads downward.  */
  struct args_size slot_offset;
  /* Size of this argument on the stack, rounded up for any padding it gets,
     parts of the argument passed in registers do not count.
     If REG_PARM_STACK_SPACE is defined, then register parms
     are counted here as well.  */
  struct args_size size;
  /* Location on the stack at which parameter should be stored.  The store
     has already been done if STACK == VALUE.  */
  rtx stack;
  /* Location on the stack of the start of this argument slot.  This can
     differ from STACK if this arg pads downward.  This location is known
     to be aligned to FUNCTION_ARG_BOUNDARY.  */
  rtx stack_slot;
  /* Place that this stack area has been saved, if needed.  */
  rtx save_area;
  /* If an argument's alignment does not permit direct copying into registers,
     copy in smaller-sized pieces into pseudos.  These are stored in a
     block pointed to by this field.  The next field says how many
     word-sized pseudos we made.  */
  rtx *aligned_regs;
  int n_aligned_regs;
  /* The amount that the stack pointer needs to be adjusted to
     force alignment for the next argument.  */
  struct args_size alignment_pad;
};

/* A vector of one char per byte of stack space.  A byte if non-zero if
   the corresponding stack location has been used.
   This vector is used to prevent a function call within an argument from
   clobbering any stack already set up.  */
static char *stack_usage_map;

/* Size of STACK_USAGE_MAP.  */
static int highest_outgoing_arg_in_use;

/* A bitmap of virtual-incoming stack space.  Bit is set if the corresponding
   stack location's tail call argument has been already stored into the stack.
   This bitmap is used to prevent sibling call optimization if function tries
   to use parent's incoming argument slots when they have been already
   overwritten with tail call arguments.  */
static sbitmap stored_args_map;

/* stack_arg_under_construction is nonzero when an argument may be
   initialized with a constructor call (including a C function that
   returns a BLKmode struct) and expand_call must take special action
   to make sure the object being constructed does not overlap the
   argument list for the constructor call.  */
int stack_arg_under_construction;

static int calls_function	PARAMS ((tree, int));
static int calls_function_1	PARAMS ((tree, int));

/* Nonzero if this is a call to a `const' function.  */
#define ECF_CONST		1
/* Nonzero if this is a call to a `volatile' function.  */
#define ECF_NORETURN		2
/* Nonzero if this is a call to malloc or a related function.  */
#define ECF_MALLOC		4
/* Nonzero if it is plausible that this is a call to alloca.  */
#define ECF_MAY_BE_ALLOCA	8
/* Nonzero if this is a call to a function that won't throw an exception.  */
#define ECF_NOTHROW		16
/* Nonzero if this is a call to setjmp or a related function.  */
#define ECF_RETURNS_TWICE	32
/* Nonzero if this is a call to `longjmp'.  */
#define ECF_LONGJMP		64
/* Nonzero if this is a syscall that makes a new process in the image of
   the current one.  */
#define ECF_FORK_OR_EXEC	128
#define ECF_SIBCALL		256
/* Nonzero if this is a call to "pure" function (like const function,
   but may read memory.  */
#define ECF_PURE		512
/* Nonzero if this is a call to a function that returns with the stack
   pointer depressed.  */
#define ECF_SP_DEPRESSED	1024
/* Nonzero if this call is known to always return.  */
#define ECF_ALWAYS_RETURN	2048
/* Create libcall block around the call.  */
#define ECF_LIBCALL_BLOCK	4096

static void emit_call_1		PARAMS ((rtx, tree, tree, HOST_WIDE_INT,
					 HOST_WIDE_INT, HOST_WIDE_INT, rtx,
					 rtx, int, rtx, int,
					 CUMULATIVE_ARGS *));
static void precompute_register_parameters	PARAMS ((int,
							 struct arg_data *,
							 int *));
static int store_one_arg	PARAMS ((struct arg_data *, rtx, int, int,
					 int));
static void store_unaligned_arguments_into_pseudos PARAMS ((struct arg_data *,
							    int));
static int finalize_must_preallocate		PARAMS ((int, int,
							 struct arg_data *,
							 struct args_size *));
static void precompute_arguments 		PARAMS ((int, int,
							 struct arg_data *));
static int compute_argument_block_size		PARAMS ((int,
							 struct args_size *,
							 int));
static void initialize_argument_information	PARAMS ((int,
							 struct arg_data *,
							 struct args_size *,
							 int, tree, tree,
							 CUMULATIVE_ARGS *,
							 int, rtx *, int *,
							 int *, int *));
static void compute_argument_addresses		PARAMS ((struct arg_data *,
							 rtx, int));
static rtx rtx_for_function_call		PARAMS ((tree, tree));
static void load_register_parameters		PARAMS ((struct arg_data *,
							 int, rtx *, int));
static rtx emit_library_call_value_1 		PARAMS ((int, rtx, rtx,
							 enum libcall_type,
							 enum machine_mode,
							 int, va_list));
static int special_function_p			PARAMS ((tree, int));
static int flags_from_decl_or_type 		PARAMS ((tree));
static rtx try_to_integrate			PARAMS ((tree, tree, rtx,
							 int, tree, rtx));
static int check_sibcall_argument_overlap_1	PARAMS ((rtx));
static int check_sibcall_argument_overlap	PARAMS ((rtx, struct arg_data *));

static int combine_pending_stack_adjustment_and_call
                                                PARAMS ((int, struct args_size *, int));

#ifdef REG_PARM_STACK_SPACE
static rtx save_fixed_argument_area	PARAMS ((int, rtx, int *, int *));
static void restore_fixed_argument_area	PARAMS ((rtx, rtx, int, int));
#endif

/* If WHICH is 1, return 1 if EXP contains a call to the built-in function
   `alloca'.

   If WHICH is 0, return 1 if EXP contains a call to any function.
   Actually, we only need return 1 if evaluating EXP would require pushing
   arguments on the stack, but that is too difficult to compute, so we just
   assume any function call might require the stack.  */

static tree calls_function_save_exprs;

static int
calls_function (exp, which)
     tree exp;
     int which;
{
  int val;

  calls_function_save_exprs = 0;
  val = calls_function_1 (exp, which);
  calls_function_save_exprs = 0;
  return val;
}

/* Recursive function to do the work of above function.  */

static int
calls_function_1 (exp, which)
     tree exp;
     int which;
{
  int i;
  enum tree_code code = TREE_CODE (exp);
  int class = TREE_CODE_CLASS (code);
  int length = first_rtl_op (code);

  /* If this code is language-specific, we don't know what it will do.  */
  if ((int) code >= NUM_TREE_CODES)
    return 1;

  switch (code)
    {
    case CALL_EXPR:
      if (which == 0)
	return 1;
      else if ((TREE_CODE (TREE_TYPE (TREE_TYPE (TREE_OPERAND (exp, 0))))
		== FUNCTION_TYPE)
	       && (TYPE_RETURNS_STACK_DEPRESSED
		   (TREE_TYPE (TREE_TYPE (TREE_OPERAND (exp, 0))))))
	return 1;
      else if (TREE_CODE (TREE_OPERAND (exp, 0)) == ADDR_EXPR
	       && (TREE_CODE (TREE_OPERAND (TREE_OPERAND (exp, 0), 0))
		   == FUNCTION_DECL)
	       && (special_function_p (TREE_OPERAND (TREE_OPERAND (exp, 0), 0),
				       0)
		   & ECF_MAY_BE_ALLOCA))
	return 1;

      break;

    case CONSTRUCTOR:
      {
	tree tem;

	for (tem = CONSTRUCTOR_ELTS (exp); tem != 0; tem = TREE_CHAIN (tem))
	  if (calls_function_1 (TREE_VALUE (tem), which))
	    return 1;
      }

      return 0;

    case SAVE_EXPR:
      if (SAVE_EXPR_RTL (exp) != 0)
	return 0;
      if (value_member (exp, calls_function_save_exprs))
	return 0;
      calls_function_save_exprs = tree_cons (NULL_TREE, exp,
					     calls_function_save_exprs);
      return (TREE_OPERAND (exp, 0) != 0
	      && calls_function_1 (TREE_OPERAND (exp, 0), which));

    case BLOCK:
      {
	tree local;
	tree subblock;

	for (local = BLOCK_VARS (exp); local; local = TREE_CHAIN (local))
	  if (DECL_INITIAL (local) != 0
	      && calls_function_1 (DECL_INITIAL (local), which))
	    return 1;

	for (subblock = BLOCK_SUBBLOCKS (exp);
	     subblock;
	     subblock = TREE_CHAIN (subblock))
	  if (calls_function_1 (subblock, which))
	    return 1;
      }
      return 0;

    case TREE_LIST:
      for (; exp != 0; exp = TREE_CHAIN (exp))
	if (calls_function_1 (TREE_VALUE (exp), which))
	  return 1;
      return 0;

    default:
      break;
    }

  /* Only expressions, references, and blocks can contain calls.  */
  if (! IS_EXPR_CODE_CLASS (class) && class != 'r' && class != 'b')
    return 0;

  for (i = 0; i < length; i++)
    if (TREE_OPERAND (exp, i) != 0
	&& calls_function_1 (TREE_OPERAND (exp, i), which))
      return 1;

  return 0;
}

/* Force FUNEXP into a form suitable for the address of a CALL,