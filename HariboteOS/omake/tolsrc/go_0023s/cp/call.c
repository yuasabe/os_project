/* Functions related to invoking methods and overloaded functions.
   Copyright (C) 1987, 1992, 1993, 1994, 1995, 1996, 1997, 1998,
   1999, 2000, 2001, 2002 Free Software Foundation, Inc.
   Contributed by Michael Tiemann (tiemann@cygnus.com) and
   modified by Brendan Kehoe (brendan@cygnus.com).

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


/* High-level class interface.  */

/* !kawai! */
#include "../gcc/config.h"
#include "../gcc/system.h"
#include "../gcc/tree.h"
#include "cp-tree.h"
#include "../gcc/output.h"
#include "../gcc/flags.h"
#include "../gcc/rtl.h"
#include "../gcc/toplev.h"
#include "../gcc/expr.h"
#include "../gcc/ggc.h"
#include "../gcc/diagnostic.h"
/* end of !kawai! */

extern int inhibit_warnings;

static tree build_new_method_call PARAMS ((tree, tree, tree, tree, int));

static tree build_field_call PARAMS ((tree, tree, tree, tree));
static struct z_candidate * tourney PARAMS ((struct z_candidate *));
static int equal_functions PARAMS ((tree, tree));
static int joust PARAMS ((struct z_candidate *, struct z_candidate *, int));
static int compare_ics PARAMS ((tree, tree));
static tree build_over_call PARAMS ((struct z_candidate *, tree, int));
static tree build_java_interface_fn_ref PARAMS ((tree, tree));
#define convert_like(CONV, EXPR) ¥
  convert_like_real ((CONV), (EXPR), NULL_TREE, 0, 0)
#define convert_like_with_context(CONV, EXPR, FN, ARGNO) ¥
  convert_like_real ((CONV), (EXPR), (FN), (ARGNO), 0)
static tree convert_like_real PARAMS ((tree, tree, tree, int, int));
static void op_error PARAMS ((enum tree_code, enum tree_code, tree, tree,
			    tree, const char *));
static tree build_object_call PARAMS ((tree, tree));
static tree resolve_args PARAMS ((tree));
static struct z_candidate * build_user_type_conversion_1
	PARAMS ((tree, tree, int));
static void print_z_candidates PARAMS ((struct z_candidate *));
static tree build_this PARAMS ((tree));
static struct z_candidate * splice_viable PARAMS ((struct z_candidate *));
static int any_viable PARAMS ((struct z_candidate *));
static struct z_candidate * add_template_candidate
	PARAMS ((struct z_candidate *, tree, tree, tree, tree, tree, int,
	       unification_kind_t));
static struct z_candidate * add_template_candidate_real
	PARAMS ((struct z_candidate *, tree, tree, tree, tree, tree, int,
	       tree, unification_kind_t));
static struct z_candidate * add_template_conv_candidate 
        PARAMS ((struct z_candidate *, tree, tree, tree, tree));
static struct z_candidate * add_builtin_candidates
	PARAMS ((struct z_candidate *, enum tree_code, enum tree_code,
	       tree, tree *, int));
static struct z_candidate * add_builtin_candidate
	PARAMS ((struct z_candidate *, enum tree_code, enum tree_code,
	       tree, tree, tree, tree *, tree *, int));
static int is_complete PARAMS ((tree));
static struct z_candidate * build_builtin_candidate 
	PARAMS ((struct z_candidate *, tree, tree, tree, tree *, tree *,
	       int));
static struct z_candidate * add_conv_candidate 
	PARAMS ((struct z_candidate *, tree, tree, tree));
static struct z_candidate * add_function_candidate 
	PARAMS ((struct z_candidate *, tree, tree, tree, int));
static tree implicit_conversion PARAMS ((tree, tree, tree, int));
static tree standard_conversion PARAMS ((tree, tree, tree));
static tree reference_binding PARAMS ((tree, tree, tree, int));
static tree non_reference PARAMS ((tree));
static tree build_conv PARAMS ((enum tree_code, tree, tree));
static int is_subseq PARAMS ((tree, tree));
static tree maybe_handle_ref_bind PARAMS ((tree*));
static void maybe_handle_implicit_object PARAMS ((tree*));
static struct z_candidate * add_candidate PARAMS ((struct z_candidate *,
						   tree, tree, int));
static tree source_type PARAMS ((tree));
static void add_warning PARAMS ((struct z_candidate *, struct z_candidate *));
static int reference_related_p PARAMS ((tree, tree));
static int reference_compatible_p PARAMS ((tree, tree));
static tree convert_class_to_reference PARAMS ((tree, tree, tree));
static tree direct_reference_binding PARAMS ((tree, tree));
static int promoted_arithmetic_type_p PARAMS ((tree));
static tree conditional_conversion PARAMS ((tree, tree));

tree
build_vfield_ref (datum, type)
     tree datum, type;
{
  tree rval;

  if (datum == error_mark_node)
    return error_mark_node;

  if (TREE_CODE (TREE_TYPE (datum)) == REFERENCE_TYPE)
    datum = convert_from_reference (datum);

  if (! TYPE_BASE_CONVS_MAY_REQUIRE_CODE_P (type))
    rval = build (COMPONENT_REF, TREE_TYPE (TYPE_VFIELD (type)),
		  datum, TYPE_VFIELD (type));
  else
    rval = build_component_ref (datum, DECL_NAME (TYPE_VFIELD (type)), NULL_TREE, 0);

  return rval;
}

/* Build a call to a member of an object.  I.e., one that overloads
   operator ()(), or is a pointer-to-function or pointer-to-method.  */

static tree
build_field_call (basetype_path, instance_ptr, name, parms)
     tree basetype_path, instance_ptr, name, parms;
{
  tree field, instance;

  if (IDENTIFIER_CTOR_OR_DTOR_P (name))
    return NULL_TREE;

  /* Speed up the common case.  */
  if (instance_ptr == current_class_ptr
      && IDENTIFIER_CLASS_VALUE (name) == NULL_TREE)
    return NULL_TREE;

  field = lookup_field (basetype_path, name, 1, 0);

  if (field == error_mark_node || field == NULL_TREE)
    return field;

  if (TREE_CODE (field) == FIELD_DECL || TREE_CODE (field) == VAR_DECL)
    {
      /* If it's a field, try overloading operator (),
	 or calling if the field is a pointer-to-function.  */
      instance = build_indirect_ref (instance_ptr, NULL);
      instance = build_component_ref_1 (instance, field, 0);

      if (instance == error_mark_node)
	return error_mark_node;

      if (IS_AGGR_TYPE (TREE_TYPE (instance)))
	return build_opfncall (CALL_EXPR, LOOKUP_NORMAL,
			       instance, parms, NULL_TREE);
      else if (TREE_CODE (TREE_TYPE (instance)) == FUNCTION_TYPE
	       || (TREE_CODE (TREE_TYPE (instance)) == POINTER_TYPE
		   && (TREE_CODE (TREE_TYPE (TREE_TYPE (instance)))
		       == FUNCTION_TYPE)))
	return build_function_call (instance, parms);
    }

  return NULL_TREE;
}

/* Returns nonzero iff the destructor name specified in NAME
   (a BIT_NOT_EXPR) matches BASETYPE.  The operand of NAME can take many
   forms...  */

int
check_dtor_name (basetype, name)
     tree basetype, name;
{
  name = TREE_OPERAND (name, 0);

  /* Just accept something we've already complained about.  */
  if (name == error_mark_node)
    return 1;

  if (TREE_CODE (name) == TYPE_DECL)
    name = TREE_TYPE (name);
  else if (TYPE_P (name))
    /* OK */;
  else if (TREE_CODE (name) == IDENTIFIER_NODE)
    {
      if ((IS_AGGR_TYPE (basetype) && name == constructor_name (basetype))
	  || (TREE_CODE (basetype) == ENUMERAL_TYPE
	      && name == TYPE_IDENTIFIER (basetype)))
	name = basetype;
      else
	name = get_type_value (name);
    }
  /* In the case of:
      
       template <class T> struct S { ‾S(); };
       int i;
       i.‾S();

     NAME will be a class template.  */
  else if (DECL_CLASS_TEMPLATE_P (name))
    return 0;
  else
    abort ();

  if (name && TYPE_MAIN_VARIANT (basetype) == TYPE_MAIN_VARIANT (name))
    return 1;
  return 0;
}

/* Build a method call of the form `EXP->SCOPES::NAME (PARMS)'.
   This is how virtual function calls are avoided.  */

tree
build_scoped_method_call (exp, basetype, name, parms)
     tree exp, basetype, name, parms;
{
  /* Because this syntactic form does not allow
     a pointer to a base class to be `stolen',
     we need not protect the derived->base conversion
     that happens here.
     
     @@ But we do have to check access privileges later.  */
  tree binfo, decl;
  tree type = TREE_TYPE (exp);

  if (type == error_mark_node
      || basetype == error_mark_node)
    return error_mark_node;

  if (processing_template_decl)
    {
      if (TREE_CODE (name) == BIT_NOT_EXPR
	  && TREE_CODE (TREE_OPERAND (name, 0)) == IDENTIFIER_NODE)
	{
	  tree type = get_aggr_from_typedef (TREE_OPERAND (name, 0), 0);
	  if (type)
	    name = build_min_nt (BIT_NOT_EXPR, type);
	}
      name = build_min_nt (SCOPE_REF, basetype, name);
      return build_min_nt (METHOD_CALL_EXPR, name, exp, parms, NULL_TREE);
    }

  if (TREE_CODE (type) == REFERENCE_TYPE)
    type = TREE_TYPE (type);

  if (TREE_CODE (basetype) == TREE_VEC)
    {
      binfo = basetype;
      basetype = BINFO_TYPE (binfo);
    }
  else
    binfo = NULL_TREE;

  /* Check the destructor call syntax.  */
  if (TREE_CODE (name) == BIT_NOT_EXPR)
    {
      /* We can get here if someone writes their destructor call like
	 `obj.NS::‾T()'; this isn't really a scoped method call, so hand
	 it off.  */
      if (TREE_CODE (basetype) == NAMESPACE_DECL)
	return build_method_call (exp, name, parms, NULL_TREE, LOOKUP_NORMAL);

      if (! check_dtor_name (basetype, name))
	error ("qualified type `%T' does not match destructor name `‾%T'",
		  basetype, TREE_OPERAND (name, 0));

      /* Destructors can be "called" for simple types; see 5.2.4 and 12.4 Note
	 that explicit ‾int is caught in the parser; this deals with typedefs
	 and template parms.  */
      if (! IS_AGGR_TYPE (basetype))
	{
	  if (TYPE_MAIN_VARIANT (type) != TYPE_MAIN_VARIANT (basetype))
	    error ("type of `%E' does not match destructor type `%T' (type was `%T')",
		      exp, basetype, type);

	  return cp_convert (void_type_node, exp);
	}
    }

  if (TREE_CODE (basetype) == NAMESPACE_DECL)
    {
      error ("`%D' is a namespace", basetype);
      return error_mark_node;
    }
  if (! is_aggr_type (basetype, 1))
    return error_mark_node;

  if (! IS_AGGR_TYPE (type))
    {
      error ("base object `%E' of scoped method call is of non-aggregate type `%T'",
		exp, type);
      return error_mark_node;
    }

  if (! binfo)
    {
      binfo = lookup_base (type, basetype, ba_check, NULL);
      if (binfo == error_mark_node)
	return error_mark_node;
      if (! binfo)
	error_not_base_type (basetype, type);
    }

  if (binfo)
    {
      if (TREE_CODE (exp) == INDIRECT_REF)
	{
	  decl = build_base_path (PLUS_EXPR,
				  build_unary_op (ADDR_EXPR, exp, 0),
				  binfo, 1);
	  decl = build_indirect_ref (decl, NULL);
	}
      else
	decl = build_scoped_ref (exp, basetype);

      /* Call to a destructor.  */
      if (TREE_CODE (name) == BIT_NOT_EXPR)
	{
	  if (! TYPE_HAS_DESTRUCTOR (TREE_TYPE (decl)))
	    return cp_convert (void_type_node, exp);
	  
	  return build_delete (TREE_TYPE (decl), decl, 
			       sfk_complete_destructor,
			       LOOKUP_NORMAL|LOOKUP_NONVIRTUAL|LOOKUP_DESTRUCTOR,
			       0);
	}

      /* Call to a method.  */
      return build_method_call (decl, name, parms, binfo,
				LOOKUP_NORMAL|LOOKUP_NONVIRTUAL);
    }
  return error_mark_node;
}

/* We want the address of a function or method.  We avoid creating a
   pointer-to-member function.  */

tree
build_addr_func (function)
     tree function;
{
  tree type = TREE_TYPE (function);

  /* We have to do these by hand to avoid real pointer to member
     functions.  */
  if (TREE_CODE (type) == METHOD_TYPE)
    {
      tree addr;

      type = build_pointer_type (type);

      if (mark_addressable (function) == 0)
	return error_mark_node;

      addr = build1 (ADDR_EXPR, type, function);

      /* Address of a static or external vari