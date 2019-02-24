/* Register renaming for the GNU compiler.
   Copyright (C) 2000, 2001, 2002 Free Software Foundation, Inc.

   This file is part of GCC.

   GCC is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   GCC is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License
   along with GCC; see the file COPYING.  If not, write to the Free
   Software Foundation, 59 Temple Place - Suite 330, Boston, MA
   02111-1307, USA.  */

#define REG_OK_STRICT

/* !kawai! */
#include "config.h"
#include "system.h"
#include "rtl.h"
#include "tm_p.h"
#include "insn-config.h"
#include "regs.h"
#include "hard-reg-set.h"
#include "basic-block.h"
#include "reload.h"
#include "output.h"
#include "function.h"
#include "recog.h"
#include "flags.h"
#include "toplev.h"
#include "../include/obstack.h"
/* end of !kawai! */

#define obstack_chunk_alloc xmalloc
#define obstack_chunk_free free

#ifndef REGNO_MODE_OK_FOR_BASE_P
#define REGNO_MODE_OK_FOR_BASE_P(REGNO, MODE) REGNO_OK_FOR_BASE_P (REGNO)
#endif

#ifndef REG_MODE_OK_FOR_BASE_P
#define REG_MODE_OK_FOR_BASE_P(REGNO, MODE) REG_OK_FOR_BASE_P (REGNO)
#endif

static const char *const reg_class_names[] = REG_CLASS_NAMES;

struct du_chain
{
  struct du_chain *next_chain;
  struct du_chain *next_use;

  rtx insn;
  rtx *loc;
  enum reg_class class;
  unsigned int need_caller_save_reg:1;
  unsigned int earlyclobber:1;
};

enum scan_actions
{
  terminate_all_read,
  terminate_overlapping_read,
  terminate_write,
  terminate_dead,
  mark_read,
  mark_write
};

static const char * const scan_actions_name[] =
{
  "terminate_all_read",
  "terminate_overlapping_read",
  "terminate_write",
  "terminate_dead",
  "mark_read",
  "mark_write"
};

static struct obstack rename_obstack;

static void do_replace PARAMS ((struct du_chain *, int));
static void scan_rtx_reg PARAMS ((rtx, rtx *, enum reg_class,
				  enum scan_actions, enum op_type, int));
static void scan_rtx_address PARAMS ((rtx, rtx *, enum reg_class,
				      enum scan_actions, enum machine_mode));
static void scan_rtx PARAMS ((rtx, rtx *, enum reg_class,
			      enum scan_actions, enum op_type, int));
static struct du_chain *build_def_use PARAMS ((basic_block));
static void dump_def_use_chain PARAMS ((struct du_chain *));
static void note_sets PARAMS ((rtx, rtx, void *));
static void clear_dead_regs PARAMS ((HARD_REG_SET *, enum machine_mode, rtx));
static void merge_overlapping_regs PARAMS ((basic_block, HARD_REG_SET *,
					    struct du_chain *));

/* Called through note_stores from update_life.  Find sets of registers, and
   record them in *DATA (which is actually a HARD_REG_SET *).  */

static void
note_sets (x, set, data)
     rtx x;
     rtx set ATTRIBUTE_UNUSED;
     void *data;
{
  HARD_REG_SET *pset = (HARD_REG_SET *) data;
  unsigned int regno;
  int nregs;
  if (GET_CODE (x) != REG)
    return;
  regno = REGNO (x);
  nregs = HARD_REGNO_NREGS (regno, GET_MODE (x));

  /* There must not be pseudos at this point.  */
  if (regno + nregs > FIRST_PSEUDO_REGISTER)
    abort ();

  while (nregs-- > 0)
    SET_HARD_REG_BIT (*pset, regno + nregs);
}

/* Clear all registers from *PSET for which a note of kind KIND can be found
   in the list NOTES.  */

static void
clear_dead_regs (pset, kind, notes)
     HARD_REG_SET *pset;
     enum machine_mode kind;
     rtx notes;
{
  rtx note;
  for (note = notes; note; note = XEXP (note, 1))
    if (REG_NOTE_KIND (note) == kind && REG_P (XEXP (note, 0)))
      {
	rtx reg = XEXP (note, 0);
	unsigned int regno = REGNO (reg);
	int nregs = HARD_REGNO_NREGS (regno, GET_MODE (reg));

	/* There must not be pseudos at this point.  */
	if (regno + nregs > FIRST_PSEUDO_REGISTER)
	  abort ();

	while (nregs-- > 0)
	  CLEAR_HARD_REG_BIT (*pset, regno + nregs);
      }
}

/* For a def-use chain CHAIN in basic block B, find which registers overlap
   its lifetime and set the corresponding bits in *PSET.  */

static void
merge_overlapping_regs (b, pset, chain)
     basic_block b;
     HARD_REG_SET *pset;
     struct du_chain *chain;
{
  struct du_chain *t = chain;
  rtx insn;
  HARD_REG_SET live;

  REG_SET_TO_HARD_REG_SET (live, b->global_live_at_start);
  insn = b->head;
  while (t)
    {
      /* Search forward until the next reference to the register to be
	 renamed.  */
      while (insn != t->insn)
	{
	  if (INSN_P (insn))
	    {
	      clear_dead_regs (&live, REG_DEAD, REG_NOTES (insn));
	      note_stores (PATTERN (insn), note_sets, (void *) &live);
	      /* Only record currently live regs if we are inside the
		 reg's live range.  */
	      if (t != chain)
		IOR_HARD_REG_SET (*pset, live);
	      clear_dead_regs (&live, REG_UNUSED, REG_NOTES (insn));  
	    }
	  insn = NEXT_INSN (insn);
	}

      IOR_HARD_REG_SET (*pset, live);

      /* For the last reference, also merge in all registers set in the
	 same insn.
	 @@@ We only have take earlyclobbered sets into account.  */
      if (! t->next_use)
	note_stores (PATTERN (insn), note_sets, (void *) pset);

      t = t->next_use;
    }
}

/* Perform register renaming on the current function.  */

void
regrename_optimize ()
{
  int tick[FIRST_PSEUDO_REGISTER];
  int this_tick = 0;
  int b;
  char *first_obj;

  memset (tick, 0, sizeof tick);

  gcc_obstack_init (&rename_obstack);
  first_obj = (char *) obstack_alloc (&rename_obstack, 0);

  for (b = 0; b < n_basic_blocks; b++)
    {
      basic_block bb = BASIC_BLOCK (b);
      struct du_chain *all_chains = 0;
      HARD_REG_SET unavailable;
      HARD_REG_SET regs_seen;

      CLEAR_HARD_REG_SET (unavailable);

      if (rtl_dump_file)
	fprintf (rtl_dump_file, "¥nBasic block %d:¥n", b);

      all_chains = build_def_use (bb);

      if (rtl_dump_file)
	dump_def_use_chain (all_chains);

      CLEAR_HARD_REG_SET (unavailable);
      /* Don't clobber traceback for noreturn functions.  */
      if (frame_pointer_needed)
	{
	  int i;
	  
	  for (i = HARD_REGNO_NREGS (FRAME_POINTER_REGNUM, Pmode); i--;)
	    SET_HARD_REG_BIT (unavailable, FRAME_POINTER_REGNUM + i);
	  
#if FRAME_POINTER_REGNUM != HARD_FRAME_POINTER_REGNUM
	  for (i = HARD_REGNO_NREGS (HARD_FRAME_POINTER_REGNUM, Pmode); i--;)
	    SET_HARD_REG_BIT (unavailable, HARD_FRAME_POINTER_REGNUM + i);
#endif
	}

      CLEAR_HARD_REG_SET (regs_seen);
      while (all_chains)
	{
	  int new_reg, best_new_reg = -1;
	  int n_uses;
	  struct du_chain *this = all_chains;
	  struct du_chain *tmp, *last;
	  HARD_REG_SET this_unavailable;
	  int reg = REGNO (*this->loc);
	  int i;

	  all_chains = this->next_chain;
	  
#if 0 /* This just disables optimization opportunities.  */
	  /* Only rename once we've seen the reg more than once.  */
	  if (! TEST_HARD_REG_BIT (regs_seen, reg))
	    {
	      SET_HARD_REG_BIT (regs_seen, reg);
	      continue;
	    }
#endif

	  if (fixed_regs[reg] || global_regs[reg]
#if FRAME_POINTER_REGNUM != HARD_FRAME_POINTER_REGNUM
	      || (frame_pointer_needed && reg == HARD_FRAME_POINTER_REGNUM)
#else
	      || (frame_pointer_needed && reg == FRAME_POINTER_REGNUM)
#endif
	      )
	    continue;

	  COPY_HARD_REG_SET (this_unavailable, unavailable);

	  /* Find last entry on chain (which has the need_caller_save bit),
	     count number of uses, and narrow the set of registers we can
	     use for renaming.  */
	  n_uses = 0;
	  for (last = this; last->next_use; last = last->next_use)
	    {
	      n_uses++;
	      IOR_COMPL_HARD_REG_SET (this_unavailable,
				      reg_class_contents[last->class]);
	    }
	  if (n_uses < 1)
	    continue;

	  IOR_COMPL_HARD_REG_SET (this_unavailable,
				  reg_class_contents[last->class]);

	  if (this->need_caller_save_reg)
	    IOR_HARD_REG_SET (this_unavailable, call_used_reg_set);

	  merge_overlapping_regs (bb, &this_unavailable, this);

	  /* Now potential_regs is a reasonable approximation, let's
	     have a closer look at each register still in there.  */
	  for (new_reg = 0; new_reg < FIRST_PSEUDO_REGISTER; new_reg++)
	    {
	      int nregs = HARD_REGNO_NREGS (new_reg, GET_MODE (*this->loc));

	      for (i = nregs - 1; i >= 0; --i)
	        if (TEST_HARD_REG_BIT (this_unavailable, new_reg + i)
		    || fixed_regs[new_reg + i]
		    || global_regs[new_reg + i]
		    /* Can't use regs which aren't saved by the prologue.  */
		    || (! regs_ever_live[new_reg + i]
			&& ! call_used_regs[new_reg + i])
#ifdef LEAF_REGISTERS
		    /* We can't use a non-leaf register if we're in a 
		       leaf function.  */
		    || (current_function_is_leaf 
			&& !LEAF_REGISTERS[new_reg + i])
#endif
#ifdef HARD_REGNO_RENAME_OK
		    || ! HARD_REGNO_RENAME_OK (reg + i, new_reg + i)
#endif
		    )
		  break;
	      if (i >= 0)
		continue;

	      /* See whether it accepts all modes that occur in
		 definition and uses.  */
	      for (tmp = this; tmp; tmp = tmp->next_use)
		if (! HARD_REGNO_MODE_OK (new_reg, GET_MODE (*tmp->loc))
		    || (tmp->need_caller_save_reg
			&& ! (HARD_REGNO_CALL_PART_CLOBBERED
			      (reg, GET_MODE (*tmp->loc)))
			&& (HARD_REGNO_CALL_PART_CLOBBERED
			    (new_reg, GET_MODE (*tmp->loc)))))
		  break;
	      if (! tmp)
		{
		  if (best_new_reg == -1
		      || tick[best_new_reg] > tick[new_reg])
		    best_new_reg = new_reg;
		}
	    }

	  if (rtl_dump_file)
	    {
	      fprintf (rtl_dump_file, "Register %s in insn %d",
		       reg_names[reg], INSN_UID (last->insn));
	      if (last->need_caller_save_reg)
		fprintf (rtl_dump_file, " crosses a call");
	      }

	  if (best_new_reg == -1)
	    {
	      if (rtl_dump_file)
		fprintf (rtl_dump_file, "; no available registers¥n");
	      continue;
	    }

	  do_replace (this, best_new_reg);
	  tick[best_new_reg] = this_tick++;

	  if (rtl_dump_file)
	    fprintf (rtl_dump_file, ", renamed as %s¥n", reg_names[best_new_reg]);
	}

      obstack_free (&rename_obstack, first_obj);
    }

  obstack_free (&rename_obstack, NULL);

  if (rtl_dump_file)
    fputc ('¥n', rtl_dump_file);

  count_or_remove_death_notes (NULL, 1);
  update_life_info (NULL, UPDATE_LIFE_LOCAL,
		    PROP_REG_INFO | PROP_DEATH_NOTES);
}

static void
do_replace (chain, reg)
     struct du_chain *chain;
     int reg;
{
  while (chain)
    {
      unsigned int regno = ORIGINAL_REGNO (*chain->loc);
      *chain->loc = gen_raw_REG (GET_MODE (*chain->loc), reg);
      if (regno >= FIRST_PSEUDO_REGISTER)
	ORIGINAL_REGNO (*chain->loc) = regno;
      chain = chain->next_use;
    }
}


static struct du_chain *open_chains;
static struct du_chain *closed_chains;

static void
scan_rtx_reg (insn, loc, class, action, type, earlyclobber)
     rtx insn;
     rtx *loc;
     enum reg_class class;
     enum scan_actions action;
     enum op_type type;
     int earlyclobber;
{
  struct du_chain **p;
  rtx x = *loc;
  enum machine_mode mode = GET_MODE (x);
  int this_regno = REGNO (x);
  int this_nregs = HARD_REGNO_NREGS (this_regno, mode);

  if (action == mark_write)
    {
      if (type == OP_OUT)
	{
	  struct du_chain *this = (struct du_chain *)
	    obstack_alloc (&rename_obstack, sizeof (struct du_chain));
	  this->next_use = 0;
	  this->next_chain = open_chains;
	  this->loc = loc;
	  this->insn = insn;
	  this->class = class;
	  this->need_caller_save_reg = 0;
	  this->earlyclobber = earlyclobber;
	  open_chains = this;
	}
      return;
    }

  if ((type == OP_OUT && action != terminate_write)
      || (type != OP_OUT && action == terminate_write))
    return;

  for (p = &open_chains; *p;)
    {
      struct du_chain *this = *p;

      /* Check if the chain has been terminated if it has then skip to
	 the next chain.

	 This can happen when we've already appended the location to
	 the chain in Step 3, but are trying to hide in-out operands
	 from terminate_write in Step 5.  */

      if (*this->loc == cc0_rtx)
	p = &this->next_chain;
      else
        {
	  int regno = REGNO (*this->loc);
	  int nregs = HARD_REGNO_NREGS (regno, GET_MODE (*this->loc));
	  int exact_match = (regno == this_regno && nregs == this_nregs);

	  if (regno + nregs <= this_regno
	      || this_regno + this_nregs <= regno)
	    {
	      p = &this->next_chain;
	      continue;
	    }

	  if (action == mark_read)
	    {
	      if (! exact_match)
		abort ();

	      /* ??? Class NO_REGS can happen if the md file makes use of 
		 EXTRA_CONSTRAINTS to match registers.  Which is arguably
		 wrong, but there we are.  Since we know not what this may
		 be replaced with, terminate the chain.  */
	      if (class != NO_REGS)
		{
		  this = (struct du_chain *)
		    obstack_alloc (&rename_obstack, sizeof (struct du_chain));
		  this->next_use = 0;
		  this->next_chain = (*p)->next_chain;
		  this->loc = loc;
		  this->insn = insn;
		  this->class = class;
		  this->need_caller_save_reg = 0;
		  while (*p)
		    p = &(*p)->next_use;
		  *p = this;
		  return;
		}
	    }

	  if (action != terminate_overlapping_read || ! exact_match)
	    {
	      struct du_chain *next = this->next_chain;

	      /* Whether the terminated chain can be used for renaming
	         depends on the action and this being an exact match.
	         In either case, we remove this element from open_chains.  */

	      if ((action == terminate_dead || action == terminate_write)
		  && exact_match)
		{
		  this->next_chain = closed_chains;
		  closed_chains = this;
		  if (rtl_dump_file)
		    fprintf (rtl_dump_file,
			     "Closing chain %s at insn %d (%s)¥n",
			     reg_names[REGNO (*this->loc)], INSN_UID (insn),
			     scan_actions_name[(int) action]);
		}
	      else
		{
		  if (rtl_dump_file)
		    fprintf (rtl_dump_file,
			     "Discarding chain %s at insn %d (%s)¥n",
			     reg_names[REGNO (*this->loc)], INSN_UID (insn),
			     scan_actions_name[(int) action]);
		}
	      *p = next;
	    }
	  else
	    p = &this->next_chain;
	}
    }
}

/* Adapted from find_reloads_address_1.  CLASS is INDEX_REG_CLASS or
   BASE_REG_CLASS depending on how the register is being considered.  */

static void
scan_rtx_address (insn, loc, class, action, mode)
     rtx insn;
     rtx *loc;
     enum reg_class class;
     enum scan_actions action;
     enum machine_mode mode;
{
  rtx x = *loc;
  RTX_CODE code = GET_CODE (x);
  const char *fmt;
  int i, j;

  if (action == mark_write)
    return;

  switch (code)
    {
    case PLUS:
      {
	rtx orig_op0 = XEXP (x, 0);
	rtx orig_op1 = XEXP (x, 1);
	RTX_CODE code0 = GET_CODE (orig_op0);
	RTX_CODE code1 = GET_CODE (orig_op1);
	rtx op0 = orig_op0;
	rtx op1 = orig_op1;
	rtx *locI = NULL;
	rtx *locB = NULL;

	if (GET_CODE (op0) == SUBREG)
	  {
	    op0 = SUBREG_REG (op0);
	    code0 = GET_CODE (op0);
	  }

	if (GET_CODE (op1) == SUBREG)
	  {
	    op1 = SUBREG_REG (op1);
	    code1 = GET_CODE (op1);
	  }

	if (code0 == MULT || code0 == SIGN_EXTEND || code0 == TRUNCATE
	    || code0 == ZERO_EXTEND || code1 == MEM)
	  {
	    locI = &XEXP (x, 0);
	    locB = &XEXP (x, 1);
	  }
	else if (code1 == MULT || code1 == SIGN_EXTEND || code1 == TRUNCATE
		 || code1 == ZERO_EXTEND || code0 == MEM)
	  {
	    locI = &XEXP (x, 1);
	    locB = &XEXP (x, 0);
	  }
	else if (code0 == CONST_INT || code0 == CONST
		 || code0 == SYMBOL_REF || code0 == LABEL_REF)
	  locB = &XEXP (x, 1);
	else if (code1 == CONST_INT || code1 == CONST
		 || code1 == SYMBOL_REF || code1 == LABEL_REF)
	  locB = &XEXP (x, 0);
	else if (code0 == REG && code1 == REG)
	  {
	    int index_op;

	    if (REG_OK_FOR_INDEX_P (op0)
		&& REG_MODE_OK_FOR_BASE_P (op1, mode))
	      index_op = 0;
	    else if (REG_OK_FOR_INDEX_P (op1)
		     && REG_MO