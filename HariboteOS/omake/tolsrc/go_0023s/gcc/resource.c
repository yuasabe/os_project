/* Definitions for computing resource usage of specific insns.
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

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
#include "tm_p.h"
#include "hard-reg-set.h"
#include "basic-block.h"
#include "function.h"
#include "regs.h"
#include "flags.h"
#include "output.h"
#include "resource.h"
#include "except.h"
#include "insn-attr.h"
#include "params.h"

/* This structure is used to record liveness information at the targets or
   fallthrough insns of branches.  We will most likely need the information
   at targets again, so save them in a hash table rather than recomputing them
   each time.  */

struct target_info
{
  int uid;			/* INSN_UID of target.  */
  struct target_info *next;	/* Next info for same hash bucket.  */
  HARD_REG_SET live_regs;	/* Registers live at target.  */
  int block;			/* Basic block number containing target.  */
  int bb_tick;			/* Generation count of basic block info.  */
};

#define TARGET_HASH_PRIME 257

/* Indicates what resources are required at the beginning of the epilogue.  */
static struct resources start_of_epilogue_needs;

/* Indicates what resources are required at function end.  */
static struct resources end_of_function_needs;

/* Define the hash table itself.  */
static struct target_info **target_hash_table = NULL;

/* For each basic block, we maintain a generation number of its basic
   block info, which is updated each time we move an insn from the
   target of a jump.  This is the generation number indexed by block
   number.  */

static int *bb_ticks;

/* Marks registers possibly live at the current place being scanned by
   mark_target_live_regs.  Also used by update_live_status.  */

static HARD_REG_SET current_live_regs;

/* Marks registers for which we have seen a REG_DEAD note but no assignment.
   Also only used by the next two functions.  */

static HARD_REG_SET pending_dead_regs;

static void update_live_status		PARAMS ((rtx, rtx, void *));
static int find_basic_block		PARAMS ((rtx, int));
static rtx next_insn_no_annul		PARAMS ((rtx));
static rtx find_dead_or_set_registers	PARAMS ((rtx, struct resources*,
						rtx*, int, struct resources,
						struct resources));

/* Utility function called from mark_target_live_regs via note_stores.
   It deadens any CLOBBERed registers and livens any SET registers.  */

static void
update_live_status (dest, x, data)
     rtx dest;
     rtx x;
     void *data ATTRIBUTE_UNUSED;
{
  int first_regno, last_regno;
  int i;

  if (GET_CODE (dest) != REG
      && (GET_CODE (dest) != SUBREG || GET_CODE (SUBREG_REG (dest)) != REG))
    return;

  if (GET_CODE (dest) == SUBREG)
    first_regno = subreg_regno (dest);
  else
    first_regno = REGNO (dest);

  last_regno = first_regno + HARD_REGNO_NREGS (first_regno, GET_MODE (dest));

  if (GET_CODE (x) == CLOBBER)
    for (i = first_regno; i < last_regno; i++)
      CLEAR_HARD_REG_BIT (current_live_regs, i);
  else
    for (i = first_regno; i < last_regno; i++)
      {
	SET_HARD_REG_BIT (current_live_regs, i);
	CLEAR_HARD_REG_BIT (pending_dead_regs, i);
      }
}

/* Find the number of the basic block with correct live register
   information that starts closest to INSN.  Return -1 if we couldn't
   find such a basic block or the beginning is more than
   SEARCH_LIMIT instructions before INSN.  Use SEARCH_LIMIT = -1 for
   an unlimited search.

   The delay slot filling code destroys the control-flow graph so,
   instead of finding the basic block containing INSN, we search
   backwards toward a BARRIER where the live register information is
   correct.  */

static int
find_basic_block (insn, search_limit)
     rtx insn;
     int search_limit;
{
  int i;

  /* Scan backwards to the previous BARRIER.  Then see if we can find a
     label that starts a basic block.  Return the basic block number.  */
  for (insn = prev_nonnote_insn (insn);
       insn && GET_CODE (insn) != BARRIER && search_limit != 0;
       insn = prev_nonnote_insn (insn), --search_limit)
    ;

  /* The closest BARRIER is too far away.  */
  if (search_limit == 0)
    return -1;

  /* The start of the function is basic block zero.  */
  else if (insn == 0)
    return 0;

  /* See if any of the upcoming CODE_LABELs start a basic block.  If we reach
     anything other than a CODE_LABEL or note, we can't find this code.  */
  for (insn = next_nonnote_insn (insn);
       insn && GET_CODE (insn) == CODE_LABEL;
       insn = next_nonnote_insn (insn))
    {
      for (i = 0; i < n_basic_blocks; i++)
	if (insn == BLOCK_HEAD (i))
	  return i;
    }

  return -1;
}

/* Similar to next_insn, but ignores insns in the delay slots of
   an annulled branch.  */

static rtx
next_insn_no_annul (insn)
     rtx insn;
{
  if (insn)
    {
      /* If INSN is an annulled branch, skip any insns from the target
	 of the branch.  */
      if (INSN_ANNULLED_BRANCH_P (insn)
	  && NEXT_INSN (PREV_INSN (insn)) != insn)
	while (INSN_FROM_TARGET_P (NEXT_INSN (insn)))
	  insn = NEXT_INSN (insn);

      insn = NEXT_INSN (insn);
      if (insn && GET_CODE (insn) == INSN
	  && GET_CODE (PATTERN (insn)) == SEQUENCE)
	insn = XVECEXP (PATTERN (insn), 0, 0);
    }

  return insn;
}

/* Given X, some rtl, and RES, a pointer to a `struct resource', mark
   which resources are referenced by the insn.  If INCLUDE_DELAYED_EFFECTS
   is TRUE, resources used by the called routine will be included for
   CALL_INSNs.  */

void
mark_referenced_resources (x, res, include_delayed_effects)
     rtx x;
     struct resources *res;
     int include_delayed_effects;
{
  enum rtx_code code = GET_CODE (x);
  int i, j;
  unsigned int r;
  const char *format_ptr;

  /* Handle leaf items for which we set resource flags.  Also, special-case
     CALL, SET and CLOBBER operators.  */
  switch (code)
    {
    case CONST:
    case CONST_INT:
    case CONST_DOUBLE:
    case CONST_VECTOR:
    case PC:
    case SYMBOL_REF:
    case LABEL_REF:
      return;

    case SUBREG:
      if (GET_CODE (SUBREG_REG (x)) != REG)
	mark_referenced_resources (SUBREG_REG (x), res, 0);
      else
	{
	  unsigned int regno = subreg_regno (x);
	  unsigned int last_regno
	    = regno + HARD_REGNO_NREGS (regno, GET_MODE (x));

	  if (last_regno > FIRST_PSEUDO_REGISTER)
	    abort ();
	  for (r = regno; r < last_regno; r++)
	    SET_HARD_REG_BIT (res->regs, r);
	}
      return;

    case REG:
	{
	  unsigned int regno = REGNO (x);
	  unsigned int last_regno
	    = regno + HARD_REGNO_NREGS (regno, GET_MODE (x));

	  if (last_regno > FIRST_PSEUDO_REGISTER)
	    abort ();
	  for (r = regno; r < last_regno; r++)
	    SET_HARD_REG_BIT (res->regs, r);
	}
      return;

    case MEM:
      /* If this memory shouldn't change, it really isn't referencing
	 memory.  */
      if (RTX_UNCHANGING_P (x))
	res->unch_memory = 1;
      else
	res->memory = 1;
      res->volatil |= MEM_VOLATILE_P (x);

      /* Mark registers used to access memory.  */
      mark_referenced_resources (XEXP (x, 0), res, 0);
      return;

    case CC0:
      res->cc = 1;
      return;

    case UNSPEC_VOLATILE:
    case ASM_INPUT:
      /* Traditional asm's are always volatile.  */
      res->volatil = 1;
      return;

    case TRAP_IF:
      res->volatil = 1;
      break;

    case ASM_OPERANDS:
      res->volatil |= MEM_VOLATILE_P (x);

      /* For all ASM_OPERANDS, we must traverse the vector of input operands.
	 We can not just fall through here since then we would be confused
	 by the ASM_INPUT rtx inside ASM_OPERANDS, which do not indicate
	 traditional asms unlike their normal usage.  */
      
      for (i = 0; i < ASM_OPERANDS_INPUT_LENGTH (x); i++)
	mark_referenced_resources (ASM_OPERANDS_INPUT (x, i), res, 0);
      return;

    case CALL:
      /* The first operand will be a (MEM (xxx)) but doesn't really reference
	 memory.  The second operand may be referenced, though.  */
      mark_referenced_resources (XEXP (XEXP (x, 0), 0), res, 0);
      mark_referenced_resources (XEXP (x, 1), res, 0);
      return;

    case SET:
      /* Usually, the first operand of SET is set, not referenced.  But
	 registers used to access memory are referenced.  SET_DEST is
	 also referenced if it is a ZERO_EXTRACT or SIGN_EXTRACT.  */

      mark_referenced_resources (SET_SRC (x), res, 0);

      x = SET_DEST (x);
      if (GET_CODE (x) == SIGN_EXTRACT
	  || GET_CODE (x) == ZERO_EXTRACT
	  || GET_CODE (x) == STRICT_LOW_PART)
	mark_referenced_resources (x, res, 0);
      else if (GET_CODE (x) == SUBREG)
	x = SUBREG_REG (x);
      if (GET_CODE (x) == MEM)
	mark_referenced_resources (XEXP (x, 0), res, 0);
      return;

    case CLOBBER:
      return;

    case CALL_INSN:
      if (include_delayed_effects)
	{
	  /* A CALL references memory, the frame pointer if it exists, the
	     stack pointer, any global registers and any registers given in
	     USE insns immediately in front of the CALL.

	     However, we may have moved some of the parameter loading insns
	     into the delay slot of this CALL.  If so, the USE's for them
	     don't count and should be skipped.  */
	  rtx insn = PREV_INSN (x);
	  rtx sequence = 0;
	  int seq_size = 0;
	  int i;

	  /* If we are part of a delay slot sequence, point at the SEQUENCE.  */
	  if (NEXT_INSN (insn) != x)
	    {
	      sequence = PATTERN (NEXT_INSN (insn));
	      seq_size = XVECLEN (sequence, 0);
	      if (GET_CODE (sequence) != SEQUENCE)
		abort ();
	    }

	  res->memory = 1;
	  SET_HARD_REG_BIT (res->regs, STACK_POINTER_REGNUM);
	  if (frame_pointer_needed)
	    {
	      SET_HARD_REG_BIT (res->regs, FRAME_POINTER_REGNUM);
#if FRAME_POINTER_REGNUM != HARD_FRAME_POINTER_REGNUM
	      SET_HARD_REG_BIT (res->regs, HARD_FRAME_POINTER_REGNUM);
#endif
	    }

	  for (i = 0; i < FIRST_PSEUDO_REGISTER; i++)
	    if (global_regs[i])
	      SET_HARD_REG_BIT (res->regs, i);

	  /* Check for a REG_SETJMP.  If it exists, then we must
	     assume that this call can need any register.

	     This is done to be more conservative about how we handle setjmp.
	     We assume that they both use and set all registers.  Using all
	     registers ensures that a register will not be considered dead
	     just because it crosses a setjmp call.  A register should be
	     considered dead only if the setjmp call returns non-zero.  */
	  if (find_reg_note (x, REG_SETJMP, NULL))
	    SET_HARD_REG_SET (res->regs);

	  {
	    rtx link;

	    for (link = CALL_INSN_FUNCTION_USAGE (x);
		 link;
		 link = XEXP (link, 1))
	      if (GET_CODE (XEXP (link, 0)) == USE)
		{
		  for (i = 1; i < seq_size; i++)
		    {
		      rtx slot_pat = PATTERN (XVECEXP (sequence, 0, i));
		      if (GET_CODE (slot_pat) == SET
			  && rtx_equal_p (SET_DEST (slot_pat),
					  XEXP (XEXP (link, 0), 0)))
			break;
		    }
		  if (i >= seq_size)
		    mark_referenced_resources (XEXP (XEXP (link, 0), 0),
					       res, 0);
		}
	  }
	}

      /* ... fall through to other INSN processing ...  */

    case INSN:
    case JUMP_INSN:

#ifdef INSN_REFERENCES_ARE_DELAYED
      if (! include_delayed_effects
	  && INSN_REFERENCES_ARE_DELAYED (x))
	return;
#endif

      /* No special processing, just speed up.  */
      mark_referenced_resources (PATTERN (x