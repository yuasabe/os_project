/* Build expressions with type checking for C compiler.
   Copyright (C) 1987, 1988, 1991, 1992, 1993, 1994, 1995, 1996, 1997,
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


/* This file is part of the C front end.
   It contains routines to build C expressions given their operands,
   including computing the types of the result, C-specific error checks,
   and some optimization.

   There are also routines to build RETURN_STMT nodes and CASE_STMT nodes,
   and to process initializations in declarations (since they work
   like a strange sort of assignment).  */

#include "config.h"
#include "system.h"
#include "rtl.h"
#include "tree.h"
#include "c-tree.h"
#include "tm_p.h"
#include "flags.h"
#include "output.h"
#include "expr.h"
#include "toplev.h"
#include "intl.h"
#include "ggc.h"
#include "target.h"

/* Nonzero if we've already printed a "missing braces around initializer"
   message within this initializer.  */
static int missing_braces_mentioned;

/* 1 if we explained undeclared var errors.  */
static int undeclared_variable_notice;

static tree qualify_type		PARAMS ((tree, tree));
static int comp_target_types		PARAMS ((tree, tree));
static int function_types_compatible_p	PARAMS ((tree, tree));
static int type_lists_compatible_p	PARAMS ((tree, tree));
static tree decl_constant_value_for_broken_optimization PARAMS ((tree));
static tree default_function_array_conversion	PARAMS ((tree));
static tree lookup_field		PARAMS ((tree, tree));
static tree convert_arguments		PARAMS ((tree, tree, tree, tree));
static tree pointer_diff		PARAMS ((tree, tree));
static tree unary_complex_lvalue	PARAMS ((enum tree_code, tree, int));
static void pedantic_lvalue_warning	PARAMS ((enum tree_code));
static tree internal_build_compound_expr PARAMS ((tree, int));
static tree convert_for_assignment	PARAMS ((tree, tree, const char *,
						 tree, tree, int));
static void warn_for_assignment		PARAMS ((const char *, const char *,
						 tree, int));
static tree valid_compound_expr_initializer PARAMS ((tree, tree));
static void push_string			PARAMS ((const char *));
static void push_member_name		PARAMS ((tree));
static void push_array_bounds		PARAMS ((int));
static int spelling_length		PARAMS ((void));
static char *print_spelling		PARAMS ((char *));
static void warning_init		PARAMS ((const char *));
static tree digest_init			PARAMS ((tree, tree, int, int));
static void output_init_element		PARAMS ((tree, tree, tree, int));
static void output_pending_init_elements PARAMS ((int));
static int set_designator		PARAMS ((int));
static void push_range_stack		PARAMS ((tree));
static void add_pending_init		PARAMS ((tree, tree));
static void set_nonincremental_init	PARAMS ((void));
static void set_nonincremental_init_from_string	PARAMS ((tree));
static tree find_init_member		PARAMS ((tree));

/* Do `exp = require_complete_type (exp);' to make sure exp
   does not have an incomplete type.  (That includes void types.)  */

tree
require_complete_type (value)
     tree value;
{
  tree type = TREE_TYPE (value);

  if (value == error_mark_node || type == error_mark_node)
    return error_mark_node;

  /* First, detect a valid value with a complete type.  */
  if (COMPLETE_TYPE_P (type))
    return value;

  incomplete_type_error (value, type);
  return error_mark_node;
}

/* Print an error message for invalid use of an incomplete type.
   VALUE is the expression that was used (or 0 if that isn't known)
   and TYPE is the type that was invalid.  */

void
incomplete_type_error (value, type)
     tree value;
     tree type;
{
  const char *type_code_string;

  /* Avoid duplicate error message.  */
  if (TREE_CODE (type) == ERROR_MARK)
    return;

  if (value != 0 && (TREE_CODE (value) == VAR_DECL
		     || TREE_CODE (value) == PARM_DECL))
    error ("`%s' has an incomplete type",
	   IDENTIFIER_POINTER (DECL_NAME (value)));
  else
    {
    retry:
      /* We must print an error message.  Be clever about what it says.  */

      switch (TREE_CODE (type))
	{
	case RECORD_TYPE:
	  type_code_string = "struct";
	  break;

	case UNION_TYPE:
	  type_code_string = "union";
	  break;

	case ENUMERAL_TYPE:
	  type_code_string = "enum";
	  break;

	case VOID_TYPE:
	  error ("invalid use of void expression");
	  return;

	case ARRAY_TYPE:
	  if (TYPE_DOMAIN (type))
	    {
	      if (TYPE_MAX_VALUE (TYPE_DOMAIN (type)) == NULL)
		{
		  error ("invalid use of flexible array member");
		  return;
		}
	      type = TREE_TYPE (type);
	      goto retry;
	    }
	  error ("invalid use of array with unspecified bounds");
	  return;

	default:
	  abort ();
	}

      if (TREE_CODE (TYPE_NAME (type)) == IDENTIFIER_NODE)
	error ("invalid use of undefined type `%s %s'",
	       type_code_string, IDENTIFIER_POINTER (TYPE_NAME (type)));
      else
	/* If this type has a typedef-name, the TYPE_NAME is a TYPE_DECL.  */
	error ("invalid use of incomplete typedef `%s'",
	       IDENTIFIER_POINTER (DECL_NAME (TYPE_NAME (type))));
    }
}

/* Return a variant of TYPE which has all the type qualifiers of LIKE
   as well as those of TYPE.  */

static tree
qualify_type (type, like)
     tree type, like;
{
  return c_build_qualified_type (type, 
				 TYPE_QUALS (type) | TYPE_QUALS (like));
}

/* Return the common type of two types.
   We assume that comptypes has already been done and returned 1;
   if that isn't so, this may crash.  In particular, we assume that qualifiers
   match.

   This is the type for the result of most arithmetic operations
   if the operands have the given two types.  */

tree
common_type (t1, t2)
     tree t1, t2;
{
  enum tree_code code1;
  enum tree_code code2;
  tree attributes;

  /* Save time if the two types are the same.  */

  if (t1 == t2) return t1;

  /* If one type is nonsense, use the other.  */
  if (t1 == error_mark_node)
    return t2;
  if (t2 == error_mark_node)
    return t1;

  /* Merge the attributes.  */
  attributes = (*targetm.merge_type_attributes) (t1, t2);

  /* Treat an enum type as the unsigned integer type of the same width.  */

  if (TREE_CODE (t1) == ENUMERAL_TYPE)
    t1 = type_for_size (TYPE_PRECISION (t1), 1);
  if (TREE_CODE (t2) == ENUMERAL_TYPE)
    t2 = type_for_size (TYPE_PRECISION (t2), 1);

  code1 = TREE_CODE (t1);
  code2 = TREE_CODE (t2);

  /* If one type is complex, form the common type of the non-complex
     components, then make that complex.  Use T1 or T2 if it is the
     required type.  */
  if (code1 == COMPLEX_TYPE || code2 == COMPLEX_TYPE)
    {
      tree subtype1 = code1 == COMPLEX_TYPE ? TREE_TYPE (t1) : t1;
      tree subtype2 = code2 == COMPLEX_TYPE ? TREE_TYPE (t2) : t2;
      tree subtype = common_type (subtype1, subtype2);

      if (code1 == COMPLEX_TYPE && TREE_TYPE (t1) == subtype)
	return build_type_attribute_variant (t1, attributes);
      else if (code2 == COMPLEX_TYPE && TREE_TYPE (t2) == subtype)
	return build_type_attribute_variant (t2, attributes);
      else
	return build_type_attribute_variant (build_complex_type (subtype),
					     attributes);
    }

  switch (code1)
    {
    case INTEGER_TYPE:
    case REAL_TYPE:
      /* If only one is real, use it as the result.  */

      if (code1 == REAL_TYPE && code2 != REAL_TYPE)
	return build_type_attribute_variant (t1, attributes);

      if (code2 == REAL_TYPE && code1 != REAL_TYPE)
	return build_type_attribute_variant (t2, attributes);

      /* Both real or both integers; use the one with greater precision.  */

      if (TYPE_PRECISION (t1) > TYPE_PRECISION (t2))
	return build_type_attribute_variant (t1, attributes);
      else if (TYPE_PRECISION (t2) > TYPE_PRECISION (t1))
	return build_type_attribute_variant (t2, attributes);

      /* Same precision.  Prefer longs to ints even when same size.  */

      if (TYPE_MAIN_VARIANT (t1) == long_unsigned_type_node
	  || TYPE_MAIN_VARIANT (t2) == long_unsigned_type_node)
	return build_type_attribute_variant (long_unsigned_type_node,
					     attributes);

      if (TYPE_MAIN_VARIANT (t1) == long_integer_type_node
	  || TYPE_MAIN_VARIANT (t2) == long_integer_type_node)
	{
	  /* But preserve unsignedness from the other type,
	     since long cannot hold all the values of an unsigned int.  */
	  if (TREE_UNSIGNED (t1) || TREE_UNSIGNED (t2))
	     t1 = long_unsigned_type_node;
	  else
	     t1 = long_integer_type_node;
	  return build_type_attribute_variant (t1, attributes);
	}

      /* Likewise, prefer long double to double even if same size.  */
      if (TYPE_MAIN_VARIANT (t1) == long_double_type_node
	  || TYPE_MAIN_VARIANT (t2) == long_double_type_node)
	return build_type_attribute_variant (long_double_type_node,
					     attributes);

      /* Otherwise prefer the unsigned one.  */

      if (TREE_UNSIGNED (t1))
	return build_type_attribute_variant (t1, attributes);
      else
	return build_type_attribute_variant (t2, attributes);

    case POINTER_TYPE:
      /* For two pointers, do this recursively on the target type,
	 and combine the qualifiers of the two types' targets.  */
      /* This code was turned off; I don't know why.
	 But ANSI C specifies doing this with the qualifiers.
	 So I turned it on again.  */
      {
	tree pointed_to_1 = TREE_TYPE (t1);
	tree pointed_to_2 = TREE_TYPE (t2);
	tree target = common_type (TYPE_MAIN_VARIANT (pointed_to_1),
				   TYPE_MAIN_VARIANT (pointed_to_2));
	t1 = build_pointer_type (c_build_qualified_type 
				 (target, 
				  TYPE_QUALS (pointed_to_1) | 
				  TYPE_QUALS (pointed_to_2)));
	return build_type_attribute_variant (t1, attributes);
      }
#if 0
      t1 = build_pointer_type (common_type (TREE_TYPE (t1), TREE_TYPE (t2)));
      return build_type_attribute_variant (t1, attributes);
#endif

    case ARRAY_TYPE:
      {
	tree elt = common_type (TREE_TYPE (t1), TREE_TYPE (t2));
	/* Save space: see if the result is identical to one of the args.  */
	if (elt == TREE_TYPE (t1) && TYPE_DOMAIN (t1))
	  return build_type_attribute_variant (t1, attributes);
	if (elt == TREE_TYPE (t2) && TYPE_DOMAIN (t2))
	  return build_type_attribute_variant (t2, attributes);
	/* Merge the element types, and have a size if either arg has one.  */
	t1 = build_array_type (elt, TYPE_DOMAIN (TYPE_DOMAIN (t1) ? t1 : t2));
	return build_type_attribute_variant (t1, attributes);
      }

    case FUNCTION_TYPE:
      /* Function types: prefer the one that specified arg types.
	 If both do, merge the arg types.  Also merge the return types.  */
      {
	tree valtype = common_type (TREE_TYPE (t1), TREE_TYPE (t2));
	tree p1 = TYPE_ARG_TYPES (t1);
	tree p2 = TYPE_ARG_TYPES (t2);
	int len;
	tree newargs, n;
	int i;

	/* Save space: see if the result is identical to one of the args.  */
	if (valtype == TREE_TYPE (t1) && ! TYPE_ARG_TYPES (t2))
	  return build_type_attribute_variant (t1, attributes);
	if (valtype == TREE_TYPE (t2) && ! TYPE_ARG_TYPES (t1))
	  return build_type_attribute_variant (t2, attributes);

	/* Simple way if one arg fails to specify argument types.  */
	if (TYPE_ARG_TYPES (t1) == 0)
	 {
	   t1 = build_function_type (valtype, TYPE_ARG_TYPES (t2));
	   return build_type_attribute_variant (t1, attributes);
	 }
	if (TYPE_ARG_TYPES (t2) == 0)
	 {
	   t1 = build_function_type (valtype, TYPE_ARG_TYPES (t1));
	   return build_type_attribute_variant (t1, attributes);
	 }

	/* If both args specify argument types, we must merge the two
	   lists, argument by argument.  */

	pushlevel (0);
	declare_parm_level (1);

	len = list_length (p1);
	newargs = 0;

	for (i = 0; i < len; i++)
	  newargs = tree_cons (NULL_TREE, NULL_TREE, newargs);

	n = newargs;

	for (; p1;
	     p1 = TREE_CHAIN (p1), p2 = TREE_CHAIN (p2), n = TREE_CHAIN (n))
	  {
	    /* A null type means arg type is not specified.
	       Take whatever the other function type has.  */
	    if (TREE_VALUE (p1) == 0)
	      {
		TREE_VALUE (n) = TREE_VALUE (p2);
		goto parm_done;
	      }
	    if (TREE_VALUE (p2) == 0)
	      {
		TREE_VALUE (n) = TREE_VALUE (p1);
		goto parm_done;
	      }
	      
	    /* Given  wait (union {union wait *u; int *i} *)
	       and  wait (union wait *),
	       prefer  union wait *  as type of parm.  */
	    if (TREE_CODE (TREE_VALUE (p1)) == UNION_TYPE
		&& TREE_VALUE (p1) != TREE_VALUE (p2))
	      {
		tree memb;
		for (memb = TYPE_FIELDS (TREE_VALUE (p1));
		     memb; memb = TREE_CHAIN (memb))
		  if (comptypes (TREE_TYPE (memb), TREE_VALUE (p2)))
		    {
		      TREE_VALUE (n) = TREE_VALUE (p2);
		      if (pedantic)
			pedwarn ("function types not truly compatible in ISO C");
		      goto parm_done;
		    }
	      }
	    if (TREE_CODE (TREE_VALUE (p2)) == UNION_TYPE
		&& TREE_VALUE (p2) != TREE_VALUE (p1))
	      {
		tree memb;
		for (memb = TYPE_FIELDS (TREE_VALUE (p2));
		     memb; memb = TREE_CHAIN (memb))
		  if (comptypes (TREE_TYPE (memb), TREE_VALUE (p1)))
		    {
		      TREE_VALUE (n) = TREE_VALUE (p1);
		      if (pedantic)
			pedwarn ("function types not truly compatible in ISO C");
		      goto parm_done;
		    }
	      }
	    TREE_VALUE (n) = common_type (TREE_VALUE (p1), TREE_VALUE (p2));
	  parm_done: ;
	  }

	poplevel (0, 0, 0);

	t1 = build_function_type (valtype, newargs);
	/* ... falls through ...  */
      }

    default:
      return build_type_attribute_variant (t1, attributes);
    }

}

/* Return 1 if TYPE1 and TYPE2 are compatible types for assignment
   or various other operations.  Return 2 if they are compatible
   but a warning may be needed if you use them together.  */

int
comptypes (type1, type2)
     tree type1, type2;
{
  tree t1 = type1;
  tree t2 = type2;
  int attrval, val;

  /* Suppress errors caused by previously reported errors.  */

  if (t1 == t2 || !t1 || !t2
      || TREE_CODE (t1) == ERROR_MARK || TREE_CODE (t2) == ERROR_MARK)
    return 1;

  /* If either type is the internal version of sizetype, return the
     language version.  */
  if (TREE_CODE (t1) == INTEGER_TYPE && TYPE_IS_SIZETYPE (t1)
      && TYPE_DOMAIN (t1) != 0)
    t1 = TYPE_DOMAIN (t1);

  if (TREE_CODE (t2) == INTEGER_TYPE && TYPE_IS_SIZETYPE (t2)
      && TYPE_DOMAIN (t2) != 0)
    t2 = TYPE_DOMAIN (t2);

  /* Treat an enum type as the integer type of the same width and 
     signedness.  */

  if (TREE_CODE (t1) == ENUMERAL_TYPE)
    t1 = type_for_size (TYPE_PRECISION (t1), TREE_UNSIGNED (t1));
  if (TREE_CODE (t2) == ENUMERAL_TYPE)
    t2 = type_for_size (TYPE_PRECISION (t2), TREE_UNSIGNED (t2));

  if (t1 == t2)
    return 1;

  /* Different classes of types can't be compatible.  */

  if (TREE_CODE (t1) != TREE_CODE (t2)) return 0;

  /* Qualifiers must match.  */

  if (TYPE_QUALS (t1) != TYPE_QUALS (t2))
    return 0;

  /* Allow for two different type nodes which have essentially the same
     definition.  Note that we already checked for equality of the type
     qualifiers (just above).  */

  if (TYPE_MAIN_VARIANT (t1) == TYPE_MAIN_VARIANT (t2))
    return 1;

  /* 1 if no need for warning yet, 2 if warning cause has been seen.  */
  if (! (attrval = (*targetm.comp_type_attributes) (t1, t2)))
     return 0;

  /* 1 if no need for warning yet, 2 if warning cause has been seen.  */
  val = 0;

  switch (TREE_CODE (t1))
    {
    case POINTER_TYPE:
      val = (TREE_TYPE (t1) == TREE_TYPE (t2)
	      ? 1 : comptypes (TREE_TYPE (t1), TREE_TYPE (t2)));
      break;

    case FUNCTION_TYPE:
      val = function_types_compatible_p (t1, t2);
      break;

    case ARRAY_TYPE:
      {
	tree d1 = TYPE_DOMAIN (t1);
	tree d2 = TYPE_DOMAIN (t2);
	bool d1_variable, d2_variable;
	bool d1_zero, d2_zero;
	val = 1;

	/* Target types must match incl. qualifiers.  */
	if (TREE_TYPE (t1) != TREE_TYPE (t2)
	    && 0 == (val = comptypes (TREE_TYPE (t1), TREE_TYPE (t2))))
	  return 0;

	/* Sizes must match unless one is missing or variable.  */
	if (d1 == 0 || d2 == 0 || d1 == d2)
	  break;

	d1_zero = ! TYPE_MAX_VALUE (d1);
	d2_zero = ! TYPE_MAX_VALUE (d2);

	d1_variable = (! d1_zero
		       && (TREE_CODE (TYPE_MIN_VALUE (d1)) != INTEGER_CST
			   || TREE_CODE (TYPE_MAX_VALUE (d1)) != INTEGER_CST));
	d2_variable = (! d2_zero
		       && (TREE_CODE (TYPE_MIN_VALUE (d2)) != INTEGER_CST
			   || TREE_CODE (TYPE_MAX_VALUE (d2)) != INTEGER_CST));

	if (d1_variable || d2_variable)
	  break;
	if (d1_zero && d2_zero)
	  break;
	if (d1_zero || d2_zero
	    || ! tree_int_cst_equal (TYPE_MIN_VALUE (d1), TYPE_MIN_VALUE (d2))
	    || ! tree_int_cst_equal (TYPE_MAX_VALUE (d1), TYPE_MAX_VALUE (d2)))
	  val = 0;

        break;
      }

    case RECORD_TYPE:
      if (maybe_objc_comptypes (t1, t2, 0) == 1)
	val = 1;
      break;

    default:
      break;
    }
  return attrval == 2 && val == 1 ? 2 : val;
}

/* Return 1 if TTL and TTR are pointers to types that are equivalent,
   ignoring their qualifiers.  */

static int
comp_target_types (ttl, ttr)
     tree ttl, ttr;
{
  int val;

  /* Give maybe_objc_comptypes a crack at letting these types through.  */
  if ((val = maybe_objc_comptypes (ttl, ttr, 1)) >= 0)
    return val;

  val = comptypes (TYPE_MAIN_VARIANT (TREE_TYPE (ttl)),
		   TYPE_MAIN_VARIANT (TREE_TYPE (ttr)));

  if (val == 2 && pedantic)
    pedwarn ("types are not quite compatible");
  return val;
}

/* Subroutines of `comptypes'.  */

/* Return 1 if two function types F1 and F2 are compatible.
   If either type specifies no argument types,
   the other must specify a fixed number of self-promoting arg types.
   Otherwise, if one type specifies only the number of arguments, 
   the other must specify that number of self-promoting arg types.
   Otherwise, the argument types must match.  */

static int
function_types_compatible_p (f1, f2)
     tree f1, f2;
{
  tree args1, args2;
  /* 1 if no need for warning yet, 2 if warning cause has been seen.  */
  int val = 1;
  int val1;

  if (!(TREE_TYPE (f1) == TREE_TYPE (f2)
	|| (val = comptypes (TREE_TYPE (f1), TREE_TYPE (f2)))))
    return 0;

  args1 = TYPE_ARG_TYPES (f1);
  args2 = TYPE_ARG_TYPES (f2);

  /* An unspecified parmlist matches any specified parmlist
     whose argument types don't need default promotions.  */

  if (args1 == 0)
    {
      if (!self_promoting_args_p (args2))
	return 0;
      /* If one of these types comes from a non-prototype fn definition,
	 compare that with the other type's arglist.
	 If they don't match, ask for a warning (but no error).  */
      if (TYPE_ACTUAL_ARG_TYPES (f1)
	  && 1 != type_lists_compatible_p (args2, TYPE_ACTUAL_ARG_TYPES (f1)))
	val = 2;
      return val;
    }
  if (args2 == 0)
    {
      if (!self_promoting_args_p (args1))
	return 0;
      if (TYPE_ACTUAL_ARG_TYPES (f2)
	  && 1 != type_lists_compatible_p (args1, TYPE_ACTUAL_ARG_TYPES (f2)))
	val = 2;
      return val;
    }

  /* Both types have argument lists: compare them and propagate results.  */
  val1 = type_lists_compatible_p (args1, args2);
  return val1 != 1 ? val1 : val;
}

/* Check two lists of types for compatibility,
   returning 0 for incompatible, 1 for compatible,
   or 2 for compatible with warning.  */

static int
type_lists_compatible_p (args1, args2)
     tree args1, args2;
{
  /* 1 if no need for warning yet, 2 if warning cause has been seen.  */
  int val = 1;
  int newval = 0;

  while (1)
    {
      if (args1 == 0 && args2 == 0)
	return val;
      /* If one list is shorter than the other,
	 they fail to match.  */
      if (args1 == 0 || args2 == 0)
	return 0;
      /* A null pointer instead of a type
	 means there is supposed to be an argument
	 but nothing is specified about what type it has.
	 So match anything that self-promotes.  */
      if (TREE_VALUE (args1) == 0)
	{
	  if (simple_type_promotes_to (TREE_VALUE (args2)) != NULL_TREE)
	    return 0;
	}
      else if (TREE_VALUE (args2) == 0)
	{
	  if (simple_type_promotes_to (TREE_VALUE (args1)) != NULL_TREE)
	    return 0;
	}
      else if (! (newval = comptypes (TYPE_MAIN_VARIANT (TREE_VALUE (args1)), 
				      TYPE_MAIN_VARIANT (TREE_VALUE (args2)))))
	{
	  /* Allow  wait (union {union wait *u; int *i} *)
	     and  wait (union wait *)  to be compatible.  */
	  if (TREE_CODE (TREE_VALUE (args1)) == UNION_TYPE
	      && (TYPE_NAME (TREE_VALUE (args1)) == 0
		  || TYPE_TRANSPARENT_UNION (TREE_VALUE (args1)))
	      && TREE_CODE (TYPE_SIZE (TREE_VALUE (args1))) == INTEGER_CST
	      && tree_int_cst_equal (TYPE_SIZE (TREE_VALUE (args1)),
				     TYPE_SIZE (TREE_VALUE (args2))))
	    {
	      tree memb;
	      for (memb = TYPE_FIELDS (TREE_VALUE (args1));
		   memb; memb = TREE_CHAIN (memb))
		if (comptypes (TREE_TYPE (memb), TREE_VALUE (args2)))
		  break;
	      if (memb == 0)
		return 0;
	    }
	  else if (TREE_CODE (TREE_VALUE (args2)) == UNION_TYPE
		   && (TYPE_NAME (TREE_VALUE (args2)) == 0
		       || TYPE_TRANSPARENT_UNION (TREE_VALUE (args2)))
		   && TREE_CODE (TYPE_SIZE (TREE_VALUE (args2))) == INTEGER_CST
		   && tree_int_cst_equal (TYPE_SIZE (TREE_VALUE (args2)),
					  TYPE_SIZE (TREE_VALUE (args1))))
	    {
	      tree memb;
	      for (memb = TYPE_FIELDS (TREE_VALUE (args2));
		   memb; memb = TREE_CHAIN (memb))
		if (comptypes (TREE_TYPE (memb), TREE_VALUE (args1)))
		  break;
	      if (memb == 0)
		return 0;
	    }
	  else
	    return 0;
	}

      /* comptypes said ok, but record if it said to warn.  */
      if (newval > val)
	val = newval;

      args1 = TREE_CHAIN (args1);
      args2 = TREE_CHAIN (args2);
    }
}

/* Compute the value of the `sizeof' operator.  */

tree
c_sizeof (type)
     tree type;
{
  enum tree_code code = TREE_CODE (type);
  tree size;

  if (code == FUNCTION_TYPE)
    {
      if (pedantic || warn_pointer_arith)
	pedwarn ("sizeof applied to a function type");
      size = size_one_node;
    }
  else if (code == VOID_TYPE)
    {
      if (pedantic || warn_pointer_arith)
	pedwarn ("sizeof applied to a void type");
      size = size_one_node;
    }
  else if (code == ERROR_MARK)
    size = size_one_node;
  else if (!COMPLETE_TYPE_P (type))
    {
      error ("sizeof applied to an incomplete type");
      size = size_zero_node;
    }
  else
    /* Convert in case a char is more than one unit.  */
    size = size_binop (CEIL_DIV_EXPR, TYPE_SIZE_UNIT (type),
		       size_int (TYPE_PRECISION (char_type_node)
			         / BITS_PER_UNIT));

  /* SIZE will have an integer type with TYPE_IS_SIZETYPE set.
     TYPE_IS_SIZETYPE means that certain things (like overflow) will
     never happen.  However, this node should really have type
     `size_t', which is just a typedef for an ordinary integer type.  */
  return fold (build1 (NOP_EXPR, c_size_type_node, size));
}

tree
c_sizeof_nowarn (type)
     tree type;
{
  enum tree_code code = TREE_CODE (type);
  tree size;

  if (code == FUNCTION_TYPE || code == VOID_TYPE || code == ERROR_MARK)
    size = size_one_node;
  else if (!COMPLETE_TYPE_P (type))
    size = size_zero_node;
  else
    /* Convert in case a char is more than one unit.  */
    size = size_binop (CEIL_DIV_EXPR, TYPE_SIZE_UNIT (type),
		       size_int (TYPE_PRECISION (char_type_node)
			         / BITS_PER_UNIT));

  /* SIZE will have an integer type with TYPE_IS_SIZETYPE set.
     TYPE_IS_SIZETYPE means that certain things (like overflow) will
     never happen.  However, this node should really have type
     `size_t', which i