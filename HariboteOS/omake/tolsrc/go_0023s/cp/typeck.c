/* Build expressions with type checking for C++ compiler.
   Copyright (C) 1987, 1988, 1989, 1992, 1993, 1994, 1995, 1996, 1997, 1998,
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


/* This file is part of the C++ front end.
   It contains routines to build C++ expressions given their operands,
   including computing the types of the result, C and C++ specific error
   checks, and some optimization.

   There are also routines to build RETURN_STMT nodes and CASE_STMT nodes,
   and to process initializations in declarations (since they work
   like a strange sort of assignment).  */

/* !kawai! */
#include "../gcc/config.h"
#include "../gcc/system.h"
#include "../gcc/tree.h"
#include "../gcc/rtl.h"
#include "../gcc/expr.h"
#include "cp-tree.h"
#include "../gcc/tm_p.h"
#include "../gcc/flags.h"
#include "../gcc/output.h"
#include "../gcc/toplev.h"
#include "../gcc/diagnostic.h"
#include "../gcc/target.h"
/* end of !kawai! */

static tree convert_for_assignment PARAMS ((tree, tree, const char *, tree,
					  int));
static tree cp_pointer_int_sum PARAMS ((enum tree_code, tree, tree));
static tree rationalize_conditional_expr PARAMS ((enum tree_code, tree));
static int comp_target_parms PARAMS ((tree, tree));
static int comp_ptr_ttypes_real PARAMS ((tree, tree, int));
static int comp_ptr_ttypes_const PARAMS ((tree, tree));
static int comp_ptr_ttypes_reinterpret PARAMS ((tree, tree));
static int comp_except_types PARAMS ((tree, tree, int));
static int comp_array_types PARAMS ((int (*) (tree, tree, int), tree,
				   tree, int));
static tree common_base_type PARAMS ((tree, tree));
static tree lookup_anon_field PARAMS ((tree, tree));
static tree pointer_diff PARAMS ((tree, tree, tree));
static tree build_component_addr PARAMS ((tree, tree));
static tree qualify_type_recursive PARAMS ((tree, tree));
static tree get_delta_difference PARAMS ((tree, tree, int));
static int comp_cv_target_types PARAMS ((tree, tree, int));
static void casts_away_constness_r PARAMS ((tree *, tree *));
static int casts_away_constness PARAMS ((tree, tree));
static void maybe_warn_about_returning_address_of_local PARAMS ((tree));
static tree strip_all_pointer_quals PARAMS ((tree));

/* Return the target type of TYPE, which means return T for:
   T*, T&, T[], T (...), and otherwise, just T.  */

tree
target_type (type)
     tree type;
{
  if (TREE_CODE (type) == REFERENCE_TYPE)
    type = TREE_TYPE (type);
  while (TREE_CODE (type) == POINTER_TYPE
	 || TREE_CODE (type) == ARRAY_TYPE
	 || TREE_CODE (type) == FUNCTION_TYPE
	 || TREE_CODE (type) == METHOD_TYPE
	 || TREE_CODE (type) == OFFSET_TYPE)
    type = TREE_TYPE (type);
  return type;
}

/* Do `exp = require_complete_type (exp);' to make sure exp
   does not have an incomplete type.  (That includes void types.)
   Returns the error_mark_node if the VALUE does not have
   complete type when this function returns.  */

tree
require_complete_type (value)
     tree value;
{
  tree type;

  if (processing_template_decl || value == error_mark_node)
    return value;

  if (TREE_CODE (value) == OVERLOAD)
    type = unknown_type_node;
  else
    type = TREE_TYPE (value);

  /* First, detect a valid value with a complete type.  */
  if (COMPLETE_TYPE_P (type))
    return value;

  /* If we see X::Y, we build an OFFSET_TYPE which has
     not been laid out.  Try to avoid an error by interpreting
     it as this->X::Y, if reasonable.  */
  if (TREE_CODE (value) == OFFSET_REF
      && current_class_ref != 0
      && TREE_OPERAND (value, 0) == current_class_ref)
    {
      value = resolve_offset_ref (value);
      return require_complete_type (value);
    }

  if (complete_type_or_else (type, value))
    return value;
  else
    return error_mark_node;
}

/* Try to complete TYPE, if it is incomplete.  For example, if TYPE is
   a template instantiation, do the instantiation.  Returns TYPE,
   whether or not it could be completed, unless something goes
   horribly wrong, in which case the error_mark_node is returned.  */

tree
complete_type (type)
     tree type;
{
  if (type == NULL_TREE)
    /* Rather than crash, we return something sure to cause an error
       at some point.  */
    return error_mark_node;

  if (type == error_mark_node || COMPLETE_TYPE_P (type))
    ;
  else if (TREE_CODE (type) == ARRAY_TYPE && TYPE_DOMAIN (type))
    {
      tree t = complete_type (TREE_TYPE (type));
      if (COMPLETE_TYPE_P (t) && ! processing_template_decl)
	layout_type (type);
      TYPE_NEEDS_CONSTRUCTING (type)
	= TYPE_NEEDS_CONSTRUCTING (TYPE_MAIN_VARIANT (t));
      TYPE_HAS_NONTRIVIAL_DESTRUCTOR (type)
	= TYPE_HAS_NONTRIVIAL_DESTRUCTOR (TYPE_MAIN_VARIANT (t));
    }
  else if (CLASS_TYPE_P (type) && CLASSTYPE_TEMPLATE_INSTANTIATION (type))
    instantiate_class_template (TYPE_MAIN_VARIANT (type));

  return type;
}

/* Like complete_type, but issue an error if the TYPE cannot be
   completed.  VALUE is used for informative diagnostics.
   Returns NULL_TREE if the type cannot be made complete.  */

tree
complete_type_or_else (type, value)
     tree type;
     tree value;
{
  type = complete_type (type);
  if (type == error_mark_node)
    /* We already issued an error.  */
    return NULL_TREE;
  else if (!COMPLETE_TYPE_P (type))
    {
      incomplete_type_error (value, type);
      return NULL_TREE;
    }
  else
    return type;
}

/* Return truthvalue of whether type of EXP is instantiated.  */

int
type_unknown_p (exp)
     tree exp;
{
  return (TREE_CODE (exp) == OVERLOAD
          || TREE_CODE (exp) == TREE_LIST
	  || TREE_TYPE (exp) == unknown_type_node
	  || (TREE_CODE (TREE_TYPE (exp)) == OFFSET_TYPE
	      && TREE_TYPE (TREE_TYPE (exp)) == unknown_type_node));
}

/* Return a pointer or pointer to member type similar to T1, with a
   cv-qualification signature that is the union of the cv-qualification
   signatures of T1 and T2: [expr.rel], [expr.eq].  */

static tree
qualify_type_recursive (t1, t2)
     tree t1, t2;
{
  if ((TYPE_PTR_P (t1) && TYPE_PTR_P (t2))
      || (TYPE_PTRMEM_P (t1) && TYPE_PTRMEM_P (t2)))
    {
      tree tt1 = TREE_TYPE (t1);
      tree tt2 = TREE_TYPE (t2);
      tree b1;
      int type_quals;
      tree tgt;
      tree attributes = (*targetm.merge_type_attributes) (t1, t2);

      if (TREE_CODE (tt1) == OFFSET_TYPE)
	{
	  b1 = TYPE_OFFSET_BASETYPE (tt1);
	  tt1 = TREE_TYPE (tt1);
	  tt2 = TREE_TYPE (tt2);
	}
      else
	b1 = NULL_TREE;

      type_quals = (cp_type_quals (tt1) | cp_type_quals (tt2));
      tgt = qualify_type_recursive (tt1, tt2);
      tgt = cp_build_qualified_type (tgt, type_quals);
      if (b1)
	tgt = build_offset_type (b1, tgt);
      t1 = build_pointer_type (tgt);
      t1 = build_type_attribute_variant (t1, attributes);
    }
  return t1;
}

/* Return the common type of two parameter lists.
   We assume that comptypes has already been done and returned 1;
   if that isn't so, this may crash.

   As an optimization, free the space we allocate if the parameter
   lists are already common.  */

tree
commonparms (p1, p2)
     tree p1, p2;
{
  tree oldargs = p1, newargs, n;
  int i, len;
  int any_change = 0;

  len = list_length (p1);
  newargs = tree_last (p1);

  if (newargs == void_list_node)
    i = 1;
  else
    {
      i = 0;
      newargs = 0;
    }

  for (; i < len; i++)
    newargs = tree_cons (NULL_TREE, NULL_TREE, newargs);

  n = newargs;

  for (i = 0; p1;
       p1 = TREE_CHAIN (p1), p2 = TREE_CHAIN (p2), n = TREE_CHAIN (n), i++)
    {
      if (TREE_PURPOSE (p1) && !TREE_PURPOSE (p2))
	{
	  TREE_PURPOSE (n) = TREE_PURPOSE (p1);
	  any_change = 1;
	}
      else if (! TREE_PURPOSE (p1))
	{
	  if (TREE_PURPOSE (p2))
	    {
	      TREE_PURPOSE (n) = TREE_PURPOSE (p2);
	      any_change = 1;
	    }
	}
      else
	{
	  if (1 != simple_cst_equal (TREE_PURPOSE (p1), TREE_PURPOSE (p2)))
	    any_change = 1;
	  TREE_PURPOSE (n) = TREE_PURPOSE (p2);
	}
      if (TREE_VALUE (p1) != TREE_VALUE (p2))
	{
	  any_change = 1;
	  TREE_VALUE (n) = merge_types (TREE_VALUE (p1), TREE_VALUE (p2));
	}
      else
	TREE_VALUE (n) = TREE_VALUE (p1);
    }
  if (! any_change)
    return oldargs;

  return newargs;
}

/* Given a type, perhaps copied for a typedef,
   find the "original" version of it.  */
tree
original_type (t)
     tree t;
{
  while (TYPE_NAME (t) != NULL_TREE)
    {
      tree x = TYPE_NAME (t);
      if (TREE_CODE (x) != TYPE_DECL)
	break;
      x = DECL_ORIGINAL_TYPE (x);
      if (x == NULL_TREE)
	break;
      t = x;
    }
  return t;
}

/* T1 and T2 are arithmetic or enumeration types.  Return the type
   that will result from the "usual arithmetic conversions" on T1 and
   T2 as described in [expr].  */

tree
type_after_usual_arithmetic_conversions (t1, t2)
     tree t1;
     tree t2;
{
  enum tree_code code1 = TREE_CODE (t1);
  enum tree_code code2 = TREE_CODE (t2);
  tree attributes;

  /* FIXME: Attributes.  */
  my_friendly_assert (ARITHMETIC_TYPE_P (t1) 
		      || TREE_CODE (t1) == COMPLEX_TYPE
		      || TREE_CODE (t1) == ENUMERAL_TYPE,
		      19990725);
  my_friendly_assert (ARITHMETIC_TYPE_P (t2) 
		      || TREE_CODE (t2) == COMPLEX_TYPE
		      || TREE_CODE (t2) == ENUMERAL_TYPE,
		      19990725);

  /* In what follows, we slightly generalize the rules given in [expr] so
     as to deal with `long long' and `complex'.  First, merge the
     attributes.  */
  attributes = (*targetm.merge_type_attributes) (t1, t2);

  /* If one type is complex, form the common type of the non-complex
     components, then make that complex.  Use T1 or T2 if it is the
     required type.  */
  if (code1 == COMPLEX_TYPE || code2 == COMPLEX_TYPE)
    {
      tree subtype1 = code1 == COMPLEX_TYPE ? TREE_TYPE (t1) : t1;
      tree subtype2 = code2 == COMPLEX_TYPE ? TREE_TYPE (t2) : t2;
      tree subtype
	= type_after_usual_arithmetic_conversions (subtype1, subtype2);

      if (code1 == COMPLEX_TYPE && TREE_TYPE (t1) == subtype)
	return build_type_attribute_variant (t1, attributes);
      else if (code2 == COMPLEX_TYPE && TREE_TYPE (t2) == subtype)
	return build_type_attribute_variant (t2, attributes);
      else
	return build_type_attribute_variant (build_complex_type (subtype),
					     attributes);
    }

  /* If only one is real, use it as the result.  */
  if (code1 == REAL_TYPE && code2 != REAL_TYPE)
    return build_type_attribute_variant (t1, attributes);
  if (code2 == REAL_TYPE && code1 != REAL_TYPE)
    return build_type_attribute_variant (t2, attributes);

  /* Perform the integral promotions.  */
  if (code1 != REAL_TYPE)
    {
      t1 = type_promotes_to (t1);
      t2 = type_promotes_to (t2);
    }

  /* Both real or both integers; use the one with greater precision.  */
  if (TYPE_PRECISION (t1) > TYPE_PRECISION (t2))
    return build_type_attribute_variant (t1, attributes);
  else if (TYPE_PRECISION (t2) > TYPE_PRECISION (t1))
    return build_type_attribute_variant (t2, attributes);

  /* The types are the same; no need to do anything fancy.  */
  if (TYPE_MAIN_VARIANT (t1) == TYPE_MAIN_VARIANT (t2))
    return build_type_attribute_variant (t1, attributes);

  if (code1 != REAL_TYPE)
    {
      /* If one is a sizetype, use it so size_binop doesn't blow up.  */
      if (TYPE_IS_SIZETYPE (t1) > TYPE_IS_SIZETYPE (t2))
	return build_type_attribute_variant (t1, attributes);
      if (TYPE_IS_SIZETYPE (t2) > TYPE_IS_SIZETYPE (t1))
	return build_type_attribute_variant (t2, attributes);

      /* If one is unsigned long long, then convert the other to unsigned
	 long long.  */
      if (same_type_p (TYPE_MAIN_VARIANT (t1), long_long_unsigned_type_node)
	  || same_type_p (TYPE_MAIN_VARIANT (t2), long_long_unsigned_type_node))
	return build_type_attribute_variant (long_long_unsigned_type_node,
					     attributes);
      /* If one is a long long, and the other is an unsigned long, and
	 long long can represent all the values of an unsigned long, then
	 convert to a long long.  Otherwise, convert to an unsigned long
	 long.  Otherwise, if either operand is long long, convert the
	 other to long long.
	 
	 Since we're here, we know the TYPE_PRECISION is the same;
	 therefore converting to long long cannot represent all the values
	 of an unsigned long, so we choose unsigned long long in that
	 case.  */
      if (same_type_p (TYPE_MAIN_VARIANT (t1), long_long_integer_type_node)
	  || same_type_p (TYPE_MAIN_VARIANT (t2), long_long_integer_type_node))
	{
	  tree t = ((TREE_UNSIGNED (t1) || TREE_UNSIGNED (t2))
		    ? long_long_unsigned_type_node 
		    : long_long_integer_type_node);
	  return build_type_attribute_variant (t, attributes);
	}
      
      /* Go through the same procedure, but for longs.  */
      if (same_type_p (TYPE_MAIN_VARIANT (t1), long_unsigned_type_node)
	  || same_type_p (TYPE_MAIN_VARIANT (t2), long_unsigned_type_node))
	return build_type_attribute_variant (long_unsigned_type_node,
					     attributes);
      if (same_type_p (TYPE_MAIN_VARIANT (t1), long_integer_type_node)
	  || same_type_p (TYPE_MAIN_VARIANT (t2), long_integer_type_node))
	{
	  tree t = ((TREE_UNSIGNED (t1) || TREE_UNSIGNED (t2))
		    ? long_unsigned_type_node : long_integer_type_node);
	  return build_type_attribute_variant (t, attributes);
	}
      /* Otherwise prefer the unsigned one.  */
      if (TREE_UNSIGNED (t1))
	return build_type_attribute_variant (t1, attributes);
      else
	return build_type_attribute_variant (t2, attributes);
    }
  else
    {
      if (same_type_p (TYPE_MAIN_VARIANT (t1), long_double_type_node)
	  || same_type_p (TYPE_MAIN_VARIANT (t2), long_double_type_node))
	return build_type_attribute_variant (long_double_type_node,
					     attributes);
      if (same_type_p (TYPE_MAIN_VARIANT (t1), double_type_node)
	  || same_type_p (TYPE_MAIN_VARIANT (t2), double_type_node))
	return build_type_attribute_variant (double_type_node,
					     attributes);
      if (same_type_p (TYPE_MAIN_VARIANT (t1), float_type_node)
	  || same_type_p (TYPE_MAIN_VARIANT (t2), float_type_node))
	return build_type_attribute_variant (float_type_node,
					     attributes);
      
      /* Two floating-point types whose TYPE_MAIN_VARIANTs are none of
         the standard C++ floating-point types.  Logic earlier in this
         function has already eliminated the possibility that
         TYPE_PRECISION (t2) != TYPE_PRECISION (t1), so there's no
         compelling reason to choose one or the other.  */
      return build_type_attribute_variant (t1, attributes);
    }
}

/* Return the composite pointer type (see [expr.rel]) for T1 and T2.
   ARG1 and ARG2 are the values with those types.  The LOCATION is a
   string describing the current location, in case an error occurs.  */

tree 
composite_pointer_type (t1, t2, arg1, arg2, location)
     tree t1;
     tree t2;
     tree arg1;
     tree arg2;
     const char* location;
{
  tree result_type;
  tree attributes;

  /* [expr.rel]

     If one operand is a null pointer constant, the composite pointer
     type is the type of the other operand.  */
  if (null_ptr_cst_p (arg1))
    return t2;
  if (null_ptr_cst_p (arg2))
    return t1;
 
  /* Deal with pointer-to-member functions in the same way as we deal
     with pointers to functions. */
  if (TYPE_PTRMEMFUNC_P (t1))
    t1 = TYPE_PTRMEMFUNC_FN_TYPE (t1);
  if (TYPE_PTRMEMFUNC_P (t2))
    t2 = TYPE_PTRMEMFUNC_FN_TYPE (t2);
  
  /* Merge the attributes.  */
  attributes = (*targetm.merge_type_attributes) (t1, t2);

  /* We have:

       [expr.rel]

       If one of the operands has type "pointer to cv1 void*", then
       the other has type "pointer to cv2T", and the composite pointer
       type is "pointer to cv12 void", where cv12 is the union of cv1
       and cv2.

    If either type is a pointer to void, make sure it is T1.  */
  if (VOID_TYPE_P (TREE_TYPE (t2)))
    {
      tree t;
      t = t1;
      t1 = t2;
      t2 = t;
    }
  /* Now, if T1 is a pointer to void, merge the qualifiers.  */
  if (VOID_TYPE_P (TREE_TYPE (t1)))
    {
      if (pedantic && TYPE_PTRFN_P (t2))
	pedwarn ("ISO C++ forbids %s between pointer of type `void *' and pointer-to-function", location);
      t1 = TREE_TYPE (t1);
      t2 = TREE_TYPE (t2);
      result_type = cp_build_qualified_type (void_type_node,
					     (cp_type_quals (t1)
					      | cp_type_quals (t2)));
      result_type = build_pointer_type (result_type);
    }
  else
    {
      tree full1 = qualify_type_recursive (t1, t2);
      tree full2 = qualify_type_recursive (t2, t1);

      int val = comp_target_types (full1, full2, 1);

      if (val > 0)
	result_type = full1;
      else if (val < 0)
	result_type = full2;
      else
	{
	  pedwarn ("%s between distinct pointer types `%T' and `%T' lacks a cast",
		      location, t1, t2);
	  result_type = ptr_type_node;
	}
    }

  return build_type_attribute_variant (result_type, attributes);
}

/* Return the merged type of two types.
   We assume that comptypes has already been done and returned 1;
   if that isn't so, this may crash.

   This just combines attributes and default arguments; any other
   differences would cause the two types to compare unalike.  */

tree
merge_types (t1, t2)
     tree t1, t2;
{
  register enum tree_code code1;
  register enum tree_code code2;
  tree attributes;

  /* Save time if the two types are the same.  */
  if (t1 == t2)
    return t1;
  if (original_type (t1) == original_type (t2))
    return t1;

  /* If one type is nonsense, use the other.  */
  if (t1 == error_mark_node)
    return t2;
  if (t2 == error_mark_node)
    return t1;

  /* Merge the attributes.  */
  attributes = (*targetm.merge_type_attributes) (t1, t2);

  /* Treat an enum type as the unsigned integer type of the same width.  */

  if (TYPE_PTRMEMFUNC_P (t1))
    t1 = TYPE_PTRMEMFUNC_FN_TYPE (t1);
  if (TYPE_PTRMEMFUNC_P (t2))
    t2 = TYPE_PTRMEMFUNC_FN_TYPE (t2);

  code1 = TREE_CODE (t1);
  code2 = TREE_CODE (t2);

  switch (code1)
    {
    case POINTER_TYPE:
    case REFERENCE_TYPE:
      /* For two pointers, do this recursively on the target type.  */
      {
	tree target = merge_types (TREE_TYPE (t1), TREE_TYPE (t2));
	int quals = cp_type_quals (t1);

	if (code1 == POINTER_TYPE)
	  t1 = build_pointer_type (target);
	else
	  t1 = build_reference_type (target);
	t1 = build_type_attribute_variant (t1, attributes);
	t1 = cp_build_qualified_type (t1, quals);

	if (TREE_CODE (target) == METHOD_TYPE)
	  t1 = build_ptrmemfunc_type (t1);

	return t1;
      }

    case OFFSET_TYPE:
      {
	tree base = TYPE_OFFSET_BASETYPE (t1);
	tree target = merge_types (TREE_TYPE (t1), TREE_TYPE (t2));
	t1 = build_offset_type (base, target);
	break;
      }

    case ARRAY_TYPE:
      {
	tree elt = merge_types (TREE_TYPE (t1), TREE_TYPE (t2));
	/* Save space: see if the result is identical to one of the args.  */
	if (elt == TREE_TYPE (t1) && TYPE_DOMAIN (t1))
	  return build_type_attribute_variant (t1, attributes);
	if (elt == TREE_TYPE (t2) && TYPE_DOMAIN (t2))
	  return build_type_attribute_variant (t2, attributes);
	/* Merge the element types, and have a size if either arg has one.  */
	t1 = build_cplus_array_type
	  (elt, TYPE_DOMAIN (TYPE_DOMAIN (t1) ? t1 : t2));
	break;