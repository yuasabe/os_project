/* Handle exceptional things in C++.
   Copyright (C) 1989, 1992, 1993, 1994, 1995, 1996, 1997, 1998, 1999,
   2000, 2001  Free Software Foundation, Inc.
   Contributed by Michael Tiemann <tiemann@cygnus.com>
   Rewritten by Mike Stump <mrs@cygnus.com>, based upon an
   initial re-implementation courtesy Tad Hunt.

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
#include "../gcc/rtl.h"
#include "../gcc/expr.h"
#include "../gcc/libfuncs.h"
#include "cp-tree.h"
#include "../gcc/flags.h"
#include "../include/obstack.h"
#include "../gcc/output.h"
#include "../gcc/except.h"
#include "../gcc/toplev.h"
/* end of !kawai! */

static void push_eh_cleanup PARAMS ((tree));
static tree prepare_eh_type PARAMS ((tree));
static tree build_eh_type_type PARAMS ((tree));
static tree do_begin_catch PARAMS ((void));
static int dtor_nothrow PARAMS ((tree));
static tree do_end_catch PARAMS ((tree));
static void push_eh_cleanup PARAMS ((tree));
static bool decl_is_java_type PARAMS ((tree decl, int err));
static void initialize_handler_parm PARAMS ((tree, tree));
static tree do_allocate_exception PARAMS ((tree));
static int complete_ptr_ref_or_void_ptr_p PARAMS ((tree, tree));
static bool is_admissible_throw_operand PARAMS ((tree));
static int can_convert_eh PARAMS ((tree, tree));
static void check_handlers_1 PARAMS ((tree, tree));
static tree cp_protect_cleanup_actions PARAMS ((void));

/* !kawai */
#include "decl.h"
/* #include "obstack.h" */
/* end of !kawai! */

/* Sets up all the global eh stuff that needs to be initialized at the
   start of compilation.  */

void
init_exception_processing ()
{
  tree tmp;

  /* void std::terminate (); */
  push_namespace (std_identifier);
  tmp = build_function_type (void_type_node, void_list_node);
  terminate_node = build_cp_library_fn_ptr ("terminate", tmp);
  TREE_THIS_VOLATILE (terminate_node) = 1;
  TREE_NOTHROW (terminate_node) = 1;
  pop_namespace ();

  /* void __cxa_call_unexpected(void *); */
  tmp = tree_cons (NULL_TREE, ptr_type_node, void_list_node);
  tmp = build_function_type (void_type_node, tmp);
  call_unexpected_node
    = push_throw_library_fn (get_identifier ("__cxa_call_unexpected"), tmp);

  eh_personality_libfunc = init_one_libfunc (USING_SJLJ_EXCEPTIONS
					     ? "__gxx_personality_sj0"
					     : "__gxx_personality_v0");

  lang_eh_runtime_type = build_eh_type_type;
  lang_protect_cleanup_actions = &cp_protect_cleanup_actions;
}

/* Returns an expression to be executed if an unhandled exception is
   propagated out of a cleanup region.  */

static tree
cp_protect_cleanup_actions ()
{
  /* [except.terminate]

     When the destruction of an object during stack unwinding exits
     using an exception ... void terminate(); is called.  */
  return build_call (terminate_node, NULL_TREE);
}     

static tree
prepare_eh_type (type)
     tree type;
{
  if (type == NULL_TREE)
    return type;
  if (type == error_mark_node)
    return error_mark_node;

  /* peel back references, so they match.  */
  if (TREE_CODE (type) == REFERENCE_TYPE)
    type = TREE_TYPE (type);

  /* Peel off cv qualifiers.  */
  type = TYPE_MAIN_VARIANT (type);

  return type;
}

/* Build the address of a typeinfo decl for use in the runtime
   matching field of the exception model.   */

static tree
build_eh_type_type (type)
     tree type;
{
  tree exp;

  if (type == NULL_TREE || type == error_mark_node)
    return type;

  if (decl_is_java_type (type, 0))
    exp = build_java_class_ref (TREE_TYPE (type));
  else
    exp = get_tinfo_decl (type);

  mark_used (exp);
  exp = build1 (ADDR_EXPR, ptr_type_node, exp);

  return exp;
}

tree
build_exc_ptr ()
{
  return build (EXC_PTR_EXPR, ptr_type_node);
}

/* Build up a call to __cxa_begin_catch, to tell the runtime that the
   exception has been handled.  */

static tree
do_begin_catch ()
{
  tree fn;

  fn = get_identifier ("__cxa_begin_catch");
  if (IDENTIFIER_GLOBAL_VALUE (fn))
    fn = IDENTIFIER_GLOBAL_VALUE (fn);
  else
    {
      /* Declare void* __cxa_begin_catch (void *).  */
      tree tmp = tree_cons (NULL_TREE, ptr_type_node, void_list_node);
      fn = push_library_fn (fn, build_function_type (ptr_type_node, tmp));
    }

  return build_function_call (fn, tree_cons (NULL_TREE, build_exc_ptr (),
					     NULL_TREE));
}

/* Returns nonzero if cleaning up an exception of type TYPE (which can be
   NULL_TREE for a ... handler) will not throw an exception.  */

static int
dtor_nothrow (type)
     tree type;
{
  tree fn;

  if (type == NULL_TREE)
    return 0;

  if (! TYPE_HAS_DESTRUCTOR (type))
    return 1;

  fn = lookup_member (type, dtor_identifier, 0, 0);
  fn = TREE_VALUE (fn);
  return TREE_NOTHROW (fn);
}

/* Build up a call to __cxa_end_catch, to destroy the exception object
   for the current catch block if no others are currently using it.  */

static tree
do_end_catch (type)
     tree type;
{
  tree fn, cleanup;

  fn = get_identifier ("__cxa_end_catch");
  if (IDENTIFIER_GLOBAL_VALUE (fn))
    fn = IDENTIFIER_GLOBAL_VALUE (fn);
  else
    {
      /* Declare void __cxa_end_catch ().  */
      fn = push_void_library_fn (fn, void_list_node);
      /* This can throw if the destructor for the exception throws.  */
      TREE_NOTHROW (fn) = 0;
    }

  cleanup = build_function_call (fn, NULL_TREE);
  TREE_NOTHROW (cleanup) = dtor_nothrow (type);

  return cleanup;
}

/* This routine creates the cleanup for the current exception.  */

static void
push_eh_cleanup (type)
     tree type;
{
  finish_decl_cleanup (NULL_TREE, do_end_catch (type));
}

/* Return nonzero value if DECL is a Java type suitable for catch or
   throw.  */

static bool
decl_is_java_type (decl, err)
     tree decl;
     int err;
{
  bool r = (TREE_CODE (decl) == POINTER_TYPE
	    && TREE_CODE (TREE_TYPE (decl)) == RECORD_TYPE
	    && TYPE_FOR_JAVA (TREE_TYPE (decl)));

  if (err)
    {
      if (TREE_CODE (decl) == REFERENCE_TYPE
	  && TREE_CODE (TREE_TYPE (decl)) == RECORD_TYPE
	  && TYPE_FOR_JAVA (TREE_TYPE (decl)))
	{
	  /* Can't throw a reference.  */
	  error ("type `%T' is disallowed in Java `throw' or `catch'",
		    decl);
	}

      if (r)
	{
	  tree jthrow_node
	    = IDENTIFIER_GLOBAL_VALUE (get_identifier ("jthrowable"));

	  if (jthrow_node == NULL_TREE)
	    fatal_error
	      ("call to Java `catch' or `throw' with `jthrowable' undefined");

	  jthrow_node = TREE_TYPE (TREE_TYPE (jthrow_node));

	  if (! DERIVED_FROM_P (jthrow_node, TREE_TYPE (decl)))
	    {
	      /* Thrown object must be a Throwable.  */
	      error ("type `%T' is not derived from `java::lang::Throwable'",
			TREE_TYPE (decl));
	    }
	}
    }

  return r;
}

/* Select the personality routine to be used for exception handling,
   or issue an error if we need two different ones in the same
   translation unit.
   ??? At present eh_personality_libfunc is set to
   __gxx_personality_(sj|v)0 in init_exception_processing - should it
   be done here instead?  */
void
choose_personality_routine (lang)
     enum languages lang;
{
  static enum {
    chose_none,
    chose_cpp,
    chose_java,
    gave_error
  } state;

  switch (state)
    {
    case gave_error:
      return;

    case chose_cpp:
    