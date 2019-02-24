/* Move registers around to reduce number of move instructions needed.
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


/* This module looks for cases where matching constraints would force
   an instruction to need a reload, and this reload would be a register
   to register move.  It then attempts to change the registers used by the
   instruction to avoid the move instruction.  */

#include "config.h"
#include "system.h"
#include "rtl.h" /* stdio.h must precede rtl.h for FFS.  */
#include "tm_p.h"
#include "insn-config.h"
#include "recog.h"
#include "output.h"
#include "regs.h"
#include "hard-reg-set.h"
#include "flags.h"
#include "function.h"
#include "expr.h"
#include "basic-block.h"
#include "except.h"
#include "toplev.h"
#include "reload.h"


/* Turn STACK_GROWS_DOWNWARD into a boolean.  */
#ifdef STACK_GROWS_DOWNWARD
#undef STACK_GROWS_DOWNWARD
#define STACK_GROWS_DOWNWARD 1
#else
#define STACK_GROWS_DOWNWARD 0
#endif

static int perhaps_ends_bb_p	PARAMS ((rtx));
static int optimize_reg_copy_1	PARAMS ((rtx, rtx, rtx));
static void optimize_reg_copy_2	PARAMS ((rtx, rtx, rtx));
static void optimize_reg_copy_3	PARAMS ((rtx, rtx, rtx));
static void copy_src_to_dest	PARAMS ((rtx, rtx, rtx, int));
static int *regmove_bb_head;

struct match {
  int with[MAX_RECOG_OPERANDS];
  enum { READ, WRITE, READWRITE } use[MAX_RECOG_OPERANDS];
  int commutative[MAX_RECOG_OPERANDS];
  int early_clobber[MAX_RECOG_OPERANDS];
};

static rtx discover_flags_reg PARAMS ((void));
static void mark_flags_life_zones PARAMS ((rtx));
static void flags_set_1 PARAMS ((rtx, rtx, void *));

static int try_auto_increment PARAMS ((rtx, rtx, rtx, rtx, HOST_WIDE_INT, int));
static int find_matches PARAMS ((rtx, struct match *));
static void replace_in_call_usage PARAMS ((rtx *, unsigned int, rtx, rtx));
static int fixup_match_1 PARAMS ((rtx, rtx, rtx, rtx, rtx, int, int, int, FILE *))
;
static int reg_is_remote_constant_p PARAMS ((rtx, rtx, rtx));
static int stable_and_no_regs_but_for_p PARAMS ((rtx, rtx, rtx));
static int regclass_compatible_p PARAMS ((int, int));
static int replacement_quality PARAMS ((rtx));
static int fixup_match_2 PARAMS ((rtx, rtx, rtx, rtx, FILE *));

/* Return non-zero if registers with CLASS1 and CLASS2 can be merged without
   causing too much register allocation problems.  */
static int
regclass_compatible_p (class0, class1)
     int class0, class1;
{
  return (class0 == class1
	  || (reg_class_subset_p (class0, class1)
	      && ! CLASS_LIKELY_SPILLED_P (class0))
	  || (reg_class_subset_p (class1, class0)
	      && ! CLASS_LIKELY_SPILLED_P (class1)));
}

/* INC_INSN is an instruction that adds INCREMENT to REG.
   Try to fold INC_INSN as a post/pre in/decrement into INSN.
   Iff INC_INSN_SET is nonzero, inc_insn has a destination different from src.
   Return nonzero for success.  */
static int
try_auto_increment (insn, inc_insn, inc_insn_set, reg, increment, pre)
     rtx reg, insn, inc_insn ,inc_insn_set;
     HOST_WIDE_INT increment;
     int pre;
{
  enum rtx_code inc_code;

  rtx pset = single_set (insn);
  if (pset)
    {
      /* Can't use the size of SET_SRC, we might have something like
	 (sign_extend:SI (mem:QI ...  */
      rtx use = find_use_as_address (pset, reg, 0);
      if (use != 0 && use != (rtx) (size_t) 1)
	{
	  int size = GET_MODE_SIZE (GET_MODE (use));
	  if (0
	      || (HAVE_POST_INCREMENT
		  && pre == 0 && (inc_code = POST_INC, increment == size))
	      || (HAVE_PRE_INCREMENT
		  && pre == 1 && (inc_code = PRE_INC, increment == size))
	      || (HAVE_POST_DECREMENT
		  && pre == 0 && (inc_code = POST_DEC, increment == -size))
	      || (HAVE_PRE_DECREMENT
		  && pre == 1 && (inc_code = PRE_DEC, increment == -size))
	  )
	    {
	      if (inc_insn_set)
		validate_change
		  (inc_insn,
		   &SET_SRC (inc_insn_set),
		   XEXP (SET_SRC (inc_insn_set), 0), 1);
	      validate_change (insn, &XEXP (use, 0),
			       gen_rtx_fmt_e (inc_code, Pmode, reg), 1);
	      if (apply_change_group ())
		{
		  /* If there is a REG_DEAD note on this insn, we must
		     change this not to REG_UNUSED meaning that the register
		     is set, but the value is dead.  Failure to do so will
		     result in a sched1 abort -- when it recomputes lifetime
		     information, the number of REG_DEAD notes will have
		     changed.  */
		  rtx note = find_reg_note (insn, REG_DEAD, reg);
		  if (note)
		    PUT_MODE (note, REG_UNUSED);

		  REG_NOTES (insn)
		    = gen_rtx_EXPR_LIST (REG_INC,
					 reg, REG_NOTES (insn));
		  if (! inc_insn_set)
		    delete_insn (inc_insn);
		  return 1;
		}
	    }
	}
    }
  return 0;
}

/* Determine if the pattern generated by add_optab has a clobber,
   such as might be issued for a flags hard register.  To make the
   code elsewhere simpler, we handle cc0 in this same framework.

   Return the register if one was discovered.  Return NULL_RTX if
   if no flags were found.  Return pc_rtx if we got confused.  */

static rtx
discover_flags_reg ()
{
  rtx tmp;
  tmp = gen_rtx_REG (word_mode, 10000);
  tmp = gen_add3_insn (tmp, tmp, GEN_INT (2));

  /* If we get something that isn't a simple set, or a
     [(set ..) (clobber ..)], this whole function will go wrong.  */
  if (GET_CODE (tmp) == SET)
    return NULL_RTX;
  else if (GET_CODE (tmp) == PARALLEL)
    {
      int found;

      if (XVECLEN (tmp, 0) != 2)
	return pc_rtx;
      tmp = XVECEXP (tmp, 0, 1);
      if (GET_CODE (tmp) != CLOBBER)
	return pc_rtx;
      tmp = XEXP (tmp, 0);

      /* Don't do anything foolish if the md wanted to clobber a
	 scratch or something.  We only care about hard regs.
	 Moreover we don't like the notion of subregs of hard regs.  */
      if (GET_CODE (tmp) == SUBREG
	  && GET_CODE (SUBREG_REG (tmp)) == REG
	  && REGNO (SUBREG_REG (tmp)) < FIRST_PSEUDO_REGISTER)
	return pc_rtx;
      found = (GET_CODE (tmp) == REG && REGNO (tmp) < FIRST_PSEUDO_REGISTER);

      return (found ? tmp : NULL_RTX);
    }

  return pc_rtx;
}

/* It is a tedious task identifying when the flags register is live and
   when it is safe to optimize.  Since we process the instruction stream
   multiple times, locate and record these live zones by marking the
   mode of the instructions --

   QImode is used on the instruction at which the flags becomes live.

   HImode is used within the range (exclusive) that the flags are
   live.  Thus the user of the flags is not marked.

   All other instructions are cleared to VOIDmode.  */

/* Used to communicate with flags_set_1.  */
static rtx flags_set_1_rtx;
static int flags_set_1_set;

static void
mark_flags_life_zones (flags)
     rtx flags;
{
  int flags_regno;
  int flags_nregs;
  int block;

#ifdef HAVE_cc0
  /* If we found a flags register on a cc0 host, bail.  */
  if (flags == NULL_RTX)
    flags = cc0_rtx;
  else if (flags != cc0_rtx)
    flags = pc_rtx;
#endif

  /* Simple cases first: if no flags, clear all modes.  If confusing,
     mark the entire function as being in a flags shadow.  */
  if (flags == NULL_RTX || flags == pc_rtx)
    {
      enum machine_mode mode = (flags ? HImode : VOIDmode);
      rtx insn;
      for (insn = get_insns (); insn; insn = NEXT_INSN (insn))
	PUT_MODE (insn, mode);
      return;
    }

#ifdef HAVE_cc0
  flags_regno = -1;
  flags_nregs = 1;
#else
  flags_regno = REGNO (flags);
  flags_nregs = HARD_REGNO_NREGS (flags_regno, GET_MODE (flags));
#endif
  flags_set_1_rtx = flags;

  /* Process each basic block.  */
  for (block = n_basic_blocks - 1; block >= 0; block--)
    {
      rtx insn, end;
      int live;

      insn = BLOCK_HEAD (block);
      end = BLOCK_END (block);

      /* Look out for the (unlikely) case of flags being live across
	 basic block boundaries.  */
      live = 0;
#ifndef HAVE_cc0
      {
	int i;
	for (i = 0; i < flags_nregs; ++i)
          live |= REGNO_REG_SET_P (BASIC_BLOCK (block)->global_live_at_start,
				   flags_regno + i);
      }
#endif

      while (1)
	{
	  /* Process liveness in reverse order of importance --
	     alive, death, birth.  This lets more important info
	     overwrite the mode of lesser info.  */

	  if (INSN_P (insn))
	    {
#ifdef HAVE_cc0
	      /* In the cc0 case, death is not marked in reg notes,
		 but is instead the mere use of cc0 when it is alive.  */
	      if (live && reg_mentioned_p (cc0_rtx, PATTERN (insn)))
		live = 0;
#else
	      /* In the hard reg case, we watch death notes.  */
	      if (live && find_regno_note (insn, REG_DEAD, flags_regno))
		live = 0;
#endif
	      PUT_MODE (insn, (live ? HImode : VOIDmode));

	      /* In either case, birth is denoted simply by it's presence
		 as the destination of a set.  */
	      flags_set_1_set = 0;
	      note_stores (PATTERN (insn), flags_set_1, NULL);
	      if (flags_set_1_set)
		{
		  live = 1;
		  PUT_MODE (insn, QImode);
		}
	    }
	  else
	    PUT_MODE (insn, (live ? HImode : VOIDmode));

	  if (insn == end)
	    break;
	  insn = NEXT_INSN (insn);
	}
    }
}

/* A subroutine of mark_flags_life_zones, called through note_stores.  */

static void
flags_set_1 (x, pat, data)
     rtx x, pat;
     void *data ATTRIBUTE_UNUSED;
{
  if (GET_CODE (pat) == SET
      && reg_overlap_mentioned_p (x, flags_set_1_rtx))
    flags_set_1_set = 1;
}

static int *regno_src_regno;

/* Indicate how good a choice REG (which appears as a source) is to replace
   a destination register with.  The higher the returned value, the better
   the choice.  The main objective is to avoid using a register that is
   a candidate for tying to a hard register, since the output might in
   turn be a candidate to be tied to a different hard register.  */
static int
replacement_quality (reg)
     rtx reg;
{
  int src_regno;

  /* Bad if this isn't a register at all.  */
  if (GET_CODE (reg) != REG)
    return 0;

  /* If this register is not meant to get a hard register,
     it is a poor choice.  */
  if (REG_LIVE_LENGTH (REGNO (reg)) < 0)
    return 0;

  src_regno = regno_src_regno[REGNO (reg)];

  /* If it was not copied from another register, it is fine.  */
  if (src_regno < 0)
    return 3;

  /* Copied from a hard register?  */
  if (src_regno < FIRST_PSEUDO_REGISTER)
    return 1;

  /* Copied from a pseudo register - not as bad as from a hard register,
     yet still cumbersome, since the register live length will be lengthened
     when the registers get tied.  */
  return 2;
}

/* Return 1 if INSN might end a basic block.  */

static int perhaps_ends_bb_p (insn)
     rtx insn;
{
  switch (GET_CODE (insn))
    {
    case CODE_LABEL:
    case JUMP_INSN:
      /* These always end a basic block.  */
      return 1;

    case CALL_INSN:
      /* A CALL_INSN might be the last insn of a basic block, if it is inside
	 an EH region or if there are nonlocal gotos.  Note that this test is
	 very conservative.  */
      if (nonlocal_goto_handler_labels)
	return 1;
      /* FALLTHRU */
    default:
      return can_throw_internal (insn);
    }
}

/* INSN is a copy from SRC to DEST, both registers, and SRC does not die
   in INSN.

   Search forward to see if SRC dies before either it or DEST is modified,
   but don't scan past the end of a basic block.  If so, we can replace SRC
   with DEST and let SRC die in INSN.

   This will reduce the number of registers live in that range and may enable
   DEST to be tied to SRC, thus often saving one register in addition to a
   register-register copy.  */

static int
optimize_reg_copy_1 (insn, dest, src)
     rtx insn;
     rtx dest;
     rtx src;
{
  rtx p, q;
  rtx note;
  rtx dest_death = 0;
  int sregno = REGNO (src);
  int dregno = REGNO (dest);

  /* We don't want to mess with hard regs if register classes are small.  */
  if (sregno == dregno
      || (SMALL_REGISTER_CLASSES
	  && (sregno < FIRST_PSEUDO_REGISTER
	      || dregno < FIRST_PSEUDO_REGISTER))
      /* We don't see all updates to SP if they are in an auto-inc memory
	 reference, so we must disallow this optimization on them.  */
      || sregno == STACK_POINTER_REGNUM || dregno == STACK_POINTER_REGNUM)
    return 0;

  for (p = NEXT_INSN (insn); p; p = NEXT_INSN (p))
    {
      /* ??? We can't scan past the end of a basic block without updating
	 the register lifetime info (REG_DEAD/basic_block_live_at_start).  */
      if (perhaps_ends_bb_p (p))
	break;
      else if (! INSN_P (p))
	continue;

      if (reg_set_p (src, p) || reg_set_p (dest, p)
	  /* Don't change a USE of a register.  */
	  || (GET_CODE (PATTERN (p)) == USE
	      && reg_overlap_mentioned_p (src, XEXP (PATTERN (p), 0))))
	break;

      /* See if all of SRC dies in P.  This test is slightly more
	 conservative than it needs to be.  */
      if ((note = find_regno_note (p, REG_DEAD, sregno)) != 0
	  && GET_MODE (XEXP (note, 0)) == GET_MODE (src))
	{
	  int failed = 0;
	  int d_length = 0;
	  int s_length = 0;
	  int d_n_calls = 0;
	  int s_n_calls = 0;

	  /* We can do the optimization.  Scan forward from INSN again,
	     replacing regs as we go.  Set FAILED if a replacement can't
	     be done.  In that case, we can't move the death note for SRC.
	     This should be rare.  */

	  /* Set to stop at next insn.  */
	  for (q = next_real_insn (insn);
	       q != next_real_insn (p);
	       q = next_real_insn (q))
	    {
	      if (reg_overlap_mentioned_p (src, PATTERN (q)))
		{
		  /* If SRC is a hard register, we might miss some
		     overlapping registers with validate_replace_rtx,
		     so we would have to undo it.  We can't if DEST is
		     present in the insn, so fail in that combination
		     of cases.  */
		  if (sregno < FIRST_PSEUDO_REGISTER
		      && reg_mentioned_p (dest, PATTERN (q)))
		    failed = 1;

		  /* Replace all uses and make sure that the register
		     isn't still present.  */
		  else if (validate_replace_rtx (src, dest, q)
			   && (sregno >= FIRST_PSEUDO_REGISTER
			       || ! reg_overlap_mentioned_p (src,
							     PATTERN (q))))
		    ;
		  else
		    {
		      validate_replace_rtx (dest, src, q);
		      failed = 1;
		    }
		}

	      /* For SREGNO, count the total number of insns scanned.
		 For DREGNO, count the total number of insns scanned after
		 passing the death note for DREGNO.  */
	      s_length++;
	      if (dest_death)
		d_length++;

	      /* If the insn in which SRC dies is a CALL_INSN, don't count it
		 as a call that has been crossed.  Otherwise, count it.  */
	      if (q != p && GET_CODE (q) == CALL_INSN)
		{
		  /* Similarly, total calls for SREGNO, total calls beyond
		     the death note for DREGNO.  */
		  s_n_calls++;
		  if (dest_death)
		    d_n_calls++;
		}

	      /* If DEST dies here, remove the death note and save it for
		 later.  Make sure ALL of DEST dies here; again, this is
		 overly conservative.  */
	      if (dest_death == 0
		  && (dest_death = find_regno_note (q, REG_DEAD, dregno)) != 0)
		{
		  if (GET_MODE (XEXP (dest_death, 0)) != GET_MODE (dest))
		    failed = 1, dest_death = 0;
		  else
		    remove_note (q, dest_death);
		}
	    }

	  if (! failed)
	    {
	      /* These counters need to be updated if and only if we are
		 going to move the REG_DEAD note.  */
	      if (sregno >= FIRST_PSEUDO_REGISTER)
		{
		  if (REG_LIVE_LENGTH (sregno) >= 0)
		    {
		      REG_LIVE_LENGTH (sregno) -= s_length;
		      /* REG_LIVE_LENGTH is only an approximation after
			 combine if sched is not run, so make sure that we
			 still have a reasonable value.  */
		      if (REG_LIVE_LENGTH (sregno) < 2)
			REG_LIVE_LENGTH (sregno) = 2;
		    }

		  REG_N_CALLS_CROSSED (sregno) -= s_n_calls;
		}

	      /* Move death note of SRC from P to INSN.  */
	      remove_note (p, note);
	      XEXP (note, 1) = REG_NOTES (insn);
	      REG_NOTES (insn) = note;
	    }

	  /* DEST is also dead if INSN has a REG_UNUSED note for DEST.  */
	  if (! dest_death
	      && (dest_death = find_regno_note (insn, REG_UNUSED, dregno)))
	    {
	      PUT_REG_NOTE_KIND (dest_death, REG_DEAD);
	      remove_note (insn, dest_death);
	    }

	  /* Put death note of DEST on P if we saw it die.  */
	  if (dest_death)
	    {
	      XEXP (dest_death, 1) = REG_NOTES (p);
	      REG_NOTES (p) = dest_death;

	      if (dregno >= FIRST_PSEUDO_REGISTER)
		{
		  /* If and only if we are moving the death note for DREGNO,
		     then we need to update its counters.  */
		  if (REG_LIVE_LENGTH (dregno) >= 0)
		    REG_LIVE_LENGTH (dregno) += d_length;
		  REG_N_CALLS_CROSSED (dregno) += d_n_calls;
		}
	    }

	  return ! failed;
	}

      /* If SRC is a hard register which is set or killed in some other
	 way, we can't do this optimization.  */
      else if (sregno < FIRST_PSEUDO_REGISTER
	       && dead_or_set_p (p, src))
	break;
    }
  return 0;
}

/* INSN is a copy of SRC to DEST, in which SRC dies.  See if we now have
   a sequence of insns that modify DEST followed by an insn that sets
   SRC to DEST in which DEST dies, with no prior modification of DEST.
   (There is no need to check if the insns in between actually modify
   DEST.  We should not have cases where DEST is not modified, but
   the optimization is safe if no such modification is detected.)
   In that case, we can replace all uses of DEST, starting with INSN and
   ending with the set of SRC to DEST, with SRC.  We do not do this
   optimization if a CALL_INSN is crossed unless SRC already crosses a
   call or if DEST dies before the copy back to SRC.

   It is assumed that DEST and SRC are pseudos; it is too complicated to do
   this for hard registers since the substitutions we may make might fail.  */

static void
optimize_reg_copy_2 (insn, dest, src)
     rtx insn;
     rtx dest;
     rtx src;
{
  rtx p, q;
  rtx set;
  int sregno = REGNO (src);
  int dregno = REGNO (dest);

  for (p = NEXT_INSN (insn); p; p = NEXT_INSN (p))
    {
      /* ??? We can't scan past the end of a basic block without updating
	 the register lifetime info (REG_DEAD/basic_block_live_at_start).  */
      if (perhaps_ends_bb_p (p))
	break;
      else if (! INSN_P (p))
	continue;

      set = single_set (p);
      if (set && SET_SRC (set) == dest && SET_DEST (set) == src
	  && find_reg_note (p, REG_DEAD, dest))
	{
	  /* We can do the optimization.  Scan forward from INSN again,
	     replacing regs as we go.  */

	  /* Set to stop at next insn.  */
	  for (q = insn; q != NEXT_INSN (p); q = NEXT_INSN (q))
	    if (INSN_P (q))
	      {
		if (reg_mentioned_p (dest, PATTERN (q)))
		  PATTERN (q) = replace_rtx (PATTERN (q), dest, src);


	      if (GET_CODE (q) == CALL_INSN)
		{
		  REG_N_CALLS_CROSSED (dregno)--;
		  REG_N_CALLS_CROSSED (sregno)++;
		}
	      }

	  remove_note (p, find_reg_note (p, REG_DEAD, dest));
	  REG_N_DEATHS (dregno)--;
	  remove_note (insn, find_reg_note (insn, REG_DEAD, src));
	  REG_N_DEATHS (sregno)--;
	  return;
	}

      if (reg_set_p (src, p)
	  || find_reg_note (p, REG_DEAD, dest)
	  || (GET_CODE (p) == CALL_INSN && REG_N_CALLS_CROSSED (sregno) == 0))
	break;
    }
}
/* INSN is a ZERO_EXTEND or SIGN_EXTEND of SRC to DEST.
   Look if SRC dies there, and if it is only set once, by loading
   it from memory.  If so, try to encorporate the zero/sign extension
   into the me