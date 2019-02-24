/* Optimize by combining instructions for GNU compiler.
   Copyright (C) 1987, 1988, 1992, 1993, 1994, 1995, 1996, 1997, 1998,
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

/* This module is essentially the "combiner" phase of the U. of Arizona
   Portable Optimizer, but redone to work on our list-structured
   representation for RTL instead of their string representation.

   The LOG_LINKS of each insn identify the most recent assignment
   to each REG used in the insn.  It is a list of previous insns,
   each of which contains a SET for a REG that is used in this insn
   and not used or set in between.  LOG_LINKs never cross basic blocks.
   They were set up by the preceding pass (lifetime analysis).

   We try to combine each pair of insns joined by a logical link.
   We also try to combine triples of insns A, B and C when
   C has a link back to B and B has a link back to A.

   LOG_LINKS does not have links for use of the CC0.  They don't
   need to, because the insn that sets the CC0 is always immediately
   before the insn that tests it.  So we always regard a branch
   insn as having a logical link to the preceding insn.  The same is true
   for an insn explicitly using CC0.

   We check (with use_crosses_set_p) to avoid combining in such a way
   as to move a computation to a place where its value would be different.

   Combination is done by mathematically substituting the previous
   insn(s) values for the regs they set into the expressions in
   the later insns that refer to these regs.  If the result is a valid insn
   for our target machine, according to the machine description,
   we install it, delete the earlier insns, and update the data flow
   information (LOG_LINKS and REG_NOTES) for what we did.

   There are a few exceptions where the dataflow information created by
   flow.c aren't completely updated:

   - reg_live_length is not updated
   - reg_n_refs is not adjusted in the rare case when a register is
     no longer required in a computation
   - there are extremely rare cases (see distribute_regnotes) when a
     REG_DEAD note is lost
   - a LOG_LINKS entry that refers to an insn with multiple SETs may be
     removed because there is no way to know which register it was
     linking

   To simplify substitution, we combine only when the earlier insn(s)
   consist of only a single assignment.  To simplify updating afterward,
   we never combine when a subroutine call appears in the middle.

   Since we do not represent assignments to CC0 explicitly except when that
   is all an insn does, there is no LOG_LINKS entry in an insn that uses
   the condition code for the insn that set the condition code.
   Fortunately, these two insns must be consecutive.
   Therefore, every JUMP_INSN is taken to have an implicit logical link
   to the preceding insn.  This is not quite right, since non-jumps can
   also use the condition code; but in practice such insns would not
   combine anyway.  */

#include "config.h"
#include "system.h"
#include "rtl.h"
#include "tm_p.h"
#include "flags.h"
#include "regs.h"
#include "hard-reg-set.h"
#include "basic-block.h"
#include "insn-config.h"
#include "function.h"
/* Include expr.h after insn-config.h so we get HAVE_conditional_move.  */
#include "expr.h"
#include "insn-attr.h"
#include "recog.h"