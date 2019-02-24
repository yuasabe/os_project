/* Basic block reordering routines for the GNU compiler.
   Copyright (C) 2000, 2001 Free Software Foundation, Inc.

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

/* !kawai! */
#include "config.h"
#include "system.h"
#include "tree.h"
#include "rtl.h"
#include "hard-reg-set.h"
#include "basic-block.h"
#include "insn-config.h"
#include "output.h"
#include "function.h"
#include "../include/obstack.h"
#include "cfglayout.h"
/* end of !kawai! */

/* The contents of the current function definition are allocated
   in this obstack, and all are freed at the end of the function.  */
extern struct obstack flow_obstack;

/* Holds the interesting trailing notes for the function.  */
static rtx function_tail_eff_head;

static rtx skip_insns_after_block	PARAMS ((basic_block));
static void record_effective_endpoints	PARAMS ((void));
static rtx label_for_bb			PARAMS ((basic_block));
static void fixup_reorder_chain		PARAMS ((void));

static void set_block_levels		PARAMS ((tree, int));
static void change_scope		PARAMS ((rtx, tree, tree));

void verify_insn_chain			PARAMS ((void));
static void fixup_fallthru_exit_predecessor PARAMS ((void));

/* Map insn uid to lexical block.  */
static varray_type insn_scopes;

/* Skip over inter-block insns occurring after BB which are typically
   associated with BB (e.g., barriers). If there are any such insns,
   we return the last one. Otherwise, we return the end of BB.  */

static rtx
skip_insns_after_block (bb)
     basic_block bb;
{
  rtx insn, last_insn, next_head, prev;

  next_head = NULL_RTX;
  if (bb->index + 1 != n_basic_blocks)
    next_head = BASIC_BLOCK (bb->index + 1)->head;

  for (last_insn = insn = bb->end; (insn = NEXT_INSN (insn)) != 0; )
    {
      if (insn == next_head)
	break;

      switch (GET_CODE (insn))
	{
	case BARRIER:
	  last_insn = insn;
	  continue;

	case NOTE:
	  switch (NOTE_LINE_NUMBER (insn))
	    {
	    case NOTE_INSN_LOOP_END:
	    case NOTE_INSN_BLOCK_END:
	      last_insn = insn;
	      continue;
	    case NOTE_INSN_DELETED:
	    case NOTE_INSN_DELETED_LABEL:
	      continue;

	    default:
	      continue;
	      break;
	    }
	  break;

	case CODE_LABEL:
	  if (NEXT_INSN (insn)
	      && GET_CODE (NEXT_INSN (insn)) == JUMP_INSN
	      && (GET_CODE (PATTERN (NEXT_INSN (insn))) == ADDR_VEC
	          || GET_CODE (PATTERN (NEXT_INSN (insn))) == ADDR_DIFF_VEC))
	    {
	      insn = NEXT_INSN (insn);
	      last_insn = insn;
	      continue;
	    }
          break;

	default:
	  break;
	}

      break;
    }

  /* It is possible to hit contradictory sequence.  For instance:
    
     jump_insn
     NOTE_INSN_LOOP_BEG
     barrier

     Where barrier belongs to jump_insn, but the note does not.  This can be
     created by removing the basic block originally following
     NOTE_INSN_LOOP_BEG.  In such case reorder the notes.  */

  for (insn = last_insn; insn != bb->end; insn = prev)
    {
      prev = PREV_INSN (insn);
      if (GET_CODE (insn) == NOTE)
	switch (NOTE_LINE_NUMBER (insn))
	  {
          case NOTE_INSN_LOOP_END:
          case NOTE_INSN_BLOCK_END:
          case NOTE_INSN_DELETED:
          case NOTE_INSN_DELETED_LABEL:
	    continue;
          default:
	    reorder_insns (insn, insn, last_insn);
        }
    }

  return last_insn;
}

/* Locate or create a label for a given basi