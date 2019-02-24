/* Process declarations and variables for C compiler.
   Copyright (C) 1988, 1992, 1993, 1994, 1995, 1996, 1997, 1998, 1999, 2000,
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

/* Process declarations and symbol lookup for C front end.
   Also constructs types; the standard scalar types at initialization,
   and structure, union, array and enum types when they are declared.  */

/* ??? not all decl nodes are given the most useful possible
   line numbers.  For example, the CONST_DECLs for enum values.  */

#include "config.h"
#include "system.h"
#include "intl.h"
#include "tree.h"
#include "tree-inline.h"
#include "rtl.h"
#include "flags.h"
#include "function.h"
#include "output.h"
#include "expr.h"
#include "c-tree.h"
#include "c-lex.h"
#include "toplev.h"
#include "ggc.h"
#include "tm_p.h"
#include "cpplib.h"
#include "target.h"
#include "debug.h"
#include "timevar.h"
#include "c-common.h"
#include "c-pragma.h"

/* In grokdeclarator, distinguish syntactic contexts of declarators.  */
enum decl_context
{ NORMAL,			/* Ordinary declaration */
  FUNCDEF,			/* Function definition */
  PARM,				/* Declaration of parm before function body */
  FIELD,			/* Declaration inside struct or union */
  BITFIELD,			/* Likewise but with specified width */
  TYPENAME};			/* Typename (inside cast or sizeof)  */


/* Nonzero if we have seen an invalid cross reference
   to a struct, union, or enum, but not yet printed the message.  */

tree pending_invalid_xref;
/* File and line to appear in the eventual error message.  */
const char *pending_invalid_xref_file;
int pending_invalid_xref_line;

/* While defining an enum type, this is 1 plus the last enumerator
   constant value.  Note that will do not have to save this or `enum_overflow'
   around nested function definition since such a definition could only
   occur in an enum value expression and we don't use these variables in
   that case.  */

static tree enum_next_value;

/* Nonzero means that there was overflow computing enum_next_value.  */

static int enum_overflow;

/* Parsing a function declarator leaves a list of parameter names
   or a chain or parameter decls here.  */

static tree last_function_parms;

/* Parsing a function declarator leaves here a chain of structure
   and enum types declared in the parmlist.  */

static tree last_function_parm_tags;

/* After parsing the declarator that starts a function definition,
   `start_function' puts here the list of parameter names or chain of decls.
   `store_parm_decls' finds it here.  */

static tree current_function_parms;

/* Similar, for last_function_parm_tags.  */
static tree current_function_parm_tags;

/* Similar, for the file and line that the prototype came from if this is
   an old-style definition.  */
static const char *current_function_prototype_file;
static int current_function_prototype_line;

/* The current statement tree.  */

static struct stmt_tree_s c_stmt_tree;

/* The current scope statement stack.  */

static tree c_scope_stmt_stack;

/* A list (chain of TREE_LIST nodes) of all LABEL_DECLs in the function
   that have names.  Here so we can clear out their names' definitions
   at the end of the function.  */

static tree named_labels;

/* A list of LABEL_DECLs from outer contexts that are currently shadowed.  */

static tree shadowed_labels;

/* Nonzero when store_parm_decls is called indicates a varargs function.
   Value not meaningful after store_parm_decls.  */

static int c_function_varargs;

/* Set to 0 at beginning of a function definition, set to 1 if
   a return statement that specifies a return value is seen.  */

int current_function_returns_value;

/* Set to 0 at beginning of a function definition, set to 1 if
   a return statement with no argument is seen.  */

int current_function_returns_null;

/* Set to 0 at beginning of a function definition, set to 1 if
   a call to a noreturn function is seen.  */

int current_function_returns_abnormally;

/* Set to nonzero by `grokdeclarator' for a function
   whose return type is defaulted, if warnings for this are desired.  */

static int warn_about_return_type;

/* Nonzero when starting a function declared `extern inline'.  */

static int current_extern_inline;

/* For each binding contour we allocate a binding_level structure
 * which records the names defined in that contour.
 * Contours include:
 *  0) the global one
 *  1) one for each function definition,
 *     where internal declarations of the parameters appear.
 *  2) one for each compound statement,
 *     to record its declarations.
 *
 * The current meaning of a name can be found by searching the levels from
 * the current one out to the global one.
 */

/* Note that the information in the `names' component of the global contour
   is duplicated in the IDENTIFIER_GLOBAL_VALUEs of all identifiers.  */

struct binding_level
  {
    /* A chain of _DECL nodes for all variables, constants, functions,
       and typedef types.  These are in the reverse of the order supplied.
     */
    tree names;

    /* A list of structure, union and enum definitions,
     * for looking up tag names.
     * It is a chain of TREE_LIST nodes, each of whose TREE_PURPOSE is a name,
     * or NULL_TREE; and whose TREE_VALUE is a RECORD_TYPE, UNION_TYPE,
     * or ENUMERAL_TYPE node.
     */
    tree tags;

    /* For each level, a list of shadowed outer-level local definitions
       to be restored when this level is popped.
       Each link is a TREE_LIST whose TREE_PURPOSE is an identifier and
       whose TREE_VALUE is its old definition (a kind of ..._DECL node).  */
    tree shadowed;

    /* For each level (except not the global one),
       a chain of BLOCK nodes for all the levels
       that were entered and exited one level down.  */
    tree blocks;

    /* The BLOCK node for this level, if one has been preallocated.
       If 0, the BLOCK is allocated (if needed) when the level is popped.  */
    tree this_block;

    /* The binding level which this one is contained in (inherits from).  */
    struct binding_level *level_chain;

    /* Nonzero for the level that holds the parameters of a function.  */
    char parm_flag;

    /* Nonzero if this level "doesn't exist" for tags.  */
    char tag_transparent;

    /* Nonzero if sublevels of this level "don't exist" for tags.
       This is set in the parm level of a function definition
       while reading the function body, so that the outermost block
       of the function body will be tag-transparent.  */
    char subblocks_tag_transparent;

    /* Nonzero means make a BLOCK for this level regardless of all else.  */
    char keep;

    /* Nonzero means make a BLOCK if this level has any subblocks.  */
    char keep_if_subblocks;

    /* Number of decls in `names' that have incomplete
       structure or union types.  */
    int n_incomplete;

    /* A list of decls giving the (reversed) specified order of parms,
       not including any forward-decls in the parmlist.
       This is so we can put the parms in proper order for assign_parms.  */
    tree parm_order;
  };

#define NULL_BINDING_LEVEL (struct binding_level *) NULL

/* The binding level currently in effect.  */

static struct binding_level *current_binding_level;

/* A chain of binding_level structures awaiting reuse.  */

static struct binding_level *free_binding_level;

/* The outermost binding level, for names of file scope.
   This is created when the compiler is started and exists
   through the entire run.  */

static struct binding_level *global_binding_level;

/* Binding level structures are initialized by copying this one.  */

static struct binding_level clear_binding_level
  = {NULL, NULL, NULL, NULL, NULL, NULL_BINDING_LEVEL, 0, 0, 0, 0, 0, 0,
     NULL};

/* Nonzero means unconditionally make a BLOCK for the next level pushed.  */

static int keep_next_level_flag;

/* Nonzero means make a BLOCK for the next level pushed
   if it has subblocks.  */

static int keep_next_if_subblocks;

/* The chain of outer levels of label scopes.
   This uses the same data structure used for binding levels,
   but it works differently: each link in the chain records
   saved values of named_labels and shadowed_labels for
   a label binding level outside the current one.  */

static struct binding_level *label_level_chain;

/* Functions called automatically at the beginning and end of execution.  */

tree static_ctors, static_dtors;

/* Forward declarations.  */

static struct binding_level * make_binding_level	PARAMS ((void));
static void mark_binding_level		PARAMS ((void *));
static void clear_limbo_values		PARAMS ((tree));
static int duplicate_decls		PARAMS ((tree, tree, int));
static int redeclaration_error_message	PARAMS ((tree, tree));
static void storedecls			PARAMS ((tree));
static void storetags			PARAMS ((tree));
static tree lookup_tag			PARAMS ((enum tree_code, tree,
						 struct binding_level *, int));
static tree lookup_tag_reverse		PARAMS ((tree));
static tree grokdeclarator		PARAMS ((tree, tree, enum decl_context,
						 int));
static tree grokparms			PARAMS ((tree, int));
static void layout_array_type		PARAMS ((tree));
static tree c_make_fname_decl           PARAMS ((tree, int));
static void c_expand_body               PARAMS ((tree, int, int));
static void warn_if_shadowing		PARAMS ((tree, tree));

/* C-specific option variables.  */

/* Nonzero means allow type mismatches in conditional expressions;
   just make their values `void'.  */

int flag_cond_mismatch;

/* Nonzero means don't recognize the keyword `asm'.  */

int flag_no_asm;

/* Nonzero means do some things the same way PCC does.  */

int flag_traditional;

/* Nonzero means enable C89 Amendment 1 features.  */

int flag_isoc94 = 0;

/* Nonzero means use the ISO C99 dialect of C.  */

int flag_isoc99 = 0;

/* Nonzero means that we have builtin functions, and main is an int */

int flag_hosted = 1;

/* Nonzero means add default format_arg attributes for functions not
   in ISO C.  */

int flag_noniso_default_format_attributes = 1;

/* Nonzero means to allow single precision math even if we're generally
   being traditional.  */
int flag_allow_single_precision = 0;

/* Nonzero means to treat bitfields as signed unless they say `unsigned'.  */

int flag_signed_bitfields = 1;
int explicit_flag_signed_bitfields = 0;

/* Nonzero means warn about use of implicit int.  */

int warn_implicit_int;

/* Nonzero means warn about usage of long long when `-pedantic'.  */

int warn_long_long = 1;

/* Nonzero means message about use of implicit function declarations;
 1 means warning; 2 means error.  */

int mesg_implicit_function_declaration = -1;

/* Nonzero means give string constants the type `const char *'
   to get extra warnings from them.  These warnings will be too numerous
   to be useful, except in thoroughly ANSIfied programs.  */

int flag_const_strings;

/* Nonzero means warn about pointer casts that can drop a type qualifier
   from the pointer target type.  */

int warn_cast_qual;

/* Nonzero means warn when casting a function call to a type that does
   not match the return type (e.g. (float)sqrt() or (anything*)malloc()
   when there is no previous declaration of sqrt or malloc.  */

int warn_bad_function_cast;

/* Warn abou