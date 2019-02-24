/* Definitions for c-common.c.
   Copyright (C) 1987, 1993, 1994, 1995, 1997, 1998,
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

#ifndef GCC_C_COMMON_H
#define GCC_C_COMMON_H

/* !kawai! */
#include "../include/splay-tree.h"
#include "cpplib.h"
/* end of !kawai! */

/* Usage of TREE_LANG_FLAG_?:
   0: COMPOUND_STMT_NO_SCOPE (in COMPOUND_STMT).
      TREE_NEGATED_INT (in INTEGER_CST).
      IDENTIFIER_MARKED (used by search routines).
      SCOPE_BEGIN_P (in SCOPE_STMT)
      DECL_PRETTY_FUNCTION_P (in VAR_DECL)
      NEW_FOR_SCOPE_P (in FOR_STMT)
      ASM_INPUT_P (in ASM_STMT)
      STMT_EXPR_NO_SCOPE (in STMT_EXPR)
   1: C_DECLARED_LABEL_FLAG (in LABEL_DECL)
      STMT_IS_FULL_EXPR_P (in _STMT)
   2: STMT_LINENO_FOR_FN_P (in _STMT)
   3: SCOPE_NO_CLEANUPS_P (in SCOPE_STMT)
      COMPOUND_STMT_BODY_BLOCK (in COMPOUND_STMT)
   4: SCOPE_PARTIAL_P (in SCOPE_STMT)
*/

/* Reserved identifiers.  This is the union of all the keywords for C,
   C++, and Objective C.  All the type modifiers have to be in one
   block at the beginning, because they are used as mask bits.  There
   are 27 type modifiers; if we add many more we will have to redesign
   the mask mechanism.  */

enum rid
{
  /* Modifiers: */
  /* C, in empirical order of frequency.  */
  RID_STATIC = 0,
  RID_UNSIGNED, RID_LONG,    RID_CONST, RID_EXTERN,
  RID_REGISTER, RID_TYPEDEF, RID_SHORT, RID_INLINE,
  RID_VOLATILE, RID_SIGNED,  RID_AUTO,  RID_RESTRICT,

  /* C extensions */
  RID_BOUNDED, RID_UNBOUNDED, RID_COMPLEX,

  /* C++ */
  RID_FRIEND, RID_VIRTUAL, RID_EXPLICIT, RID_EXPORT, RID_MUTABLE,

  /* ObjC */
  RID_IN, RID_OUT, RID_INOUT, RID_BYCOPY, RID_BYREF, RID_ONEWAY,

  /* C */
  RID_INT,     RID_CHAR,   RID_FLOAT,    RID_DOUBLE, RID_VOID,
  RID_ENUM,    RID_STRUCT, RID_UNION,    RID_IF,     RID_ELSE,
  RID_WHILE,   RID_DO,     RID_FOR,      RID_SWITCH, RID_CASE,
  RID_DEFAULT, RID_BREAK,  RID_CONTINUE, RID_RETURN, RID_GOTO,
  RID_SIZEOF,

  /* C extensions */
  RID_ASM,       RID_TYPEOF,   RID_ALIGNOF,  RID_ATTRIBUTE,  RID_VA_ARG,
  RID_EXTENSION, RID_IMAGPART, RID_REALPART, RID_LABEL,      RID_PTRBASE,
  RID_PTREXTENT, RID_PTRVALUE, RID_CHOOSE_EXPR, RID_TYPES_COMPATIBLE_P,

  /* Too many ways of getting the name of a function as a string */
  RID_FUNCTION_NAME, RID_PRETTY_FUNCTION_NAME, RID_C99_FUNCTION_NAME,

  /* C++ */
  RID_BOOL,     RID_WCHAR,    RID_CLASS,
  RID_PUBLIC,   RID_PRIVATE,  RID_PROTECTED,
  RID_TEMPLATE, RID_NULL,     RID_CATCH,
  RID_DELETE,   RID_FALSE,    RID_NAMESPACE,
  RID_NEW,      RID_OPERATOR, RID_THIS,
  RID_THROW,    RID_TRUE,     RID_TRY,
  RID_TYPENAME, RID_TYPEID,   RID_USING,

  /* casts */
  RID_CONSTCAST, RID_DYNCAST, RID_REINTCAST, RID_STATCAST,

  /* alternate spellings */
  RID_AND, RID_AND_EQ, RID_NOT, RID_NOT_EQ,
  RID_OR,  RID_OR_EQ,  RID_XOR, RID_XOR_EQ,
  RID_BITAND, RID_BITOR, RID_COMPL,

  /* Objective C */
  RID_ID,          RID_AT_ENCODE,    RID_AT_END,
  RID_AT_CLASS,    RID_AT_ALIAS,     RID_AT_DEFS,
  RID_AT_PRIVATE,  RID_AT_PROTECTED, RID_AT_PUBLIC,
  RID_AT_PROTOCOL, RID_AT_SELECTOR,  RID_AT_INTERFACE,
  RID_AT_IMPLEMENTATION,

  RID_MAX,

  RID_FIRST_MODIFIER = RID_STATIC,
  RID_LAST_MODIFIER = RID_ONEWAY,

  RID_FIRST_AT = RID_AT_ENCODE,
  RID_LAST_AT = RID_AT_IMPLEMENTATION,
  RID_FIRST_PQ = RID_IN,
  RID_LAST_PQ = RID_ONEWAY
};

#define OBJC_IS_AT_KEYWORD(rid) ¥
  ((unsigned int)(rid) >= (unsigned int)RID_FIRST_AT && ¥
   (unsigned int)(rid) <= (unsigned int)RID_LAST_AT)

#define OBJC_IS_PQ_KEYWORD(rid) ¥
  ((unsigned int)(rid) >= (unsigned int)RID_FIRST_PQ && ¥
   (unsigned int)(rid) <= (unsigned int)RID_LAST_PQ)

/* The elements of `ridpointers' are identifier nodes for the reserved
   type names and storage classes.  It is indexed by a RID_... value.  */
extern tree *ridpointers;

/* Standard named or nameless data types of the C compiler.  */

enum c_tree_index
{
    CTI_WCHAR_TYPE,
    CTI_SIGNED_WCHAR_TYPE,
    CTI_UNSIGNED_WCHAR_TYPE,
    CTI_WINT_TYPE,
    CTI_C_SIZE_TYPE, /* The type used for the size_t typedef and the
			result type of sizeof (an ordinary type without
			TYPE_IS_SIZETYPE set, unlike the internal
			sizetype).  */
    CTI_SIGNED_SIZE_TYPE, /* For format checking only.  */
    CTI_UNSIGNED_PTRDIFF_TYPE, /* For format checking only.  */
    CTI_INTMAX_TYPE,
    CTI_UINTMAX_TYPE,
    CTI_WIDEST_INT_LIT_TYPE,
    CTI_WIDEST_UINT_LIT_TYPE,

    CTI_CHAR_ARRAY_TYPE,
    CTI_WCHAR_ARRAY_TYPE,
    CTI_INT_ARRAY_TYPE,
    CTI_STRING_TYPE,
    CTI_CONST_STRING_TYPE,

    /* Type for boolean expressions (bool in C++, int in C).  */
    CTI_BOOLEAN_TYPE,
    CTI_BOOLEAN_TRUE,
    CTI_BOOLEAN_FALSE,
    /* C99's _Bool type.  */
    CTI_C_BOOL_TYPE,
    CTI_C_BOOL_TRUE,
    CTI_C_BOOL_FALSE,
    CTI_DEFAULT_FUNCTION_TYPE,

    CTI_G77_INTEGER_TYPE,
    CTI_G77_UINTEGER_TYPE,
    CTI_G77_LONGINT_TYPE,
    CTI_G77_ULONGINT_TYPE,

    /* These are not types, but we have to look them up all the time.  */
    CTI_FUNCTION_NAME_DECL,
    CTI_PRETTY_FUNCTION_NAME_DECL,
    CTI_C99_FUNCTION_NAME_DECL,
    CTI_SAVED_FUNCTION_NAME_DECLS,
    
    CTI_VOID_ZERO,

    CTI_MAX
};

#define C_RID_CODE(id)	(((struct c_common_identifier *) (id))->node.rid_code)

/* Identifier part common to the C front ends.  Inherits from
   tree_identifier, despite appearances.  */
struct c_common_identifier
{
  struct tree_common common;
  struct cpp_hashnode node;
};

#define wchar_type_node			c_global_trees[CTI_WCHAR_TYPE]
#define signed_wchar_type_node		c_global_trees[CTI_SIGNED_WCHAR_TYPE]
#define unsigned_wchar_type_node	c_global_trees[CTI_UNSIGNED_WCHAR_TYPE]
#define wint_type_node			c_global_trees[CTI_WINT_TYPE]
#define c_size_type_node		c_global_trees[CTI_C_SIZE_TYPE]
#define signed_size_type_node		c_global_trees[CTI_SIGNED_SIZE_TYPE]
#define unsigned_ptrdiff_type_node	c_global_trees[CTI_UNSIGNED_PTRDIFF_TYPE]
#define intmax_type_node		c_global_trees[CTI_INTMAX_TYPE]
#define uintmax_type_node		c_global_trees[CTI_UINTMAX_TYPE]
#define widest_integer_literal_type_node c_global_trees[CTI_WIDEST_INT_LIT_TYPE]
#define widest_unsigned_literal_type_node c_global_trees[CTI_WIDEST_UINT_LIT_TYPE]

#define boolean_type_node		c_global_trees[CTI_BOOLEAN_TYPE]
#define boolean_true_node		c_global_trees[CTI_BOOLEAN_TRUE]
#define boolean_false_node		c_global_trees[CTI_BOOLEAN_FALSE]

#define c_bool_type_node		c_global_trees[CTI_C_BOOL_TYPE]
#define c_bool_true_node		c_global_trees[CTI_C_BOOL_TRUE]
#define c_bool_false_node		c_global_trees[CTI_C_BOOL_FALSE]

#define char_array_type_node		c_global_trees[CTI_CHAR_ARRAY_TYPE]
#define wchar_array_type_node		c_global_trees[CTI_WCHAR_ARRAY_TYPE]
#define int_array_type_node		c_global_trees[CTI_INT_ARRAY_TYPE]
#define string_type_node		c_global_trees[CTI_STRING_TYPE]
#define const_string_type_node		c_global_trees[CTI_CONST_STRING_TYPE]

#define default_function_type		c_global_trees[CTI_DEFAULT_FUNCTION_TYPE]

/* g77 integer types, which which must be kept in sync with f/com.h */
#define g77_integer_type_node		c_global_trees[CTI_G77_INTEGER_TYPE]
#define g77_uinteger_type_node		c_global_trees[CTI_G77_UINTEGER_TYPE]
#define g77_longint_type_node		c_global_trees[CTI_G77_LONGINT_TYPE]
#define g77_ulongint_type_node		c_global_trees[CTI_G77_ULONGINT_TYPE]

#define function_name_decl_node		c_global_trees[CTI_FUNCTION_NAME_DECL]
#define pretty_function_name_decl_node	c_global_trees[CTI_PRETTY_FUNCTION_NAME_DECL]
#define c99_function_name_decl_node		c_global_trees[CTI_C99_FUNCTION_NAME_DECL]
#define saved_function_name_decls	c_global_trees[CTI_SAVED_FUNCTION_NAME_DECLS]

/* A node for `((void) 0)'.  */
#define void_zero_node                  c_global_trees[CTI_VOID_ZERO]

extern tree c_global_trees[CTI_MAX];

/* Mark which labels are explicitly declared.
   These may be shadowed, and may be referenced from nested functions.  */
#define C_DECLARED_LABEL_FLAG(label) TREE_LANG_FLAG_1 (label)

/* Flag strings given by __FUNCTION__ and __PRETTY_FUNCTION__ for a
   warning if they undergo concatenation.  */
#define C_ARTIFICIAL_STRING_P(NODE) TREE_LANG_FLAG_0 (NODE)

typedef enum c_language_kind
{
  clk_c,           /* A dialect of C: K&R C, ANSI/ISO C89, C2000,
		       etc.  */
  clk_cplusplus,   /* ANSI/ISO C++ */
  clk_objective_c  /* Objective C */
}
c_language_kind;

/* Information about a statement tree.  */

struct stmt_tree_s {
  /* The last statement added to the tree.  */
  tree x_last_stmt;
  /* The type of the last expression statement.  (This information is
     needed to implement the statement-expression extension.)  */
  tree x_last_expr_type;
  /* The last filename we recorded.  */
  const char *x_last_expr_filename;
  /* In C++, Non-zero if we should treat statements as full
     expressions.  In particular, this variable is no-zero if at the
     end of a statement we should destroy any temporaries created
     during that statement.  Similarly, if, at the end of a block, we
     should destroy any local variables in this block.  Normally, this
     variable is non-zero, since those are the normal semantics of
     C++.

     However, in order to represent aggregate initialization code as
     tree structure, we use statement-expressions.  The statements
     within the statement expression should not result in cleanups
     being run until the entire enclosing statement is complete.

     This flag has no effect in C.  */
  int stmts_are_full_exprs_p;
};

typedef struct stmt_tree_s *stmt_tree;

/* Global state pertinent to the current function.  Some C dialects
   extend this structure with additional fields.  */

struct language_function {
  /* While we are parsing the function, this contains information
     about the statement-tree that we are building.  */
  struct stmt_tree_s x_stmt_tree;
  /* The stack of SCOPE_STMTs for the current function.  */
  tree x_scope_stmt_stack;
};

/* When building a statement-tree, this is the last statement added to
   the tree.  */

#define last_tree (current_stmt_tree ()->x_last_stmt)

/* The type of the last expression-statement we have seen.  */

#define last_expr_type (current_stmt_tree ()->x_last_expr_type)

/* The name of the last file we have seen.  */

#define last_expr_filename (current_stmt_tree ()->x_last_expr_filename)

/* LAST_TREE contains the last statement parsed.  These are chained
   together through the TREE_CHAIN field, but often need to be
   re-organized since the parse is performed bottom-up.  This macro
   makes LAST_TREE the indicated SUBSTMT of STMT.  */

#define RECHAIN_STMTS(stmt, substmt)		¥
  do {						¥
    substmt = TREE_CHAIN (stmt);		¥
    TREE_CHAIN (stmt) = NULL_TREE;		¥
    last_tree = stmt;				¥
  } while (0)

/* Language-specific hooks.  */

extern int (*lang_statement_code_p)             PARAMS ((enum tree_code));
extern void (*lang_expand_stmt)                 PARAMS ((tree));
extern void (*lang_expand_decl_stmt)            PARAMS ((tree));
extern void (*lang_expand_function_end)         PARAMS ((void));

/* Callback that determines if it's ok for a function to have no
   noreturn attribute.  */
extern int (*lang_missing_noreturn_ok_p)	PARAMS ((tree));


extern stmt_tree current_stmt_tree              PARAMS ((void));
extern tree *current_scope_stmt_stack           PARAMS ((void));
extern void begin_stmt_tree                     PARAMS ((tree *));
extern tree add_stmt				PARAMS ((tree));
extern void add_decl_stmt                       PARAMS ((tree));
extern tree add_scope_stmt                      PARAMS ((int, int));
extern void finish_stmt_tree                    PARAMS ((tree *));

extern int statement_code_p                     PARAMS ((enum tree_code));
extern tree walk_stmt_tree			PARAMS ((tree *,
							 walk_tree_fn,
							 void *));
extern void prep_stmt                           PARAMS ((tree));
extern void expand_stmt                         PARAMS ((tree));
extern void mark_stmt_tree                      PARAMS ((void *));
extern void shadow_warning			PARAMS ((const char *,
							 tree, tree));
extern tree c_begin_if_stmt			PARAMS ((void));
extern tree c_begin_while_stmt			PARAMS ((void));
extern void c_finish_while_stmt_cond		PARAMS ((tree, tree));


/* Extra information associated with a DECL.  Other C dialects extend
   this structure in various ways.  The C front-end only uses this
   structure for FUNCTION_DECLs; all other DECLs have a NULL
   DECL_LANG_SPECIFIC field.  */

struct c_lang_decl {
  unsigned declared_inline : 1;
};

/* In a FUNCTION_DECL for which DECL_BUILT_IN does not hold, this is
     the approximate number of statements in this function.  There is
     no need for this number to be exact; it is only used in various
     heuristics regarding optimization.  */
#define DECL_NUM_STMTS(NODE) ¥
  (FUNCTION_DECL_CHECK (NODE)->decl.u1.i)

extern void c_mark_lang_decl                    PARAMS ((struct c_lang_decl *));

/* The variant of the C language being processed.  Each C language
   front-end defines this variable.  */

extern c_language_kind c_language;

/* Nonzero means give string constants the type `const char *', rather
   than `char *'.  */

extern int flag_const_strings;

/* Nonzero means give `double' the same size as `float'.  */

extern int flag_short_double;

/* Nonzero means give `wchar_t' the same size as `short'.  */

extern int flag_short_wchar;

/* Warn about *printf or *scanf format/argument anomalies.  */

extern int warn_format;

/* Warn about Y2K problems with strftime formats.  */

extern int warn_format_y2k;

/* Warn about excess arguments to formats.  */

extern int warn_format_extra_args;

/* Warn about non-literal format arguments.  */

extern int warn_format_nonliteral;

/* Warn about possible security problems with calls to format functions.  */

extern int warn_format_security;

/* Warn about possible violations of sequence point rules.  */

extern int warn_sequence_point;

/* Warn about functions which might be candidates for format attributes.  */

extern int warn_missing_format_attribute;

/* Nonzero means warn about sizeof (function) or addition/subtraction
   of function pointers.  */

extern int warn_pointer_arith;

/* Nonzero means to warn about compile-time division by zero.  */
extern int warn_div_by_zero;

/* Nonzero means do some things the same way PCC does.  */

extern int flag_traditional;

/* Nonzero means enable C89 Amendment 1 features.  */

extern int flag_isoc94;

/* Nonzero means use the ISO C99 dialect of C.  */

extern int flag_isoc99;

/* Nonzero means environment is hosted (i.e., not freestanding) */

extern int flag_hosted;

/* Nonzero means add default format_arg attributes for functions not
   in ISO C.  */

extern int flag_noniso_default_format_attributes;

/* Nonzero means don't recognize any builtin functions.  */

extern int flag_no_builtin;

/* Nonzero means don't recognize the non-ANSI builtin functions.
   -ansi sets this.  */

extern int flag_no_nonansi_builtin;

/* Nonzero means warn about suggesting putting in ()'s.  */

extern int warn_parentheses;

/* Warn if a type conversion is done that might have confusing results.  */

extern int warn_conversion;

/* Nonzero means warn about usage of long long,
   when `-pedantic' and not C99.  */

extern int warn_long_long;

/* C types are partitioned into three subse