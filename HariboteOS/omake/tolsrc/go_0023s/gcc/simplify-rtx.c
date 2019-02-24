/* RTL simplification functions for GNU compiler.
   Copyright (C) 1987, 1988, 1989, 1992, 1993, 1994, 1995, 1996, 1997, 1998,
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
#include "tm_p.h"
#include "regs.h"
#include "hard-reg-set.h"
#include "flags.h"
#include "real.h"
#include "insn-config.h"
#include "recog.h"
#include "function.h"
#include "expr.h"
#include "toplev.h"
#include "output.h"
#include "ggc.h"

/* Simplification and canonicalization of RTL.  */

/* Nonzero if X has the form (PLUS frame-pointer integer).  We check for
   virtual regs here because the simplify_*_operation routines are called
   by integrate.c, which is called before virtual register instantiation.

   ?!? FIXED_BASE_PLUS_P and NONZERO_BASE_PLUS_P need to move into 
   a header file so that their definitions can be shared with the
   simplification routines in simplify-rtx.c.  Until then, do not
   change these macros without also changing the copy in simplify-rtx.c.  */

#define FIXED_BASE_PLUS_P(X)					¥
  ((X) == frame_pointer_rtx || (X) == hard_frame_pointer_rtx	¥
   || ((X) == arg_pointer_rtx && fixed_regs[ARG_POINTER_REGNUM])¥
   || (X) == virtual_stack_vars_rtx				¥
   || (X) == virtual_incoming_args_rtx				¥
   || (GET_CODE (X) == PLUS && GET_CODE (XEXP (X, 1)) == CONST_INT ¥
       && (XEXP (X, 0) == frame_pointer_rtx			¥
	   || XEXP (X, 0) == hard_frame_pointer_rtx		¥
	   || ((X) == arg_pointer_rtx				¥
	       && fixed_regs[ARG_POINTER_REGNUM])		¥
	   || XEXP (X, 0) == virtual_stack_vars_rtx		¥
	   || XEXP (X, 0) == virtual_incoming_args_rtx))	¥
   || GET_CODE (X) == ADDRESSOF)

/* Similar, but also allows reference to the stack pointer.

   This used to include FIXED_BASE_PLUS_P, however, we can't assume that
   arg_pointer_rtx by itself is nonzero, because on at least one machine,
   the i960, the arg pointer is zero when it is unused.  */

#define NONZERO_BASE_PLUS_P(X)					¥
  ((X) == frame_pointer_rtx || (X) == hard_frame_pointer_rtx	¥
   || (X) == virtual_stack_vars_rtx				¥
   || (X) == virtual_incoming_args_rtx				¥
   || (GET_CODE (X) == PLUS && GET_CODE (XEXP (X, 1)) == CONST_INT ¥
       && (XEXP (X, 0) == frame_pointer_rtx			¥
	   || XEXP (X, 0) == hard_frame_pointer_rtx		¥
	   || ((X) == arg_pointer_rtx				¥
	       && fixed_regs[ARG_POINTER_REGNUM])		¥
	   || XEXP (X, 0) == virtual_stack_vars_rtx		¥
	   || XEXP (X, 0) == virtual_incoming_args_rtx))	¥
   || (X) == stack_pointer_rtx					¥
   || (X) == virtual_stack_dynamic_rtx				¥
   || (X) == virtual_outgoing_args_rtx				¥
   || (GET_CODE (X) == PLUS && GET_CODE (XEXP (X, 1)) == CONST_INT ¥
       && (XEXP (X, 0) == stack_pointer_rtx			¥
	   || XEXP (X, 0) == virtual_stack_dynamic_rtx		¥
	   || XEXP (X, 0) == virtual_outgoing_args_rtx))	¥
   || GET_CODE (X) == ADDRESSOF)

/* Much code operates on (low, high) pairs; the low value is an
   unsigned wide int, the high value a signed wide int.  We
   occasionally need to sign extend from low to high as if low were a
   signed wide int.  */
#define HWI_SIGN_EXTEND(low) ¥
 ((((HOST_WIDE_INT) low) < 0) ? ((HOST_WIDE_INT) -1) : ((HOST_WIDE_INT) 0))

static rtx neg_const_int PARAMS ((enum machine_mode, rtx));
static int simplify_plus_minus_op_data_cmp PARAMS ((const void *,
						    const void *));
static rtx simplify_plus_minus		PARAMS ((enum rtx_code,
						 enum machine_mode, rtx,
						 rtx, int));
static void check_fold_consts		PARAMS ((PTR));
#if ! defined (REAL_IS_NOT_DOUBLE) || defined (REAL_ARITHMETIC)
static void simplify_unary_real		PARAMS ((PTR));
static void simplify_binary_real	PARAMS ((PTR));
#endif
static void simplify_binary_is2orm1	PARAMS ((PTR));


/* Negate a CONST_INT rtx, truncating (because a conversion from a
   maximally negative number can overflow).  */
static rtx
neg_const_int (mode, i)
     enum machine_mode mode;
     rtx i;
{
  return GEN_INT (trunc_int_for_mode (- INTVAL (i), mode));
}


/* Make a binary operation by properly ordering the operands and 
   seeing if the expression folds.  */

rtx
simplify_gen_binary (code, mode, op0, op1)
     enum rtx_code code;
     enum machine_mode mode;
     rtx op0, op1;
{
  rtx tem;

  /* Put complex operands first and constants second if commutative.  */
  if (GET_RTX_CLASS (code) == 'c'
      && swap_commutative_operands_p (op0, op1))
    tem = op0, op0 = op1, op1 = tem;

  /* If this simplifies, do it.  */
  tem = simplify_binary_operation (code, mode, op0, op1);
  if (tem)
    return tem;

  /* Handle addition and subtraction specially.  Otherwise, just form
     the operation.  */

  if (code == PLUS || code == MINUS)
    {
      tem = simplify_plus_minus (code, mode, op0, op1, 1);
      if (tem)
	return tem;
    }

  return gen_rtx_fmt_ee (code, mode, op0, op1);
}

/* If X is a MEM referencing the constant pool, return the real value.
   Otherwise return X.  */
rtx
avoid_constant_pool_reference (x)
     rtx x;
{
  rtx c, addr;
  enum machine_mode cmode;

  if (GET_CODE (x) != MEM)
    return x;
  addr = XEXP (x, 0);

  if (GET_CODE (addr) != SYMBOL_REF
      || ! CONSTANT_POOL_ADDRESS_P (addr))
    return x;

  c = get_pool_constant (addr);
  cmode = get_pool_mode (addr);

  /* If we're accessing the constant in a different mode than it was
     originally stored, attempt to fix that up via subreg simplifications.
     If that fails we have no choice but to return the original memory.  */
  if (cmode != GET_MODE (x))
    {
      c = simplify_subreg (GET_MODE (x), c, cmode, 0);
      return c ? c : x;
    }

  return c;
}

/* Make a unary operation by first seeing if it folds and otherwise making
   the specified operation.  */

rtx
simplify_gen_unary (code, mode, op, op_mode)
     enum rtx_code code;
     enum machine_mode mode;
     rtx op;
     enum machine_mode op_mode;
{
  rtx tem;

  /* If this simplifies, use it.  */
  if ((tem = simplify_unary_operation (code, mode, op, op_mode)) != 0)
    return tem;

  return gen_rtx_fmt_e (code, mode, op);
}

/* Likewise for ternary operations.  */

rtx
simplify_gen_ternary (code, mode, op0_mode, op0, op1, op2)
     enum rtx_code code;
     enum machine_mode mode, op0_mode;
     rtx op0, op1, op2;
{
  rtx tem;

  /* If this simplifies, use it.  */
  if (0 != (tem = simplify_ternary_operation (code, mode, op0_mode,
					      op0, op1, op2)))
    return tem;

  return gen_rtx_fmt_eee (code, mode, op0, op1, op2);
}

/* Likewise, for relational operations.
   CMP_MODE specifies mode comparison is done in.
  */

rtx
simplify_gen_relational (code, mode, cmp_mode, op0, op1)
     enum rtx_code code;
     enum machine_mode mode;
     enum machine_mode cmp_mode;
     rtx op0, op1;
{
  rtx tem;

  if ((tem = simplify_relational_operation (code, cmp_mode, op0, op1)) != 0)
    return tem;

  /* Put complex operands first and constants second.  */
  if (swap_commutative_operands_p (op0, op1))
    tem = op0, op0 = op1, op1 = tem, code = swap_condition (code);

  return gen_rtx_fmt_ee (code, mode, op0, op1);
}

/* Replace all occurrences of OLD in X with NEW and try to simplify the
   resulting RTX.  Return a new RTX which is as simplified as possible.  */

rtx
simplify_replace_rtx (x, old, new)
     rtx x;
     rtx old;
     rtx new;
{
  enum rtx_code code = GET_CODE (x);
  enum machine_mode mode = GET_MODE (x);

  /* If X is OLD, return NEW.  Otherwise, if this is an expression, try
     to build a new expression substituting recursively.  If we can't do
     anything, return our input.  */

  if (x == old)
    return new;

  switch (GET_RTX_CLASS (code))
    {
    case '1':
      {
	enum machine_mode op_mode = GET_MODE (XEXP (x, 0));
	rtx op = (XEXP (x, 0) == old
		  ? new : simplify_replace_rtx (XEXP (x, 0), old, new));

	return simplify_gen_unary (code, mode, op, op_mode);
      }

    case '2':
    case 'c':
      return
	simplify_gen_binary (code, mode,
			     simplify_replace_rtx (XEXP (x, 0), old, new),
			     simplify_replace_rtx (XEXP (x, 1), old, new));
    case '<':
      {
	enum machine_mode op_mode = (GET_MODE (XEXP (x, 0)) != VOIDmode
				     ? GET_MODE (XEXP (x, 0))
				     : GET_MODE (XEXP (x, 1)));
	rtx op0 = simplify_replace_rtx (XEXP (x, 0), old, new);
	rtx op1 = simplify_replace_rtx (XEXP (x, 1), old, new);

	return
	  simplify_gen_relational (code, mode,
				   (op_mode != VOIDmode
				    ? op_mode
				    : GET_MODE (op0) != VOIDmode
				    ? GET_MODE (op0)
				    : GET_MODE (op1)),
				   op0, op1);
      }

    case '3':
    case 'b':
      {
	enum machine_mode op_mode = GET_MODE (XEXP (x, 0));
	rtx op0 = simplify_replace_rtx (XEXP (x, 0), old, new);

	return
	  simplify_gen_ternary (code, mode, 
				(op_mode != VOIDmode
				 ? op_mode
				 : GET_MODE (op0)),
				op0,
				simplify_replace_rtx (XEXP (x, 1), old, new),
				simplify_replace_rtx (XEXP (x, 2), old, new));
      }

    case 'x':
      /* The only case we try to handle is a SUBREG.  */
      if (code == SUBREG)
	{
	  rtx exp;
	  exp = simplify_gen_subreg (GET_MODE (x),
				     simplify_replace_rtx (SUBREG_REG (x),
				     			   old, new),
				     GET_MODE (SUBREG_REG (x)),
				     SUBREG_BYTE (x));
	  if (exp)
	   x = exp;
	}
      return x;

    default:
      if (GET_CODE (x) == MEM)
	return
	  replace_equiv_address_nv (x,
				    simplify_replace_rtx (XEXP (x, 0),
							  old, new));

      return x;
    }
  return x;
}

#if ! defined (REAL_IS_NOT_DOUBLE) || defined (REAL_ARITHMETIC)
/* Subroutine of simplify_unary_operation, called via do_float_handler.
   Handles simplification of unary ops on floating point values.  */
struct simplify_unary_real_args
{
  rtx operand;
  rtx result;
  enum machine_mode mode;
  enum rtx_code code;
  bool want_integer;
};
#define REAL_VALUE_ABS(d_) ¥
   (REAL_VALUE_NEGATIVE (d_) ? REAL_VALUE_NEGATE (d_) : (d_))

static void
simplify_unary_real (p)
     PTR p;
{
  REAL_VALUE_TYPE d;

  struct simplify_unary_real_args *args =
    (struct simplify_unary_real_args *) p;

  REAL_VALUE_FROM_CONST_DOUBLE (d, args->operand);

  if (args->want_integer)
    {
      HOST_WIDE_INT i;

      switch (args->code)
	{
	case FIX:		i = REAL_VALUE_FIX (d);		  break;
	case UNSIGNED_FIX:	i = REAL_VALUE_UNSIGNED_FIX (d);  break;
	default:
	  abort ();
	}
      args->result = GEN_INT (trunc_int_for_mode (i, args->mode));
    }
  else
    {
      switch (args->code)
	{
	case SQRT:
	  /* We don't attempt to optimize this.  */
	  args->result = 0;
	  return;

	case ABS:	      d = REAL_VALUE_ABS (d);			break;
	case NEG:	      d = REAL_VALUE_NEGATE (d);		break;
	case FLOAT_TRUNCATE:  d = real_value_truncate (args->mode, d);  break;
	case FLOAT_EXTEND:    /* All this does is change the mode.  */  break;
	case FIX:	      d = REAL_VALUE_RNDZINT (d);		break;
	case UNSIGNED_FIX:    d = REAL_VALUE_UNSIGNED_RNDZINT (d);	break;
	default:
	  abort ();
	}
      args->result = CONST_DOUBLE_FROM_REAL_VALUE (d, args->mode);
    }
}
#endif

/* Try to simplify a unary operation CODE whose output mode is to be
   MODE with input operand OP whose mode was originally OP_MODE.
   Return zero if no simplification can be made.  */
rtx
simplify_unary_operation (code, mode, op, op_mode)
     enum rtx_code code;
     enum machine_mo