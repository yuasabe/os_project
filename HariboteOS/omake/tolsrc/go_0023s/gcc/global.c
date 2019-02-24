/* Allocate registers for pseudo-registers that span basic blocks.
   Copyright (C) 1987, 1988, 1991, 1994, 1996, 1997, 1998,
   1999, 2000, 2002 Free Software Foundation, Inc.

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

#include "machmode.h"
#include "hard-reg-set.h"
#include "rtl.h"
#include "tm_p.h"
#include "flags.h"
#include "basic-block.h"
#include "regs.h"
#include "function.h"
#include "insn-config.h"
#include "reload.h"
#include "output.h"
#include "toplev.h"

/* This pass of the compiler performs global register allocation.
   It assigns hard register numbers to all the pseudo registers
   that were not handled in local_alloc.  Assignments are recorded
   in the vector reg_renumber, not by changing the rtl code.
   (Such changes are made by final).  The entry point is
   the function global_alloc.

   After allocation is complete, the reload pass is run as a subroutine
   of this pass, so that when a pseudo reg loses its hard reg due to
   spilling it is possible to make a second attempt to find a hard
   reg for it.  The reload pass is independent in other respects
   and it is run even when stupid register allocation is in use.

   1. Assign allocation-numbers (allocnos) to the pseudo-registers
   still needing allocations and to the pseudo-registers currently
   allocated by local-alloc which may be spilled by reload.
   Set up tables reg_allocno and allocno_reg to map 
   reg numbers to allocnos and vice versa.
   max_allocno gets the number of allocnos in use.

   2. Allocate a max_allocno by max_allocno conflict bit matrix and clear it.
   Allocate a max_allocno by FIRST_PSEUDO_REGISTER conflict matrix
   for conflicts between allocnos and explicit hard register use
   (which includes use of pseudo-registers allocated by local_alloc).

   3. For each basic block
    walk forward through the block, recording which
    pseudo-registers and which hardware registers are live.
    Build the conflict matrix between the pseudo-registers
    and another of pseudo-registers versus hardware registers.
    Also record the preferred hardware registers
    for each pseudo-register.

   4. Sort a table of the allocnos into order of
   desirability of the variables.

   5. Allocate the variables in that order; each if possible into
   a preferred register, else into another register.  */

/* Number of pseudo-registers which are candidates for allocation.  */

static int max_allocno;

/* Indexed by (pseudo) reg number, gives the allocno, or -1
   for pseudo registers which are not to be allocated.  */

static int *reg_allocno;

struct allocno
{
  int reg;
  /* Gives the number of consecutive hard registers needed by that
     pseudo reg.  */
  int size;

  /* Number of calls crossed by each allocno.  */
  int calls_crossed;

  /* Number of refs to each allocno.  */
  int n_refs;

  /* Frequency of uses of each allocno.  */
  int freq;

  /* Guess at live length of each allocno.
     This is actually the max of the live lengths of the regs.  */
  int live_length;

  /* Set of hard regs conflicting with allocno N.  */

  HARD_REG_SET hard_reg_conflicts;

  /* Set of hard regs preferred by allocno N.
     This is used to make allocnos go into regs that are copied to or from them,
     when possible, to reduce register shuffling.  */

  HARD_REG_SET hard_reg_preferences;

  /* Similar, but just counts register preferences made in simple copy
     operations, rather than arithmetic.  These are given priority because
     we can always eliminate an insn by using these, but using a register
     in the above list won't always eliminate an insn.  */

  HARD_REG_SET hard_reg_copy_preferences;

  /* Similar to hard_reg_preferences, but includes bits for subsequent
     registers when an allocno is multi-word.  The above variable is used for
     allocation while this is used to build reg_someone_prefers, below.  */

  HARD_REG_SET hard_reg_full_preferences;

  /* Set of hard registers that some later allocno has a preference for.  */

  HARD_REG_SET regs_someone_prefers;
};

static struct allocno *allocno;

/* A vector of the integers from 0 to max_allocno-1,
   sorted in the order of first-to-be-allocated first.  */

static int *allocno_order;

/* Indexed by (pseudo) reg number, gives the number of another
   lower-numbered pseudo reg which can share a hard reg with this pseudo
   *even if the two pseudos would otherwise appear to conflict*.  */

static int *reg_may_share;

/* Define the number of bits in each element of `conflicts' and what
   type that element has.  We use the largest integer format on the
   host machine.  */

#define INT_BITS HOST_BITS_PER_WIDE_INT
#define INT_TYPE HOST_WIDE_INT

/* max_allocno by max_allocno array of bits,
   recording whether two allocno's conflict (can't go in the same
   hardware register).

   `conflicts' is symmetric after the call to mirror_conflicts.  */

static INT_TYPE *conflicts;

/* Number of ints require to hold max_allocno bits.
   This is the length of a row in `conflicts'.  */

static int allocno_row_words;

/* Two macros to test or store 1 in an element of `conflicts'.  */

#define CONFLICTP(I, J) ¥
 (conflicts[(I) * allocno_row_words + (unsigned) (J) / INT_BITS]	¥
  & ((INT_TYPE) 1 << ((unsigned) (J) % INT_BITS)))

#define SET_CONFLICT(I, J) ¥
 (conflicts[(I) * allocno_row_words + (unsigned) (J) / INT_BITS]	¥
  |= ((INT_TYPE) 1 << ((unsigned) (J) % INT_BITS)))

/* For any allocno set in ALLOCNO_SET, set ALLOCNO to that allocno,
   and execute CODE.  */
#define EXECUTE_IF_SET_IN_ALLOCNO_SET(ALLOCNO_SET, ALLOCNO, CODE)	¥
do {									¥
  int i_;								¥
  int allocno_;								¥
  INT_TYPE *p_ = (ALLOCNO_SET);						¥
									¥
  for (i_ = allocno_row_words - 1, allocno_ = 0; i_ >= 0;		¥
       i_--, allocno_ += INT_BITS)					¥
    {									¥
      unsigned INT_TYPE word_ = (unsigned INT_TYPE) *p_++;		¥
									¥
      for ((ALLOCNO) = allocno_; word_; word_ >>= 1, (ALLOCNO)++)	¥
	{								¥
	  if (word_ & 1)						¥
	    {CODE;}							¥
	}								¥
    }									¥
} while (0)

/* This doesn't work for non-GNU C due to the way CODE is macro expanded.  */
#if 0
/* For any allocno that conflicts with IN_ALLOCNO, set OUT_ALLOCNO to
   the conflicting allocno, and execute CODE.  This macro assumes that
   mirror_conflicts has been run.  */
#define EXECUTE_IF_CONFLICT(IN_ALLOCNO, OUT_ALLOCNO, CODE)¥
  EXECUTE_IF_SET_IN_ALLOCNO_SET (conflicts + (IN_ALLOCNO) * allocno_row_words,¥
				 OUT_ALLOCNO, (CODE))
#endif

/* Set of hard regs currently live (during scan of all insns).  */

static HARD_REG_SET hard_regs_live;

/* Set of registers that global-alloc isn't supposed to use.  */

static HARD_REG_SET no_global_alloc_regs;

/* Set of registers used so far.  */

static HARD_REG_SET regs_used_so_far;

/* Number of refs to each hard reg, as used by local alloc.
   It is zero for a reg that contains global pseudos or is explicitly used.  */

static int local_reg_n_refs[FIRST_PSEUDO_REGISTER];

/* Frequency of uses of given hard reg.  */
static int local_reg_freq[FIRST_PSEUDO_REGISTER];

/* Guess at live length of each hard reg, as used by local alloc.
   This is actually the sum of the live lengths of the specific regs.  */

static int local_reg_live_length[FIRST_PSEUDO_REGISTER];

/* Test a bit in TABLE, a vector of HARD_REG_SETs,
   for vector element I, and hard register number J.  */

#define REGBITP(TABLE, I, J)     TEST_HARD_REG_BIT (allocno[I].TABLE, J)

/* Set to 1 a bit in a vector of HARD_REG_SETs.  Works like REGBITP.  */

#define SET_REGBIT(TABLE, I, J)  SET_HARD_REG_BIT (allocno[I].TABLE, J)

/* Bit mask for allocnos live at current point in the scan.  */

static INT_TYPE *allocnos_live;

/* Test, set or clear bit number I in allocnos_live,
   a bit vector indexed by allocno.  */

#define ALLOCNO_LIVE_P(I)				¥
  (allocnos_live[(unsigned) (I) / INT_BITS]		¥
   & ((INT_TYPE) 1 << ((unsigned) (I) % INT_BITS)))

#define SET_ALLOCNO_LIVE(I)				¥
  (allocnos_live[(unsigned) (I) / INT_BITS]		¥
     |= ((INT_TYPE) 1 << ((unsigned) (I) % INT_BITS)))

#define CLEAR_ALLOCNO_LIVE(I)				¥
  (allocnos_live[(unsigned) (I) / INT_BITS]		¥
     &= ‾((INT_TYPE) 1 << ((unsigned) (I) % INT_BITS)))

/* This is turned off because it doesn't work right for DImode.
   (And it is only used for DImode, so the other cases are worthless.)
   The problem is that it isn't true that there is NO possibility of conflict;
   only that there is no conflict if the two pseudos get the exact same regs.
   If they were allocated with a partial overlap, there would be a conflict.
   We can't safely turn off the conflict unless we have another way to
   prevent the partial overlap.

   Idea: change hard_reg_conflicts so that instead of recording which
   hard regs the allocno may not overlap, it records where the allocno
   may not start.  Change both where it is used and where it is updated.
   Then there is a way to record that (reg:DI 108) may start at 10
   but not at 9 or 11.  There is still the question of how to record
   this semi-conflict between two pseudos.  */
#if 0
/* Reg pairs for which conflict after the current insn
   is inhibited by a REG_NO_CONFLICT note.
   If the table gets full, we ignore any other notes--that is conservative.  */
#define NUM_NO_CONFLICT_PAIRS 4
/* Number of pairs in use in this insn.  */
int n_no_conflict_pairs;
static struct { int allocno1, allocno2;}
  no_conflict_pairs[NUM_NO_CONFLICT_PAIRS];
#endif /* 0 */

/* Record all regs that are set in any one insn.
   Communication from mark_reg_{store,clobber} and global_conflicts.  */

static rtx *regs_set;
static int n_regs_set;

/* All registers that can be eliminated.  */

static HARD_REG_SET eliminable_regset;

static int allocno_compare	PARAMS ((const PTR, const PTR));
static void global_conflicts	PARAMS ((void));
static void mirror_conflicts	PARAMS ((void));
static void expand_preferences	PARAMS ((void));
static void prune_preferences	PARAMS ((void));
static void find_reg		PARAMS ((int, HARD_REG_SET, int, int, int));
static void record_one_conflict PARAMS ((int));
static void record_conflicts	PARAMS ((int *, int));
static void mark_reg_store	PARAMS ((rtx, rtx, void *));
static void mark_reg_clobber	PARAMS ((rtx, rtx, void *));
static void mark_reg_conflicts	PARAMS ((rtx));
static void mark_reg_death	PARAMS ((rtx));
static void mark_reg_live_nc	PARAMS ((int, enum machine_mode));
static void set_preference	PARAMS ((rtx, rtx));
static void dump_conflicts	PARAMS ((FILE *));
static void reg_becomes_live	PARAMS ((rtx, rtx, void *));
static void reg_dies		PARAMS ((int, enum machine_mode,
				       struct insn_chain *));

/* Perform allocation of pseudo-registers not allocated by local_alloc.
   FILE is a file to output debugging information on,
   or zero if such output is not desired.

   Return value is nonzero if reload failed
   and we must not do any more for this function.  */

int
global_alloc (file)
     FILE *file;
{
  int retval;
#ifdef ELIMINABLE_REGS
  static const struct {const int from, to; } eliminables[] = ELIMINABLE_REGS;
#endif
  int need_fp
    = (! flag_omit_frame_pointer
#ifdef EXIT_IGNORE_STACK
       || (current_function_calls_alloca && EXIT_IGNORE_STACK)
#endif
       || FRAME_POINTER_REQUIRED);

  size_t i;
  rtx x;

  max_allocno = 0;

  /* A machine may have certain hard registers that
     are safe to use only within a basic block.  */

  CLEAR_HARD_REG_SET (no_global_alloc_regs);

  /* Build the regset of all eliminable registers and show we can't use those
     that we already know won't be eliminated.  */
#ifdef ELIMINABLE_REGS
  for (i = 0; i < ARRAY_SIZE (eliminables); i++)
    {
      SET_HARD_REG_BIT (eliminable_regset, eliminables[i].from);

      if (! CAN_ELIMINATE (eliminables[i].from, eliminables[i].to)
	  || (eliminables[i].to == STACK_POINTER_REGNUM && need_fp))
	SET_HARD_REG_BIT (no_global_alloc_regs, eliminables[i].from);
    }
#if FRAME_POINTER_REGNUM != HARD_FRAME_POINTER_REGNUM
  SET_HARD_REG_BIT (eliminable_regset, HARD_FRAME_POINTER_REGNUM);
  if (need_fp)
    SET_HARD_REG_BIT (no_global_alloc_regs, HARD_FRAME_POINTER_REGNUM);
#endif

#else
  SET_HARD_REG_BIT (eliminable_regset, FRAME_POINTER_REGNUM);
  if (need_fp)
    SET_HARD_REG_BIT (no_global_alloc_regs, FRAME_POINTER_REGNUM);
#endif

  /* Track which registers have already been used.  Start with registers
     explicitly in the rtl, then registers allocated by local register
     allocation.  */

  CLEAR_HARD_REG_SET (regs_used_so_far);
#ifdef LEAF_REGISTERS
  /* If we are doing the leaf function optimization, and this is a leaf
     function, it means that the registers that take work to save are those
     that need a register window.  So prefer the ones that can be used in
     a leaf function.  */
  {
    char *cheap_regs;
    char *leaf_regs = LEAF_REGISTERS;

    if (only_leaf_regs_used () && leaf_function_p ())
      cheap_regs = leaf_regs;
    else
      cheap_regs = call_used_regs;
    for (i = 0; i < FIRST_PSEUDO_REGISTER; i++)
      if (regs_ever_live[i] || cheap_regs[i])
	SET_HARD_REG_BIT (regs_used_so_far, i);
  }
#else
  /* We consider registers that do not have to be saved over calls as if
     they were already used since there is no cost in using them.  */
  for (i = 0; i < FIRST_PSEUDO_REGISTER; i++)
    if (regs_ever_live[i] || call_used_regs[i])
      SET_HARD_REG_BIT (regs_used_so_far, i);
#endif

  for (i = FIRST_PSEUDO_REGISTER; i < (size_t) max_regno; i++)
    if (reg_renumber[i] >= 0)
      SET_HARD_REG_BIT (regs_used_so_far, reg_renumber[i]);

  /* Establish mappings from register number to allocation number
     and vice versa.  In the process, count the allocnos.  */

  reg_allocno = (int *) xmalloc (max_regno * sizeof (int));

  for (i = 0; i < FIRST_PSEUDO_REGISTER; i++)
    reg_allocno[i] = -1;

  /* Initialize the shared-hard-reg mapping
     from the list of pairs that may share.  */
  reg_may_share = (int *) xcalloc (max_regno, sizeof (int));
  for (x = regs_may_share; x; x = XEXP (XEXP (x, 1), 1))
    {
      int r1 = REGNO (XEXP (x, 0));
      int r2 = REGNO (XEXP (XEXP (x, 1), 0));
      if (r1 > r2)
	reg_may_share[r1] = r2;
      else
	reg_may_share[r2] = r1;
    }

  for (i = FIRST_PSEUDO_REGISTER; i < (size_t) max_regno; i++)
    /* Note that reg_live_length[i] < 0 indicates a "constant" reg
       that we are supposed to refrain from putting in a hard reg.
       -2 means do make an allocno but don't allocate it.  */
    if (REG_N_REFS (i) != 0 && REG_LIVE_LENGTH (i) != -1
	/* Don't allocate pseudos that cross calls,
	   if this function receives a nonlocal goto.  */
	&& (! current_function_has_nonlocal_label
	    || REG_N_CALLS_CROSSED (i) == 0))
      {
	if (reg_renumber[i] < 0 && reg_may_share[i] && reg_allocno[reg_may_share[i]] >= 0)
	  reg_allocno[i] = reg_allocno[reg_may_share[i]];
	else
	  reg_allocno[i] = max_allocno++;
	if (REG_LIVE_LENGTH (i) == 0)
	  abort ();
      }
    else
      reg_allocno[i] = -1;

  allocno = (struct allocno *) xcalloc (max_allocno, sizeof (struct allocno));

  for (i = FIRST_PSEUDO_REGISTER; i < (size_t) max_regno; i++)
    if (reg_allocno[i] >= 0)
      {
	int num = reg_allocno[i];
	allocno[num].reg = i;
	allocno[num].size = PSEUDO_REGNO_SIZE (i);
	allocno[num].calls_crossed += REG_