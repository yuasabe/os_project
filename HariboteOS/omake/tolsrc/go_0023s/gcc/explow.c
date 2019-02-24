/* Subroutines for manipulating rtx's in semantically interesting ways.
   Copyright (C) 1987, 1991, 1994, 1995, 1996, 1997, 1998,
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


#include "config.h"
#include "system.h"
#include "toplev.h"
#include "rtl.h"
#include "tree.h"
#include "tm_p.h"
#include "flags.h"
#include "function.h"
#include "expr.h"
#include "optabs.h"
#include "hard-reg-set.h"
#include "insn-config.h"
#include "ggc.h"
#include "recog.h"

static rtx break_out_memory_refs	PARAMS ((rtx));
static void emit_stack_probe		PARAMS ((rtx));


/* Truncate and perhaps sign-extend C as appropriate for MODE.  */

HOST_WIDE_INT
trunc_int_for_mode (c, mode)
     HOST_WIDE_INT c;
     enum machine_mode mode;
{
  int width = GET_MODE_BITSIZE (mode);

  /* Canonicalize BImode to 0 and STORE_FLAG_VALUE.  */
  if (mode == BImode)
    return c & 1 ? STORE_FLAG_VALUE : 0;

  /* Sign-extend for the requested mode.  */

  if (width < HOST_BITS_PER_WIDE_INT)
    {
      HOST_WIDE_INT sign = 1;
      sign <<= width - 1;
      c &= (sign << 1) - 1;
      c ^= sign;
      c -= sign;
    }

  return c;
}

/* Return an rtx for the sum of X and the integer C.

   This function should be used via the `plus_constant' macro.  */

rtx
plus_constant_wide (x, c)
     rtx x;
     HOST_WIDE_INT c;
{
  RTX_CODE code;
  rtx y;
  enum machine_mode mode;
  rtx tem;
  int all_constant = 0;

  if (c == 0)
    return x;

 restart:

  code = GET_CODE (x);
  mode = GET_MODE (x);
  y = x;

  switch (code)
    {
    case CONST_INT:
      return GEN_INT (INTVAL (x) + c);

    case CONST_DOUBLE:
      {
	unsigned HOST_WIDE_INT l1 = CONST_DOUBLE_LOW (x);
	HOST_WIDE_INT h1 = CONST_DOUBLE_HIGH (x);
	unsigned HOST_WIDE_INT l2 = c;
	HOST_WIDE_INT h2 = c < 0 ? ‾0 : 0;
	unsigned HOST_WIDE_INT lv;
	HOST_WIDE_INT hv;

	add_double (l1, h1, l2, h2, &lv, &hv);

	return immed_double_const (lv, hv, VOIDmode);
      }

    case MEM:
      /* If this is a reference to the constant pool, try replacing it with
	 a reference to a new constant.  If the resulting address isn't
	 valid, don't return it because we have no way to validize it.  */
      if (GET_CODE (XEXP (x, 0)) == SYMBOL_REF
	  && CONSTANT_POOL_ADDRESS_P (XEXP (x, 0)))
	{
	  tem
	    = force_const_mem (GET_MODE (x),
			       plus_constant (get_pool_constant (XEXP (x, 0)),
					      c));
	  if (memory_address_p (GET_MODE (tem), XEXP (tem, 0)))
	    return tem;
	}
      break;

    case CONST:
      /* If adding to something entirely constant, set a flag
	 so that we can add a CONST around the result.  */
      x = XEXP (x, 0);
      all_constant = 1;
      goto restart;

    case SYMBOL_REF:
    case LABEL_REF:
      all_constant = 1;
      break;

    case PLUS:
      /* The interesting case is adding the integer to a sum.
	 Look for constant term in the sum and combine
	 with C.  For an integer constant term, we make a combined
	 integer.  For a constant term that is not an explicit integer,
	 we cannot really combine, but group them together anyway.

	 Restart or use a recursive call in case the remaining operand is
	 something that we handle specially, such as a SYMBOL_REF.

	 We may not immediately return from the recursive call here, lest
	 all_constant gets lost.  */

      if (GET_CODE (XEXP (x, 1)) == CONST_INT)
	{
	  c += INTVAL (XEXP (x, 1));

	  if (GET_MODE (x) != VOIDmode)
	    c = trunc_int_for_mode (c, GET_MODE (x));

	  x = XEXP (x, 0);
	  goto restart;
	}
      else if (CONSTANT_P (XEXP (x, 1)))
	{
	  x = gen_rtx_PLUS (mode, XEXP (x, 0), plus_constant (XEXP (x, 1), c));
	  c = 0;
	}
      else if (find_constant_term_loc (&y))
	{
	  /* We need to be careful since X may be shared and we can't
	     modify it in place.  */
	  rtx copy = copy_rtx (x);
	  rtx *const_loc = find_constant_term_loc (&copy);

	  *const_loc = plus_constant (*const_loc, c);
	  x = copy;
	  c = 0;
	}
      break;

    default:
      break;
    }

  if (c != 0)
    x = gen_rtx_PLUS (mode, x, GEN_INT (c));

  if (GET_CODE (x) == SYMBOL_REF || GET_CODE (x) == LABEL_REF)
    return x;
  else if (all_constant)
    return gen_rtx_CONST (mode, x);
  else
    return x;
}

/* If X is a sum, return a new sum like X but lacking any constant terms.
   Add all the removed constant terms into *CONSTPTR.
   X itself is not altered.  The result != X if and only if
   it is not isomorphic to X.  */

rtx
eliminate_constant_term (x, constptr)
     rtx x;
     rtx *constptr;
{
  rtx x0, x1;
  rtx tem;

  if (GET_CODE (x) != PLUS)
    return x;

  /* First handle constants appearing at this level explicitly.  */
  if (GET_CODE (XEXP (x, 1)) == CONST_INT
      && 0 != (tem = simplify_binary_operation (PLUS, GET_MODE (x), *constptr,
						XEXP (x, 1)))
      && GET_CODE (tem) == CONST_INT)
    {
      *constptr = tem;
      return eliminate_constant_term (XEXP (x, 0), constptr);
    }

  tem = const0_rtx;
  x0 = eliminate_constant_term (XEXP (x, 0), &tem);
  x1 = eliminate_constant_term (XEXP (x, 1), &tem);
  if ((x1 != XEXP (x, 1) || x0 != XEXP (x, 0))
      && 0 != (tem = simplify_binary_operation (PLUS, GET_MODE (x),
						*constptr, tem))
      && GET_CODE (tem) == CONST_INT)
    {
      *constptr = tem;
      return gen_rtx_PLUS (GET_MODE (x), x0, x1);
    }

  return x;
}

/* Returns the insn that next references REG after INSN, or 0
   if REG is clobbered before next referenced or we cannot find
   an insn that references REG in a straight-line piece of code.  */

rtx
find_next_ref (reg, insn)
     rtx reg;
     rtx insn;
{
  rtx next;

  for (insn = NEXT_INSN (insn); insn; insn = next)
    {
      next = NEXT_INSN (insn);
      if (GET_CODE (insn) == NOTE)
	continue;
      if (GET_CODE (insn) == CODE_LABEL
	  || GET_CODE (insn) == BARRIER)
	return 0;
      if (GET_CODE (insn) == INSN
	  || GET_CODE (insn) == JUMP_INSN
	  || GET_CODE (insn) == CALL_INSN)
	{
	  if (reg_set_p (reg, insn))
	    return 0;
	  if (reg_mentioned_p (reg, PATTERN (insn)))
	    return insn;
	  if (GET_CODE (insn) == JUMP_INSN)
	    {
	      if (any_uncondjump_p (insn))
		next = JUMP_LABEL (insn);
	      else
		return 0;
	    }
	  if (GET_CODE (insn) == CALL_INSN
	      && REGNO (reg) < FIRST_PSEUDO_REGISTER
	      && call_used_regs[REGNO (reg)])
	    return 0;
	}
      else
	abort ();
    }
  return 0;
}

/* Return an rtx for the size in bytes of the value of EXP.  */

rtx
expr_size (exp)
     tree exp;
{
  tree size;

  if (TREE_CODE_CLASS (TREE_CODE (exp)) == 'd'
      && DECL_SIZE_UNIT (exp) != 0)
    size = DECL_SIZE_UNIT (exp);
  else
    size = size_in_bytes (TREE_TYPE (exp));

  if (TREE_CODE (size) != INTEGER_CST
      && contains_placeholder_p (size))
    size = build (WITH_RECORD_EXPR, sizetype, size, exp);

  return expand_expr (size, NULL_RTX, TYPE_MODE (sizetype), 0);

}

/* Return a copy of X in which all memory references
   and all constants that involve symbol refs
   have been replaced with new temporary registers.
   Also emit code to load the memory locations and constants
   into those registers.

   If X contains no such constants or memory references,
   X itself (not a copy) is returned.

   If a constant is found in the address that is not a legitimate constant
   in an insn, it is left alone in the hope that it might be valid in the
   address.

   X may contain no arithmetic except addition, subtraction and multiplication.
   Values returned by expand_expr with 1 for sum_ok fit this constraint.  */

static rtx
break_out_memory_refs (x)
     rtx x;
{
  if (GET_CODE (x) == MEM
      || (CONSTANT_P (x) && CONSTANT_ADDRESS_P (x)
	  && GET_MODE (x) != VOIDmode))
    x = force_reg (GET_MODE (x), x);
  else if (GET_CODE (x) == PLUS || GET_CODE (x) == MINUS
	   || GET_CODE (x) == MULT)
    {
      rtx op0 = break_out_memory_refs (XEXP (x, 0));
      rtx op1 = break_out_memory_refs (XEXP (x, 1));

      if (op0 != XEXP (x, 0) || op1 != XEXP (x, 1))
	x = gen_rtx_fmt_ee (GET_CODE (x), Pmode, op0, op1);
    }

  return x;
}

#ifdef POINTERS_EXTEND_UNSIGNED

/* Given X, a memory address in ptr_mode, convert it to an address
   in Pmode, or vice versa (TO_MODE says which way).  We take advantage of
   the fact that pointers are not allowed to overflow by commuting arithmetic
   operations over conversions so that address arithmetic insns can be
   used.  */

rtx
convert_memory_address (to_mode, x)
     enum machine_mode to_mode;
     rtx x;
{
  enum machine_mode from_mode = to_mode == ptr_mode ? Pmode : ptr_mode;
  rtx temp;

  /* Here we handle some special cases.  If none of them apply, fall through
     to the default case.  */
  switch (GET_CODE (x))
    {
    case CONST_INT:
    case CONST_DOUBLE:
      return x;

    case SUBREG:
      if (POINTERS_EXTEND_UNSIGNED >= 0
	  && (SUBREG_PROMOTED_VAR_P (x) || REG_POINTER (SUBREG_REG (x)))
	  && GET_MODE (SUBREG_REG (x)) == to_mode)
	return SUBREG_REG (x);
      break;

    case LABEL_REF:
      if (POINTERS_EXTEND_UNSIGNED >= 0)
	{
	  temp = gen_rtx_LABEL_REF (to_mode, XEXP (x, 0));
	  LABEL_REF_NONLOCAL_P (temp) = LABEL_REF_NONLOCAL_P (x);
	  return temp;
	}
      break;

    case SYMBOL_REF:
      if (POINTERS_EXTEND_UNSIGNED >= 0)
	{
	  temp = gen_rtx_SYMBOL_REF (to_mode, XSTR (x, 0));
	  SYMBOL_REF_FLAG (temp) = SYMBOL_REF_FLAG (x);
	  CONSTANT_POOL_ADDRESS_P (temp) = CONSTANT_POOL_ADDRESS_P (x);
	  STRING_POOL_ADDRESS_P (temp) = STRING_POOL_ADDRESS_P (x);
	  return temp;
	}
      break;

    case CONST:
      if (POINTERS_EXTEND_UNSIGNED >= 0)
        return gen_rtx_CONST (to_mode,
			      convert_memory_address (to_mode, XEXP (x, 0)));
      break;

    case PLUS:
    case MULT:
      /* For addition the second operand is a small constant, we can safely
	 permute the conversion and addition operation.  We can always safely
	 permute them if we are making the address narrower.  In addition,
	 always permute the operations if this is a constant.  */
      if (POINTERS_EXTEND_UNSIGNED >= 0
	  && (GET_MODE_SIZE (to_mode) < GET_MODE_SIZE (from_mode)
	      || (GET_CODE (x) == PLUS && GET_CODE (XEXP (x, 1)) == CONST_INT
		  && (INTVAL (XEXP (x, 1)) + 20000 < 40000
		      || CONSTANT_P (XEXP (x, 0))))))
	return gen_rtx_fmt_ee (GET_CODE (x), to_mode,
			       convert_memory_address (to_mode, XEXP (x, 0)),
			       convert_memory_address (to_mode, XEXP (x, 1)));
      break;

    default:
      break;
    }

  return convert_modes (to_mode, from_mode,
			x, POINTERS_EXTEND_UNSIGNED);
}
#endif

/* Given a memory address or facsimile X, construct a new address,
   currently equivalent, that is stable: future stores won't change it.

   X must be composed of constants, register and memory references
   combined with addition, subtraction and multiplication:
   in other words, just what you can get from expand_expr if sum_ok is 1.

   Works by making copies of all regs and memory locations used
   by X and combining them the same way X does.
   You could also stabilize the reference to this address
   by copying the address to a register with copy_to_reg;
   but then you wouldn't get indexed addressing in the reference.  */

rtx
copy_all_regs (x)
     rtx x;
{
  if (GET_CODE (x) == REG)
    {
      if (REGNO (x) != FRAME_POINTER_REGNUM
#if HARD_FRAME_POINTER_REGNUM != FRAME_POINTER_REGNUM
	  && REGNO (x) != HARD_FRAME_POINTER_REGNUM
#endif
	  )
	x = copy_to_reg (x);
    }
  else if (GET_CODE (x) == MEM)
    x = copy_to_reg (x);
  else if (GET_CODE (x) == PLUS || GET_CODE (x) == MINUS
	   || GET_CODE (x) == MULT)
    {
      rtx op0 = copy_all_regs (XEXP (x, 0));
      rtx op1 = copy_all_regs (XEXP (x, 1));
      if (op0 != XEXP (x, 0) || op1 != XEXP (x, 1))
	x = gen_rtx_fmt_ee (GET_CODE (x), Pmode, op0, op1);
    }
  return x;
}

/* Return something equivalent to X but valid as a memory address
   for something of mode MODE.  When X is not itself valid, this
   works by copying X or subexpressions of it into registers.  */

rtx
memory_address (mode, x)
     enum machine_mode mode;
     rtx x;
{
  rtx oldx = x;

  if (GET_CODE (x) == ADDRESSOF)
    return x;

#ifdef POINTERS_EXTEND_UNSIGNED
  if (GET_MODE (x) != Pmode)
    x = convert_memory_address (Pmode, x);
#endif

  /* By passing constant addresses thru registers
     we get a chance to cse them.  */
  if (! cse_not_expected && CONSTANT_P (x) && CONSTANT_ADDRESS_P (x))
    x = force_reg (Pmode, x);

  /* Accept a QUEUED that refers to a REG
     even though that isn't a valid address.
     On attempting to put this in an insn we will call protect_from_queue
     which will turn it into a REG, which is valid.  */
  else if (GET_CODE (x) == QUEUED
      && GET_CODE (QUEUED_VAR (x)) == REG)
    ;

  /* We get better cse by rejecting indirect addressing at this stage.
     Let the combiner create indirect addresses where appropriate.
     For now, generate the code so that the subexpressions useful to share
     are visible.  But not if cse won't be done!  */
  else
    {
      if (! cse_not_expected && GET_CODE (x) != REG)
	x = break_out_memory_refs (x);

      /* At this point, any valid address is accepted.  */
      GO_IF_LEGITIMATE_ADDRESS (mode, x, win);

      /* If it was valid before but breaking out memory refs invalidated it,
	 use it the old way.  */
      if (memory_address_p (mode, oldx))
	goto win2;

      /* Perform machine-dependent transformations on X
	 in certain cases.  This is not necessary since the code
	 below can handle all possible cases, but machine-dependent
	 transformations can make better code.  */
      LEGITIMIZE_ADDRESS (x, oldx, mode, win);

      /* PLUS and MULT can appear in special ways
	 as the result of attempts to make an address usable for indexing.
	 Usually they are dealt with by calling force_operand, below.
	 But a sum containing constant terms is special
	 if removing them makes the sum a valid address:
	 then we generate that address in a register
	 and index off of it.  We do this because it often makes
	 shorter code, and because the addresses thus generated
	 in registers often become common subexpressions.  */
      if (GET_CODE (x) == PLUS)
	{
	  rtx constant_term = const0_rtx;
	  rtx y = eliminate_constant_term (x, &constant_term);
	  if (constant_term == const0_rtx
	      || ! memory_address_p (mode, y))
	    x = force_operand (x, NULL_RTX);
	  else
	    {
	      y = gen_rtx_PLUS (GET_MODE (x), copy_to_reg (y), constant_term);
	      if (! memory_address_p (mode, y))
		x = force_operand (x, NULL_RTX);
	      else
		x = y;
	    }
	}

      else if (GET_CODE (x) == MULT || GET_CODE (x) == MINUS)
	x = force_operand (x, NULL_RTX);

      /* If we have a register that's an invalid address,
	 it must be a hard reg of the wrong class.  Copy it to a pseudo.  */
      else if (GET_CODE (x) == REG)
	x = copy_to_reg (x);

      /* Last resort: copy the value to a register, since
	 the register is a valid address.  */
      else
	x = force_reg (Pmode, x);

      goto done;

    win2:
      x = oldx;
    win:
      if (flag_force_addr && ! cse_not_expected && GET_CODE (x) != REG
	  /* Don't copy an addr via a reg if it is one of our stack slots.  */
	  && ! (GET_CODE (x) == PLUS
		&& (XEXP (x, 0) == virtual_stack_vars_rtx
	