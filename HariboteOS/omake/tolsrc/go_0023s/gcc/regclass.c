/* Compute register class preferences for pseudo-registers.
   Copyright (C) 1987, 1988, 1991, 1992, 1993, 1994, 1995, 1996
   1997, 1998, 1999, 2000, 2001, 2002 Free Software Foundation, Inc.

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


/* This file contains two passes of the compiler: reg_scan and reg_class.
   It also defines some tables of information about the hardware registers
   and a function init_reg_sets to initialize the tables.  */

#include "config.h"
#include "system.h"
#include "rtl.h"
#include "expr.h"
#include "tm_p.h"
#include "hard-reg-set.h"
#include "flags.h"
#include "basic-block.h"
#include "regs.h"
#include "function.h"
#include "insn-config.h"
#include "recog.h"
#include "reload.h"
#include "real.h"
#include "toplev.h"
#include "output.h"
#include "ggc.h"

#ifndef REGISTER_MOVE_COST
#define REGISTER_MOVE_COST(m, x, y) 2
#endif

static void init_reg_sets_1	PARAMS ((void));
static void init_reg_modes	PARAMS ((void));

/* If we have auto-increment or auto-decrement and we can have secondary
   reloads, we are not allowed to use classes requiring secondary
   reloads for pseudos auto-incremented since reload can't handle it.  */

#ifdef AUTO_INC_DEC
#if defined(SECONDARY_INPUT_RELOAD_CLASS) || defined(SECONDARY_OUTPUT_RELOAD_CLASS)
#define FORBIDDEN_INC_DEC_CLASSES
#endif
#endif

/* Register tables used by many passes.  */

/* Indexed by hard register number, contains 1 for registers
   that are fixed use (stack pointer, pc, frame pointer, etc.).
   These are the registers that cannot be used to allocate
   a pseudo reg for general use.  */

char fixed_regs[FIRST_PSEUDO_REGISTER];

/* Same info as a HARD_REG_SET.  */

HARD_REG_SET fixed_reg_set;

/* Data for initializing the above.  */

static const char initial_fixed_regs[] = FIXED_REGISTERS;

/* Indexed by hard register number, contains 1 for registers
   that are fixed use or are clobbered by function calls.
   These are the registers that cannot be used to allocate
   a pseudo reg whose life crosses calls unless we are able
   to save/restore them across the calls.  */

char call_used_regs[FIRST_PSEUDO_REGISTER];

/* Same info as a HARD_REG_SET.  */

HARD_REG_SET call_used_reg_set;

/* HARD_REG_SET of registers we want to avoid caller saving.  */
HARD_REG_SET losing_caller_save_reg_set;

/* Data for initializing the above.  */

static const char initial_call_used_regs[] = CALL_USED_REGISTERS;

/* This is much like call_used_regs, except it doesn't have to
   be a superset of FIXED_REGISTERS. This vector indicates
   what is really call clobbered, and is used when defining 
   regs_invalidated_by_call.  */

#ifdef CALL_REALLY_USED_REGISTERS
char call_really_used_regs[] = CALL_REALLY_USED_REGISTERS;
#endif
  
/* Indexed by hard register number, contains 1 for registers that are
   fixed use or call used registers that cannot hold quantities across
   calls even if we are willing to save and restore them.  call fixed
   registers are a subset of call used registers.  */

char call_fixed_regs[FIRST_PSEUDO_REGISTER];

/* The same info as a HARD_REG_SET.  */

HARD_REG_SET call_fixed_reg_set;

/* Number of non-fixed registers.  */

int n_non_fixed_regs;

/* Indexed by hard register number, contains 1 for registers
   that are being used for global register decls.
   These must be exempt from ordinary flow analysis
   and are also considered fixed.  */

char global_regs[FIRST_PSEUDO_REGISTER];

/* Contains 1 for registers that are set or clobbered by calls.  */
/* ??? Ideally, this would be just call_used_regs plus global_regs, but
   for someone's bright idea to have call_used_regs strictly include
   fixed_regs.  Which leaves us guessing as to the set of fixed_regs
   that are actually preserved.  We know for sure that those associated
   with the local stack frame are safe, but scant others.  */

HARD_REG_SET regs_invalidated_by_call;

/* Table of register numbers in the order in which to try to use them.  */
#ifdef REG_ALLOC_ORDER
int reg_alloc_order[FIRST_PSEUDO_REGISTER] = REG_ALLOC_ORDER;

/* The inverse of reg_alloc_order.  */
int inv_reg_alloc_order[FIRST_PSEUDO_REGISTER];
#endif

/* For each reg class, a HARD_REG_SET saying which registers are in it.  */

HARD_REG_SET reg_class_contents[N_REG_CLASSES];

/* The same information, but as an array of unsigned ints.  We copy from
   these unsigned ints to the table above.  We do this so the tm.h files
   do not have to be aware of the wordsize for machines with <= 64 regs.
   Note that we hard-code 32 here, not HOST_BITS_PER_INT.  */

#define N_REG_INTS  ¥
  ((FIRST_PSEUDO_REGISTER + (32 - 1)) / 32)

static const unsigned int_reg_class_contents[N_REG_CLASSES][N_REG_INTS] 
  = REG_CLASS_CONTENTS;

/* For each reg class, number of regs it contains.  */

unsigned int reg_class_size[N_REG_CLASSES];

/* For each reg class, table listing all the containing classes.  */

enum reg_class reg_class_superclasses[N_REG_CLASSES][N_REG_CLASSES];

/* For each reg class, table listing all the classes contained in it.  */

enum reg_class reg_class_subclasses[N_REG_CLASSES][N_REG_CLASSES];

/* For each pair of reg classes,
   a largest reg class contained in their union.  */

enum reg_class reg_class_subunion[N_REG_CLASSES][N_REG_CLASSES];

/* For each pair of reg classes,
   the smallest reg class containing their union.  */

enum reg_class reg_class_superunion[N_REG_CLASSES][N_REG_CLASSES];

/* Array containing all of the register names.  Unless
   DEBUG_REGISTER_NAMES is defined, use the copy in print-rtl.c.  */

#ifdef DEBUG_REGISTER_NAMES
const char * reg_names[] = REGISTER_NAMES;
#endif

/* For each hard register, the widest mode object that it can contain.
   This will be a MODE_INT mode if the register can hold integers.  Otherwise
   it will be a MODE_FLOAT or a MODE_CC mode, whichever is valid for the
   register.  */

enum machine_mode reg_raw_mode[FIRST_PSEUDO_REGISTER];

/* 1 if class does contain register of given mode.  */

static char contains_reg_of_mode [N_REG_CLASSES] [MAX_MACHINE_MODE];

/* Maximum cost of moving from a register in one class to a register in
   another class.  Based on REGISTER_MOVE_COST.  */

static int move_cost[MAX_MACHINE_MODE][N_REG_CLASSES][N_REG_CLASSES];

/* Similar, but here we don't have to move if the first index is a subset
   of the second so in that case the cost is zero.  */

static int may_move_in_cost[MAX_MACHINE_MODE][N_REG_CLASSES][N_REG_CLASSES];

/* Similar, but here we don't have to move if the first index is a superset
   of the second so in that case the cost is zero.  */

static int may_move_out_cost[MAX_MACHINE_MODE][N_REG_CLASSES][N_REG_CLASSES];

#ifdef FORBIDDEN_INC_DEC_CLASSES

/* These are the classes that regs which are auto-incremented or decremented
   cannot be put in.  */

static int forbidden_inc_dec_class[N_REG_CLASSES];

/* Indexed by n, is non-zero if (REG n) is used in an auto-inc or auto-dec
   context.  */

static char *in_inc_dec;

#endif /* FORBIDDEN_INC_DEC_CLASSES */

#ifdef CLASS_CANNOT_CHANGE_MODE

/* These are the classes containing only registers that can be used in
   a SUBREG expression that changes the mode of the register in some
   way that is illegal.  */

static int class_can_change_mode[N_REG_CLASSES];

/* Registers, including pseudos, which change modes in some way that
   is illegal.  */

static regset reg_changes_mode;

#endif /* CLASS_CANNOT_CHANGE_MODE */

#ifdef HAVE_SECONDARY_RELOADS

/* Sample MEM values for use by memory_move_secondary_cost.  */

static rtx top_of_stack[MAX_MACHINE_MODE];

#endif /* HAVE_SECONDARY_RELOADS */

/* Linked list of reg_info structures allocated for reg_n_info array.
   Grouping all of the allocated structures together in one lump
   means only one call to bzero to clear them, rather than n smaller
   calls.  */
struct reg_info_data {
  struct reg_info_data *next;	/* next set of reg_info structures */
  size_t min_index;		/* minimum index # */
  size_t max_index;		/* maximum index # */
  char used_p;			/* non-zero if this has been used previously */
  reg_info data[1];		/* beginning of the reg_info data */
};

static struct reg_info_data *reg_info_head;

/* No more global register variables may be declared; true once
   regclass has been initialized.  */

static int no_global_reg_vars = 0;


/* Function called only once to initialize the above data on reg usage.
   Once this is done, various switches may override.  */

void
init_reg_sets ()
{
  int i, j;

  /* First copy the register information from the initial int form into
     the regsets.  */

  for (i = 0; i < N_REG_CLASSES; i++)
    {
      CLEAR_HARD_REG_SET (reg_class_contents[i]);

      /* Note that we hard-code 32 here, not HOST_BITS_PER_INT.  */
      for (j = 0; j < FIRST_PSEUDO_REGISTER; j++)
	if (int_reg_class_contents[i][j / 32]
	    & ((unsigned) 1 << (j % 32)))
	  SET_HARD_REG_BIT (reg_class_contents[i], j);
    }

  memcpy (fixed_regs, initial_fixed_regs, sizeof fixed_regs);
  memcpy (call_used_regs, initial_call_used_regs, sizeof call_used_regs);
  memset (global_regs, 0, sizeof global_regs);

  /* Do any additional initialization regsets may need */
  INIT_ONCE_REG_SET ();

#ifdef REG_ALLOC_ORDER
  for (i = 0; i < FIRST_PSEUDO_REGISTER; i++)
    inv_reg_alloc_order[reg_alloc_order[i]] = i;
#endif
}

/* After switches have been processed, which perhaps alter
   `fixed_regs' and `call_used_regs', convert them to HARD_REG_SETs.  */

static void
init_reg_sets_1 ()
{
  unsigned int i, j;
  unsigned int /* enum machine_mode */ m;
  char allocatable_regs_of_mode [MAX_MACHINE_MODE];

  /* This macro allows the fixed or call-used registers
     and the register classes to depend on target flags.  */

#ifdef CONDITIONAL_REGISTER_USAGE
  CONDITIONAL_REGISTER_USAGE;
#endif

  /* Compute number of hard regs in each class.  */

  memset ((char *) reg_class_size, 0, sizeof reg_class_size);
  for (i = 0; i < N_REG_CLASSES; i++)
    for (j = 0; j < FIRST_PSEUDO_REGISTER; j++)
      if (TEST_HARD_REG_BIT (reg_class_contents[i], j))
	reg_class_size[i]++;

  /* Initialize the table of subunions.
     reg_class_subunion[I][J] gets the largest-numbered reg-class
     that is contained in the union of classes I and J.  */

  for (i = 0; i < N_REG_CLASSES; i++)
    {
      for (j = 0; j < N_REG_CLASSES; j++)
	{
#ifdef HARD_REG_SET
	  register		/* Declare it register if it's a scalar.  */
#endif
	    HARD_REG_SET c;
	  int k;

	  COPY_HARD_REG_SET (c, reg_class_contents[i]);
	  IOR_HARD_REG_SET (c, reg_class_contents[j]);
	  for (k = 0; k < N_REG_CLASSES; k++)
	    {
	      GO_IF_HARD_REG_SUBSET (reg_class_contents[k], c,
				     subclass1);
	      continue;

	    subclass1:
	      /* keep the largest subclass */		/* SPEE 900308 */
	      GO_IF_HARD_REG_SUBSET (reg_class_contents[k],
				     reg_class_contents[(int) reg_class_subunion[i][j]],
				     subclass2);
	      reg_class_subunion[i][j] = (enum reg_class) k;
	    subclass2:
	      ;
	    }
	}
    }

  /* Initialize the table of superunions.
     reg_class_superunion[I][J] gets the smallest-numbered reg-class
     containing the union of classes I and J.  */

  for (i = 0; i < N_REG_CLASSES; i++)
    {
      for (j = 0; j < N_REG_CLASSES; j++)
	{
#ifdef HARD_REG_SET
	  register		/* Declare it register if it's a sca