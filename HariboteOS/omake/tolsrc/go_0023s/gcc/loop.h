/* Loop optimization definitions for GNU C-Compiler
   Copyright (C) 1991, 1995, 1998, 1999, 2000, 2001, 2002
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

#include "bitmap.h"
#include "sbitmap.h"
#include "hard-reg-set.h"
#include "basic-block.h"

/* Flags passed to loop_optimize.  */
#define LOOP_UNROLL 1
#define LOOP_BCT 2
#define LOOP_PREFETCH 4
#define LOOP_FIRST_PASS 8

/* Get the loop info pointer of a loop.  */
#define LOOP_INFO(LOOP) ((struct loop_info *) (LOOP)->aux)

/* Get a pointer to the loop movables structure.  */
#define LOOP_MOVABLES(LOOP) (&LOOP_INFO (LOOP)->movables)

/* Get a pointer to the loop registers structure.  */
#define LOOP_REGS(LOOP) (&LOOP_INFO (LOOP)->regs)

/* Get a pointer to the loop induction variables structure.  */
#define LOOP_IVS(LOOP) (&LOOP_INFO (LOOP)->ivs)

/* Get the luid of an insn.  Catch the error of trying to reference the LUID
   of an insn added during loop, since these don't have LUIDs.  */

#define INSN_LUID(INSN)			¥
  (INSN_UID (INSN) < max_uid_for_loop ? uid_luid[INSN_UID (INSN)] ¥
   : (abort (), -1))

#define REGNO_FIRST_LUID(REGNO) uid_luid[REGNO_FIRST_UID (REGNO)]
#define REGNO_LAST_LUID(REGNO) uid_luid[REGNO_LAST_UID (REGNO)]


/* A "basic induction variable" or biv is a pseudo reg that is set
   (within this loop) only by incrementing or decrementing it.  */
/* A "general induction variable" or giv is a pseudo reg whose
   value is a linear function of a biv.  */

/* Bivs are recognized by `basic_induction_var';
   Givs by `general_induction_var'.  */

/* An enum for the two different types of givs, those that are used
   as memory addresses and those that are calculated into registers.  */
enum g_types
{
  DEST_ADDR,
  DEST_REG
};


/* A `struct induction' is created for every instruction that sets
   an induction variable (either a biv or a giv).  */

struct induction
{
  rtx insn;			/* The insn that sets a biv or giv */
  rtx new_reg;			/* New register, containing strength reduced
				   version of this giv.  */
  rtx src_reg;			/* Biv from which this giv is computed.
				   (If this is a biv, then this is the biv.) */
  enum g_types giv_type;	/* Indicate whether DEST_ADDR or DEST_REG */
  rtx dest_reg;			/* Destination register for insn: this is the
				   register which was the biv or giv.
				   For a biv, this equals src_reg.
				   For a DEST_ADDR type giv, this is 0.  */
  rtx *location;		/* Place in the insn where this giv occurs.
				   If GIV_TYPE is DEST_REG, this is 0.  */
				/* For a biv, this is the place where add_val
				   was found.  */
  enum machine_mode mode;	/* The mode of this biv or giv */
  rtx mem;			/* For DEST_ADDR, the memory object.  */
  rtx mult_val;			/* Multiplicative factor for src_reg.  */
  rtx add_val;			/* Additive constant for that product.  */
  int benefit;			/* Gain from eliminating this insn.  */
  rtx final_value;		/* If the giv is used outside the loop, and its
				   final value could be calculated, it is put
				   here, and the giv is made replaceable.  Set
				   the giv to this value before the loop.  */
  unsigned combined_with;	/* The number of givs this giv has been
				   combined with.  If nonzero, this giv
				   cannot combine with any other giv.  */
  unsigned replaceable : 1;	/* 1 if we can substitute the strength-reduced
				   variab