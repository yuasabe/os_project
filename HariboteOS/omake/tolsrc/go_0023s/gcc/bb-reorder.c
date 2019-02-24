/* Basic block reordering routines for the GNU compiler.
   Copyright (C) 2000, 2002 Free Software Foundation, Inc.

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

/* References:

   "Profile Guided Code Positioning"
   Pettis and Hanson; PLDI '90.

   TODO:

   (1) Consider:

		if (p) goto A;		// predict taken
		foo ();
	      A:
		if (q) goto B;		// predict taken
		bar ();
	      B:
		baz ();
		return;

       We'll currently reorder this as

		if (!p) goto C;
	      A:
		if (!q) goto D;
	      B:
		baz ();
		return;
	      D:
		bar ();
		goto B;
	      C:
		foo ();
		goto A;

       A better ordering is

		if (!p) goto C;
		if (!q) goto D;
	      B:
		baz ();
		return;
	      C:
		foo ();
		if (q) goto B;
	      D:
		bar ();
		goto B;

       This requires that we be able to duplicate the jump at A, and
       adjust the graph traversal such that greedy placement doesn't
       fix D before C is considered.

   (2) Coordinate with shorten_branches to minimize the number of
       long branches.

   (3) Invent a method by which sufficiently non-predicted code can
       be moved to either the end of the section or another section
       entirely.  Some sort of NOTE_INSN note would work fine.

       This completely scroggs all debugging formats, so the user
       would have to explicitly ask for it.
*/

#include "config.h"
#include "system.h"
#include "tree.h"
#include "rtl.h"
#include "hard-reg-set.h"
#include "basic-block.h"
#include "flags.h"
#include "output.h"
#include "cfglayout.h"
#include "target.h"

/* Local function prototypes.  */
static void make_reorder_chain		PARAMS ((void));
static basic_block make_reorder_chain_1	PARAMS ((basic_block, basic_block));

/* Compute an ordering for a subgraph beginning with block BB.  Record the
   ordering in RBI()->index and chained through RBI()->next.  */

static void
make_reorder_chain ()
{
  basic_block prev = NULL;
  int nbb_m1 = n_basic_blocks - 1;
  basic_block next;

  /* Loop until we've placed every block.  */
  do
    {
      int i;

      next = NULL;

      /* Find the next unplaced block.  */
      /* ??? Get rid of this loop, and track which blocks are not yet
	 placed more directly, so as to avoid the O(N^2) worst case.
	 Perhaps keep a doubly-linked list of all to-be-placed blocks;
	 remove from the list as we place.  The head of that list is
	 what we're looking for here.  */

      for (i = 0; i <= nbb_m1 && !next; ++i)
	{
	  basic_block bb = BASIC_BLOCK (i);
	  if (! RBI (bb)->visited)
	    next = bb;
	}
      if (next)
        prev = make_reorder_chain_1 (next, prev);
    }
  while (next);
  RBI (prev)->next = NULL;
}

/* A helper function for make_reorder_chain.

   We do not follow EH edges, or non-fallthru edges to noreturn blocks.
   These are assumed to be the error condition and we wish to cluster
   all of them at the very end of the function for the benefit of cache
   locality for the rest of the function.

   ??? We could do slightly better by noticing earlier that some subgraph
   has all paths leading to noreturn functions, but for there to be more
   than one block in such a subgraph is rare.  */

static basic_block
make_reorder_chain_1 (bb, prev)
     basic_block bb;
     basic_block prev;
{
  edge e;
  basic_block next;
  rt