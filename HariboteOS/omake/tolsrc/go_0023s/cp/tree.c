/* Language-dependent node constructors for parse phase of GNU compiler.
   Copyright (C) 1987, 1988, 1992, 1993, 1994, 1995, 1996, 1997, 1998,
   1999, 2000, 2001, 2002 Free Software Foundation, Inc.
   Hacked by Michael Tiemann (tiemann@cygnus.com)

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
#include "../include/obstack.h"
#include "../gcc/tree.h"
#include "cp-tree.h"
#include "../gcc/flags.h"
#include "../gcc/rtl.h"
#include "../gcc/toplev.h"
#include "../gcc/ggc.h"
#include "../gcc/insn-config.h"
#include "../gcc/integrate.h"
#include "../gcc/tree-inline.h"
/* end of !kawai! */

static tree bot_manip PARAMS ((tree *, int *, void *));
static tree bot_replace PARAMS ((tree *, int *, void *));
static tree build_cplus_array_type_1 PARAMS ((tree, tree));
static int list_hash_eq PARAMS ((const void *, const void *));
static hashval_t list_hash_pieces PARAMS ((tree, tree, tree));
static hashval_t list_hash PARAMS ((const void *));
static cp_lvalue_kind lvalue_p_1 PARAMS ((tree, int));
static tree no_linkage_helper PARAMS ((tree *, int *, void *));
static tree build_srcloc PARAMS ((const char *, int));
static tree mark_local_for_remap_r PARAMS ((tree *, int *, void *));
static tree cp_unsave_r PARAMS ((tree *, int *, void *));
static void cp_unsave PARAMS ((tree *));
static tree build_target_expr PARAMS ((tree, tree));
static tree count_trees_r PARAMS ((tree *, int *, void *));
static tree verify_stmt_tree_r PARAMS ((tree *, int *, void *));
static tree find_tree_r PARAMS ((tree *, int *, void *));
extern int cp_statement_code_p PARAMS ((enum tree_code));

static tree handle_java_interface_attribute PARAMS ((tree *, tree, tree, int, bool *));
static tree handle_com_interface_attribute PARAMS ((tree *, tree, tree, int, bool *));
static tree handle_init_priority_attribute PARAMS ((tree *, tree, tree, int, bool *));

/* If REF is an lvalue, returns the kind of lvalue that REF is.
   Otherwise, returns clk_none.  If TREAT_CLASS_RVALUES_AS_LVALUES is
   non-zero, rvalues of class type are considered lvalues.  */

static cp_lvalue_kind
lvalue_p_1 (ref, treat_class_rvalues_as_lvalues)
     tree ref;
     int treat_class_rvalues_as_lvalues;
{
  cp_lvalue_kind op1_lvalue_kind = clk_none;
  cp_lvalue_kind op2_lvalue_kind = clk_none;

  if (TREE_CODE (TREE_TYPE (ref)) == REFERENCE_TYPE)
    return clk_ordinary;

  if (ref == current_class_ptr)
    return clk_none;

  switch (TREE_CODE (ref))
    {
      /* preincrements and predecrements are valid lvals, provided
	 what they refer to are valid lvals.  */
    case PREINCREMENT_EXPR:
    case PREDECREMENT_EXPR:
    case SAVE_EXPR:
    case UNSAVE_EXPR:
    case TRY_CATCH_EXPR:
    case WITH_CLEANUP_EXPR:
    case REALPART_EXPR:
    case IMAGPART_EXPR:
      /* This shouldn't be here, but there are lots of places in the compiler
         that are sloppy about tacking on NOP_EXPRs to the same type when
	 no actual conversion is happening.  */
    case NOP_EXPR:
      return lvalue_p_1 (TREE_OPERAND (ref, 0),
			 treat_class_rvalues_as_lvalues);

    case COMPONENT_REF:
      op1_lvalue_kind = lvalue_p_1 (TREE_OPERAND (ref, 0),
				    treat_class_rvalues_as_lvalues);
      if (op1_lvalue_kind 
	  /* The "field" can be a FUNCTION_DECL or an OVERLOAD in some
	     situations.  */
	  && TREE_CODE (TREE_OPERAND (ref, 1)) == FIELD_DECL
	  && DECL_C_BIT_FIELD (TREE_OPERAND (ref, 1)))
	{
	  /* Clear the ordinary bit.  If this object was a class
	     rvalue we want to preserve that information.  */
	  op1_lvalue_kind &= ‾clk_ordinary;
	  /* The lvalue is for a btifield.  */
	  op1_lvalue_kind |= clk_bitfield;
	}
      return op1_lvalue_kind;

    case STRING_CST:
      return clk_ordinary;

    case VAR_DECL:
      if (TREE_READONLY (ref) && ! TREE_STATIC (ref)
	  && DECL_LANG_SPECIFIC (ref)
	  && DECL_IN_AGGR_P (ref))
	return clk_none;
    case INDIRECT_REF:
    case ARRAY_REF:
    case PARM_DECL:
    case RESULT_DECL:
      if (TREE_CODE (TREE_TYPE (ref)) != METHOD_TYPE)
	return clk_ordinary;
      break;

      /* A currently unresolved scope ref.  */
    case SCOPE_REF:
      abort ();
    case OFFSET_REF:
      if (TREE_CODE (TREE_OPERAND (ref, 1)) == FUNCTION_DECL)
	return clk_ordinary;
      /* Fall through.  */
    case MAX_EXPR:
    case MIN_EXPR:
      op1_lvalue_kind = lvalue_p_1 (TREE_OPERAND (ref, 0),
				    treat_class_rvalues_as_lvalues);
      op2_lvalue_kind = lvalue_p_1 (TREE_OPERAND (ref, 1),
				    treat_class_rvalues_as_lvalues);
      break;

    case COND_EXPR:
      op1_lvalue_kind = lvalue_p_1 (TREE_OPERAND (ref, 1),
				    treat_class_rvalues_as_lvalues);
      op2_lvalue_kind = lvalue_p_1 (TREE_OPERAND (ref, 2),
				    treat_class_rvalues_as_lvalues);
      break;

    case MODIFY_EXPR:
      return clk_ordinary;

    case COMPOUND_EXPR:
      return lvalue_p_1 (TREE_OPERAND (ref, 1),
			 treat_class_rvalues_as_lvalues);

    case TARGET_EXPR:
      return treat_class_rvalues_as_lvalues ? clk_class : clk_none;

    case CALL_EXPR:
    case VA_ARG_EXPR:
      return ((treat_class_rvalues_as_lvalues
	       && IS_AGGR_TYPE (TREE_TYPE (ref)))
	      ? clk_class : clk_none);

    case FUNCTION_DECL:
      /* All functions (except non-static-member functions) are
	 lvalues.  */
      return (DECL_NONSTATIC_MEMBER_FUNCTION_P (ref) 
	      ? clk_none : clk_ordinary);

    default:
      break;
    }

  /* If one operand is not an lvalue at all, then this expression is
     not an lvalue.  */
  if (!op1_lvalue_kind || !op2_lvalue_kind)
    return clk_none;

  /* Otherwise, it's an lvalue, and it has all the odd properties
     contributed by either operand.  */
  op1_lvalue_kind = op1_lvalue_kind | op2_lvalue_kind;
  /* It's not an ordinary lvalue if it involves either a bit-field or
     a class rvalue.  */
  if ((op1_lvalue_kind & ‾clk_ordinary) != clk_none)
    op1_lvalue_kind &= ‾clk_ordinary;
  return op1_lvalue_kind;
}

/* If REF is an lvalue, returns the kind of lvalue that REF is.
   Otherwise, returns clk_none.  Lvalues can be assigned, unless they
   have TREE_READONLY, or unless they are FUNCTION_DECLs.  Lvalues can
   have their address taken, unless they have DECL_REGISTER.  */

cp_lvalue_kind
real_lvalue_p (ref)
     tree ref;
{
  return lvalue_p_1 (ref, /*treat_class_rvalues_as_lvalues=*/0);
}

/* This differs from real_lvalue_p in that class rvalues are
   considered lvalues.  */

int
lvalue_p (ref)
     tree ref;
{
  return 
    (lvalue_p_1 (ref, /*treat_class_rvalues_as_lvalues=*/1) != clk_none);
}

/* Return nonzero if REF is an lvalue valid for this language;
   otherwise, print an error message and return zero.  */

int
lvalue_or_else (ref, string)
     tree ref;
     const char *string;
{
  int win = lvalue_p (ref);
  if (! win)
    error ("non-lvalue in %s", string);
  return win;
}

/* Build a TARGET_EXPR, initializing the DECL with the VALUE.  */

static tree
build_target_expr (decl, value)
     tree decl;
     tree value;
{
  tree t;

  t = build (TARGET_EXPR, TREE_TYPE (decl), decl, value, 
	     maybe_build_cleanup (decl), NULL_TREE);
  /* We always set TREE_SIDE_EFFECTS so that expand_expr does not
     ignore the TARGET_EXPR.  If there really turn out to be no
     side-effects, then the optimizer should be able to 