/* Define per-register tables for data flow info and register allocation.
   Copyright (C) 1987, 1993, 1994, 1995, 1996, 1997, 1998,
   1999, 2000 Free Software Foundation, Inc.

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


#include "varray.h"

#define REG_BYTES(R) mode_size[(int) GET_MODE (R)]

/* When you only have the mode of a pseudo register before it has a hard
   register chosen for it, this reports the size of each hard register
   a pseudo in such a mode would get allocated to.  A target may
   override this.  */

#ifndef REGMODE_NATURAL_SIZE
#define REGMODE_NATURAL_SIZE(MODE)	UNITS_PER_WORD
#endif

#ifndef SMALL_REGISTER_CLASSES
#define SMALL_REGISTER_CLASSES 0
#endif

/* Maximum register number used in this function, plus one.  */

extern int max_regno;

/* Register information indexed by register number */
typedef struct reg_info_def
{				/* fields set by reg_scan */
  int first_uid;		/* UID of first insn to use (REG n) */
  int last_uid;			/* UID of last insn to use (REG n) */
  int last_note_uid;		/* UID of last note to use (REG n) */

				/* fields set by reg_scan & flow_analysis */
  int sets;			/* # of times (REG n) is set */

				/* fields set by flow_analysis */
  int refs;			/* # of times (REG n) is used or set */
  int freq;			/* # estimated frequency (REG n) is used or set */
  int deaths;			/* # of times (REG n) dies */
  int live_length;		/* # of instructions (REG n) is live */
  int calls_crossed;		/* # of calls (REG n) is live across */
  int basic_block;		/* # of basic blocks (REG n) is used in */
  char changes_mode;		/* whether (SUBREG (REG n)) exists and 
				   is illegal.  */
} reg_info;

extern varray_type reg_n_info;

/* Indexed by n, gives number of times (REG n) is used or set.  */

#define REG_N_REFS(N) (VARRAY_REG (reg_n_info, N)->refs)

/* Estimate frequency of references to register N.  */

#define REG_FREQ(N) (VARRAY_REG (reg_n_info, N)->freq)

/* The weights for each insn varries from 0 to REG_FREQ_BASE. 
   This constant does not need to be high, as in infrequently executed
   regions we want to count instructions equivalently to optimize for
   size instead of speed.  */
#define REG_FREQ_MAX 1000

/* Compute register frequency from the BB frequency.  When optimizing for size,
   or profile driven feedback is available and the function is never executed,
   frequency is always equivalent.  Otherwise rescale the basic block
   frequency.  */
#define REG_FREQ_FROM_BB(bb) (optimize_size				      ¥
			      || (flag_branch_probabilities		      ¥
				  && !ENTRY_BLOCK_PTR->count)		      ¥
			      ? REG_FREQ_MAX				      ¥
			      : ((bb)->frequency * REG_FREQ_MAX / BB_FREQ_MAX)¥
			      ? ((bb)->frequency * REG_FREQ_MAX / BB_FREQ_MAX)¥
			      : 1)

/* Indexed by n, gives number of times (REG n) is set.
   ??? both regscan and flow allocate space for this.  We should settle
   on just copy.  */

#define REG_N_SETS(N) (VARRAY_REG (reg_n_info, N)->sets)

/* Indexed by N, gives number of insns in which register N dies.
   Note that if register N is live around loops, it can die
   in transitions between basic blocks, and that is not counted here.
   So this is only a reliable indicator of how many regions of life there are
   for registers that are contained in one basic block.  */

#define REG_N_DEATHS(N) (VARRAY_REG (reg_n_info, N)->deaths)

/* Inde