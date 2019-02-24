/* Search an insn for pseudo regs that must be in hard regs and are not.
   Copyright (C) 1987, 1988, 1989, 1992, 1993, 1994, 1995, 1996, 1997, 1998,
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

/* This file contains subroutines used only from the file reload1.c.
   It knows how to scan one insn for operands and values
   that need to be copied into registers to make valid code.
   It also finds other operands and values which are valid
   but for which equivalent values in registers exist and
   ought to be used instead.

   Before processing the first insn of the function, call `init_reload'.

   To scan an insn, call `find_reloads'.  This does two things:
   1. sets up tables describing which values must be reloaded
   for this insn, and what kind of hard regs they must be reloaded into;
   2. optionally record the locations where those values appear in
   the data, so they can be replaced properly later.
   This is done only if the second arg to `find_reloads' is nonzero.

   The third arg to `find_reloads' specifies the number of levels
   of indirect addressing supported by the machine.  If it is zero,
   indirect addressing is not valid.  If it is one, (MEM (REG n))
   is valid even if (REG n) did not get a hard register; if it is two,
   (MEM (MEM (REG n))) is also valid even if (REG n) did not get a
   hard register, and similarly for higher values.

   Then you must choose the hard regs to reload those pseudo regs into,
   and generate appropriate load insns before this insn and perhaps
   also store insns after this insn.  Set up the array `reload_reg_rtx'
   to contain the REG rtx's for the registers you used.  In some
   cases `find_reloads' will return a nonzero value in `reload_reg_rtx'
   for certain reloads.  Then that tells you which register to use,
   so you do not need to allocate one.  But you still do need to add extra
   instructions to copy the value into and out of that register.

   Finally you must call `subst_reloads' to substitute the reload reg rtx's
   into the locations already recorded.

NOTE SIDE EFFECTS:

   find_reloads can alter the operands of the instruction it is called on.

   1. Two operands of any sort may be interchanged, if they are in a
   commutative instruction.
   This happens only if find_reloads thinks the instruction will compile
   better that way.

   2. Pseudo-registers that are equivalent to constants are replaced
   with those constants if they are not in hard registers.

1 happens every time find_reloads is called.
2 happens only when REPLACE is 1, which is only when
actually doing the reloads, not when just counting them.

Using a reload register for several reloads in one insn:

When an insn has reloads, it is considered as having three parts:
the input reloads, the insn itself after reloading, and the output reloads.
Reloads of values used in memory addresses are often needed for only one part.

When this is so, reload_when_needed records which part needs the reload.
Two reloads for different parts of the insn can share the same reload
register.

When a reload is used for addresses in multiple parts, or when it is
an ordinary operand, it is classified as RELOAD_OTHER, and cannot share
a register with any other reload.  */

#define REG_OK_STRICT

#include "config.h"
#include "system.h"
#include "rtl.h"
#include "tm_p.h"
#include "insn-config.h"
#include "expr.h"
#include "optabs.h"
#include "recog.h"
#include "reload.h"
#include "regs.h"
#include "hard-reg-set.h"
#include "flags.h"
#include "real.h"
#include "output.h"
#include "function.h"
#include "toplev.h"

#ifndef REGISTER_MOVE_COST
#define REGISTER_MOVE_COST(m, x, y) 2
#endif

#ifndef REGNO_MODE_OK_FOR_BASE_P
#define REGNO_MODE_OK_FOR_BASE_P(REGNO, MODE) REGNO_OK_FOR_BASE_P (REGNO)
#endif

#ifndef REG_MODE_OK_FOR_BASE_P
#define REG_MODE_OK_FOR_BASE_P(REGNO, MODE) REG_OK_FOR_BASE_P (REGNO)
#endif

/* All reloads of the current insn are recorded here.  See reload.h for
   comments.  */
int n_reloads;
struct reload rld[MAX_RELOADS];

/* All the "earlyclobber" operands of the current insn
   are recorded here.  */
int n_earlyclobbers;
rtx reload_earlyclobbers[MAX_RECOG_OPERANDS];

int reload_n_operands;

/* Replacing reloads.

   If `replace_reloads' is nonzero, then as each reload is recorded
   an entry is made for it in the table `replacements'.
   Then later `subst_reloads' can look through that table and
   perform all the replacements needed.  */

/* Nonzero means record the places to replace.  */
static int replace_reloads;

/* Each replacement is recorded with a structure like this.  */
struct replacement
{
  rtx *where;			/* Location to store in */
  rtx *subreg_loc;		/* Location of SUBREG if WHERE is inside
				   a SUBREG; 0 otherwise.  */
  int what;			/* which reload this is for */
  enum machine_mode mode;	/* mode it must have */
};

static struct replacement replacements[MAX_RECOG_OPERANDS * ((MAX_REGS_PER_ADDRESS * 2) + 1)];

/* Number of replacements currently recorded.  */
static int n_replacements;

/* Used to track what is modified by an operand.  */
struct decomposition
{
  int reg_flag;		/* Nonzero if referencing a register.  */
  int safe;		/* Nonzero if this can't conflict with anything.  */
  rtx base;		/* Base address for MEM.  */
  HOST_WIDE_INT start;	/* Starting offset or register number.  */
  HOST_WIDE_INT end;	/* Ending offset or register number.  */
};

#ifdef SECONDARY_MEMORY_NEEDED

/* Save MEMs needed to copy from one class of registers to another.  One MEM
   is used per mode, but normally only one or two modes are ever used.

   We keep two versions, before and after register elimination.  The one
   after register elimination is record separately for each operand.  This
   is done in case the address is not valid to be sure that we separately
   reload each.  */

static rtx secondary_memlocs[NUM_MACHINE_MODES];
static rtx secondary_memlocs_elim[NUM_MACHINE_MODES][MAX_RECOG_OPERANDS];
#endif

/* The instruction we are doing reloads for;
   so we can test whether a register dies in it.  */
static rtx this_insn;

/* Nonzero if this instruction is a user-specified asm with operands.  */
static int this_insn_is_asm;

/* If hard_regs_live_known is nonzero,
   we can tell which hard regs are currently live,
   at least enough to succeed in choosing dummy reloads.  */
static int hard_regs_live_known;

/* Indexed by hard reg number,
   element is nonnegative if hard reg has been spilled.
   This vector is passed to `find_reloads' as an argument
   and is not changed here.  */
static short *static_reload_reg_p;

/* Set to 1 in subst_reg_equivs if it changes anything.  */
static int subst_reg_equivs_changed;

/* On return from push_reload, holds the reload-number for the OUT
   operand, which can be different for that from the input operand.  */
static int output_reloadnum;

  /* Compare two RTX's.  */
#define MATCHES(x, y) ¥
 (x == y || (x != 0 && (GET_CODE (x) == REG				¥
			? GET_CODE (y) == REG && REGNO (x) == REGNO (y)	¥
			: rtx_equal_p (x, y) && ! side_effects_p (x))))

  /* Indicates if two reloads purposes are for similar enough things that we
     can merge their reloads.  */
#define MERGABLE_RELOADS(when1, when2, op1, op2) ¥
  ((when1) == RELOAD_OTHER || (when2) == RELOAD_OTHER	¥
   || ((when1) == (when2) && (op1) == (op2))		¥
   || ((when1) == RELOAD_FOR_INPUT && (when2) == RELOAD_FOR_INPUT) ¥
   || ((when1) == RELOAD_FOR_OPERAND_ADDRESS		¥
       && (when2) == RELOAD_FOR_OPERAND_ADDRESS)	¥
   || ((when1) == RELOAD_FOR_OTHER_ADDRESS		¥
       && (when2) == RELOAD_FOR_OTHER_ADDRESS))

  /* Nonzero if these two reload purposes produce RELOAD_OTHER when merged.  */
#define MERGE_TO_OTHER(when1, when2, op1, op2) ¥
  ((when1) != (when2)					¥
   || ! ((op1) == (op2)					¥
	 || (when1) == RELOAD_FOR_INPUT			¥
	 || (when1) == RELOAD_FOR_OPERAND_ADDRESS	¥
	 || (when1) == RELOAD_FOR_OTHER_ADDRESS))

  /* If we are going to reload an address, compute the reload type to
     use.  */
#define ADDR_TYPE(type)					¥
  ((type) == RELOAD_FOR_INPUT_ADDRESS			¥
   ? RELOAD_FOR_INPADDR_ADDRESS				¥
   : ((type) == RELOAD_FOR_OUTPUT_ADDRESS		¥
      ? RELOAD_FOR_OUTADDR_ADDRESS			¥
      : (type)))

#ifdef HAVE_SECONDARY_RELOADS
static int push_secondary_reload PARAMS ((int, rtx, int, int, enum reg_class,
					enum machine_mode, enum reload_type,
					enum insn_code *));
#endif
static enum reg_class find_valid_class PARAMS ((enum machine_mode, int,
						unsigned int));
static int reload_inner_reg_of_subreg PARAMS ((rtx, enum machine_mode));
static void push_replacement	PARAMS ((rtx *, int, enum machine_mode));
static void combine_reloads	PARAMS ((void));
static int find_reusable_reload	PARAMS ((rtx *, rtx, enum reg_class,
				       enum reload_type, int, int));
static rtx find_dummy_reload	PARAMS ((rtx, rtx, rtx *, rtx *,
				       enum machine_mode, enum machine_mode,
				       enum reg_class, int, int));
static int hard_reg_set_here_p	PARAMS ((unsigned int, unsigned int, rtx));
static struct decomposition decompose PARAMS ((rtx));
static int immune_p		PARAMS ((rtx, rtx, struct decomposition));
static int alternative_allows_memconst PARAMS ((const char *, int));
static rtx find_reloads_toplev	PARAMS ((rtx, int, enum reload_type, int,
					 int, rtx, int *));
static rtx make_memloc		PARAMS ((rtx, int));
static int find_reloads_address	PARAMS ((enum machine_mode, rtx *, rtx, rtx *,
				       int, enum reload_type, int, rtx));
static rtx subst_reg_equivs	PARAMS ((rtx, rtx));
static rtx subst_indexed_address PARAMS ((rtx));
static void update_auto_inc_notes PARAMS ((rtx, int, int));
static int find_reloads_address_1 PARAMS ((enum machine_mode, rtx, int, rtx *,
					 int, enum reload_type,int, rtx));
static void find_reloads_address_part PARAMS ((rtx, rtx *, enum reg_class,
					     enum machine_mode, int,
					     enum reload_type, int));
static rtx find_reloads_subreg_address PARAMS ((rtx, int, int,
						enum reload_type, int, rtx));
static void copy_replacements_1 PARAMS ((rtx *, rtx *, int));
static int find_inc_amount	PARAMS ((rtx, rtx));

#ifdef HAVE_SECONDARY_RELOADS

/* Determine if any secondary reloads are needed for loading (if IN_P is
   non-zero) or storing (if IN_P is zero) X to or from a reload register of
   register class RELOAD_CLASS in mode RELOAD_MODE.  If secondary reloads
   are needed, push them.

   Return the reload number of the secondary reload we made, or -1 if
   we didn't need one.  *PICODE is set to the insn_code to use if we do
   need a secondary reload.  */

static int
push_secondary_reload (in_p, x, opnum, optional, reload_class, reload_mode,
		       type, picode)
     int in_p;
     rtx x;
     int opnum;
     int optional;
     enum reg_class reload_class;
     enum machine_mode reload_mode;
     enum reload_type type;
     enum insn_code *picode;
{
  enum reg_class class = NO_REGS;
  enum machine_mode mode = reload_mode;
  enum insn_code icode = CODE_FOR_nothing;
  enum reg_class t_class = NO_REGS;
  enum machine_mode t_mode = VOIDmode;
  enum insn_code t_icode = CODE_FOR_nothing;
  enum reload_type secondary_type;
  int s_reload, t_reload = -1;

  if (type == RELOAD_FOR_INPUT_ADDRESS
      || type == RELOAD_FOR_OUTPUT_ADDRESS
      || type == RELOAD_FOR_INPADDR_ADDRESS
      || type == RELOA