/* Definitions for code generation pass of GNU compiler.
   Copyright (C) 1987, 1991, 1992, 1993, 1994, 1995, 1996, 1997, 1998,
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

/* The default branch cost is 1.  */
#ifndef BRANCH_COST
#define BRANCH_COST 1
#endif

/* Macros to access the slots of a QUEUED rtx.
   Here rather than in rtl.h because only the expansion pass
   should ever encounter a QUEUED.  */

/* The variable for which an increment is queued.  */
#define QUEUED_VAR(P) XEXP (P, 0)
/* If the increment has been emitted, this is the insn
   that does the increment.  It is zero before the increment is emitted.
   If more than one insn is emitted, this is the first insn.  */
#define QUEUED_INSN(P) XEXP (P, 1)
/* If a pre-increment copy has been generated, this is the copy
   (it is a temporary reg).  Zero if no copy made yet.  */
#define QUEUED_COPY(P) XEXP (P, 2)
/* This is the body to use for the insn to do the increment.
   It is used to emit the increment.  */
#define QUEUED_BODY(P) XEXP (P, 3)
/* Next QUEUED in the queue.  */
#define QUEUED_NEXT(P) XEXP (P, 4)

/* This is the 4th arg to `expand_expr'.
   EXPAND_SUM means it is ok to return a PLUS rtx or MULT rtx.
   EXPAND_INITIALIZER is similar but also record any labels on forced_labels.
   EXPAND_CONST_ADDRESS means it is ok to return a MEM whose address
    is a constant that is not a legitimate address.
   EXPAND_WRITE means we are only going to write to the resulting rtx.  */
enum expand_modifier {EXPAND_NORMAL, EXPAND_SUM, EXPAND_CONST_ADDRESS,
			EXPAND_INITIALIZER, EXPAND_WRITE};

/* Prevent the compiler from deferring stack pops.  See
   inhibit_defer_pop for more information.  */
#define NO_DEFER_POP (inhibit_defer_pop += 1)

/* Allow the compiler to defer stack pops.  See inhibit_defer_pop for
   more information.  */
#define OK_DEFER_POP (inhibit_defer_pop -= 1)

#ifdef TREE_CODE /* Don't lose if tree.h not included.  */
/* Structure to record the size of a sequence of arguments
   as the sum of a tree-expression and a constant.  This structure is
   also used to store offsets from the stack, which might be negative,
   so the variable part must be ssizetype, not sizetype.  */

struct args_size
{
  HOST_WIDE_INT constant;
  tree var;
};
#endif

/* Add the value of the tree INC to the `struct args_size' TO.  */

#define ADD_PARM_SIZE(TO, INC)				¥
do {							¥
  tree inc = (INC);					¥
  if (host_integerp (inc, 0))				¥
    (TO).constant += tree_low_cst (inc, 0);		¥
  else if ((TO).var == 0)				¥
    (TO).var = convert (ssizetype, inc);		¥
  else							¥
    (TO).var = size_binop (PLUS_EXPR, (TO).var,		¥
			   convert (ssizetype, inc));	¥
} while (0)

#define SUB_PARM_SIZE(TO, DEC)				¥
do {							¥
  tree dec = (DEC);					¥
  if (host_integerp (dec, 0))				¥
    (TO).constant -= tree_low_cst (dec, 0);		¥
  else if ((TO).var == 0)				¥
    (TO).var = size_binop (MINUS_EXPR, ssize_int (0),	¥
			   convert (ssizetype, dec));	¥
  else							¥
    (TO).var = size_binop (MINUS_EXPR, (TO).var,	¥
			   convert (ssizetype, dec));	¥
} while (0)

/* Convert the implicit sum in a `struct args_size' into a tree
   of type ssizetype.  */
#define ARGS_SIZE_TREE(SIZE)					¥
((SIZE).var == 0 ? ssize_int ((SIZE).constant)			¥
 : size_binop (PLUS_EXPR, convert (ssizetype, (SIZE).var),	¥
	  