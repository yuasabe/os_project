/* RunTime Type Identification
   Copyright (C) 1995, 1996, 1997, 1998, 1999, 2000, 2001, 2002
   Free Software Foundation, Inc.
   Mostly written by Jason Merrill (jason@cygnus.com).

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
#include "../gcc/tree.h"
#include "cp-tree.h"
#include "../gcc/flags.h"
#include "../gcc/output.h"
#include "../gcc/toplev.h"
/* end of !kawai! */

/* C++ returns type information to the user in struct type_info
   objects. We also use type information to implement dynamic_cast and
   exception handlers. Type information for a particular type is
   indicated with an ABI defined structure derived from type_info.
   This would all be very straight forward, but for the fact that the
   runtime library provides the definitions of the type_info structure
   and the ABI defined derived classes. We cannot build declarations
   of them directly in the compiler, but we need to layout objects of
   their type.  Somewhere we have to lie.

   We define layout compatible POD-structs with compiler-defined names
   and generate the appropriate initializations for them (complete
   with explicit mention of their vtable). When we have to provide a
   type_info to the user we reinterpret_cast the internal compiler
   type to type_info.  A well formed program can only explicitly refer
   to the type_infos of complete types (& cv void).  However, we chain
   pointer type_infos to the pointed-to-type, and that can be
   incomplete.  We only need the addresses of such incomplete
   type_info objects for static initialization.

   The type information VAR_DECL of a type is held on the
   IDENTIFIER_GLOBAL_VALUE of the type's mangled name. That VAR_DECL
   will be the internal type.  It will usually have the correct
   internal type reflecting the kind of type it represents (pointer,
   array, function, class, inherited class, etc).  When the type it
   represents is incomplete, it will have the internal type
   corresponding to type_info.  That will only happen at the end of
   translation, when we are emitting the type info objects.  */

/* Accessors for the type_info objects. We need to remember several things
   about each of the type_info types. The global tree nodes such as
   bltn_desc_type_node are TREE_LISTs, and these macros are used to access
   the required information. */
/* The RECORD_TYPE of a type_info derived class. */
#define TINFO_PSEUDO_TYPE(NODE) TREE_TYPE (NODE)
/* The VAR_DECL of the vtable for the type_info derived class.
   This is only filled in at the end of the translation. */
#define TINFO_VTABLE_DECL(NODE) TREE_VALUE (NODE)
/* The IDENTIFIER_NODE naming the real class. */
#define TINFO_REAL_NAME(NODE) TREE_PURPOSE (NODE)

static tree build_headof PARAMS((tree));
static tree ifnonnull PARAMS((tree, tree));
static tree tinfo_name PARAMS((tree));
static tree build_dynamic_cast_1 PARAMS((tree, tree));
static tree throw_bad_cast PARAMS((void));
static tree throw_bad_typeid PARAMS((void));
static tree get_tinfo_decl_dynamic PARAMS((tree));
static tree get_tinfo_ptr PARAMS((tree));
static bool typeid_ok_p PARAMS((void));
static int qualifier_flags PARAMS((tree));
static int target_incomplete_p PARAMS((tree));
static tree tinfo_base_init PARAMS((tree, tree));
static tree generic_initializer PARAMS((tree, tree));
static tree ptr_initializer PARAMS((tree, tree, int *));
static tree ptm_initializer PARAMS((tree, tree, int *));
static tree dfs_class_hint_mark PARAMS ((tree, void *));
static tree dfs_class_hint_unmark PARAMS ((tree, void *));
static int class_hint_flags PARAMS((tree));
static tree class_initializer PARAMS((tree, tree, tree));
static tree create_pseudo_type_info PARAMS((const char *, int, ...));
static tree get_pseudo_ti_init PARAMS ((tree, tree, int *));
static tree get_pseudo_ti_desc PARAMS((tree));
static void create_tinfo_types PARAMS((void));
static int typeinfo_in_lib_p PARAMS((tree));

static int doing_runtime = 0;


/* Declare language defined type_info type and a pointer to const
   type_info.  This is incomplete here, and will be completed when
   the user #includes <typeinfo>.  There are language defined
   restrictions on what can be done until that is included.  Create
   the internal versions of the ABI types.  */

void
init_rtti_processing ()
{
  push_namespace (std_identifier);
  type_info_type_node = xref_tag
    (class_type_node, get_identifier ("type_info"), 1);
  pop_namespace ();
  type_info_ptr_type = 
    build_pointer_type
     (build_qualified_type (type_info_type_node, TYPE_QUAL_CONST));

  create_tinfo_types ();
}

/* Given the expression EXP of type `class *', return the head of the
   object pointed to by EXP with type cv void*, if the class has any
   virtual functions (TYPE_POLYMORPHIC_P), else just return the
   expression.  */

static tree
build_headof (exp)
     tree exp;
{
  tree type = TREE_TYPE (exp);
  tree offset;
  tree index;

  my_friendly_assert (TREE_CODE (type) == POINTER_TYPE, 20000112);
  type = TREE_TYPE (type);

  if (!TYPE_POLYMORPHIC_P (type))
    return exp;

  /* We use this a couple of times below, protect it.  */
  exp = save_expr (exp);

  /* The offset-to-top field is at index -2 from the vptr.  */
  index = build_int_2 (-2, -1);

  offset = build_vtbl_ref (build_indirect_ref (exp, NULL), index);

  type = build_qualified_type (ptr_type_node, 
			       cp_type_quals (TREE_TYPE (exp)));
  return build (PLUS_EXPR, type, exp,
		cp_convert (ptrdiff_type_node, offset));
}

/* Get a bad_cast node for the program to throw...

   See libstdc++/exception.cc for __throw_bad_cast */

static tree
throw_bad_cast ()
{
  tree fn = get_identifier ("__cxa_bad_cast");
  if (IDENTIFIER_GLOBAL_VALUE (fn))
    fn = IDENTIFIER_GLOBAL_VALUE (fn);
  else
    fn = push_throw_library_fn (fn, build_function_type (ptr_type_node,
							 void_list_node));
  
  return build_call (fn, NULL_TREE);
}

static tree
throw_bad_typeid ()
{
  tree fn = get_identifier ("__cxa_bad_typeid");
  if (IDENTIFIER_GLOBAL_VALUE (fn))
    fn = IDENTIFIER_GLOBAL_VALUE (fn);
  else
    {
      tree t = build_qualified_type (type_info_type_node, TYPE_QUAL_CONST);
      t = build_function_type (build_reference_type (t), void_list_node);
      fn = push_throw_library_fn (fn, t);
    }

  return build_call (fn, NULL_TREE);
}

/* Return a pointer to type_info function associated with the expression EXP.
   If EXP is a reference to a polymorphic class, return the dynamic type;
   otherwise return the static type of the expression.  */

static tree
get_tinfo_decl_dynamic (exp)
     tree exp;
{
  tree type;
  
  if (exp == error_mark_node)
    return error_mark_node;

  type = TREE_TYPE (exp);

  /* peel back references, so they match.  */
  if (TREE_CODE (type) == REFERENCE_TYPE)
    type = TREE_TYPE (type);

  /* Peel off cv qualifiers.  */
  type = TYPE_MAIN_VARIANT (type);
  
  if (!VOID_TYPE_P (type))
    type = complete_type_or_else (type, exp);
  
  if (!type)
    return error_mark_node;

  /* If exp is a reference to polymorphic type, get the real type_info.  */
  if (TYPE_POLYMORPHIC_P (type) && ! resolves_to_fixed_type_p (exp, 0))
    {
      /* build reference to type_info from vtable.  */
      tree t;
      tree index;

      /* The RTTI information is at index -1.  */
      index = integer_minus_one_node;
      t = build_vtbl_ref (exp, index);
      TREE_TYPE (t) = type_info_ptr_type;
      return t;
    }

  /* Otherwise return the type_info for the static type of the expr.  */
  return get_tinfo_ptr (TYPE_MAIN_VARIANT (type));
}

static bool
typeid_ok_p ()
{
  if (! flag_rtti)
    {
      error ("cannot use typeid with -fno-rtti");
      return false;
    }
  
  if (!COMPLETE_TYPE_P (type_info_type_node))
    {
      error ("must #include <typeinfo> before using typeid");
      return false;
    }
  
  return true;
}

tree
build_typeid (exp)
     tree exp;
{
  tree cond = NULL_TREE;
  int nonnull = 0;

  if (exp == error_mark_node || !typeid_ok_p ())
    return error_mark_node;

  if (processing_template_decl)
    return build_min_nt (TYPEID_EXPR, exp);

  if (TREE_CODE (exp) == INDIRECT_REF
      && TREE_CODE (TREE_TYPE (TREE_OPERAND (exp, 0))) == POINTER_TYPE
      && TYPE_POLYMORPHIC_P (TREE_TYPE (exp))
      && ! resolves_to_fixed_type_p (exp, &nonnull)
      && ! nonnull)
    {
      exp = stabilize_reference (exp);
      cond = cp_convert (boolean_type_node, TREE_OPERAND (exp, 0));
    }

  exp = get_tinfo_decl_dynamic (exp);

  if (exp == error_mark_node)
    return error_mark_node;

  exp = build_indirect_ref (exp, NULL);

  if (cond)
    {
      tree bad = throw_bad_typeid ();

      exp = build (COND_EXPR, TREE_TYPE (exp), cond, exp, bad);
    }

  return convert_from_reference (exp);
}

/* Generate the NTBS name of a type.  */
static tree
tinfo_name (type)
     tree type;
{
  const char *name;
  tree name_string;

  name = mangle_type_string (type);
  name_string = combine_strings (build_string (strlen (name) + 1, name));
  return name_string;
}

/* Return a VAR_DECL for the internal ABI defined type_info object for
   TYPE. You must arrange that the decl is mark_used, if actually use
   it --- decls in vtables are only used if the vtable is output.  */ 

tree
get_tinfo_decl (type)
     tree type;
{
  tree name;
  tree d;

  if (COMPLETE_TYPE_P (type) 
      && TREE_CODE (TYPE_SIZE (type)) != INTEGER_CST)
    {
      error ("cannot create type information for type `%T' because its size is variable", 
	     type);
      return error_mark_node;
    }

  if (TREE_CODE (type) == OFFSET_TYPE)
    type = TREE_TYPE (type);
  if (TREE_CODE (type) == METHOD_TYPE)
    type = build_function_type (TREE_TYPE (type),
				TREE_CHAIN (TYPE_ARG_TYPES (type)));

  name = mangle_typeinfo_for_type (type);

  d = IDENTIFIER_GLOBAL_VALUE (name);
  if (!d)
    {
      tree var_desc = get_pseudo_ti_desc (type);

      d = build_lang_decl (VAR_DECL, name, TINFO_PSEUDO_TYPE (var_desc));
      
      DECL_ARTIFICIAL (d) = 1;
      TREE_READONLY (d) = 1;
      TREE_STATIC (d) = 1;
      DECL_EXTERNAL (d) = 1;
      SET_DECL_ASSEMBLER_NAME (d, name);
      DECL_COMDAT (d) = 1;
      cp_finish_decl (d, NULL_TREE, NULL_TREE, 0);

      pushdecl_top_level (d);

      /* Remember the type it is for.  */
      TREE_TYPE (name) = type;
    }

  return d;
}

/* Return a pointer to a type_info object describing TYPE, suitably
   cast to the language defined type.  */

static tree
get_tinfo_ptr (type)
     tree type;
{
  tree exp = get_tinfo_decl (type);
  
   /* Convert to type_info type.  */
  exp = build_unary_op (ADDR_EXPR, exp, 0);
  exp = ocp_convert (type_info_ptr_type, exp, CONV_REINTERPRET, 0);

  return exp;
}

/* Return the type_info object for TYPE.  */

tree
get_typeid (type)
     tree type;
{
  if (type == error_mark_node || !typeid_ok_p ())
    return error_mark_node;
  
  if (processing_template_decl)
    return build_min_nt (TYPEID_EXPR, type);

  /* If the type of the type-id is a reference type, the result of the
     typeid expression refers to a type_info object representing the
     referenced type.  */
  if (TREE_CODE (type) == REFERENCE_TYPE)
    type = TREE_TYPE (type);

  /* The top-level cv-qualifiers of the lvalue expression or the type-id
     that is the operand of typeid are always ignored.  */
  type = TYPE_MAIN_VARIANT (type);

  if (!VOID_TYPE_P (type))
    type = complete_type_or_else (type, NULL_TREE);
  
  if (!type)
    return error_mark_node;

  return build_indirect_ref (get_tinfo_ptr (type), NULL);
}

/* Check whether TEST is null before returning RESULT.  If TEST is used in
   RESULT, it must have previously had a save_expr applied to it.  */

static tree
ifnonnull (test, result)
     tree test, result;
{
  return build (COND_EXPR, TREE_TYPE (result),
		build (EQ_EXPR, boolean_type_node, test, integer_zero_node),
		cp_convert (TREE_TYPE (result), integer_zero_node),
		result);
}

/* Execute a dynamic cast, as described in section 5.2.6 of the 9/93 working
   paper.  */

static tree
build_dynamic_cast_1 (type, expr)
     tree type, expr;
{
  enum tree_code tc = TREE_CODE (type);
  tree exprtype = TREE_TYPE (expr);
  tree dcast_fn;
  tree old_expr = expr;
  const char *errstr = NULL;

  /* T shall be a pointer or reference to a complete class type, or
     `pointer to cv void''.  */
  switch (tc)
    {
    case POINTER_TYPE:
      if (TREE_CODE (TREE_TYPE (type)) == VOID_TYPE)
	break;
    case REFERENCE_TYPE:
      if (! IS_AGGR_TYPE (TREE_TYPE (type)))
	{
	  errstr = "target is not pointer or reference to class";
	  goto fail;
	}
      if (!COMPLETE_TYPE_P (complete_type (TREE_TYPE (type))))
	{
	  errstr = "target is not pointer or reference to complete type";
	  goto fail;
	}
      break;

    default:
      errstr = "target is not pointer or reference";
      goto fail;
    }

  if (TREE_CODE (expr) == OFFSET_REF)
    {
      expr = resolve_offset_ref (expr);
      exprtype = TREE_TYPE (expr);
    }

  if (tc == POINTER_TYPE)
    expr = convert_from_reference (expr);
  else if (TREE_CODE (exprtype) != REFERENCE_TYPE)
    {
      /* Apply trivial conversion T -> T& for dereferenced ptrs.  */
      exprtype = build_reference_type (exprtype);
      expr = convert_to_reference (exprtype, expr, CONV_IMPLICIT,
				   LOOKUP_NORMAL, NULL_TREE);
    }

  exprtype = TREE_TYPE (expr);

  if (tc == POINTER_TYPE)
    {
      /* If T is a pointer type, v shall be an rvalue of a pointer to
	 complete class type, and the result is an rvalue of type T.  */

      if (TREE_CODE (exprtype) != POINTER_TYPE)
	{
	  errstr = "source is not a pointer";
	  goto fail;
	}
      if (! IS_AGGR_TYPE (TREE_TYPE (exprtype)))
	{
	  errstr = "source is not a pointer to class";
	  goto fail;
	}
      if (!COMPLETE_TYPE_P (complete_type (TREE_TYPE (exprtype))))
	{
	  errstr = "source is a pointer to incomplete type";
	  goto fail;
	}
    }
  else
    {
      /* T is a reference type, v shall be an lvalue of a complete class
	 type, and the result is an lvalue of the type referred to by T.  */

      if (! IS_AGGR_TYPE (TREE_TYPE (exprtype)))
	{
	  errstr = "source is not of class type";
	  goto fail;
	}
      if (!COMPLETE_TYPE_P (complete_type (TREE_TYPE (exprtype))))
	{
	  errstr = "source is of incomplete class type";
	  goto fail;
	}
      
    }

  /* The dynamic_cast operator shall not cast away constness.  */
  if (!at_least_as_qualified_p (TREE_TYPE (type),
				TREE_TYPE (exprtype)))
    {
      errstr = "conversion casts away constness";
      goto fail;
    }

  /* If *type is an unambiguous accessible base class of *exprtype,
     convert statically.  */
  {
    tree binfo;

    binfo = lookup_base (TREE_TYPE (exprtype), TREE_TYPE (type),
			 ba_not_special, NULL);

    if (binfo)
      {
	expr = build_base_path (PLUS_EXPR, convert_from_reference (expr),
				binfo, 0);
	if (TREE_CODE (exprtype) == POINTER_TYPE)
	  expr = non_lvalue (expr);
	return expr;
      }
  }

  /* Otherwise *exprtype must be a polymorphic class (have a vtbl).  */
  if (TYPE_POLYMORPHIC_P (TREE_TYPE (exprtype)))
    {
      tree expr1;
      /* if TYPE is `void *', return pointer to complete object.  */
      if (tc == POINTER_TYPE && VOID_TYPE_P (TREE_TYPE (type)))
	{
	  /* if b is an object, dynamic_cast<void *>(&b) == (void *)&b.  */
	  if (TREE_CODE (expr) == ADDR_EXPR
	      && TREE_CODE (TREE_OPERAND (expr, 0)) == VAR_DECL
	      && TREE_CODE (TREE_TYPE (TREE_OPERAND (expr, 0))) == RECORD_TYPE)
	    return build1 (NOP_EXPR, type, expr);

	  /* Since expr is used twice below, save it.  */
	  expr = save_expr (expr);

	  expr1 = build_headof (expr);
	  if (TREE_TYPE (expr1) != type)
	    expr1 = build1 (NOP_EXPR, type, expr1);
	  return ifnonnull (expr, expr1);
	}
      else
	{
	  tree retval;
          tree result, td2, td3, elems;
          tree static_type, target_type, boff;

 	  /* If we got here, we can't convert statically.  Therefore,
	     dynamic_cast<D&>(b) (b an object) cannot succeed.  */
	  if (tc == REFERENCE_TYPE)
	    {
	      if (TREE_CODE (old_expr) == VAR_DECL
		  && TREE_CODE (TREE_TYPE (old_expr)) == RECORD_TYPE)
		{
	          tree expr = throw_bad_cast ();
		  warning ("dynamic_cast of `%#D' to `%#T' can never succeed",
			      old_expr, type);
	          /* Bash it to the expected type.  */
	          TREE_TYPE (expr) = type;
		  return expr;
		}
	    }
	  /* Ditto for dynamic_cast<D*>(&b).  */
	  else if (TREE_CODE (expr) == ADDR_EXPR)
	    {
	      tree op = TREE_OPERAND (expr, 0);
	      if (TREE_CODE (op) == VAR_DECL
		  && TREE_CODE (TREE_TYPE (op)) == RECORD_TYPE)
		{
		  warning ("dynamic_cast of `%#D' to `%#T' can never succeed",
			      op, type);
		  retval = build_int_2 (0, 0); 
		  TREE_TYPE (retval) = type; 
		  return retval;
		}
	    }

	  target_type = TYPE_MAIN_VARIANT (TREE_TYPE (type));
	  static_type = TYPE_MAIN_VARIANT (TREE_TYPE (exprtype));
	  td2 = build_unary_op (ADDR_EXPR, get_tinfo_decl (target_type), 0);
	  td3 = build_unary_op (ADDR_EXPR, get_tinfo_decl (static_type), 0);

          /* Determine how T and V are related.  */
          boff = get_dynamic_cast_base_type (static_type, target_type);
          
	  /* Since expr is used twice below, save it.  */
	  expr = save_expr (expr);

	  expr1 = expr;
	  if (tc == REFERENCE_TYPE)
	    expr1 = build_unary_op (ADDR_EXPR, expr1, 0);

	  elems = tree_cons
	    (NULL_TREE, expr1, tree_cons
	     (NULL_TREE, td3, tree_cons
	      (NULL_TREE, td2, tree_cons
	       (NULL_TREE, boff, NULL_TREE))));

	  dcast_fn = dynamic_cast_node;
	  if (!dcast_fn)
	    {
	      tree tmp;
	      tree tinfo_ptr;
	      tree ns = abi_node;
	      const char *name;
	      
	      push_nested_namespace (ns);
	      tinfo_ptr = xref_tag (class_type_node,
				    get_identifier ("__class_type_info"),
				    1);
	      
	      tinfo_ptr = build_pointer_type
		(build_qualified_type
		 (tinfo_ptr, TYPE_QUAL_CONST));
	      name = "__dynamic_cast";
	      tmp = tree_cons
		(NULL_TREE, const_ptr_type_node, tree_cons
		 (NULL_TREE, tinfo_ptr, tree_cons
		  (NULL_TREE, tinfo_ptr, tree_cons
		   (NULL_TREE, ptrdiff_type_node, void_list_node))));
	      tmp = build_function_type (ptr_type_node, tmp);
	      dcast_fn = build_library_fn_ptr (name, tmp);
              pop_nested_namespace (ns);
              dynamic_cast_node = dcast_fn;
	    }
          result = build_call (dcast_fn, elems);

	  if (tc == REFERENCE_TYPE)
	    {
	      tree bad = throw_bad_cast ();
	      
	      result = save_expr (result);
	      return build (COND_EXPR, type, result, result, bad);
	    }

	  /* Now back to the type we want from a void*.  */
	  result = cp_convert (type, result);
          return ifnonnull (expr, result);
	}
    }
  else
    errstr = "source type is not polymorphic";

 fail:
  error ("cannot dynamic_cast `%E' (of type `%#T') to type `%#T' (%s)",
	    expr, exprtype, type, errstr);
  return error_mark_node;
}

tree
build_dynamic_cast (type, expr)
     tree type, expr;
{
  if (type == error_mark_node || expr == error_mark_node)
    return error_mark_node;
  
  if (processing_template_decl)
    return build_min (DYNAMIC_CAST_EXPR, type, ex