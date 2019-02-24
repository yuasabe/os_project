/* Tree-dumping functionality for intermediate representation.
   Copyright (C) 1999, 2000, 2001, 2002 Free Software Foundation, Inc.
   Written by Mark Mitchell <mark@codesourcery.com>

This file is part of GNU CC.

GNU CC is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

GNU CC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU CC; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */

/* !kawai! */
#include "../gcc/config.h"
#include "../gcc/system.h"
#include "../gcc/tree.h"
#include "cp-tree.h"
#include "../gcc/tree-dump.h"
/* end of !kawai! */

static void dump_access
  PARAMS ((dump_info_p, tree));

static void dump_op
  PARAMS ((dump_info_p, tree));

/* Dump a representation of the accessibility information associated
   with T.  */

static void
dump_access (di, t)
     dump_info_p di;
     tree t;
{
  if (TREE_PROTECTED(t))
    dump_string (di, "protected");
  else if (TREE_PRIVATE(t))
    dump_string (di, "private");
  else
    dump_string (di, "public");
}

/* Dump a representation of the specific operator for an overloaded
   operator associated with node t.
*/

static void
dump_op (di, t)
     dump_info_p di;
     tree t;
{
  switch (DECL_OVERLOADED_OPERATOR_P (t)) {
    case NEW_EXPR:
      dump_string (di, "new");
      break;
    case VEC_NEW_EXPR:
      dump_string (di, "vecnew");
      break;
    case DELETE_EXPR:
      dump_string (di, "delete");
      break;
    case VEC_DELETE_EXPR:
      dump_string (di, "vecdelete");
      break;
    case CONVERT_EXPR:
      dump_string (di, "pos");
      break;
    case NEGATE_EXPR:
      dump_string (di, "neg");
      break;
    case ADDR_EXPR:
      dump_string (di, "addr");
      break;
    case INDIRECT_REF:
      dump_string(di, "deref");
      break;
    case BIT_NOT_EXPR:
      dump_string(di, "not");
      break;
    case TRUTH_NOT_EXPR:
      dump_string(di, "lnot");
      break;
    case PREINCREMENT_EXPR:
      dump_string(di, "preinc");
      break;
    case PREDECREMENT_EXPR:
      dump_string(di, "predec");
      break;
    case PLUS_EXPR:
      if (DECL_ASSIGNMENT_OPERATOR_P (t))
        dump_string (di, "plusassign");
      else
        dump_string(di, "plus");
      break;
    case MINUS_EXPR:
      if (DECL_ASSIGNMENT_OPERATOR_P (t))
        dump_string (di, "minusassign");
      else
        dump_string(di, "minus");
      break;
    case MULT_EXPR:
      if (DECL_ASSIGNMENT_OPERATOR_P (t))
        dump_string (di, "multassign");
      else
        dump_string (di, "mult");
      break;
    case TRUNC_DIV_EXPR:
      if (DECL_ASSIGNMENT_OPERATOR_P (t))
        dump_string (di, "divassign");
      else
        dump_string (di, "div");
      break;
    case TRUNC_MOD_EXPR:
      if (DECL_ASSIGNMENT_OPERATOR_P (t))
         dump_string (di, "modassign");
      else
        dump_string (di, "mod");
      break;
    case BIT_AND_EXPR:
      if (DECL_ASSIGNMENT_OPERATOR_P (t))
        dump_string (di, "andassign");
      else
        dump_string (di, "and");
      break;
    case BIT_IOR_EXPR:
      if (DECL_ASSIGNMENT_OPERATOR_P (t))
        dump_string (di, "orassign");
      else
        dump_string (di, "or");
      break;
    case BIT_XOR_EXPR:
      if (DECL_ASSIGNMENT_OPERATOR_P (t))
        dump_string (di, "xorassign");
      else
        dump_string (di, "xor");
      break;
    case LSHIFT_EXPR:
      if (DECL_ASSIGNMENT_OPERATOR_P (t))
        dump_string (di, "lshiftassign");
      else
        dump_string (di