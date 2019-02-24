/* Subroutines shared by all languages that are variants of C.
   Copyright (C) 1992, 1993, 1994, 1995, 1996, 1997, 1998, 1999, 2000,
   2001, 2002 Free Software Foundation, Inc.

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
#include "flags.h"
#include "toplev.h"
#include "output.h"
#include "c-pragma.h"
#include "rtl.h"
#include "ggc.h"
#include "expr.h"
#include "c-common.h"
#include "tree-inline.h"
#include "diagnostic.h"
#include "tm_p.h"
#include "../include/obstack.h"
#include "c-lex.h"
#include "cpplib.h"
#include "target.h"
cpp_reader *parse_in;		/* Declared in c-lex.h.  */
/* end of !kawai! */

#undef WCHAR_TYPE_SIZE
#define WCHAR_TYPE_SIZE TYPE_PRECISION (wchar_type_node)

/* We let tm.h override the types used here, to handle trivial differences
   such as the choice of unsigned int or long unsigned int for size_t.
   When machines start needing nontrivial differences in the size type,
   it would be best to do something here to figure out automatically
   from other information what type to use.  */

#ifndef SIZE_TYPE
#define SIZE_TYPE "long unsigned int"
#endif

#ifndef WCHAR_TYPE
#define WCHAR_TYPE "int"
#endif

#ifndef PTRDIFF_TYPE
#define PTRDIFF_TYPE "long int"
#endif

#ifndef WINT_TYPE
#define WINT_TYPE "unsigned int"
#endif

#ifndef INTMAX_TYPE
#define INTMAX_TYPE ((INT_TYPE_SIZE == LONG_LONG_TYPE_SIZE)	¥
		     ? "int"					¥
		     : ((LONG_TYPE_SIZE == LONG_LONG_TYPE_SIZE)	¥
			? "long int"				¥
			: "long long int"))
#endif

#ifndef UINTMAX_TYPE
#define UINTMAX_TYPE ((INT_TYPE_SIZE == LONG_LONG_TYPE_SIZE)	¥
		     ? "unsigned int"				¥
		     : ((LONG_TYPE_SIZE == LONG_LONG_TYPE_SIZE)	¥
			? "long unsigned int"			¥
			: "long long unsigned int"))
#endif

/* The variant of the C language being processed.  */

enum c_language_kind c_language;

/* The following symbols are subsumed in the c_global_trees array, and
   listed here individually for documentation purposes.

   INTEGER_TYPE and REAL_TYPE nodes for the standard data types.

	tree short_integer_type_node;
	tree long_integer_type_node;
	tree long_long_integer_type_node;

	tree short_unsigned_type_node;
	tree long_unsigned_type_node;
	tree long_long_unsigned_type_node;

	tree boolean_type_node;
	tree boolean_false_node;
	tree boolean_true_node;

	tree ptrdiff_type_node;

	tree unsigned_char_type_node;
	tree signed_char_type_node;
	tree wchar_type_node;
	tree signed_wchar_type_node;
	tree unsigned_wchar_type_node;

	tree float_type_node;
	tree double_type_node;
	tree long_double_type_node;

	tree complex_integer_type_node;
	tree complex_float_type_node;
	tree complex_double_type_node;
	tree complex_long_double_type_node;

	tree intQI_type_node;
	tree intHI_type_node;
	tree intSI_type_node;
	tree intDI_type_node;
	tree intTI_type_node;

	tree unsigned_intQI_type_node;
	tree unsigned_intHI_type_node;
	tree unsigned_intSI_type_node;
	tree unsigned_intDI_type_node;
	tree unsigned_intTI_type_node;

	tree widest_integer_literal_type_node;
	tree widest_unsigned_literal_type_node;

   Nodes for types `void *' and `const void *'.

	tree ptr_type_node, const_ptr_type_node;

   Nodes for types `char *' and `const char *'.

	tree string_type_node, const_string_type_node;

   Type `char[SOMENUMBER]'.
   Used when an array of char is needed and the size is irrelevant.

	tree char_array_type_node;

   Type `int[SOMENUMBER]' or something like it.
   Used when an array of int needed and the size is irrelevant.

	tree int_array_type_node;

   Type `wchar_t[SOMENUMBER]' or something like it.
   Used when a wide string literal is created.

	tree wchar_array_type_node;

   Type `int ()' -- used for implicit declaration of functions.

	tree default_function_type;

   A VOID_TYPE node, packaged in a TREE_LIST.

	tree void_list_node;

  The lazily created VAR_DECLs for __FUNCTION__, __PRETTY_FUNCTION__,
  and __func__. (C doesn't generate __FUNCTION__ and__PRETTY_FUNCTION__
  VAR_DECLS, but C++ does.)

	tree function_name_decl_node;
	tree pretty_function_name_decl_node;
	tree c99_function_name_decl_node;

  Stack of nested function name VAR_DECLs.
  
	tree saved_function_name_decls;

*/

tree c_global_trees[CTI_MAX];

/* Nonzero means don't recognize the non-ANSI builtin functions.  */

int flag_no_builtin;

/* Nonzero means don't recognize the non-ANSI builtin functions.
   -ansi sets this.  */

int flag_no_nonansi_builtin;

/* Nonzero means give `double' the same size as `float'.  */

int flag_short_double;

/* Nonzero means give `wchar_t' the same size as `short'.  */

int flag_short_wchar;

/* Nonzero means warn about possible violations of sequence point rules.  */

int warn_sequence_point;

/* Nonzero means to warn about compile-time division by zero.  */
int warn_div_by_zero = 1;

/* The elements of `ridpointers' are identifier nodes for the reserved
   type names and storage classes.  It is indexed by a RID_... value.  */
tree *ridpointers;

tree (*make_fname_decl)                PARAMS ((tree, int));

/* If non-NULL, the address of a language-specific function that
   returns 1 for language-specific statement codes.  */
int (*lang_statement_code_p)           PARAMS ((enum tree_code));

/* If non-NULL, the address of a language-specific function that takes
   any action required right before expand_function_end is called.  */
void (*lang_expand_function_end)       PARAMS ((void));

/* Nonzero means the expression being parsed will never be evaluated.
   This is a count, since unevaluated expressions can nest.  */
int skip_evaluation;

/* Information about how a function name is generated.  */
struct fname_var_t
{
  tree *const decl;	/* pointer to the VAR_DECL.  */
  const unsigned rid;	/* RID number for the identifier.  */
  const int pretty;	/* How pretty is it? */
};

/* The three ways of getting then name of the current function.  */

const struct fname_var_t fname_vars[] =
{
  /* C99 compliant __func__, must be first.  */
  {&c99_function_name_decl_node, RID_C99_FUNCTION_NAME, 0},
  /* GCC __FUNCTION__ compliant.  */
  {&function_name_decl_node, RID_FUNCTION_NAME, 0},
  /* GCC __PRETTY_FUNCTION__ compliant.  */
  {&pretty_function_name_decl_node, RID_PRETTY_FUNCTION_NAME, 1},
  {NULL, 0, 0},
};

static int constant_fits_type_p		PARAMS ((tree, tree));

/* Keep a stack of if statements.  We record the number of compound
   statements seen up to the if keyword, as well as the line number
   and file of the if.  If a potentially ambiguous else is seen, that
   fact is recorded; the warning is issued when we can be sure that
   the enclosing if statement does not have an else branch.  */
typedef struct
{
  int compstmt_count;
  int line;
  const char *file;
  int needs_warning;
  tree if_stmt;
} if_elt;

static if_elt *if_stack;

/* Amount of space in the if statement stack.  */
static int if_stack_space = 0;

/* Stack pointer.  */
static int if_stack_pointer = 0;

/* Record the start of an if-then, and record the start of it
   for ambiguous else detection.

   COND is the condition for the if-then statement.

   IF_STMT is the statement node that has already been created for
   this if-then statement.  It is created before parsing the
   condition to keep line number information accurate.  */

void
c_expand_start_cond (cond, compstmt_count, if_stmt)
     tree cond;
     int compstmt_count;
     tree if_stmt;
{
  /* Make sure there is enough space on the stack.  */
  if (if_stack_space == 0)
    {
      if_stack_space = 10;
      if_stack = (if_elt *) xmalloc (10 * sizeof (if_elt));
    }
  else if (if_stack_space == if_stack_pointer)
    {
      if_stack_space += 10;
      if_stack = (if_elt *) xrealloc (if_stack, if_stack_space * sizeof (if_elt));
    }

  IF_COND (if_stmt) = cond;
  add_stmt (if_stmt);

  /* Record this if statement.  */
  if_stack[if_stack_pointer].compstmt_count = compstmt_count;
  if_stack[if_stack_pointer].file = input_filename;
  if_stack[if_stack_pointer].line = lineno;
  if_stack[if_stack_pointer].needs_warning = 0;
  if_stack[if_stack_pointer].if_stmt = if_stmt;
  if_stack_pointer++;
}

/* Called after the then-clause for an if-statement is processed.  */

void
c_finish_then ()
{
  tree if_stmt = if_stack[if_stack_pointer - 1].if_stmt;
  RECHAIN_STMTS (if_stmt, THEN_CLAUSE (if_stmt));
}

/* Record the end of an if-then.  Optionally warn if a nested
   if statement had an ambiguous else clause.  */

void
c_expand_end_cond ()
{
  if_stack_pointer--;
  if (if_stack[if_stack_pointer].needs_warning)
    warning_with_file_and_line (if_stack[if_stack_pointer].file,
				if_stack[if_stack_pointer].line,
				"suggest explicit braces to avoid ambiguous `else'");
  last_expr_type = NULL_TREE;
}

/* Called between the then-clause and the else-clause
   of an if-then-else.  */

void
c_expand_start_else ()
{
  /* An ambiguous else warning must be generated for the enclosing if
     statement, unless we see an else branch for that one, too.  */
  if (warn_parentheses
      && if_stack_pointer > 1
      && (if_stack[if_stack_pointer - 1].compstmt_count
	  == if_stack[if_stack_pointer - 2].compstmt_count))
    if_stack[if_stack_pointer - 2].needs_warning = 1;

  /* Even if a nested if statement had an else branch, it can't be
     ambiguous if this one also has an else.  So don't warn in that
     case.  Also don't warn for any if statements nested in this else.  */
  if_stack[if_stack_pointer - 1].needs_warning = 0;
  if_stack[if_stack_pointer - 1].compstmt_count--;
}

/* Called after the else-clause for an if-statement is processed.  */

void
c_finish_else ()
{
  tree if_stmt = if_stack[if_stack_pointer - 1].if_stmt;
  RECHAIN_STMTS (if_stmt, ELSE_CLAUSE (if_stmt));
}

/* Begin an if-statement.  Returns a newly created IF_STMT if
   appropriate.

   Unlike the C++ front-end, we do not call add_stmt here; it is
   probably safe to do so, but I am not very familiar with this
   code so I am being extra careful not to change its behavior
   beyond what is strictly necessary for correctness.  */

tree
c_begin_if_stmt ()
{
  tree r;
  r = build_stmt (IF_STMT, NULL_TREE, NULL_TREE, NULL_TREE);
  return r;
}

/* Begin a while statement.  Returns a newly created WHILE_STMT if
   appropriate.

   Unlike the C++ front-end, we do not call add_stmt here; it is
   probably safe to do so, but I am not very familiar with this
   code so I am being extra careful not to change its behavior
   beyond what is strictly necessary for correctness.  */

tree
c_begin_while_stmt ()
{
  tree r;
  r = build_stmt (WHILE_STMT, NULL_TREE, NULL_TREE);
  return r;
}

void
c_finish_while_stmt_cond (cond, while_stmt)
     tree while_stmt;
     tree cond;
{
  WHILE_COND (while_stmt) = cond;
}

/* Push current bindings for the function name VAR_DECLS.  */

void
start_fname_decls ()
{
  unsigned ix;
  tree saved = NULL_TREE;
  
  for (ix = 0; fname_vars[ix].decl; ix++)
    {
      tree decl = *fname_vars[ix].decl;

      if (decl)
	{
	  saved = tree_cons (decl, build_int_2 (ix, 0), saved);
	  *fname_vars[ix].decl = NULL_TREE;
	}
    }
  if (saved || saved_function_name_decls)
    /* Normally they'll have been NULL, so only push if we've got a
       stack, or they are non-NULL.  */
    saved_function_name_decls = tree_cons (saved, NULL_TREE,
					   saved_function_name_decls);
}

/* Finish up the current bindings, adding them into the
   current function's statement tree. This is done by wrapping the
   function's body in a COMPOUND_STMT containing these decls too. This
   must be done _before_ finish_stmt_tree is called. If there is no
   current function, we must be at file scope and no statements are
   involved. Pop the previous bindings.  */

void
finish_fname_decls ()
{
  unsigned ix;
  tree body = NULL_TREE;
  tree stack = saved_function_name_decls;

  for (; stack && TREE_VALUE (stack); stack = TREE_CHAIN (stack))
    body = chainon (TREE_VALUE (stack), body);
  
  if (body)
    {
      /* They were called into existence, so add to statement tree.  */
      body = chainon (body,
		      TREE_CHAIN (DECL_SAVED_TREE (current_function_decl)));
      body = build_stmt (COMPOUND_STMT, body);
      
      COMPOUND_STMT_NO_SCOPE (body) = 1;
      TREE_CHAIN (DECL_SAVED_TREE (current_function_decl)) = body;
    }
  
  for (ix = 0; fname_vars[ix].decl; ix++)
    *fname_vars[ix].decl = NULL_TREE;
  
  if (stack)
    {
      /* We had saved values, restore them.  */
      tree saved;

      for (saved = TREE_PURPOSE (stack); saved; saved = TREE_CHAIN (saved))
	{
	  tree decl = TREE_PURPOSE (saved);
	  unsigned ix = TREE_INT_CST_LOW (TREE_VALUE (saved));
	  
	  *fname_vars[ix].decl = decl;
	}
      stack = TREE_CHAIN (stack);
    }
  saved_function_name_decls = stack;
}

/* Return the text name of the current function, suitable prettified
   by PRETTY_P.  */

const char *
fname_as_string (pretty_p)
     int pretty_p;
{
  const char *name = NULL;
  
  if (pretty_p)
    name = (current_function_decl
	    ? (*decl_printable_name) (current_function_decl, 2)
	    : "top level");
  else if (current_function_decl && DECL_NAME (current_function_decl))
    name = IDENTIFIER_POINTER (DECL_NAME (current_function_decl));
  else
    name = "";
  return name;
}

/* Return the text name of the current function, formatted as
   required by the supplied RID value.  */

const char *
fname_string (rid)
     unsigned rid;
{
  unsigned ix;
  
  for (ix = 0; fname_vars[ix].decl; ix++)
    if (fname_vars[ix].rid == rid)
      break;
  return fname_as_string (fname_vars[ix].pretty);
}

/* Return the VAR_DECL for a const char array naming the current
   function. If the VAR_DECL has not yet been created, create it
   now. RID indicates how it should be formatted and IDENTIFIER_NODE
   ID is its name (unfortunately C and C++ hold the RID values of
   keywords in different places, so we can't derive RID from ID in
   this language independent code.  */

tree
fname_decl (rid, id)
     unsigned rid;
     tree id;
{
  unsigned ix;
  tree decl = NULL_TREE;

  for (ix = 0; fname_vars[ix].decl; ix++)
    if (fname_vars[ix].rid == rid)
      break;

  decl = *fname_vars[ix].decl;
  if (!decl)
    {
      tree saved_last_tree = last_tree;
      
      decl = (*make_fname_decl) (id, fname_vars[ix].pretty);
      if (last_tree != saved_last_tree)
	{
	  /* We created some statement tree for the decl. This belongs
	     at the start of the function, so remove it now and reinsert
	     it after the function is complete.  */
	  tree stmts = TREE_CHAIN (saved_last_tree);

	  TREE_CHAIN (saved_last_tree) = NULL_TREE;
	  last_tree = saved_last_tree;
	  saved_function_name_decls = tree_cons (decl, stmts,
						 saved_function_name_decls);
	}
      *fname_vars[ix].decl = decl;
    }
  if (!ix && !current_function_decl)
    pedwarn_with_decl (decl, "`%s' is not defined outside of function scope");
  
  return decl;
}

/* Given a chain of STRING_CST nodes,
   concatenate them into one STRING_CST
   and give it a suitable array-of-chars data type.  */

tree
combine_strings (strings)
     tree strings;
{
  tree value, t;
  int length = 1;
  int wide_length = 0;
  int wide_flag = 0;
  int wchar_bytes = TYPE_PRECISION (wchar_type_nod