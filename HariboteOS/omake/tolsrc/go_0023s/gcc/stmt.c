/* Expands front end tree to back end RTL for GNU C-Compiler
   Copyright (C) 1987, 1988, 1989, 1992, 1993, 1994, 1995, 1996, 1997,
   1998, 1999, 2000, 2001, 2002 Free Software Foundation, Inc.

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

/* This file handles the generation of rtl code from tree structure
   above the level of expressions, using subroutines in exp*.c and emit-rtl.c.
   It also creates the rtl expressions for parameters and auto variables
   and has full responsibility for allocating stack slots.

   The functions whose names start with `expand_' are called by the
   parser to generate RTL instructions for various kinds of constructs.

   Some control and binding constructs require calling several such
   functions at different times.  For example, a simple if-then
   is expanded by calling `expand_start_cond' (with the condition-expression
   as argument) before parsing the then-clause and calling `expand_end_cond'
   after parsing the then-clause.  */

#include "config.h"
#include "system.h"

/* !kawai! */
#include "rtl.h"
#include "tree.h"
#include "tm_p.h"
#include "flags.h"
#include "except.h"
#include "function.h"
#include "insn-config.h"
#include "expr.h"
#include "libfuncs.h"
#include "hard-reg-set.h"
#include "../include/obstack.h"
#include "loop.h"
#include "recog.h"
#include "machmode.h"
#include "toplev.h"
#include "output.h"
#include "ggc.h"
/* end of !kawai! */

#define obstack_chunk_alloc xmalloc
#define obstack_chunk_free free
struct obstack stmt_obstack;

/* Assume that case vectors are not pc-relative.  */
#ifndef CASE_VECTOR_PC_RELATIVE
#define CASE_VECTOR_PC_RELATIVE 0
#endif

/* Functions and data structures for expanding case statements.  */

/* Case label structure, used to hold info on labels within case
   statements.  We handle "range" labels; for a single-value label
   as in C, the high and low limits are the same.

   An AVL tree of case nodes is initially created, and later transformed
   to a list linked via the RIGHT fields in the nodes.  Nodes with
   higher case values are later in the list.

   Switch statements can be output in one of two forms.  A branch table
   is used if there are more than a few labels and the labels are dense
   within the range between the smallest and largest case value.  If a
   branch table is used, no further manipulations are done with the case
   node chain.

   The alternative to the use of a branch table is to generate a series
   of compare and jump insns.  When that is done, we use the LEFT, RIGHT,
   and PARENT fields to hold a binary tree.  Initially the tree is
   totally unbalanced, with everything on the right.  We balance the tree
   with nodes on the left having lower case values than the parent
   and nodes on the right having higher values.  We then output the tree
   in order.  */

struct case_node
{
  struct case_node	*left;	/* Left son in binary tree */
  struct case_node	*right;	/* Right son in binary tree; also node chain */
  struct case_node	*parent; /* Parent of node in binary tree */
  tree			low;	/* Lowest index value for this label */
  tree			high;	/* Highest index value for this label */
  tree			code_label; /* Label to jump to when node matches */
  int			balance;
};

typedef struct case_node case_node;
typedef struct case_node *case_node_ptr;

/* These are used by estimate_case_cos