/* Instruction scheduling pass.
   Copyright (C) 1992, 1993, 1994, 1995, 1996, 1997, 1998,
   1999, 2000, 2001 Free Software Foundation, Inc.
   Contributed by Michael Tiemann (tiemann@cygnus.com) Enhanced by,
   and currently maintained by, Jim Wilson (wilson@cygnus.com)

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
#include "regs.h"
#include "function.h"
#include "flags.h"
#include "insn-config.h"
#include "insn-attr.h"
#include "except.h"
#include "toplev.h"
#include "recog.h"
#include "cfglayout.h"
#include "sched-int.h"

/* The number of insns to be scheduled in total.  */
static int target_n_insns;
/* The number of insns scheduled so far.  */
static int sched_n_insns;

/* Implementations of the sched_info functions for region scheduling.  */
static void init_ready_list PARAMS ((struct ready_list *));
static int can_schedule_ready_p PARAMS ((rtx));
static int new_ready PARAMS ((rtx));
static int schedule_more_p PARAMS ((void));
static const char *print_insn PARAMS ((rtx, int));
static int rank PARAMS ((rtx, rtx));
static int contributes_to_priority PARAMS ((rtx, rtx));
static void compute_jump_reg_dependencies PARAMS ((rtx, regset));
static void schedule_ebb PARAMS ((rtx, rtx));

/* Return nonzero if there are more insns that should be scheduled.  */

static int
schedule_more_p ()
{
  return sched_n_insns < target_n_insns;
}

/* Add all insns that are initially ready to the ready list READY.  Called
   once before scheduling a set of insns.  */

static void
init_ready_list (ready)
     struct ready_list *ready;
{
  rtx prev_head = current_sched_info->prev_head;
  rtx next_tail = current_sched_info->next_tail;
  rtx insn;

  target_n_insns = 0;
  sched_n_insns = 0;

#if 0
  /* Print debugging information.  */
  if (sched_verbose >= 5)
    debug_dependencies ();
#endif

  /* Initialize ready list with all 'ready' insns in target block.
     Count number of insns in the target block being scheduled.  */
  for (insn = NEXT_INSN (prev_head); insn != next_tail; insn = NEXT_INSN (insn))
    {
      rtx next;

      if (! INSN_P (insn))
	continue;
      next = NEXT_INSN (insn);

      if (INSN_DEP_COUNT (insn) == 0
	  && (SCHED_GROUP_P (next) == 0 || ! INSN_P (next)))
	ready_add (ready, insn);
      if (!(SCHED_GROUP_P (insn)))
	target_n_insns++;
    }
}

/* Called after taking INSN from the ready list.  Returns nonzero if this
   insn can be scheduled, nonzero if we should silently discard it.  */

static int
can_schedule_ready_p (insn)
     rtx insn ATTRIBUTE_UNUSED;
{
  sched_n_insns++;
  return 1;
}

/* Called after INSN has all its dependencies resolved.  Return nonzero
   if it should be moved to the ready list or the queue, or zero if we
   should silently discard it.  */
static int
new_ready (next)
     rtx next ATTRIBUTE_UNUSED;
{
  return 1;
}

/* Return a string that contains the insn uid and optionally anything else
   necessary to identify this insn in an output.  It's valid to use a
   static buffer for this.  The ALIGNED parameter should cause the string
   to be formatted so that multiple output lines will line up nicely.  */

static const char *
print_insn (insn, aligned)
     rtx insn;
     int aligned ATTR