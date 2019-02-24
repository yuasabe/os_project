/* Front-end tree definitions for GNU compiler.
   Copyright (C) 1989, 1993, 1994, 1995, 1996, 1997, 1998, 1999, 2000,
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

#include "machmode.h"
#include "version.h"

/* Codes of tree nodes */

#define DEFTREECODE(SYM, STRING, TYPE, NARGS)   SYM,

enum tree_code {
#include "tree.def"

  LAST_AND_UNUSED_TREE_CODE	/* A convenient way to get a value for
				   NUM_TREE_CODE.  */
};

#undef DEFTREECODE

/* Number of language-independent tree codes.  */
#define NUM_TREE_CODES ((int) LAST_AND_UNUSED_TREE_CODE)

/* Indexed by enum tree_code, contains a character which is
   `<' for a comparison expression, `1', for a unary arithmetic
   expression, `2' for a binary arithmetic expression, `e' for
   other types of expressions, `r' for a reference, `c' for a
   constant, `d' for a decl, `t' for a type, `s' for a statement,
   and `x' for anything else (TREE_LIST, IDENTIFIER, etc).  */

#define MAX_TREE_CODES 256
extern char tree_code_type[MAX_TREE_CODES];
#define TREE_CODE_CLASS(CODE)	tree_code_type[(int) (CODE)]

/* Returns non-zero iff CLASS is the tree-code class of an
   expression.  */

#define IS_EXPR_CODE_CLASS(CLASS) ¥
  ((CLASS) == '<' || (CLASS) == '1' || (CLASS) == '2' || (CLASS) == 'e')

/* Number of argument-words in each kind of tree-node.  */

extern int tree_code_length[MAX_TREE_CODES];
#define TREE_CODE_LENGTH(CODE)	tree_code_length[(int) (CODE)]

/* Names of tree components.  */

extern const char *tree_code_name[MAX_TREE_CODES];

/* Classify which part of the compiler has defined a given builtin function.
   Note that we assume below that this is no more than two bits.  */
enum built_in_class
{
  NOT_BUILT_IN = 0,
  BUILT_IN_FRONTEND,
  BUILT_IN_MD,
  BUILT_IN_NORMAL
};

/* Names for the above.  */
extern const char *const built_in_class_names[4];

/* Codes that identify the various built in functions
   so that expand_call can identify them quickly.  */

#define DEF_BUILTIN(ENUM, N, C, T, LT, B, F, NA) ENUM,
enum built_in_function
{
#include "builtins.def"

  /* Upper bound on non-language-specific builtins.  */
  END_BUILTINS
};
#undef DEF_BUILTIN

/* Names for the above.  */
extern const char *const built_in_names[(int) END_BUILTINS];

/* An array of _DECL trees for the above.  */
extern tree built_in_decls[(int) END_BUILTINS];

/* The definition of tree nodes fills the next several pages.  */

/* A tree node can represent a data type, a variable, an expression
   or a statement.  Each node has a TREE_CODE which says what kind of
   thing it represents.  Some common codes are:
   INTEGER_TYPE -- represents a type of integers.
   ARRAY_TYPE -- represents a type of pointer.
   VAR_DECL -- represents a declared variable.
   INTEGER_CST -- represents a constant integer value.
   PLUS_EXPR -- represents a sum (an expression).

   As for the contents of a tree node: there are some fields
   that all nodes share.  Each TREE_CODE has various special-purpose
   fields as well.  The fields of a node are never accessed directly,
   always through accessor macros.  */

/* Every kind of tree node starts with this structure,
   so all nodes have these fields.

   See the accessor macros, defined below, for documentation of the
   fields.  */

struct tree_common
{
  tree chain;
  tree type;

  ENUM_BITFIELD(tree_code) code : 8;

  unsigned side_effects_flag : 1;
  unsigned constant_flag : 1;
  unsigned addressable_flag : 1;
  unsigned volatile_flag : 1;
  unsigned readonly_flag : 1;
  unsigned unsigned_flag : 1;
  unsigned asm_written_flag: 1;
  unsigned unused_0 : 1;

  unsigned used_flag : 1;
  unsigned nothrow_flag : 1;
  unsigned static_flag : 1;
  unsigned public_flag : 1;
  unsigned private_flag : 1;
  unsigned protected_flag : 1;
  unsigned bounded_flag : 1;
  unsigned deprecated_flag : 1;

  unsigned lang_flag_0 : 1;
  unsigned lang_flag_1 : 1;
  unsigned lang_flag_2 : 1;
  unsigned lang_flag_3 : 1;
  unsigned lang_flag_4 : 1;
  unsigned lang_flag_5 : 1;
  unsigned lang_flag_6 : 1;
  unsigned unused_1 : 1;
};

/* The following table lists the uses of each of the above flags and
   for which types of nodes they are defined.  Note that expressions
   include decls.

   addressable_flag:

       TREE_ADDRESSABLE in
   	   VAR_DECL, FUNCTION_DECL, FIELD_DECL, CONSTRUCTOR, LABEL_DECL,
	   ..._TYPE, IDENTIFIER_NODE.
	   In a STMT_EXPR, it means we want the result of the enclosed
	   expression.

   static_flag:

       TREE_STATIC in
           VAR_DECL, FUNCTION_DECL, CONSTRUCTOR, ADDR_EXPR
       TREE_NO_UNUSED_WARNING in
           CONVERT_EXPR, NOP_EXPR, COMPOUND_EXPR
       TREE_VIA_VIRTUAL in
           TREE_LIST or TREE_VEC
       TREE_CONSTANT_OVERFLOW in
           INTEGER_CST, REAL_CST, COMPLEX_CST, VECTOR_CST
       TREE_SYMBOL_REFERENCED in
           IDENTIFIER_NODE
       CLEANUP_EH_ONLY in
           TARGET_EXPR, WITH_CLEANUP_EXPR, CLEANUP_STMT,
	   TREE_LIST elements of a block's cleanup list.

   public_flag:

       TREE_OVERFLOW in
           INTEGER_CST, REAL_CST, COMPLEX_CST, VECTOR_CST
       TREE_PUBLIC in
           VAR_DECL or FUNCTION_DECL or IDENTIFIER_NODE
       TREE_VIA_PUBLIC in
           TREE_LIST or TREE_VEC
       EXPR_WFL_EMIT_LINE_NOTE in
           EXPR_WITH_FILE_LOCATION

   private_flag:

       TREE_VIA_PRIVATE in
           TREE_LIST or TREE_VEC
       TREE_PRIVATE in
           ..._DECL

   protected_flag:

       TREE_VIA_PROTECTED in
           TREE_LIST
	   TREE_VEC
       TREE_PROTECTED in
           BLOCK
	   ..._DECL

   side_effects_flag:

       TREE_SIDE_EFFECTS in
           all expressions

   volatile_flag:

       TREE_THIS_VOLATILE in
           all expressions
       TYPE_VOLATILE in
           ..._TYPE

   readonly_flag:

       TREE_READONLY in
           all expressions
       TYPE_READONLY in
           ..._TYPE

   constant_flag:

       TREE_CONSTANT in
           all expressions

   unsigned_flag:

       TREE_UNSIGNED in
           INTEGER_TYPE, ENUMERAL_TYPE, FIELD_DECL
       DECL_BUILT_IN_NONANSI in
           FUNCTION_DECL
       SAVE_EXPR_NOPLACEHOLDER in
	   SAVE_EXPR

   asm_written_flag:

       TREE_ASM_WRITTEN in
           VAR_DECL, FUNCTION_DECL, RECORD_TYPE, UNION_TYPE, QUAL_UNION_TYPE
	   BLOCK

   used_flag:

       TREE_USED in
           expressions, IDENTIFIER_NODE

   nothrow_flag:

       TREE_NOTHROW in
           CALL_EXPR, FUNCTION_DECL

   bounded_flag:

       TREE_BOUNDED in
	   expressions, VAR_DECL, PARM_DECL, FIELD_DECL, FUNCTION_DECL,
	   IDENTIFIER_NODE
       TYPE_BOUNDED in
	   ..._TYPE

   deprecated_flag:

	TREE_DEPRECATED in
	   ..._DECL
*/

/* Define accessors for the fields that all tree nodes have
   (though some fields are not used for all kinds of nodes).  */

/* The tree-code says what kind of node it is.
   Codes are defined in tree.def.  */
#define TREE_CODE(NODE) ((enum tree_code) (NODE)->common.code)
#define TREE_SET_CODE(NODE, VALUE) ¥
((NODE)->common.code = (ENUM_BITFIELD (tree_code)) (VALUE))

/* When checking is enabled, errors will be generated if a tree node
   is accessed incorrectly. The macros abort with a fatal error.  */
#if defined ENABLE_TREE_CHECKING && (GCC_VERSION >= 2007)

#define TREE_CHECK(t, code) __extension__				¥
({  const tree __t = (t);						¥
    if (TREE_CODE(__t) != (code))					¥
      tree_check_failed (__t, code, __FILE__, __LINE__, __FUNCTION__);	¥
    __t; })
#define TREE_CLASS_CHECK(t, class) __extension__			¥
({  const tree __t = (t);						¥
    if (TREE_CODE_CLASS(TREE_CODE(__t)) != (class))			¥
      tree_class_check_failed (__t, class, __FILE__, __LINE__,		¥
			       __FUNCTION__);				¥
    __t; })

/* These checks have to be special cased.  */
#define CST_OR_CONSTRUCTOR_CHECK(t) __extension__			¥
({  const tree __t = (t);						¥
    enum tree_code const __c = TREE_CODE(__t);				¥
    if (__c != CONSTRUCTOR && TREE_CODE_CLASS(__c) != 'c')		¥
      tree_check_failed (__t, CONSTRUCTOR, __FILE__, __LINE__,		¥
			 __FUNCTION__);					¥
    __t; })
#define EXPR_CHECK(t) __extension__					¥
({  const tree __t = (t);						¥
    char const __c = TREE_CODE_CLASS(TREE_CODE(__t));			¥
    if (__c != 'r' && __c != 's' && __c != '<'				¥
	&& __c != '1' && __c != '2' && __c != 'e')			¥
      tree_class_check_failed (__t, 'e', __FILE__, __LINE__,		¥
			       __FUNCTION__);				¥
    __t; })

extern void tree_check_failed PARAMS ((const tree, enum tree_code,
				       const char *, int, const char *))
    ATTRIBUTE_NORETURN;
extern void tree_class_check_failed PARAMS ((const tree, int,
					     const char *, int, const char *))
    ATTRIBUTE_NORETURN;

#else /* not ENABLE_TREE_CHECKING, or not gcc */

#define TREE_CHECK(t, code)		(t)
#define TREE_CLASS_CHECK(t, code)	(t)
#define CST_OR_CONSTRUCTOR_CHECK(t)	(t)
#define EXPR_CHECK(t)			(t)

#endif

#include "tree-check.h"

#define TYPE_CHECK(tree)	TREE_CLASS_CHECK  (tree, 't')
#define DECL_CHECK(tree)	TREE_CLASS_CHECK  (tree, 'd')
#define CST_CHECK(tree)		TREE_CLASS_CHECK  (tree, 'c')

/* In all nodes that are expressions, this is the data type of the expression.
   In POINTER_TYPE nodes, this is the type that the pointer points to.
   In ARRAY_TYPE nodes, this is the type of the elements.
   In VECTOR_TYPE nodes, this is the type of the elements.  */
#define TREE_TYPE(NODE) ((NODE)->common.type)

/* Here is how primitive or already-canonicalized types' hash codes
   are made.  */
#define TYPE_HASH(TYPE) ((size_t) (TYPE) & 0777777)

/* Nodes are chained together for many purposes.
   Types are chained together to record them for being output to the debugger
   (see the function `chain_type').
   Decls in the same scope are chained together to record the contents
   of the scope.
   Statement nodes for successive statements used to be chained together.
   Often lists of things are represented by TREE_LIST nodes that
   are chained together.  */

#define TREE_CHAIN(NODE) ((NODE)->common.chain)

/* Given an expression as a tree, strip any NON_LVALUE_EXPRs and NOP_EXPRs
   that don't change the machine mode.  */

#define STRIP_NOPS(EXP)						¥
  while ((TREE_CODE (EXP) == NOP_EXPR				¥
	  || TREE_CODE (EXP) == CONVERT_EXPR			¥
	  || TREE_CODE (EXP) == NON_LVALUE_EXPR)		¥
	 && TREE_OPERAND (EXP, 0) != error_mark_node		¥
	 && (TYPE_MODE (TREE_TYPE (EXP))			¥
	     == TYPE_MODE (TREE_TYPE (TREE_OPERAND (EXP, 0)))))	¥
    (EXP) = TREE_OPERAND (EXP, 0)

/* Like STRIP_NOPS, but don't let the signedness change either.  */

#define STRIP_SIGN_NOPS(EXP) ¥
  while ((TREE_CODE (EXP) == NOP_EXPR				¥
	  || TREE_CODE (EXP) == CONVERT_EXPR			¥
	  || TREE_CODE (EXP) == NON_LVALUE_EXPR)		¥
	 && TREE_OPERAND (EXP, 0) != error_mark_node		¥
	 && (TYPE_MODE (TREE_TYPE (EXP))			¥
	     == TYPE_MODE (TREE_TYPE (TREE_OPERAND (EXP, 0))))	¥
	 && (TREE_UNSIGNED (TREE_TYPE (EXP))			¥
	     == TREE_UNSIGNED (TREE_TYPE (TREE_OPERAND (EXP, 0))))) ¥
    (EXP) = TREE_OPERAND (EXP, 0)

/* Like STRIP_NOPS, but don't alter the TREE_TYPE either.  */

#define STRIP_TYPE_NOPS(EXP) ¥
  while ((TREE_CODE (EXP) == NOP_EXPR				¥
	  || TREE_CODE (EXP) == CONVERT_EXPR			¥
	  || TREE_CODE (EXP) == NON_LVALUE_EXPR)		¥
	 && TREE_OPERAND (EXP, 0) != error_mark_node		¥
	 && (TREE_TYPE (EXP)					¥
	     == TREE_TYPE (TREE_OPERAND