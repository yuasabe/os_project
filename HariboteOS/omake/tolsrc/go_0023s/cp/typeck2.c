/* Report error messages, build initializers, and perform
   some front-end optimizations for C++ compiler.
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
#include "cp-tree.h"
#include "../gcc/flags.h"
#include "../gcc/toplev.h"
#include "../gcc/output.h"
#include "../gcc/diagnostic.h"
/* end of !kawai! */

static tree process_init_constructor PARAMS ((tree, tree, tree *));

/* Print an error message stemming from an attempt to use
   BASETYPE as a base class for TYPE.  */

tree
error_not_base_type (basetype, type)
     tree basetype, type;
{
  if (TREE_CODE (basetype) == FUNCTION_DECL)
    basetype = DECL_CONTEXT (basetype);
  error ("type `%T' is not a base type for type `%T'", basetype, type);
  return error_mark_node;
}

tree
binfo_or_else (base, type)
     tree base, type;
{
  tree binfo = lookup_base (type, base, ba_ignore, NULL);

  if (binfo == error_mark_node)
    return NULL_TREE;
  else if (!binfo)
    error_not_base_type (base, type);
  return binfo;
}

/* According to ARM $7.1.6, "A `const' object may be initialized, but its
   value may not be changed thereafter.  Thus, we emit hard errors for these,
   rather than just pedwarns.  If `SOFT' is 1, then we just pedwarn.  (For
   example, conversions to references.)  */

void
readonly_error (arg, string, soft)
     tree arg;
     const char *string;
     int soft;
{
  const char *fmt;
  void (*fn) PARAMS ((const char *, ...));

  if (soft)
    fn = pedwarn;
  else
    fn = error;

  if (TREE_CODE (arg) == COMPONENT_REF)
    {
      if (TYPE_READONLY (TREE_TYPE (TREE_OPERAND (arg, 0))))
        fmt = "%s of data-member `%D' in read-only structure";
      else
        fmt = "%s of read-only data-member `%D'";
      (*fn) (fmt, string, TREE_OPERAND (arg, 1));
    }
  else if (TREE_CODE (arg) == VAR_DECL)
    {
      if (DECL_LANG_SPECIFIC (arg)
	  && DECL_IN_AGGR_P (arg)
	  && !TREE_STATIC (arg))
	fmt = "%s of constant field `%D'";
      else
	fmt = "%s of read-only variable `%D'";
      (*fn) (fmt, string, arg);
    }
  else if (TREE_CODE (arg) == PARM_DECL)
    (*fn) ("%s of read-only parameter `%D'", string, arg);
  else if (TREE_CODE (arg) == INDIRECT_REF
           && TREE_CODE (TREE_TYPE (TREE_OPERAND (arg, 0))) == REFERENCE_TYPE
           && (TREE_CODE (TREE_OPERAND (arg, 0)) == VAR_DECL
               || TREE_CODE (TREE_OPERAND (arg, 0)) == PARM_DECL))
    (*fn) ("%s of read-only reference `%D'", string, TREE_OPERAND (arg, 0));
  else if (TREE_CODE (arg) == RESULT_DECL)
    (*fn) ("%s of read-only named return value `%D'", string, arg);
  else if (TREE_CODE (arg) == FUNCTION_DECL)
    (*fn) ("%s of function `%D'", string, arg);
  else
    (*fn) ("%s of read-only location", string);
}

/* If TYPE has abstract virtual functions, issue an error about trying
   to create an object of that type.  DECL is the object declared, or
   NULL_TREE if the declaration is unavailable.  Returns 1 if an error
   occurred; zero if all was well.  */

int
abstract_virtuals_error (decl, type)
     tree decl;
     tree type;
{
  tree u;
  tree tu;

  if (!CLASS_TYPE_P (type) || !CLASSTYPE_PURE_VIRTUALS (type))
    return 0;

  if (!TYPE_SIZE (type))
    /* TYPE is being defined, and during that time
       CLASSTYPE_PURE_VIRTUALS holds the inline friends.  */
    return 0;

  u = CLASSTYPE_PURE_VIRTUALS (type);
  if (decl)
    {
      if (TREE_CODE (decl) == RESULT_DECL)
	return 0;

      if (TREE_CODE (decl) == VAR_DECL)
	error ("cannot declare variable `%D' to be of type `%T'",
		    decl, type);
      else if (TREE_CODE (decl) == PARM_DECL)
	error ("cannot declare parameter `%D' to be of type `%T'",
		    decl, type);
      else if (TREE_CODE (decl) == FIELD_DECL)
	error ("cannot declare field `%D' to be of type `%T'",
		    decl, type);
      else if (TREE_CODE (decl) == FUNCTION_DECL
	       && TREE_CODE (TREE_TYPE (decl)) == METHOD_TYPE)
	error ("invalid return type for member function `%#D'", decl);
      else if (TREE_CODE (decl) == FUNCTION_DECL)
	error ("invalid return type for function `%#D'", decl);
    }
  else
    error ("cannot allocate an object of type `%T'", type);

  /* Only go through this once.  */
  if (TREE_PURPOSE (u) == NULL_TREE)
    {
      TREE_PURPOSE (u) = error_mark_node;

      error ("  because the following virtual functions are abstract:");
      for (tu = u; tu; tu = TREE_CHAIN (tu))
	cp_error_at ("¥t%#D", TREE_VALUE (tu));
    }
  else
    error ("  since type `%T' has abstract virtual functions", type);

  return 1;
}

/* Print an error message for invalid use of an incomplete type.
   VALUE is the expression that was used (or 0 if that isn't known)
   and TYPE is the type that was invalid.  */

void
incomplete_type_error (value, type)
     tree value;
     tree type;
{
  int decl = 0;
  
  /* Avoid duplicate error message.  */
  if (TREE_CODE (type) == ERROR_MARK)
    return;

  if (value != 0 && (TREE_CODE (value) == VAR_DECL
		     || TREE_CODE (value) == PARM_DECL
		     || TREE_CODE (value) == FIELD_DECL))
    {
      cp_error_at ("`%D' has incomplete type", value);
      decl = 1;
    }
retry:
  /* We must print an error message.  Be clever about what it says.  */

  switch (TREE_CODE (type))
    {
    case RECORD_TYPE:
    case UNION_TYPE:
    case ENUMERAL_TYPE:
      if (!decl)
        error ("invalid use of undefined type `%#T'", type);
      if (!TYPE_TEMPLATE_INFO (type))
	cp_error_at ("forward declaration of `%#T'", type);
      else
	cp_error_at ("declaration of `%#T'", type);
      break;

    case VOID_TYPE:
      error ("invalid use of `%T'", type);
      break;

    case ARRAY_TYPE:
      if (TYPE_DOMAIN (type))
        {
          type = TREE_TYPE (type);
          goto retry;
        }
      error ("invalid use of array with unspecified bounds");
      break;

    case OFFSET_TYPE:
    bad_member:
      error ("invalid use of member (did you forget the `&' ?)");
      break;

    case TEMPLATE_TYPE_PARM:
      error ("invalid use of template type parameter");
      break;

    case UNKNOWN_TYPE:
      if (value && TREE_CODE (value) == COMPONENT_REF)
        goto bad_member;
      else if (value && TREE_CODE (value) == ADDR_EXPR)
        error ("address of overloaded function with no contextual type information");
      else if (value && TREE_CODE (value) == OVERLOAD)
        error ("overloaded function with no contextual type information");
      else
        error ("insufficient contextual information to determine type");
      break;
    
    default:
      abort ();
    }
}


/* Perform appropriate conversions on the initial value of a variable,
   store it in the declaration DECL,
   and print any error messages that are appropriate.
   If the init is invalid, store an ERROR_MARK.

   C++: Note that INIT might be a TREE_LIST, which would mean that it is
   a base class initializer for some aggregate type, hopefully compatible
   with DECL.  If INIT is a single element, and DECL is an aggregate
   type, we silently convert INIT into a TREE_LIST, allowing a constructor
   to be called.

   If INIT is a TREE_LIST and there is no constructor, turn INIT
   into a CONSTRUCTOR and use standard initialization techniques.
   Perhaps a warning should be generated?

   Returns value of initializer if initialization could not be
   performed for static variable.  In that case, caller must do
   the storing.  */

tree
store_init_value (decl, init)
     tree decl, init;
{
  register tree value, type;

  /* If variable's type was invalidly declared, just ignore it.  */

  type = TREE_TYPE (decl);
  if (TREE_CODE (type) == ERROR_MARK)
    return NULL_TREE;

#if 0
  /* This breaks arrays, and should not have any effect for other decls.  */
  /* Take care of C++ business up here.  */
  type = TYPE_MAIN_VARIANT (type);
#endif

  if (IS_AGGR_TYPE (type))
    {
      if (! TYPE_HAS_TRIVIAL_INIT_REF (type)
	  && TREE_CODE (init) != CONSTRUCTOR)
	abort ();

      if (TREE_CODE (init) == TREE_LIST)
	{
	  error ("constructor syntax used, but no constructor declared for type `%T'", type);
	  init = build_nt (CONSTRUCTOR, NULL_TREE, nreverse (init));
	}
#if 0
      if (TREE_CODE (init) == CONSTRUCTOR)
	{
	  tree field;

	  /* Check that we're really an aggregate as ARM 8.4.1 defines it.  */
	  if (CLASSTYPE_N_BASECLASSES (type))
	    cp_error_at ("initializer list construction invalid for derived class object `%D'", decl);
	  if (CLASSTYPE_VTBL_PTR (type))
	    cp_error_at ("initializer list construction invalid for polymorphic class object `%D'", decl);
	  if (TYPE_NEEDS_CONSTRUCTING (type))
	    {
	      cp_error_at ("initializer list construction invalid for `%D'", decl);
	      error ("due to the presence of a constructor");
	    }
	  for (field = TYPE_FIELDS (type); field; field = TREE_CHAIN (field))
	    if (TREE_PRIVATE (field) || TREE_PROTECTED (field))
	      {
		cp_error_at ("initializer list construction invalid for `%D'", decl);
		cp_error_at ("due to non-public access of member `%D'", field);
	      }
	  for (field = TYPE_METHODS (type); field; field = TREE_CHAIN (field))
	    if (TREE_PRIVATE (field) || TREE_PROTECTED (field))
	      {
		cp_error_at ("initializer list construction invalid for `%D'", decl);
		cp_error_at ("due to non-public access of member `%D'", field);
	      }
	}
#endif
    }
  else if (TREE_CODE (init) == TREE_LIST
	   && TREE_TYPE (init) != unknown_type_node)
    {
      if (TREE_CODE (decl) == RESULT_DECL)
	{
	  if (TREE_CHAIN (init))
	    {
	      warning ("comma expression used to initialize return value");
	      init = build_compound_expr (init);
	    }
	  else
	    init = TREE_VALUE (init);
	}
      else if (TREE_CODE (init) == TREE_LIST
	       && TREE_CODE (TREE_TYPE (decl)) == ARRAY_TYPE)
	{
	  error ("cannot initialize arrays using this syntax");
	  return NULL_TREE;
	}
      else
	{
	  /* We get here with code like `int a (2);' */
	     
	  if (TREE_CHAIN (init) != NULL_TREE)
	    {
	      pedwarn ("initializer list being treated as compound expression");
	      init = build_compound_expr (init);
	    }
	  else
	    init = TREE_VALUE (init);
	}
    }

  /* End of special C++ code.  */

  /* We might have already run this bracketed initializer through
     digest_init.  Don't do so again.  */
  if (TREE_CODE (init) == CONSTRUCTOR && TREE_HAS_CONSTRUCTOR (init)
      && TREE_TYPE (init)
      && TYPE_MAIN_VARIANT (TREE_TYPE (init)) == TYPE_MAIN_VARIANT (type))
    value = init;
  else
    /* Digest the specified initializer into an expression.  */
    value = digest_init (type, init, (tree *) 0);

  /* Store the expression if valid; else report error.  */

  if (TREE_CODE (value) == ERROR_MARK)
    ;
  /* Other code expects that initializers for objects of types that need
     constructing never make it into DECL_INITIAL, and passes 'init' to
     build_aggr_init without checking DECL_INITIAL.  So just return.  */
  else if (TYPE_NEEDS_CONSTRUCTING (type))
    return value;
  else if (TREE_STATIC (decl)
	   && (! TREE_CONSTANT (value)
	       || ! initializer_constant_valid_p (value, TREE_TYPE (value))
#if 0
	       /* A STATIC PUBLIC int variable doesn't have to be
		  run time inited when doing pic.  (mrs) */
	       /* Since ctors and dtors are the only things that can
		  reference vtables, and they are always written down
		  the vtable definition, we can leave the
		  vtables in initialized data space.
		  However, other initialized data cannot be initialized
		  this way.  Instead a global file-level initializer
		  must do the job.  */
	       || (flag_pic && !DECL_VIRTUAL_P (decl) && TREE_PUBLIC (decl))
#endif
	       ))

    return value;
#if 0 /* No, that's C.  jason 9/19/94 */
  else
    {
      if (pedantic && TREE_CODE (value) == CONSTRUCTOR)
	{
	  if (! TREE_CONSTANT (value) || ! TREE_STATIC (value))
	    pedwarn ("ISO C++ forbids non-constant aggregate initializer expressions");
	}
    }
#endif
  
  /* Store the VALUE in DECL_INITIAL.  If we're building a
     statement-tree we will actually expand the initialization later
     when we output this function.  */
  DECL_INITIAL (decl) = value;
  return NULL_TREE;
}

/* Same as store_init_value, but used for known-to-be-valid static
   initializers.  Used to introduce a static initializer even in data
   structures that may require dynamic initialization.  */

tree
force_store_init_value (decl, init)
     tree decl, init;
{
  tree type = TREE_TYPE (decl);
  int needs_constructing = TYPE_NEEDS_CONSTRUCTING (type);

  TYPE_NEEDS_CONSTRUCTING (type) = 0;

  init = store_init_value (decl, init);
  if (init)
    abort ();

  TYPE_NEEDS_CONSTRUCTING (type) = needs_constructing;

  return init;
}  

/* Digest the parser output INIT as an initializer for type TYPE.
   Return a C expression of type TYPE to represent the initial value.

   If TAIL is nonzero, it points to a variable holding a list of elements
   of which INIT is the first.  We update the list stored there by
   removing from the head all the elements that we use.
   Normally this is only one; we use more than one element only if
   TYPE is an aggregate and INIT is not a constructor.  */

tree
digest_init (type, init, tail)
     tree type, init, *tail;
{
  enum tree_code code = TREE_CODE (type);
  tree element = NULL_TREE;
  tree old_tail_contents = NULL_TREE;
  /* Nonzero if INIT is a braced grouping, which comes in as a CONSTRUCTOR
     tree node which has no TREE_TYPE.  */
  int raw_constructor;

  /* By default, assume we use one element from a list.
     We correct this later in the sole case where it is not true.  */

  if (tail)
    {
      old_tail_contents = *tail;
      *tail = TREE_CHAIN (*tail);
    }

  if (init == error_mark_node || (TREE_CODE (init) == TREE_LIST
				  && TREE_VALUE (init) == error_mark_node))
    return error_mark_node;

  if (TREE_CODE (init) == ERROR_MARK)
    /* __PRETTY_FUNCTION__'s initializer is a bogus expression inside
       a template function. This gets substituted during instantiation. */
    return init;

  /* We must strip the outermost array type when completing the type,
     because the its bounds might be incomplete at the moment.  */
  if (!complete_type_or_else (TREE_CODE (type) == ARRAY_TYPE
			      ? TREE_TYPE (type) : type, NULL_TREE))
    return error_mark_node;
  
  /* Strip NON_LVALUE_EXPRs since we aren't using as an lvalue.  */
  if (TREE_CODE (init) == NON_LVALUE_EXPR)
    init = TREE_OPERAND (init, 0);

  if (TREE_CODE (init) == CONSTRUCTOR && TREE_TYPE (init) == type)
    return init;

  raw_constructor = TREE_CODE (init) == CONSTRUCTOR && TREE_TYPE (init) == 0