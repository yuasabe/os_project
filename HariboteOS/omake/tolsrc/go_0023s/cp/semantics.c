/* Perform the semantic phase of parsing, i.e., the process of
   building tree structure, checking semantic consistency, and
   building RTL.  These routines are used both during actual parsing
   and during the instantiation of template functions. 

   Copyright (C) 1998, 1999, 2000, 2001, 2002 Free Software Foundation, Inc.
   Written by Mark Mitchell (mmitchell@usa.net) based on code found
   formerly in parse.y and pt.c.  

   This file is part of GNU CC.

   GNU CC is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.
   
   GNU CC is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with GNU CC; see the file COPYING.  If not, write to the Free
   Software Foundation, 59 Temple Place - Suite 330, Boston, MA
   02111-1307, USA.  */

/* !kawai! */
#include "../gcc/config.h"
#include "../gcc/system.h"
#include "../gcc/tree.h"
#include "cp-tree.h"
#include "../gcc/tree-inline.h"
#include "../gcc/except.h"
#include "lex.h"
#include "../gcc/toplev.h"
#include "../gcc/flags.h"
#include "../gcc/ggc.h"
#include "../gcc/rtl.h"
#include "../gcc/expr.h"
#include "../gcc/output.h"
#include "../gcc/timevar.h"
#include "../gcc/debug.h"
/* end of !kawai! */

/* There routines provide a modular interface to perform many parsing
   operations.  They may therefore be used during actual parsing, or
   during template instantiation, which may be regarded as a
   degenerate form of parsing.  Since the current g++ parser is
   lacking in several respects, and will be reimplemented, we are
   attempting to move most code that is not directly related to
   parsing into this file; that will make implementing the new parser
   much easier since it will be able to make use of these routines.  */

static tree maybe_convert_cond PARAMS ((tree));
static tree simplify_aggr_init_exprs_r PARAMS ((tree *, int *, void *));
static void deferred_type_access_control PARAMS ((void));
static void emit_associated_thunks PARAMS ((tree));
static void genrtl_try_block PARAMS ((tree));
static void genrtl_eh_spec_block PARAMS ((tree));
static void genrtl_handler PARAMS ((tree));
static void genrtl_ctor_stmt PARAMS ((tree));
static void genrtl_subobject PARAMS ((tree));
static void genrtl_named_return_value PARAMS ((void));
static void cp_expand_stmt PARAMS ((tree));
static void genrtl_start_function PARAMS ((tree));
static void genrtl_finish_function PARAMS ((tree));
static tree clear_decl_rtl PARAMS ((tree *, int *, void *));

/* Finish processing the COND, the SUBSTMT condition for STMT.  */

#define FINISH_COND(COND, STMT, SUBSTMT) 		¥
  do {							¥
    if (last_tree != (STMT))				¥
      {							¥
        RECHAIN_STMTS (STMT, SUBSTMT);			¥
        if (!processing_template_decl)			¥
          {						¥
	    (COND) = build_tree_list (SUBSTMT, COND);	¥
	    (SUBSTMT) = (COND);				¥
          }						¥
      }							¥
    else						¥
      (SUBSTMT) = (COND);				¥
  } while (0)

/* Returns non-zero if the current statement is a full expression,
   i.e. temporaries created during that statement should be destroyed
   at the end of the statement.  */

int
stmts_are_full_exprs_p ()
{
  return current_stmt_tree ()->stmts_are_full_exprs_p;
}

/* Returns the stmt_tree (if any) to which statements are currently
   being added.  If there is no active statement-tree, NULL is
   returned.  */

stmt_tree
current_stmt_tree ()
{
  return (cfun 
	  ? &cfun->language->x_stmt_tree 
	  : &scope_chain->x_stmt_tree);
}

/* Nonzero if TYPE is an anonymous union or struct type.  We have to use a
   flag for this because "A union for which objects or pointers are
   declared is not an anonymous union" [class.union].  */

int
anon_aggr_type_p (node)
     tree node;
{
  return (CLASS_TYPE_P (node) && TYPE_LANG_SPECIFIC(node)->anon_aggr);
}

/* Finish a scope.  */

tree
do_poplevel ()
{
  tree block = NULL_TREE;

  if (stmts_are_full_exprs_p ())
    {
      tree scope_stmts = NULL_TREE;

      if (!processing_template_decl)
	scope_stmts = add_scope_stmt (/*begin_p=*/0, /*partial_p=*/0);

      block = poplevel (kept_level_p (), 1, 0);
      if (block && !processing_template_decl)
	{
	  SCOPE_STMT_BLOCK (TREE_PURPOSE (scope_stmts)) = block;
	  SCOPE_STMT_BLOCK (TREE_VALUE (scope_stmts)) = block;
	}
    }

  return block;
}

/* Begin a new scope.  */ 

void
do_pushlevel ()
{
  if (stmts_are_full_exprs_p ())
    {
      pushlevel (0);
      if (!processing_template_decl)
	add_scope_stmt (/*begin_p=*/1, /*partial_p=*/0);
    }
}

/* Finish a goto-statement.  */

tree
finish_goto_stmt (destination)
     tree destination;
{
  if (TREE_CODE (destination) == IDENTIFIER_NODE)
    destination = lookup_label (destination);

  /* We warn about unused labels with -Wunused.  That means we have to
     mark the used labels as used.  */
  if (TREE_CODE (destination) == LABEL_DECL)
    TREE_USED (destination) = 1;
    
  if (TREE_CODE (destination) != LABEL_DECL)
    /* We don't inline calls to functions with computed gotos.
       Those functions are typically up to some funny business,
       and may be depending on the labels being at particular
       addresses, or some such.  */
    DECL_UNINLINABLE (current_function_decl) = 1;
  
  check_goto (destination);

  return add_stmt (build_stmt (GOTO_STMT, destination));
}

/* COND is the condition-expression for an if, while, etc.,
   statement.  Convert it to a boolean value, if appropriate.  */

tree
maybe_convert_cond (cond)
     tree cond;
{
  /* Empty conditions remain empty.  */
  if (!cond)
    return NULL_TREE;

  /* Wait until we instantiate templates before doing conversion.  */
  if (processing_template_decl)
    return cond;

  /* Do the conversion.  */
  cond = convert_from_reference (cond);
  return condition_conversion (cond);
}

/* Finish an expression-statement, whose EXPRESSION is as indicated.  */

tree
finish_expr_stmt (expr)
     tree expr;
{
  tree r = NULL_TREE;
  tree expr_type = NULL_TREE;;

  if (expr != NULL_TREE)
    {
      if (!processing_template_decl
	  && !(stmts_are_full_exprs_p ())
	  && ((TREE_CODE (TREE_TYPE (expr)) == ARRAY_TYPE
	       && lvalue_p (expr))
	      || TREE_CODE (TREE_TYPE (expr)) == FUNCTION_TYPE))
	expr = default_conversion (expr);
      
      /* Remember the type of the expression.  */
      expr_type = TREE_TYPE (expr);

      if (stmts_are_full_exprs_p ())
	expr = convert_to_void (expr, "statement");
      
      r = add_stmt (build_stmt (EXPR_STMT, expr));
    }

  finish_stmt ();

  /* This was an expression-statement, so we save the type of the
     expression.  */
  last_expr_type = expr_type;

  return r;
}


/* Begin an if-statement.  Returns a newly created IF_STMT if
   appropriate.  */

tree
begin_if_stmt ()
{
  tree r;
  do_pushlevel ();
  r = build_stmt (IF_STMT, NULL_TREE, NULL_TREE, NULL_TREE);
  add_stmt (r);
  return r;
}

/* Process the COND of an if-statement, which may be given by
   IF_STMT.  */

void 
finish_if_stmt_cond (cond, if_stmt)
     tree cond;
     tree if_stmt;
{
  cond = maybe_convert_cond (cond);
  FINISH_COND (cond, if_stmt, IF_COND (if_stmt));
}

/* Finish the then-clause of an if-statement, which may be given by
   IF_STMT.  */

tree
finish_then_clause (if_stmt)
     tree if_stmt;
{
  RECHAIN_STMTS (if_stmt, THEN_CLAUSE (if_stmt));
  last_tree = if_stmt;
  return if_stmt;
}

/* Begin the else-clause of an if-statement.  */

void 
begin_else_clause ()
{
}

/* Finish the else-clause of an if-statement, which may be given by
   IF_STMT.  */

void
finish_else_clause (if_stmt)
     tree if_stmt;
{
  RECHAIN_STMTS (if_stmt, ELSE_CLAUSE (if_stmt));
}

/* Finish an if-statement.  */

void 
finish_if_stmt ()
{
  do_poplevel ();
  finish_stmt ();
}

void
clear_out_block ()
{
  /* If COND wasn't a declaration, clear out the
     block we made for it and start a new one here so the
     optimization in expand_end_loop will work.  */
  if (getdecls () == NULL_TREE)
    {
      do_poplevel ();
      do_pushlevel ();
    }
}

/* Begin a while-statement.  Returns a newly created WHILE_STMT if
   appropriate.  */

tree
begin_while_stmt ()
{
  tree r;
  r = build_stmt (WHILE_STMT, NULL_TREE, NULL_TREE);
  add_stmt (r);
  do_pushlevel ();
  return r;
}

/* Process the COND of a while-statement, which may be given by
   WHILE_STMT.  */

void 
finish_while_stmt_cond (cond, while_stmt)
     tree cond;
     tree while_stmt;
{
  cond = maybe_convert_cond (cond);
  FINISH_COND (cond, while_stmt, WHILE_COND (while_stmt));
  clear_out_block ();
}

/* Finish a while-statement, which may be given by WHILE_STMT.  */

void 
finish_while_stmt (while_stmt)
     tree while_stmt;
{
  do_poplevel ();
  RECHAIN_STMTS (while_stmt, WHILE_BODY (while_stmt));
  finish_stmt ();
}

/* Begin a do-statement.  Returns a newly created DO_STMT if
   appropriate.  */

tree
begin_do_stmt ()
{
  tree r = build_stmt (DO_STMT, NULL_TREE, NULL_TREE);
  add_stmt (r);
  return r;
}

/* Finish the body of a do-statement, which may be given by DO_STMT.  */

void
finish_do_body (do_stmt)
     tree do_stmt;
{
  RECHAIN_STMTS (do_stmt, DO_BODY (do_stmt));
}

/* Finish a do-statement, which may be given by DO_STMT, and whose
   COND is as indicated.  */

void
finish_do_stmt (cond, do_stmt)
     tree cond;
     tree do_stmt;
{
  cond = maybe_convert_cond (cond);
  DO_COND (do_stmt) = cond;
  finish_stmt ();
}

/* Finish a return-statement.  The EXPRESSION returned, if any, is as
   indicated.  */

tree
finish_return_stmt (expr)
     tree expr;
{
  tree r;

  if (!processing_template_decl)
    expr = check_return_expr (expr);
  if (!processing_template_decl)
    {
      if (DECL_DESTRUCTOR_P (current_function_decl))
	{
	  /* Similarly, all destructors must run destructors for
	     base-classes before returning.  So, all returns in a
	     destructor get sent to the DTOR_LABEL; finish_function emits
	     code to return a value there.  */
	  return finish_goto_stmt (dtor_label);
	}
    }
  r = add_stmt (build_stmt (RETURN_STMT, expr));
  finish_stmt ();

  return r;
}

/* Begin a for-statement.  Returns a new FOR_STMT if appropriate.  */

tree
begin_for_stmt ()
{
  tree r;

  r = build_stmt (FOR_STMT, NULL_TREE, NULL_TREE, 
		  NULL_TREE, NULL_TREE);
  NEW_FOR_SCOPE_P (r) = flag_new_for_scope > 0;
  add_stmt (r);
  if (NEW_FOR_SCOPE_P (r))
    {
      do_pushlevel ();
      note_level_for_for ();
    }

  return r;
}

/* Finish the for-init-statement of a for-statement, which may be
   given by FOR_STMT.  */

void
finish_for_init_stmt (for_stmt)
     tree for_stmt;
{
  if (last_tree != for_stmt)
    RECHAIN_STMTS (for_stmt, FOR_INIT_STMT (for_stmt));
  do_pushlevel ();
}

/* Finish the COND of a for-statement, which may be given by
   FOR_STMT.  */

void
finish_for_cond (cond, for_stmt)
     tree cond;
     tree for_stmt;
{
  cond = maybe_convert_cond (cond);
  FINISH_COND (cond, for_stmt, FOR_COND (for_stmt));
  clear_out_block ();
}

/* Finish the increment-EXPRESSION in a for-statement, which may be
   given by FOR_STMT.  */

void
finish_for_expr (expr, for_stmt)
     tree expr;
     tree for_stmt;
{
  FOR_EXPR (for_stmt) = expr;
}

/* Finish the body of a for-statement, which may be given by
   FOR_STMT.  The increment-EXPR for the loop must be
   provided.  */

void
finish_for_stmt (for_stmt)
     tree for_stmt;
{
  /* Pop the scope for the body of the loop.  */
  do_poplevel ();
  RECHAIN_STMTS (for_stmt, FOR_BODY (for_stmt));
  if (NEW_FOR_SCOPE_P (for_stmt))
