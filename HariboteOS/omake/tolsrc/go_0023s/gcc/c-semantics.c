/* This file contains the definitions and documentation for the common
   tree codes used in the GNU C and C++ compilers (see c-common.def
   for the standard codes).  
   Copyright (C) 2000, 2001, 2002 Free Software Foundation, Inc.
   Written by Benjamin Chelf (chelf@codesourcery.com).

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
#include "function.h"
#include "../include/splay-tree.h"
#include "varray.h"
#include "c-common.h"
#include "except.h"
#include "toplev.h"
#include "flags.h"
#include "ggc.h"
#include "rtl.h"
#include "expr.h"
#include "output.h"
#include "timevar.h"
/* end of !kawai! */

/* If non-NULL, the address of a language-specific function for
   expanding statements.  */
void (*lang_expand_stmt) PARAMS ((tree));

/* If non-NULL, the address of a language-specific function for
   expanding a DECL_STMT.  After the language-independent cases are
   handled, this function will be called.  If this function is not
   defined, it is assumed that declarations other than those for
   variables and labels do not require any RTL generation.  */
void (*lang_expand_decl_stmt) PARAMS ((tree));

/* Create an empty statement tree rooted at T.  */

void
begin_stmt_tree (t)
     tree *t;
{
  /* We create a trivial EXPR_STMT so that last_tree is never NULL in
     what follows.  We remove the extraneous statement in
     finish_stmt_tree.  */
  *t = build_nt (EXPR_STMT, void_zero_node);
  last_tree = *t;
  last_expr_type = NULL_TREE;
  last_expr_filename = input_filename;
}

/* T is a statement.  Add it to the statement-tree.  */

tree
add_stmt (t)
     tree t;
{
  if (input_filename != last_expr_filename)
    {
      /* If the filename has changed, also add in a FILE_STMT.  Do a string
	 compare first, though, as it might be an equivalent string.  */
      int add = (strcmp (input_filename, last_expr_filename) != 0);
      last_expr_filename = input_filename;
      if (add)
	{
	  tree pos = build_nt (FILE_STMT, get_identifier (input_filename));
	  add_stmt (pos);
	}
    }

  /* Add T to the statement-tree.  */
  TREE_CHAIN (last_tree) = t;
  last_tree = t;
  
  /* When we expand a statement-tree, we must know whether or not the
     statements are full-expressions.  We record that fact here.  */
  STMT_IS_FULL_EXPR_P (last_tree) = stmts_are_full_exprs_p ();

  /* Keep track of the number of statements in this function.  */
  if (current_function_decl)
    ++DECL_NUM_STMTS (current_function_decl);

  return t;
}

/* Create a declaration statement for the declaration given by the
   DECL.  */

void
add_decl_stmt (decl)
     tree decl;
{
  tree decl_stmt;

  /* We need the type to last until instantiation time.  */
  decl_stmt = build_stmt (DECL_STMT, decl);
  add_stmt (decl_stmt); 
}

/* Add a scope-statement to the statement-tree.  BEGIN_P indicates
   whether this statements opens or closes a scope.  PARTIAL_P is true
   for a partial scope, i.e, the scope that begins after a label when
   an object that needs a cleanup is created.  If BEGIN_P is nonzero,
   returns a new TREE_LIST representing the top of the SCOPE_STMT
   stack.  The TREE_PURPOSE is the new SCOPE_STMT.  If BEGIN_P is
   zero, returns a TREE_LIST whose TREE_VALUE is the new SCOPE_STMT,
   and whose TREE_PURPOSE is the matching SCOPE_S