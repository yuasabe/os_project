/* Save and restore call-clobbered registers which are live across a call.
   Copyright (C) 1989, 1992, 1994, 1995, 1997, 1998,
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
#include "rtl.h"
#include "insn-config.h"
#include "flags.h"
#include "regs.h"
#include "hard-reg-set.h"
#include "recog.h"
#include "basic-block.h"
#include "reload.h"
#include "function.h"
#include "expr.h"
#include "toplev.h"
#include "tm_p.h"

#ifndef MAX_MOVE_MAX
#define MAX_MOVE_MAX MOVE_MAX
#endif

#ifndef MIN_UNITS_PER_WORD
#define MIN_UNITS_PER_WORD UNITS_PER_WORD
#endif

#define MOVE_MAX_WORDS (MOVE_MAX / UNITS_PER_WORD)

/* Modes for each hard register that we can save.  The smallest mode is wide
   enough to save the entire contents of the register.  When saving the
   register because it is live we first try to save in multi-register modes.
   If that is not possible the save is done one register at a time.  */

static enum machine_mode 
  regno_save_mode[FIRST_PSEUDO_REGISTER][MAX_MOVE_MAX / MIN_UNITS_PER_WORD + 1];

/* For each hard register, a place on the stack where it can be saved,
   if needed.  */

static rtx 
  regno_save_mem[FIRST_PSEUDO_REGISTER][MAX_MOVE_MAX / MIN_UNITS_PER_WORD + 1];

/* We will only make a register eligible for caller-save if it can be
   saved in its widest mode with a simple SET insn as long as the memory
   address is valid.  We record the INSN_CODE is those insns here since
   when we emit them, the addresses might not be valid, so they might not
   be recognized.  */

static int
  reg_save_code[FIRST_PSEUDO_REGISTER][MAX_MACHINE_MODE];
static int 
  reg_restore_code[FIRST_PSEUDO_REGISTER][MAX_MACHINE_MODE];

/* Set of hard regs currently residing in save area (during insn scan).  */

static HARD_REG_SET hard_regs_saved;

/* Number of registers currently in hard_regs_saved.  */

static int n_regs_saved;

/* Computed by mark_referenced_regs, all regs referenced in a given
   insn.  */
static HARD_REG_SET referenced_regs;

/* Computed in mark_set_regs, holds all registers set by the current
   instruction.  */
static HARD_REG_SET this_insn_sets;


static void mark_set_regs		PARAMS ((rtx, rtx, void *));
static void mark_referenced_regs	PARAMS ((rtx));
static int insert_save			PARAMS ((struct insn_chain *, int, int,
						 HARD_REG_SET *,
						 enum machine_mode *));
static int insert_restore		PARAMS ((struct insn_chain *, int, int,
						 int, enum machine_mode *));
static struct insn_chain *insert_one_insn PARAMS ((struct insn_chain *, int,
						   int, rtx));
static void add_stored_regs		PARAMS ((rtx, rtx, void *));

/* Initialize for caller-save.

   Look at all the hard registers that are used by a call and for which
   regclass.c has not already excluded from being used across a call.

   Ensure that we can find a mode to save the register and that there is a 
   simple insn to save and restore the register.  This latter check avoids
   problems that would occur if we tried to save the MQ register of some
   machines directly into memory.  */

void
init_caller_save ()
{
  rtx addr_reg;
  int offset;
  rtx address;
  int i, j;
  enum machine_mode mode;

  /* First find all the registers that we need to deal with and all
     the modes that they can have.  If we 