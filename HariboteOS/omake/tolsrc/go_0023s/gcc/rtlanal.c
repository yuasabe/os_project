/* Analyze RTL for C-Compiler
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


#include "config.h"
#include "system.h"
#include "toplev.h"
#include "rtl.h"
#include "hard-reg-set.h"
#include "tm_p.h"

/* Forward declarations */
static void set_of_1		PARAMS ((rtx, rtx, void *));
static void insn_dependent_p_1	PARAMS ((rtx, rtx, void *));
static int computed_jump_p_1	PARAMS ((rtx));
static void parms_set 		PARAMS ((rtx, rtx, void *));

/* Bit flags that specify the machine subtype we are compiling for.
   Bits are tested using macros TARGET_... defined in the tm.h file
   and set by `-m...' switches.  Must be defined in rtlanal.c.  */

int target_flags;

/* Return 1 if the value of X is unstable
   (would be different at a different point in the program).
   The frame pointer, arg pointer, etc. are considered stable
   (within one function) and so is anything marked `unchanging'.  */

int
rtx_unstable_p (x)
     rtx x;
{
  RTX_CODE code = GET_CODE (x);
  int i;
  const char *fmt;

  switch (code)
    {
    case MEM:
      return ! RTX_UNCHANGING_P (x) || rtx_unstable_p (XEXP (x, 0));

    case QUEUED:
      return 1;

    case ADDRESSOF:
    case CONST:
    case CONST_INT:
    case CONST_DOUBLE:
    case CONST_VECTOR:
    case SYMBOL_REF:
    case LABEL_REF:
      return 0;

    case REG:
      /* As in rtx_varies_p, we have to use the actual rtx, not reg number.  */
      if (x == frame_pointer_rtx || x == hard_frame_pointer_rtx
	  /* The arg pointer varies if it is not a fixed register.  */
	  || (x == arg_pointer_rtx && fixed_regs[ARG_POINTER_REGNUM])
	  || RTX_UNCHANGING_P (x))
	return 0;
#ifndef PIC_OFFSET_TABLE_REG_CALL_CLOBBERED
      /* ??? When call-clobbered, the value is stable modulo the restore
	 that must happen after a call.  This currently screws up local-alloc
	 into believing that the restore is not needed.  */
      if (x == pic_offset_table_rtx)
	return 0;
#endif
      return 1;

    case ASM_OPERANDS:
      if (MEM_VOLATILE_P (x))
	return 1;

      /* FALLTHROUGH */

    default:
      break;
    }

  fmt = GET_RTX_FORMAT (code);
  for (i = GET_RTX_LENGTH (code) - 1; i >= 0; i--)
    if (fmt[i] == 'e')
      {
	if (rtx_unstable_p (XEXP (x, i)))
	  return 1;
      }
    else if (fmt[i] == 'E')
      {
	int j;
	for (j = 0; j < XVECLEN (x, i); j++)
	  if (rtx_unstable_p (XVECEXP (x, i, j)))
	    return 1;
      }

  return 0;
}

/* Return 1 if X has a value that can vary even between two
   executions of the program.  0 means X can be compared reliably
   against certain constants or near-constants.
   FOR_ALIAS is nonzero if we are called from alias analysis; if it is
   zero, we are slightly more conservative.
   The frame pointer and the arg pointer are considered constant.  */

int
rtx_varies_p (x, for_alias)
     rtx x;
     int for_alias;
{
  RTX_CODE code = GET_CODE (x);
  int i;
  const char *fmt;

  switch (code)
    {
    case MEM:
      return ! RTX_UNCHANGING_P (x) || rtx_varies_p (XEXP (x, 0), for_alias);

    case QUEUED:
      return 1;

    case CONST:
    case CONST_INT:
    case CONST_DOUBLE:
    case CONST_VECTOR:
    case SYMBOL_REF:
    case LABEL_REF:
      return 0;

    case REG:
      /* Note that we have to test for the actual rtx used for the frame
	 and arg pointers and not just the register number in case we have
	 eliminated the frame and/or arg pointer and are using it
	 for pseudos.  */
      if (x == frame_pointer_rtx || x == hard_frame_pointer_rtx
	  /* The arg pointer varies if it is not a fixed register.  */
	  || (x == arg_pointer_rtx && fixed_regs[ARG_POINTER_REGNUM]))
	return 0;
      if (x == pic_offset_table_rtx
#ifdef PIC_OFFSET_TABLE_REG_CALL_CLOBBERED
	  /* ??? When call-clobbered, the value is stable modulo the restore
	     that must happen after a call.  This currently screws up
	     local-alloc into believing that the restore is not needed, so we
	     must return 0 only if we are called from alias analysis.  */
	  && for_alias
#endif
	  )
	return 0;
      return 1;

    case LO_SUM:
      /* The operand 0 of a LO_SUM is considered constant
	 (in fact it is related specifically to operand 1)
	 during alias analysis.  */
      return (! for_alias && rtx_varies_p (XEXP (x, 0), for_alias))
	     || rtx_varies_p (XEXP (x, 1), for_alias);
      
    case ASM_OPERANDS:
      if (MEM_VOLATILE_P (x))
	return 1;

      /* FALLTHROUGH */

    default:
      break;
    }

  fmt = GET_RTX_FORMAT (code);
  for (i = GET_RTX_LENGTH (code) - 1; i >= 0; i--)
    if (fmt[i] == 'e')
      {
	if (rtx_varies_p (XEXP (x, i), for_alias))
	  return 1;
      }
    else if (fmt[i] == 'E')
      {
	int j;
	for (j = 0; j < XVECLEN (x, i); j++)
	  if (rtx_varies_p (XVECEXP (x, i, j), for_alias))
	    return 1;
      }

  return 0;
}

/* Return 0 if the use of X as an address in a MEM can cause a trap.  */

int
rtx_addr_can_trap_p (x)
     rtx x;
{
  enum rtx_code code = GET_CODE (x);

  switch (code)
    {
    case SYMBOL_REF:
      return SYMBOL_REF_WEAK (x);

    case LABEL_REF:
      return 0;

    case REG:
      /* As in rtx_varies_p, we have to use the actual rtx, not reg number.  */
      if (x == frame_pointer_rtx || x == hard_frame_pointer_rtx
	  || x == stack_pointer_rtx
	  /* The arg pointer varies if it is not a fixed register.  */
	  || (x == arg_pointer_rtx && fixed_regs[ARG_POINTER_REGNUM]))
	return 0;
      /* All of the virtual frame registers are stack references.  */
      if (REGNO (x) >= FIRST_VIRTUAL_REGISTER
	  && REGNO (x) <= LAST_VIRTUAL_REGISTER)
	return 0;
      return 1;

    case CONST:
      return rtx_addr_can_trap_p (XEXP (x, 0));

    case PLUS:
      /* An address is assumed not to trap if it is an address that can't
	 trap plus a constant integer or it is the pic register plus a
	 constant.  */
      return ! ((! rtx_addr_can_trap_p (XEXP (x, 0))
		 && GET_CODE (XEXP (x, 1)) == CONST_INT)
		|| (XEXP (x, 0) == pic_offset_table_rtx
		    && CONSTANT_P (XEXP (x, 1))));

    case LO_SUM:
    case PRE_MODIFY:
      return rtx_addr_can_trap_p (XEXP (x, 1));

    case PRE_DEC:
    case PRE_INC:
    case POST_DEC:
    case POST_INC:
    case POST_MODIFY:
      return rtx_addr_can_trap_p (XEXP (x, 0));

    default:
      break;
    }

  /* If it isn't one of the case above, it can cause a trap.  */
  return 1;
}

/* Return 1 if X refers to a memory location whose address 
   cannot be compared reliably with constant addresses,
   or if X refers to a BLKmode memory object. 
   FOR_ALIAS is nonzero if we are called from alias analysis; if it is
   zero, we are slightly more conservative.  */

int
rtx_addr_varies_p (x, for_alias)
     rtx x;
     int for_alias;
{
  enum rtx_code code;
  int i;
  const char *fmt;

  if (x == 0)
    return 0;

  code = GET_CODE (x);
  if (code == MEM)
    return GET_MODE (x) == BLKmode || rtx_varies_p (XEXP (x, 0), for_alias);

  fmt = GET_RTX_FORMAT (code);
  for (i = GET_RTX_LENGTH (code) - 1; i >= 0; i--)
    if (fmt[i] == 'e')
      {
	if (rtx_addr_varies_p (XEXP (x, i), for_alias))
	  return 1;
      }
    else if (fmt[i] == 'E')
      {
	int j;
	for (j = 0; j < XVECLEN (x, i); j++)
	  if (rtx_addr_varies_p (XVECEXP (x, i, j), for_alias))
	    return 1;
      }
  return 0;
}

/* Return the value of the integer term in X, if one is apparent;
   otherwise return 0.
   Only obvious integer terms are detected.
   This is used in cse.c with the `related_value' field.  */

HOST_WIDE_INT
get_integer_term (x)
     rtx x;
{
  if (GET_CODE (x) == CONST)
    x = XEXP (x, 0);

  if (GET_CODE (x) == MINUS
      && GET_CODE (XEXP (x, 1)) == CONST_INT)
    return - INTVAL (XEXP (x, 1));
  if (GET_CODE (x) == PLUS
      && GET_CODE (XEXP (x, 1)) == CONST_INT)
    return INTVAL (XEXP (x, 1));
  return 0;
}

/* If X is a constant, return the value sans apparent integer term;
   otherwise return 0.
   Only obvious integer terms are detected.  */

rtx
get_related_value (x)
     rtx x;
{
  if (GET_CODE (x) != CONST)
    return 0;
  x = XEXP (x, 0);
  if (GET_CODE (x) == PLUS
      && GET_CODE (XEXP (x, 1)) == CONST_INT)
    return XEXP (x, 0);
  else if (GET_CODE (x) == MINUS
	   && GET_CODE (XEXP (x, 1)) == CONST_INT)
    return XEXP (x, 0);
  return 0;
}

/* Given a tablejump insn INSN, return the RTL expression for the offset
   into the jump table.  If the offset cannot be determined, then return
   NULL_RTX.

   If EARLIEST is non-zero, it is a pointer to a place where the earliest
   insn used in locating the offset was found.  */

rtx
get_jump_table_offset (insn, earliest)
     rtx insn;
     rtx *earliest;
{
  rtx label;
  rtx table;
  rtx set;
  rtx old_insn;
  rtx x;
  rtx old_x;
  rtx y;
  rtx old_y;
  int i;

  if (GET_CODE (insn) != JUMP_INSN
      || ! (label = JUMP_LABEL (insn))
      || ! (table = NEXT_INSN (label))
      || GET_CODE (table) != JUMP_INSN
      || (GET_CODE (PATTERN (table)) != ADDR_VEC
	  && GET_CODE (PATTERN (table)) != ADDR_DIFF_VEC)
      || ! (set = single_set (insn)))
    return NULL_RTX;

  x = SET_SRC (set);

  /* Some targets (eg, ARM) emit a tablejump that also
     contains the out-of-range target.  */
  if (GET_CODE (x) == IF_THEN_ELSE
      && GET_CODE (XEXP (x, 2)) == LABEL_REF)
    x = XEXP (x, 1);

  /* Search backwards and locate the expression stored in X.  */
  for (old_x = NULL_RTX; GET_CODE (x) == REG && x != old_x;
       old_x = x, x = find_last_value (x, &insn, NULL_RTX, 0))
    ;

  /* If X is an expression using a relative address then strip
     off the addition / subtraction of PC, PIC_OFFSET_TABLE_REGNUM,
     or the jump table label.  */
  if (GET_CODE (PATTERN (table)) == ADDR_DIFF_VEC
      && (GET_CODE (x) == PLUS || GET_CODE (x) == MINUS))
    {
      for (i = 0; i < 2; i++)
	{
	  old_insn = insn;
	  y = XEXP (x, i);

	  if (y == pc_rtx || y == pic_offset_table_rtx)
	    break;

	  for (old_y = NULL_RTX; GET_CODE (y) == REG && y != old_y;
	       old_y = y, y = find_last_value (y, &old_insn, NULL_RTX, 0))
	    ;

	  if ((GET_CODE (y) == LABEL_REF && XEXP (y, 0) == label))
	    break;
	}

      if (i >= 2)
	return NULL_RTX;

      x = XEXP (x, 1 - i);

      for (old_x = NULL_RTX; GET_CODE (x) == REG && x != old_x;
	   old_x = x, x = find_last_value (x, &insn, NULL_RTX, 0))
	;
    }

  /* Strip off any sign or zero extension.  */
  if (GET_CODE (x) == SIGN_EXTEND || GET_CODE (x) == ZERO_EXTEND)
    {
      x = XEXP (x, 0);

      for (old_x = NULL_RTX; GET_CODE (x) == REG && x != old_x;
	   old_x = x, x = find_last_value (x, &insn, NULL_RTX, 0))
	;
    }

  /* If X isn't a MEM then this isn't a tablejump we understand.  */
  if (GET_CODE (x) != MEM)
    return NULL_RTX;

  /* Strip off the MEM.  */
  x = XEXP (x, 0);

  for (old_x = NULL_RTX; GET_CODE (x) == REG && x != old_x;
       old_x = x, x = find_last_value (x, &insn, NULL_RTX, 0))
    ;

  /* If X isn't a PLUS than this isn't a tablejump we understand.  */
  if (GET_CODE (x) != PLUS)
    return NULL_RTX;

  /* At this point we should have an expression representing the jump table
     plus an offset.  Examine each operand in order to determine which one
     represents