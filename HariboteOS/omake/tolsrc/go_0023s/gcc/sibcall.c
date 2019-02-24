/* Generic sibling call optimization support
   Copyright (C) 1999, 2000, 2001, 2002 Free Software Foundation, Inc.

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
#include "regs.h"
#include "function.h"
#include "hard-reg-set.h"
#include "flags.h"
#include "insn-config.h"
#include "recog.h"
#include "basic-block.h"
#include "output.h"
#include "except.h"
#include "tree.h"

/* In case alternate_exit_block contains copy from pseudo, to return value,
   record the pseudo here.  In such case the pseudo must be set to function
   return in the sibcall sequence.  */
static rtx return_value_pseudo;

static int identify_call_return_value	PARAMS ((rtx, rtx *, rtx *));
static rtx skip_copy_to_return_value	PARAMS ((rtx));
static rtx skip_use_of_return_value	PARAMS ((rtx, enum rtx_code));
static rtx skip_stack_adjustment	PARAMS ((rtx));
static rtx skip_pic_restore		PARAMS ((rtx));
static rtx skip_jump_insn		PARAMS ((rtx));
static int call_ends_block_p		PARAMS ((rtx, rtx));
static int uses_addressof		PARAMS ((rtx));
static int sequence_uses_addressof	PARAMS ((rtx));
static void purge_reg_equiv_notes	PARAMS ((void));
static void purge_mem_unchanging_flag	PARAMS ((rtx));
static rtx skip_unreturned_value 	PARAMS ((rtx));

/* Examine a CALL_PLACEHOLDER pattern and determine where the call's
   return value is located.  P_HARD_RETURN receives the hard register
   that the function used; P_SOFT_RETURN receives the pseudo register
   that the sequence used.  Return non-zero if the values were located.  */

static int
identify_call_return_value (cp, p_hard_return, p_soft_return)
     rtx cp;
     rtx *p_hard_return, *p_soft_return;
{
  rtx insn, set, hard, soft;

  insn = XEXP (cp, 0);
  /* Search backward through the "normal" call sequence to the CALL insn.  */
  while (NEXT_INSN (insn))
    insn = NEXT_INSN (insn);
  while (GET_CODE (insn) != CALL_INSN)
    insn = PREV_INSN (insn);

  /* Assume the pattern is (set (dest) (call ...)), or that the first
     member of a parallel is.  This is the hard return register used
     by the function.  */
  if (GET_CODE (PATTERN (insn)) == SET
      && GET_CODE (SET_SRC (PATTERN (insn))) == CALL)
    hard = SET_DEST (PATTERN (insn));
  else if (GET_CODE (PATTERN (insn)) == PARALLEL
	   && GET_CODE (XVECEXP (PATTERN (insn), 0, 0)) == SET
	   && GET_CODE (SET_SRC (XVECEXP (PATTERN (insn), 0, 0))) == CALL)
    hard = SET_DEST (XVECEXP (PATTERN (insn), 0, 0));
  else
    return 0;

  /* If we didn't get a single hard register (e.g. a parallel), give up.  */
  if (GET_CODE (hard) != REG)
    return 0;
    
  /* Stack adjustment done after call may appear here.  */
  insn = skip_stack_adjustment (insn);
  if (! insn)
    return 0;

  /* Restore of GP register may appear here.  */
  insn = skip_pic_restore (insn);
  if (! insn)
    return 0;

  /* If there's nothing after, there's no soft return value.  */
  insn = NEXT_INSN (insn);
  if (! insn)
    return 0;
  
  /* We're looking for a source of the hard return register.  */
  set = single_set (insn);
  if (! set || SET_SRC (set) != hard)
    return 0;

  soft = SET_DEST (set);
  insn = NEXT_INSN (insn);

  /* Allow this first destination to be copied to a second register,
     as might happen if the first register wasn't the particular pseudo
     we'd been ex