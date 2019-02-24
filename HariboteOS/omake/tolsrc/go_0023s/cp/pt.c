/* Handle parameterized types (templates) for GNU C++.
   Copyright (C) 1992, 1993, 1994, 1995, 1996, 1997, 1998, 1999, 2000,
   2001, 2002  Free Software Foundation, Inc.
   Written by Ken Raeburn (raeburn@cygnus.com) while at Watchmaker Computing.
   Rewritten by Jason Merrill (jason@cygnus.com).

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

/* Known bugs or deficiencies include:

     all methods must be provided in header files; can't use a source
     file that contains only the method templates and "just win".  */

/* !kawai! */
#include "../gcc/config.h"
#include "../gcc/system.h"
#include "../include/obstack.h"
#include "../gcc/tree.h"
#include "../gcc/flags.h"
#include "cp-tree.h"
#include "../gcc/tree-inline.h"
#include "decl.h"
#include "parse.h"
#include "lex.h"
#include "../gcc/output.h"
#include "../gcc/except.h"
#include "../gcc/toplev.h"
#include "../gcc/rtl.h"
#include "../gcc/ggc.h"
#include "../gcc/timevar.h"
/* end of !kawai! */

/* The type of functions taking a tree, and some additional data, and
   returning an int.  */
typedef int (*tree_fn_t) PARAMS ((tree, void*));

extern struct obstack permanent_obstack;

/* The PENDING_TEMPLATES is a TREE_LIST of templates whose
   instantiations have been deferred, either because their definitions
   were not yet available, or because we were putting off doing the
   work.  The TREE_PURPOSE of each entry is a SRCLOC indicating where
   the instantiate request occurred; the TREE_VALUE is a either a DECL
   (for a function or static data member), or a TYPE (for a class)
   indicating what we are hoping to instantiate.  */
static tree pending_templates;
static tree last_pending_template;

int processing_template_parmlist;
static int template_header_count;

static tree saved_trees;
static varray_type inline_parm_levels;
static size_t inline_parm_levels_used;

static tree current_tinst_level;

/* A map from local variable declarations in the body of the template
   presently being instantiated to the corresponding instantiated
   local variables.  */
static htab_t local_specializations;

#define obstack_chunk_alloc xmalloc
#define obstack_chunk_free free

#define UNIFY_ALLOW_NONE 0
#define UNIFY_ALLOW_MORE_CV_QUAL 1
#define UNIFY_ALLOW_LESS_CV_QUAL 2
#define UNIFY_ALLOW_DERIVED 4
#define UNIFY_ALLOW_INTEGER 8
#define UNIFY_ALLOW_OUTER_LEVEL 16
#define UNIFY_ALLOW_OUTER_MORE_CV_QUAL 32
#define UNIFY_ALLOW_OUTER_LESS_CV_QUAL 64
#define UNIFY_ALLOW_MAX_CORRECTION 128

#define GTB_VIA_VIRTUAL 1 /* The base class we are examining is
			     virtual, or a base class of a virtual
			     base.  */
#define GTB_IGNORE_TYPE 2 /* We don't need to try to unify the current
			     type with the desired type.  */

static int resolve_overloaded_unification PARAMS ((tree, tree, tree, tree,
						   unification_kind_t, int));
static int try_one_overload PARAMS ((tree, tree, tree, tree, tree,
				     unification_kind_t, int));
static int unify PARAMS ((tree, tree, tree, tree, int));
static void add_pending_template PARAMS ((tree));
static void reopen_tinst_level PARAMS ((tree));
static tree classtype_mangled_name PARAMS ((tree));
static char *mangle_class_name_for_template PARAMS ((const char *,
						     tree, tree));
static tree tsubst_initializer_list PARAMS ((tree, tree));
static int list_eq PARAMS ((tree, tree));
static tree get_class_bindings PARAMS ((tree, tree, tree));
static tree coerce_template_parms PARAMS ((tree, tree, tree,
					   tsubst_flags_t, int));
static void tsubst_enum	PARAMS ((tree, tree, tree));
static tree add_to_template_args PARAMS ((tree, tree));
static tree add_outermost_template_args PARAMS ((tree, tree));
static int maybe_adjust_types_for_deduction PARAMS ((unification_kind_t, tree*,
						     tree*)); 
static int  type_unification_real PARAMS ((tree, tree, tree, tree,
					   int, unification_kind_t, int, int));
static void note_template_header PARAMS ((int));
static tree maybe_fold_nontype_arg PARAMS ((tree));
static tree convert_nontype_argument PARAMS ((tree, tree));
static tree convert_template_argument PARAMS ((tree, tree, tree,
					       tsubst_flags_t, int, tree));
static tree get_bindings_overload PARAMS ((tree, tree, tree));
static int for_each_template_parm PARAMS ((tree, tree_fn_t, void*));
static tree build_template_parm_index PARAMS ((int, int, int, tree, tree));
static int inline_needs_template_parms PARAMS ((tree));
static void push_inline_template_parms_recursive PARAMS ((tree, int));
static tree retrieve_specialization PARAMS ((tree, tree));
static tree retrieve_local_specialization PARAMS ((tree));
static tree register_specialization PARAMS ((tree, tree, tree));
static void register_local_specialization PARAMS ((tree, tree));
static int unregister_specialization PARAMS ((tree, tree));
static tree reduce_template_parm_level PARAMS ((tree, tree, int));
static tree build_template_decl PARAMS ((tree, tree));
static int mark_template_parm PARAMS ((tree, void *));
static tree tsubst_friend_function PARAMS ((tree, tree));
static tree tsubst_friend_class PARAMS ((tree, tree));
static int can_complete_type_without_circularity PARAMS ((tree));
static tree get_bindings_real PARAMS ((tree, tree, tree, int, int, int));
static int template_decl_level PARAMS ((tree));
static tree maybe_get_template_decl_from_type_decl PARAMS ((tree));
static int check_cv_quals_for_unify PARAMS ((int, tree, tree));
static tree tsubst_template_arg_vector PARAMS ((tree, tree, tsubst_flags_t));
static tree tsubst_template_parms PARAMS ((tree, tree, tsubst_flags_t));
static void regenerate_decl_from_template PARAMS ((tree, tree));
static tree most_specialized PARAMS ((tree, tree, tree));
static tree most_specialized_class PARAMS ((tree, tree));
static int template_class_depth_real PARAMS ((tree, int));
static tree tsubst_aggr_type PARAMS ((tree, tree, tsubst_flags_t, tree, int));
static tree tsubst_decl PARAMS ((tree, tree, tree, tsubst_flags_t));
static tree tsubst_arg_types PARAMS ((tree, tree, tsubst_flags_t, tree));
static tree tsubst_function_type PARAMS ((tree, tree, tsubst_flags_t, tree));
static void check_specialization_scope PARAMS ((void));
static tree process_partial_specialization PARAMS ((tree));
static void set_current_access_from_decl PARAMS ((tree));
static void check_default_tmpl_args PARAMS ((tree, tree, int, int));
static tree tsubst_call_declarator_parms PARAMS ((tree, tree,
						  tsubst_flags_t, tree));
static tree get_template_base_recursive PARAMS ((tree, tree,
						 tree, tree, tree, int)); 
static tree get_template_base PARAMS ((tree, tree, tree, tree));
static int verify_class_unification PARAMS ((tree, tree, tree));
static tree try_class_unification PARAMS ((tree, tree, tree, tree));
static int coerce_template_template_parms PARAMS ((tree, tree, tsubst_flags_t,
						   tree, tree));
static tree determine_specialization PARAMS ((tree, tree, tree *, int));
static int template_args_equal PARAMS ((tree, tree));
static void tsubst_default_arguments PARAMS ((tree));
static tree for_each_template_parm_r PARAMS ((tree *, int *, void *));
static tree copy_default_args_to_explicit_spec_1 PARAMS ((tree, tree));
static void copy_default_args_to_explicit_spec PARAMS ((tree));
static int invalid_nontype_parm_type_p PARAMS ((tree, tsubst_flags_t));

/* Called once to initialize pt.c.  */

void
init_pt ()
{
  ggc_add_tree_root (&pending_templates, 1);
  ggc_add_tree_root (&saved_trees, 1);
  ggc_add_tree_root (&current_tinst_level, 1);
}

/* Do any processing required when DECL (a member template declaration
   using TEMPLATE_PARAMETERS as its innermost parameter list) is
   finished.  Returns the TEMPLATE_DECL corresponding to DECL, unless
   it is a specialization, in which case the DECL itself is returned.  */

tree
finish_member_template_decl (decl)
  tree decl;
{
  if (decl == NULL_TREE || decl == void_type_node)
    return NULL_TREE;
  else if (decl == error_mark_node)
    /* By returning NULL_TREE, the parser will just ignore this
       declaration.  We have already issued the error.  */
    return NULL_TREE;
  else if (TREE_CODE (decl) == TREE_LIST)
    {
      /* Assume that the class is the only declspec.  */
      decl = TREE_VALUE (decl);
      if (IS_AGGR_TYPE (decl) && CLASSTYPE_TEMPLATE_INFO (decl)
	  && ! CLASSTYPE_TEMPLATE_SPECIALIZATION (decl))
	{
	  tree tmpl = CLASSTYPE_TI_TEMPLATE (decl);
	  check_member_template (tmpl);
	  return tmpl;
	}
      return NULL_TREE;
    }
  else if (TREE_CODE (decl) == FIELD_DECL)
    error ("data member `%D' cannot be a member template", decl);
  else if (DECL_TEMPLATE_INFO (decl))
    {
      if (!DECL_TEMPLATE_SPECIALIZATION (decl))
	{
	  check_member_template (DECL_TI_TEMPLATE (decl));
	  return DECL_TI_TEMPLATE (decl);
	}
      else
	return decl;
    } 
  else
    error ("invalid member template declaration `%D'", decl);

  return error_mark_node;
}

/* Returns the template nesting level of the indicated class TYPE.
   
   For example, in:
     template <class T>
     struct A
     {
       template <class U>
       struct B {};
     };

   A<T>::B<U> has depth two, while A<T> has depth one.  
   Both A<T>::B<int> and A<int>::B<U> have depth one, if
   COUNT_SPECIALIZATIONS is 0 or if they are instantiations, not
   specializations.  

   This function is guaranteed to return 0 if passed NULL_TREE so
   that, for example, `template_class_depth (current_class_type)' is
   always safe.  */

static int 
template_class_depth_real (type, count_specializations)
     tree type;
     int count_specializations;
{
  int depth;

  for (depth = 0; 
       type && TREE_CODE (type) != NAMESPACE_DECL;
       type = (TREE_CODE (type) == FUNCTION_DECL) 
	 ? CP_DECL_CONTEXT (type) : TYPE_CONTEXT (type))
    {
      if (TREE_CODE (type) != FUNCTION_DECL)
	{
	  if (CLASSTYPE_TEMPLATE_INFO (type)
	      && PRIMARY_TEMPLATE_P (CLASSTYPE_TI_TEMPLATE (type))
	      && ((count_specializations
		   && CLASSTYPE_TEMPLATE_SPECIALIZATION (type))
		  || uses_template_parms (CLASSTYPE_TI_ARGS (type))))
	    ++depth;
	}
      else 
	{
	  if (DECL_TEMPLATE_INFO (type)
	      && PRIMARY_TEMPLATE_P (DECL_TI_TEMPLATE (type))
	      && ((count_specializations
		   && DECL_TEMPLATE_SPECIALIZATION (type))
		  || uses_template_parms (DECL_TI_ARGS (type))))
	    ++depth;
	}
    }

  return depth;
}

/* Returns the template nesting level of the indicated class TYPE.
   Like template_class_depth_real, but instantiations do not count in
   the depth.  */

int 
template_class_depth (type)
     tree type;
{
  return template_class_depth_real (type, /*count_specializations=*/0);
}

/* Returns 1 if processing DECL as part of do_pending_inlines
   needs us to push template parms.  */

static int
inline_needs_template_parms (decl)
     tree decl;
{
  if (! DECL_TEMPLATE_INFO (decl))
    return 0;

  return (TMPL_PARMS_DEPTH (DECL_TEMPLATE_PARMS (most_general_template (decl)))
	  > (processing_template_decl + DECL_TEMPLATE_SPECIALIZATION (decl)));
}

/* Subroutine of maybe_begin_member_template_processing.
   Push the template parms in PARMS, starting from LEVELS steps into the
   chain, and ending at the beginning, since template parms are listed
   innermost first.  */

static void
push_inline_template_parms_recursive (parmlist, levels)
     tree parmlist;
     int levels;
{
  tree parms = TREE_VALUE (parmlist);
  int i;

  if (levels > 1)
    push_inline_template_parms_recursive (TREE_CHAIN (parmlist), levels - 1);

  ++processing_template_decl;
  current_template_parms
    = tree_cons (size_int (processing_template_decl),
		 parms, current_template_parms);
  TEMPLATE_PARMS_FOR_INLINE (current_template_parms) = 1;

  pushlevel (0);
  for (i = 0; i < TREE_VEC_LENGTH (parms); ++i) 
    {
      tree parm = TREE_VALUE (TREE_VEC_ELT (parms, i));
      my_friendly_assert (DECL_P (parm), 0);

      switch (TREE_CODE (parm))
	{
	case TYPE_DECL:
	case TEMPLATE_DECL:
	  pushdecl (parm);
	  break;

	case PARM_DECL:
	  {
	    /* Make a CONST_DECL as is done in process_template_parm.
	       It is ugly that we recreate this here; the original
	       version built in process_template_parm is no longer
	       available.  */
	    tree decl = build_decl (CONST_DECL, DECL_NAME (parm),
				    TREE_TYPE (parm));
	    DECL_ARTIFICIAL (decl) = 1;
	    DECL_INITIAL (decl) = DECL_INITIAL (parm);
	    SET_DECL_TEMPLATE_PARM_P (decl);
	    pushdecl (decl);
	  }
	  break;

	default:
	  abort ();
	}
    }
}

/* Restore the template parameter context for a member template or
   a friend template defined in a class definition.  */

void
maybe_begin_member_template_processing (decl)
     tree decl;
{
  tree parms;
  int levels = 0;

  if (inline_needs_template_parms (decl))
    {
      parms = DECL_TEMPLATE_PARMS (most_general_template (decl));
      levels = TMPL_PARMS_DEPTH (parms) - processing_template_decl;

      if (DECL_TEMPLATE_SPECIALIZATION (decl))
	{
	  --levels;
	  parms = TREE_CHAIN (parms);
	}

      push_inline_template_parms_recursive (parms, levels);
    }

  /* Remember how many levels of template parameters we pushed so that
     we can pop them later.  */
  if (!inline_parm_levels)
    VARRAY_INT_INIT (inline_parm_levels, 4, "inline_parm_levels");
  if (inline_parm_levels_used == inline_parm_levels->num_elements)
    VARRAY_GROW (inline_parm_levels, 2 * inline_parm_levels_used);
  VARRAY_INT (inline_parm_levels, inline_parm_levels_used) = levels;
  ++inline_parm_levels_used;
}

/* Undo the effects of begin_member_template_processing. */

void 
maybe_end_member_template_processing ()
{
  int i;

  if (!inline_parm_levels_used)
    return;

  --inline_parm_levels_used;
  for (i = 0; 
       i < VARRAY_INT (inline_parm_levels, inline_parm_levels_used);
       ++i) 
    {
      --processing_template_decl;
      current_template_parms = TREE_CHAIN (current_template_parms);
      poplevel (0, 0, 0);
    }
}

/* Returns non-zero iff T is a member template function.  We must be
   careful as in

     template <class T> class C { void f(); }

   Here, f is a template function, and a member, but not a member
   template.  This function does not concern itself with the origin of
   T, only its present state.  So if we have 

     template <class T> class C { template <class U> void f(U); }

   then neither C<int>::f<char> nor C<T>::f<double> is considered
   to be a member template.  But, `template <class U> void
   C<int>::f(U)' is considered a member template.  */

int
is_member_template (t)
     tree t;
{
  if (!DECL_FUNCTION_TEMPLATE_P (t))
    /* Anything that isn't a function or a template function is
       certainly not a member template.  */
    return 0;

  /* A local class can't have member templates.  */
  if (decl_function_context (t))
    return 0;

  return (DECL_FUNCTION_MEMBER_P (DECL_TEMPLATE_RESULT (t))
	  /* If there are more levels of template parameters than
	     there are template classes surrounding the declaration,
	     then we have a member template.  */
	  && (TMPL_PARMS_DEPTH (DECL_TEMPLATE_PARMS (t)) > 
	      template_class_depth (DECL_CONTEXT (t))));
}

#if 0 /* UNUSED */
/* Returns non-zero iff T is a member template class.  See
   is_member_template for a description of what precisely constitutes
   a member template.  */

int
is_member_template_class (t)
     tree t;
{
  if (!DECL_CLASS_TEMPLATE_P (t))
    /* Anything that isn't a class template, is certainly not a member
       template.  */
    return 0;

  if (!DECL_CLASS_SCOPE_P (t))
    /* Anything whose context isn't a class type is surely not a
       member template.  */
    return 0;

  /* If there are more levels of template parameters than there are
     template classes surrounding the declaration, then we have a
     member template.  */
  return  (TMPL_PARMS_DEPTH (DECL_TEMPLATE_PARMS (t)) > 
	   template_class_depth (DECL_CONTEXT (t)));
}
#endif

/* Return a new template argument vector which contains all of ARGS,
   but has as its innermost set of arguments the EXTRA_ARGS.  */

static tree
add_to_template_args (args, extra_args)
     tree args;
     tree extra_args;
{
  tree new_args;
  int extra_depth;
  int i;
  int j;

  extra_depth = TMPL_ARGS_DEPTH (extra_args);
  new_args = make_tree_vec (TMPL_ARGS_DEPTH (args) + extra_depth);

  for (i = 1; i <= TMPL_ARGS_DEPTH (args); ++i)
    SET_TMPL_ARGS_LEVEL (new_args, i, TMPL_ARGS_LEVEL (args, i));

  for (j = 1; j <= extra_depth; ++j, ++i)
    SET_TMPL_ARGS_LEVEL (new_args, i, TMPL_ARGS_LEVEL (extra_args, j));
    
  return new_args;
}

/* Like add_to_template_args, but only the outermost ARGS are added to
   the EXTRA_ARGS.  In particular, all but TMPL_ARGS_DEPTH
   (EXTRA_ARGS) levels are added.  This function is used to combine
   the template arguments from a partial instantiation with the
   template arguments used to attain the full instantiation from the
   partial instantiation.  */

static tree
add_outermost_template_args (args, extra_args)
     tree args;
     tree extra_args;
{
  tree new_args;

  /* If there are more levels of EXTRA_ARGS than there are ARGS,
     something very fishy is going on.  */
  my_friendly_assert (TMPL_ARGS_DEPTH (args) >= TMPL_ARGS_DEPTH (extra_args),
		      0);

  /* If *all* the new arguments will be the EXTRA_ARGS, just return
     them.  */
  if (TMPL_ARGS_DEPTH (args) == TMPL_ARGS_DEPTH (extra_args))
    return extra_args;

  /* For the moment, we make ARGS look like it contains fewer levels.  */
  TREE_VEC_LENGTH (args) -= TMPL_ARGS_DEPTH (extra_args);
  
  new_args = add_to_template_args (args, extra_args);

  /* Now, we restore ARGS to its full dimensions.  */
  TREE_VEC_LENGTH (args) += TMPL_ARGS_DEPTH (extra_args);

  return new_args;
}

/* Return the N levels of innermost template arguments from the ARGS.  */

tree
get_innermost_template_args (args, n)
     tree args;
     int n;
{
  tree new_args;
  int extra_levels;
  int i;

  my_friendly_assert (n >= 0, 20000603);

  /* If N is 1, just return the innermost set of template arguments.  */
  if (n == 1)
    return TMPL_ARGS_LEVEL (args, TMPL_ARGS_DEPTH (args));
  
  /* If we're not removing anything, just return the arguments we were
     given.  */
  extra_levels = TMPL_ARGS_DEPTH (args) - n;
  my_friendly_assert (extra_levels >= 0, 20000603);
  if (extra_levels == 0)
    return args;

  /* Make a new set of arguments, not containing the outer arguments.  */
  new_args = make_tree_vec (n);
  for (i = 1; i <= n; ++i)
    SET_TMPL_ARGS_LEVEL (new_args, i, 
			 TMPL_ARGS_LEVEL (args, i + extra_levels));

  return new_args;
}

/* We've got a template header coming up; push to a new level for storing
   the parms.  */

void
begin_template_parm_list ()
{
  /* We use a non-tag-transparent scope here, which causes pushtag to
     put tags in this scope, rather than in the enclosing class or
     namespace scope.  This is the right thing, since we want
     TEMPLATE_DECLS, and not TYPE_DECLS for template classes.  For a
     global template class, push_template_decl handles putting the
     TEMPLATE_DECL into top-level scope.  For a nested template class,
     e.g.:

       template <class T> struct S1 {
         template <class T> struct S2 {}; 
       };

     pushtag contains special code to call pushdecl_with_scope on the
     TEMPLATE_DECL for S2.  */
  begin_scope (sk_template_parms);
  ++processing_template_decl;
  ++processing_template_parmlist;
  note_template_header (0);
}

/* This routine is called when a specialization is declared.  If it is
   illegal to declare a specialization here, an error is reported.  */

static void
check_specialization_scope ()
{
  tree scope = current_scope ();

  /* [temp.expl.spec] 
     
     An explicit specialization shall be declared in the namespace of
     which the template is a member, or, for member templates, in the
     namespace of which the enclosing class or enclosing class
     template is a member.  An explicit specialization of a member
     function, member class or static data member of a class template
     shall be declared in the namespace of which the class template
     is a member.  */
  if (scope && TREE_CODE (scope) != NAMESPACE_DECL)
    error ("explicit specialization in non-namespace scope `%D'",
	      scope);

  /* [temp.expl.spec] 

     In an explicit specialization declaration for a member of a class
     template or a member template that appears in namespace scope,
     the member template and some of its enclosing class templates may
     remain unspecialized, except that the declaration shall not
     explicitly specialize a class member template if its enclosing
     class templates are not explicitly specialized as well.  */
  if (current_template_parms) 
    error ("enclosing class templates are not explicitly specialized");
}

/* We've just seen template <>. */

void
begin_specialization ()
{
  begin_scope (sk_template_spec);
  note_template_header (1);
  check_specialization_scope ();
}

/* Called at then end of processing a declaration preceded by
   template<>.  */

void 
end_specialization ()
{
  finish_scope ();
  reset_specialization ();
}

/* Any template <>'s that we have seen thus far are not referring to a
   function specialization. */

void
reset_specialization ()
{
  processing_specialization = 0;
  template_header_count = 0;
}

/* We've just seen a template header.  If SPECIALIZATION is non-zero,
   it was of the form template <>.  */

static void 
note_template_header (specialization)
     int specialization;
{
  processing_specialization = specialization;
  template_header_count++;
}

/* We're beginning an explicit instantiation.  */

void
begin_explicit_instantiation ()
{
  ++processing_explicit_instantiation;
}


void
end_explicit_instantiation ()
{
  my_friendly_assert(processing_explicit_instantiation > 0, 0);
  --processing_explicit_instantiation;
}

/* The TYPE is being declared.  If it is a template type, that means it
   is a partial specialization.  Do appropriate error-checking.  */

void 
maybe_process_partial_specialization (type)
     tree type;
{
  if (IS_AGGR_TYPE (type) && CLASSTYPE_USE_TEMPLATE (type))
    {
      if (CLASSTYPE_IMPLICIT_INSTANTIATION (type)
	  && !COMPLETE_TYPE_P (type))
	{
	  if (current_namespace
	      != decl_namespace_context (CLASSTYPE_TI_TEMPLATE (type)))
	    {
	      pedwarn ("specializing `%#T' in different namespace", type);
	      cp_pedwarn_at ("  from definition of `%#D'",
			     CLASSTYPE_TI_TEMPLATE (type));
	    }
	  SET_CLASSTYPE_TEMPLATE_SPECIALIZATION (type);
	  if (processing_template_decl)
	    push_template_decl (TYPE_MAIN_DECL (type));
	}
      else if (CLASSTYPE_TEMPLATE_INSTANTIATION (type))
	error ("specialization of `%T' after instantiation", type);
    }
  else if (processing_specialization)
    error ("explicit specialization of non-template `%T'", type);
}

/* Retrieve the specialization (in the sense of [temp.spec] - a
   specialization is either an instantiation or an explicit
   specialization) of TMPL for the given template ARGS.  If there is
   no such specialization, return NULL_TREE.  The ARGS are a vector of
   arguments, or a vector of vectors of arguments, in the case of
   templates with more than one level of parameters.  */
   
static tree
retrieve_specialization (tmpl, args)
     tree tmpl;
     tree args;
{
  tree s;

  my_friendly_assert (TREE_CODE (tmpl) == TEMPLATE_DECL, 0);

  /* There should be as many levels of arguments as there are
     levels of parameters.  */
  my_friendly_assert (TMPL_ARGS_DEPTH (args) 
		      == TMPL_PARMS_DEPTH (DECL_TEMPLATE_PARMS (tmpl)),
		      0);
		      
  for (s = DECL_TEMPLATE_SPECIALIZATIONS (tmpl);
       s != NULL_TREE;
       s = TREE_CHAIN (s))
    if (comp_template_args (TREE_PURPOSE (s), args))
      return TREE_VALUE (s);

  return NULL_TREE;
}

/* Like retrieve_specialization, but for local declarations.  */

static tree
retrieve_local_specialization (tmpl)
     tree tmpl;
{
  return (tree) htab_find (local_specializations, tmpl);
}

/* Returns non-zero iff DECL is a specialization of TMPL.  */

int
is_specialization_of (decl, tmpl)
     tree decl;
     tree tmpl;
{
  tree t;

  if (TREE_CODE (decl) == FUNCTION_DECL)
    {
      for (t = decl; 
	   t != NULL_TREE;
	   t = DECL_TEMPLATE_INFO (t) ? DECL_TI_TEMPLATE (t) : NULL_TREE)
	if (t == tmpl)
	  return 1;
    }
  else 
    {
      my_friendly_assert (TREE_CODE (decl) == TYPE_DECL, 0);

      for (t = TREE_TYPE (decl);
	   t != NULL_TREE;
	   t = CLASSTYPE_USE_TEMPLATE (t)
	     ? TREE_TYPE (CLASSTYPE_TI_TEMPLATE (t)) : NULL_TREE)
	if (same_type_ignoring_top_level_qualifiers_p (t, TREE_TYPE (tmpl)))
	  return 1;
    }  

  return 0;
}

/* Register the specialization SPEC as a specialization of TMPL with
   the indicated ARGS.  Returns SPEC, or an equivalent prior
   declaration, if available.  */

static tree
register_specialization (spec, tmpl, args)
     tree spec;
     tree tmpl;
     tree args;
{
  tree s;

  my_friendly_assert (TREE_CODE (tmpl) == TEMPLATE_DECL, 0);

  if (TREE_CODE (spec) == FUNCTION_DECL 
      && uses_template_parms (DECL_TI_ARGS (spec)))
    /* This is the FUNCTION_DECL for a partial instantiation.  Don't
       register it; we want the corresponding TEMPLATE_DECL instead.
       We use `uses_template_parms (DECL_TI_ARGS (spec))' rather than
       the more obvious `uses_template_parms (spec)' to avoid problems
       with default function arguments.  In particular, given
       something like this:

          template <class T> void f(T t1, T t = T())

       the default argument expression is not substituted for in an
       instantiation unless and until it is actually needed.  */
    return spec;
    
  /* There should be as many levels of arguments as there are
     levels of parameters.  */
  my_friendly_assert (TMPL_ARGS_DEPTH (args) 
		      == TMPL_PARMS_DEPTH (DECL_TEMPLATE_PARMS (tmpl)),
		      0);

  for (s = DECL_TEMPLATE_SPECIALIZATIONS (tmpl);
       s != NULL_TREE;
       s = TREE_CHAIN (s))
    {
      tree fn = TREE_VALUE (s);

      /* We can sometimes try to re-register a specialization that we've
	 already got.  In particular, regenerate_decl_from_template
	 calls duplicate_decls which will update the specialization
	 list.  But, we'll still get called again here anyhow.  It's
	 more convenient to simply allow this than to try to prevent it.  */
      if (fn == spec)
	return spec;
      else if (comp_template_args (TREE_PURPOSE (s), args))
	{
	  if (DECL_TEMPLATE_SPECIALIZATION (spec))
	    {
	      if (DECL_TEMPLATE_INSTANTIATION (fn))
		{
		  if (TREE_USED (fn) 
		      || DECL_EXPLICIT_INSTANTIATION (fn))
		    {
		      error ("specialization of %D after instantiation",
				fn);
		      return spec;
		    }
		  else
		    {
		      /* This situation should occur only if the first
			 specialization is an implicit instantiation,
			 the second is an explicit specialization, and
			 the implicit instantiation has not yet been
			 used.  That situation can occur if we have
			 implicitly instantiated a member function and
			 then specialized it later.

			 We can also wind up here if a friend
			 declaration that looked like an instantiation
			 turns out to be a specialization:

			   template <class T> void foo(T);
			   class S { friend void foo<>(int) };
			   template <> void foo(int);  

			 We transform the existing DECL in place so that
			 any pointers to it become pointers to the
			 updated declaration.  

			 If there was a definition for the template, but
			 not for the specialization, we want this to
			 look as if there is no definition, and vice
			 versa.  */
		      DECL_INITIAL (fn) = NULL_TREE;
		      duplicate_decls (spec, fn);

		      return fn;
		    }
		}
	      else if (DECL_TEMPLATE_SPECIALIZATION (fn))
		{
		  duplicate_decls (spec, fn);
		  return fn;
		}
	    }
	}
      }

  DECL_TEMPLATE_SPECIALIZATIONS (tmpl)
     = tree_cons (args, spec, DECL_TEMPLATE_SPECIALIZATIONS (tmpl));

  return spec;
}

/* Unregister the specialization SPEC as a specialization of TMPL.
   Returns nonzero if the SPEC was listed as a specialization of
   TMPL.  */

static int
unregister_specialization (spec, tmpl)
     tree spec;
     tree tmpl;
{
  tree* s;

  for (s = &DECL_TEMPLATE_SPECIALIZATIONS (tmpl);
       *s != NULL_TREE;
       s = &TREE_CHAIN (*s))
    if (TREE_VALUE (*s) == spec)
      {
	*s = TREE_CHAIN (*s);
	return 1;
      }

  return 0;
}

/* Like register_specialization, but for local declarations.  We are
   registering SPEC, an instantiation of TMPL.  */

static void
register_local_specialization (spec, tmpl)
     tree spec;
     tree tmpl;
{
  void **slot;

  slot = htab_find_slot (local_specializations, tmpl, INSERT);
  *slot = spec;
}

/* Print the list of candidate FNS in an error message.  */

void
print_candidates (fns)
     tree fns;
{
  tree fn;

  const char *str = "candidates are:";

  for (fn = fns; fn != NULL_TREE; fn = TREE_CHAIN (fn))
    {
      tree f;

      for (f = TREE_VALUE (fn); f; f = OVL_NEXT (f))
	cp_error_at ("%s %+#D", str, OVL_CURRENT (f));
      str = "               ";
    }
}

/* Returns the template (one of the functions given by TEMPLATE_ID)
   which can be specialized to match the indicated DECL with the
   explicit template args given in TEMPLATE_ID.  The DECL may be
   NULL_TREE if none is available.  In that case, the functions in
   TEMPLATE_ID are non-members.

   If NEED_MEMBER_TEMPLATE is non-zero the function is known to be a
   specialization of a member template.

   The template args (those explicitly specified and those deduced)
   are output in a newly created vector *TARGS_OUT.

   If it is impossible to determine the result, an error message is
   issued.  The error_mark_node is returned to indicate failure.  */

static tree
determine_specialization (template_id, decl, targs_out, 
			  need_member_template)
     tree template_id;
     tree decl;
     tree* targs_out;
     int need_member_template;
{
  tree fns;
  tree targs;
  tree explicit_targs;
  tree candidates = NULL_TREE;
  tree templates = NULL_TREE;

  *targs_out = NULL_TREE;

  if (template_id == error_mark_node)
    return error_mark_node;

  fns = TREE_OPERAND (template_id, 0);
  explicit_targs = TREE_OPERAND (template_id, 1);

  if (fns == error_mark_node)
    return error_mark_node;

  /* Check for baselinks. */
  if (BASELINK_P (fns))
    fns = TREE_VALUE (fns);

  if (!is_overloaded_fn (fns))
    {
      error ("`%D' is not a function template", fns);
      return error_mark_node;
    }

  for (; fns; fns = OVL_NEXT (fns))
    {
      tree tmpl;

      tree fn = OVL_CURRENT (fns);

      if (TREE_CODE (fn) == TEMPLATE_DECL)
	/* DECL might be a specialization of FN.  */
	tmpl = fn;
      else if (need_member_template)
	/* FN is an ordinary member function, and we need a
	   specialization of a member template.  */
	continue;
      else if (TREE_CODE (fn) != FUNCTION_DECL)
	/* We can get IDENTIFIER_NODEs here in certain erroneous
	   cases.  */
	continue;
      else if (!DECL_FUNCTION_MEMBER_P (fn))
	/* This is just an ordinary non-member function.  Nothing can
	   be a specialization of that.  */
	continue;
      else if (DECL_ARTIFICIAL (fn))
	/* Cannot specialize functions that are created implicitly.  */
	continue;
      else
	{
	  tree decl_arg_types;

	  /* This is an ordinary member function.  However, since
	     we're here, we can assume it's enclosing class is a
	     template class.  For example,
	     
	       template <typename T> struct S { void f(); };
	       template <> void S<int>::f() {}

	     Here, S<int>::f is a non-template, but S<int> is a
	     template class.  If FN has the same type as DECL, we
	     might be in business.  */

	  if (!DECL_TEMPLATE_INFO (fn))
	    /* Its enclosing class is an explicit specialization
	       of a template class.  This is not a candidate.  */
	    continue;

	  if (!same_type_p (TREE_TYPE (TREE_TYPE (decl)),
			    TREE_TYPE (TREE_TYPE (fn))))
	    /* The return types differ.  */
	    continue;

	  /* Adjust the type of DECL in case FN is a static member.  */
	  decl_arg_types = TYPE_ARG_TYPES (TREE_TYPE (decl));
	  if (DECL_STATIC_FUNCTION_P (fn) 
	      && DECL_NONSTATIC_MEMBER_FUNCTION_P (decl))
	    decl_arg_types = TREE_CHAIN (decl_arg_types);

	  if (compparms (TYPE_ARG_TYPES (TREE_TYPE (fn)), 
			 decl_arg_types))
	    /* They match!  */
	    candidates = tree_cons (NULL_TREE, fn, candidates);

	  continue;
	}

      /* See whether this function might be a specialization of this
	 template.  */
      targs = get_bindings (tmpl, decl, explicit_targs);

      if (!targs)
	/* We cannot deduce template arguments that when used to
	   specialize TMPL will produce DECL.  */
	continue;

      /* Save this template, and the arguments deduced.  */
      templates = tree_cons (targs, tmpl, templates);
    }

  if (templates && TREE_CHAIN (templates))
    {
      /* We have:
	 
	   [temp.expl.spec]

	   It is possible for a specialization with a given function
	   signature to be instantiated from more than one function
	   template.  In such cases, explicit specification of the
	   template arguments must be used to uniquely identify the
	   function template specialization being specialized.

	 Note that here, there's no suggestion that we're supposed to
	 determine which of the candidate templates is most
	 specialized.  However, we, also have:

	   [temp.func.order]

	   Partial ordering of overloaded function template
	   declarations is used in the following contexts to select
	   the function template to which a function template
	   specialization refers: 

           -- when an explicit specialization refers to a function
	      template. 

	 So, we do use the partial ordering rules, at least for now.
	 This extension can only serve to make illegal programs legal,
	 so it's safe.  And, there is strong anecdotal evidence that
	 the committee intended the partial ordering rules to apply;
	 the EDG front-end has that behavior, and John Spicer claims
	 that the committee simply forgot to delete the wording in
	 [temp.expl.spec].  */
     tree tmpl = most_specialized (templates, decl, explicit_targs);
     if (tmpl && tmpl != error_mark_node)
       {
	 targs = get_bindings (tmpl, decl, explicit_targs);
	 templates = tree_cons (targs, tmpl, NULL_TREE);
       }
    }

  if (templates == NULL_TREE && candidates == NULL_TREE)
    {
      cp_error_at ("template-id `%D' for `%+D' does not match any template declaration",
		   template_id, decl);
      return error_mark_node;
    }
  else if ((templates && TREE_CHAIN (templates))
	   || (candidates && TREE_CHAIN (candidates))
	   || (templates && candidates))
    {
      cp_error_at ("ambiguous template specialization `%D' for `%+D'",
		   template_id, decl);
      chainon (candidates, templates);
      print_candidates (candidates);
      return error_mark_node;
    }

  /* We have one, and exactly one, match. */
  if (candidates)
    {
      /* It was a specialization of an ordinary member function in a
	 template class.  */
      *targs_out = copy_node (DECL_TI_ARGS (TREE_VALUE (candidates)));
      return DECL_TI_TEMPLATE (TREE_VALUE (candidates));
    }

  /* It was a specialization of a template.  */
  targs = DECL_TI_ARGS (DECL_TEMPLATE_RESULT (TREE_VALUE (templates)));
  if (TMPL_ARGS_HAVE_MULTIPLE_LEVELS (targs))
    {
      *targs_out = copy_node (targs);
      SET_TMPL_ARGS_LEVEL (*targs_out, 
			   TMPL_ARGS_DEPTH (*targs_out),
			   TREE_PURPOSE (templates));
    }
  else
    *targs_out = TREE_PURPOSE (templates);
  return TREE_VALUE (templates);
}

/* Returns a chain of parameter types, exactly like the SPEC_TYPES,
   but with the default argument values filled in from those in the
   TMPL_TYPES.  */
      
static tree
copy_default_args_to_explicit_spec_1 (spec_types,
				      tmpl_types)
     tree spec_types;
     tree tmpl_types;
{
  tree new_spec_types;

  if (!spec_types)
    return NULL_TREE;

  if (spec_types == void_list_node)
    return void_list_node;

  /* Substitute into the rest of the list.  */
  new_spec_types =
    copy_default_args_to_explicit_spec_1 (TREE_CHAIN (spec_types),
					  TREE_CHAIN (tmpl_types));
  
  /* Add the default argument for this parameter.  */
  return hash_tree_cons (TREE_PURPOSE (tmpl_types),
			 TREE_VALUE (spec_types),
			 new_spec_types);
}

/* DECL is an explicit specialization.  Replicate default arguments
   from the template it specializes.  (That way, code like:

     template <class T> void f(T = 3);
     template <> void f(double);
     void g () { f (); } 

   works, as required.)  An alternative approach would be to look up
   the correct default arguments at the call-site, but this approach
   is consistent with how implicit instantiations are handled.  */

static void
copy_default_args_to_explicit_spec (decl)
     tree decl;
{
  tree tmpl;
  tree spec_types;
  tree tmpl_types;
  tree new_spec_types;
  tree old_type;
  tree new_type;
  tree t;
  tree object_type = NULL_TREE;
  tree in_charge = NULL_TREE;
  tree vtt = NULL_TREE;

  /* See if there's anything we need to do.  */
  tmpl = DECL_TI_TEMPLATE (decl);
  tmpl_types = TYPE_ARG_TYPES (TREE_TYPE (DECL_TEMPLATE_RESULT (tmpl)));
  for (t = tmpl_types; t; t = TREE_CHAIN (t))
    if (TREE_PURPOSE (t))
      break;
  if (!t)
    return;

  old_type = TREE_TYPE (decl);
  spec_types = TYPE_ARG_TYPES (old_type);
  
  if (DECL_NONSTATIC_MEMBER_FUNCTION_P (decl))
    {
      /* Remove the this pointer, but remember the object's type for
         CV quals.  */
      object_type = TREE_TYPE (TREE_VALUE (spec_types));
      spec_types = TREE_CHAIN (spec_types);
      tmpl_types = TREE_CHAIN (tmpl_types);
      
      if (DECL_HAS_IN_CHARGE_PARM_P (decl))
        {
          /* DECL may contain more parameters than TMPL due to the extra
             in-charge parameter in constructors and destructors.  */
          in_charge = spec_types;
	  spec_types = TREE_CHAIN (spec_types);
	}
      if (DECL_HAS_VTT_PARM_P (decl))
	{
	  vtt = spec_types;
	  spec_types = TREE_CHAIN (spec_types);
	}
    }

  /* Compute the merged default arguments.  */
  new_spec_types = 
    copy_default_args_to_explicit_spec_1 (spec_types, tmpl_types);

  /* Compute the new FUNCTION_TYPE.  */
  if (object_type)
    {
      if (vtt)
        new_spec_types = hash_tree_cons (TREE_PURPOSE (vtt),
			  	         TREE_VALUE (vtt),
				         new_spec_types);

      if (in_charge)
        /* Put the in-charge parameter back.  */
        new_spec_types = hash_tree_cons (TREE_PURPOSE (in_charge),
			  	         TREE_VALUE (in_charge),
				         new_spec_types);

      new_type = build_cplus_method_type (object_type,
					  TREE_TYPE (old_type),
					  new_spec_types);
    }
  else
    new_type = build_function_type (TREE_TYPE (old_type),
				    new_spec_types);
  new_type = build_type_attribute_variant (new_type,
					   TYPE_ATTRIBUTES (old_type));
  new_type = build_exception_variant (new_type,
				      TYPE_RAISES_EXCEPTIONS (old_type));
  TREE_TYPE (decl) = new_type;
}

/* Check to see if the function just declared, as indicated in
   DECLARATOR, and in DECL, is a specialization of a function
   template.  We may also discover that the declaration is an explicit
   instantiation at this point.

   Returns DECL, or an equivalent declaration that should be used
   instead if all goes well.  Issues an error message if something is
   amiss.  Returns error_mark_node if the error is not easily
   recoverable.
   
   FLAGS is a bitmask consisting of the following flags: 

   2: The function has a definition.
   4: The function is a friend.

   The TEMPLATE_COUNT is the number of references to qualifying
   template classes that appeared in the name of the function.  For
   example, in

     template <class T> struct S { void f(); };
     void S<int>::f();
     
   the TEMPLATE_COUNT would be 1.  However, explicitly specialized
   classes are not counted in the TEMPLATE_COUNT, so that in

     template <class T> struct S {};
     template <> struct S<int> { void f(); }
     template <> void S<int>::f();

   the TEMPLATE_COUNT would be 0.  (Note that this declaration is
   illegal; there should be no template <>.)

   If the function is a specialization, it is marked as such via
   DECL_TEMPLATE_SPECIALIZATION.  Furthermore, its DECL_TEMPLATE_INFO
   is set up correctly, and it is added to the list of specializations 
   for that template.  */

tree
check_explicit_specialization (declarator, decl, template_count, flags)
     tree declarator;
     tree decl;
     int template_count;
     int flags;
{
  int have_def = flags & 2;
  int is_friend = flags & 4;
  int specialization = 0;
  int explicit_instantiation = 0;
  int member_specialization = 0;
  tree ctype = DECL_CLASS_CONTEXT (decl);
  tree dname = DECL_NAME (decl);
  tmpl_spec_kind tsk;

  tsk = current_tmpl_spec_kind (template_count);

  switch (tsk)
    {
    case tsk_none:
      if (processing_specialization) 
	{
	  specialization = 1;
	  SET_DECL_TEMPLATE_SPECIALIZATION (decl);
	}
      else if (TREE_CODE (declarator) == TEMPLATE_ID_EXPR)
	{
	  if (is_friend)
	    /* This could be something like:

	       template <class T> void f(T);
	       class S { friend void f<>(int); }  */
	    specialization = 1;
	  else
	    {
	      /* This case handles bogus declarations like template <>
		 template <class T> void f<int>(); */

	      error ("template-id `%D' in declaration of primary template",
			declarator);
	      return decl;
	    }
	}
      break;

    case tsk_invalid_member_spec:
      /* The error has already been reported in
	 check_specialization_scope.  */
      return error_mark_node;

    case tsk_invalid_expl_inst:
      error ("template parameter list used in explicit instantiation");

      /* Fall through.  */

    case tsk_expl_inst:
      if (have_def)
	error ("definition provided for explicit instantiation");
      
      explicit_instantiation = 1;
      break;

    case tsk_excessive_parms:
      error ("too many template parameter lists in declaration of `%D'", 
		decl);
      return error_mark_node;

      /* Fall through.  */
    case tsk_expl_spec:
      SET_DECL_TEMPLATE_SPECIALIZATION (decl);
      if (ctype)
	member_specialization = 1;
      else
	specialization = 1;
      break;
     
    case tsk_insufficient_parms:
      if (template_header_count)
	{
	  error("too few template parameter lists in declaration of `%D'", 
		   decl);
	  return decl;
	}
      else if (ctype != NULL_TREE
	       && !TYPE_BEING_DEFINED (ctype)
	       && CLASSTYPE_TEMPLATE_INSTANTIATION (ctype)
	       && !is_friend)
	{
	  /* For backwards compatibility, we accept:

	       template <class T> struct S { void f(); };
	       void S<int>::f() {} // Missing template <>

	     That used to be legal C++.  */
	  if (pedantic)
	    pedwarn
	      ("explicit specialization not preceded by `template <>'");
	  specialization = 1;
	  SET_DECL_TEMPLATE_SPECIALIZATION (decl);
	}
      break;

    case tsk_template:
      if (TREE_CODE (declarator) == TEMPLATE_ID_EXPR)
	{
	  /* This case handles bogus declarations like template <>
	     template <class T> void f<int>(); */

	  if (uses_template_parms (declarator))
	    error ("partial specialization `%D' of function template",
		      declarator);
	  else
	    error ("template-id `%D' in declaration of primary template",
		      declarator);
	  return decl;
	}

      if (ctype && CLASSTYPE_TEMPLATE_INSTANTIATION (ctype))
	/* This is a specialization of a member template, without
	   specialization the containing class.  Something like:

	     template <class T> struct S {
	       template <class U> void f (U); 
             };
	     template <> template <class U> void S<int>::f(U) {}
	     
	   That's a specialization -- but of the entire template.  */
	specialization = 1;
      break;

    default:
      abort ();
    }

  if (specialization || member_specialization)
    {
      tree t = TYPE_ARG_TYPES (TREE_TYPE (decl));
      for (; t; t = TREE_CHAIN (t))
	if (TREE_PURPOSE (t))
	  {
	    pedwarn
	      ("default argument specified in explicit specialization");
	    break;
	  }
      if (current_lang_name == lang_name_c)
	error ("template specialization with C linkage");
    }

  if (specialization || member_specialization || explicit_instantiation)
    {
      tree tmpl = NULL_TREE;
      tree targs = NULL_TREE;

      /* Make sure that the declarator is a TEMPLATE_ID_EXPR.  */
      if (TREE_CODE (declarator) != TEMPLATE_ID_EXPR)
	{
	  tree fns;

	  my_friendly_assert (TREE_CODE (declarator) == IDENTIFIER_NODE, 
			      0);
	  if (!ctype)
	    fns = IDENTIFIER_NAMESPACE_VALUE (dname);
	  else
	    fns = dname;

	  declarator = 
	    lookup_template_function (fns, NULL_TREE);
	}

      if (declarator == error_mark_node)
	return error_mark_node;

      if (ctype != NULL_TREE && TYPE_BEING_DEFINED (ctype))
	{
	  if (!explicit_instantiation)
	    /* A specialization in class scope.  This is illegal,
	       but the error will already have been flagged by
	       check_specialization_scope.  */
	    return error_mark_node;
	  else
	    {
	      /* It's not legal to write an explicit instantiation in
		 class scope, e.g.:

	           class C { template void f(); }

		   This case is caught by the parser.  However, on
		   something like:
	       
		   template class C { void f(); };

		   (which is illegal) we can get here.  The error will be
		   issued later.  */
	      ;
	    }

	  return decl;
	}
      else if (TREE_CODE (TREE_OPERAND (declarator, 0)) == LOOKUP_EXPR)
	{
	  /* A friend declaration.  We can't do much, because we don't
	     know what this resolves to, yet.  */
	  my_friendly_assert (is_friend != 0, 0);
	  my_friendly_assert (!explicit_instantiation, 0);
	  SET_DECL_IMPLICIT_INSTANTIATION (decl);
	  return decl;
	} 
      else if (ctype != NULL_TREE 
	       && (TREE_CODE (TREE_OPERAND (declarator, 0)) ==
		   IDENTIFIER_NODE))
	{
	  /* Find the list of functions in ctype that have the same
	     name as the declared function.  */
	  tree name = TREE_OPERAND (declarator, 0);
	  tree fns = NULL_TREE;
	  int idx;

	  if (name == constructor_name (ctype) 
	      || name == constructor_name_full (ctype))
	    {
	      int is_constructor = DECL_CONSTRUCTOR_P (decl);
	      
	      if (is_constructor ? !TYPE_HAS_CONSTRUCTOR (ctype)
		  : !TYPE_HAS_DESTRUCTOR (ctype))
		{
		  /* From [temp.expl.spec]:
		       
		     If such an explicit specialization for the member
		     of a class template names an implicitly-declared
		     special member function (clause _special_), the
		     program is ill-formed.  

		     Similar language is found in [temp.explicit].  */
		  error ("specialization of implicitly-declared special member function");
		  return error_mark_node;
		}

	      name = is_constructor ? ctor_identifier : dtor_identifier;
	    }

	  if (!DECL_CONV_FN_P (decl))
	    {
	      idx = lookup_fnfields_1 (ctype, name);
	      if (idx >= 0)
		fns = TREE_VEC_ELT (CLASSTYPE_METHOD_VEC (ctype), idx);
	    }
	  else
	    {
	      tree methods;

	      /* For a type-conversion operator, we cannot do a
		 name-based lookup.  We might be looking for `operator
		 int' which will be a specialization of `operator T'.
		 So, we find *all* the conversion operators, and then
		 select from them.  */
	      fns = NULL_TREE;

	      methods = CLASSTYPE_METHOD_VEC (ctype);
	      if (methods)
		for (idx = 2; idx < TREE_VEC_LENGTH (methods); ++idx) 
		  {
		    tree ovl = TREE_VEC_ELT (methods, idx);

		    if (!ovl || !DECL_CONV_FN_P (OVL_CURRENT (ovl)))
		      /* There are no more conversion functions.  */
		      break;

		    /* Glue all these conversion functions together
		       with those we already have.  */
		    for (; ovl; ovl = OVL_NEXT (ovl))
		      fns = ovl_cons (OVL_CURRENT (ovl), fns);
		  }
	    }
	      
	  if (fns == NULL_TREE) 
	    {
	      error ("no member function `%D' declared in `%T'",
			name, ctype);
	      return error_mark_node;
	    }
	  else
	    TREE_OPERAND (declarator, 0) = fns;
	}
      
      /* Figure out what exactly is being specialized at this point.
	 Note that for an explicit instantiation, even one for a
	 member function, we cannot tell apriori whether the
	 instantiation is for a member template, or just a member
	 function of a template class.  Even if a member template is
	 being instantiated, the member template arguments may be
	 elided if they can be deduced from the rest of the
	 declaration.  */
      tmpl = determine_specialization (declarator, decl,
				       &targs, 
				       member_specialization);
	    
      if (!tmpl || tmpl == error_mark_node)
	/* We couldn't figure out what this declaration was
	   specializing.  */
	return error_mark_node;
      else
	{
	  tree gen_tmpl = most_general_template (tmpl);

	  if (explicit_instantiation)
	    {
	      /* We don't set DECL_EXPLICIT_INSTANTIATION here; that
		 is done by do_decl_instantiation later.  */ 

	      int arg_depth = TMPL_ARGS_DEPTH (targs);
	      int parm_depth = TMPL_PARMS_DEPTH (DECL_TEMPLATE_PARMS (tmpl));

	      if (arg_depth > parm_depth)
		{
		  /* If TMPL is not the most general template (for
		     example, if TMPL is a friend template that is
		     injected into namespace scope), then there will
		     be too many levels of TARGS.  Remove some of them
		     here.  */
		  int i;
		  tree new_targs;

		  new_targs = make_tree_vec (parm_depth);
		  for (i = arg_depth - parm_depth; i < arg_depth; ++i)
		    TREE_VEC_ELT (new_targs, i - (arg_depth - parm_depth))
		      = TREE_VEC_ELT (targs, i);
		  targs = new_targs;
		}
		  
	      return instantiate_template (tmpl, targs);
	    }

	  /* If this is a specialization of a member template of a
	     template class.  In we want to return the TEMPLATE_DECL,
	     not the specialization of it.  */
	  if (tsk == tsk_template)
	    {
	      SET_DECL_TEMPLATE_SPECIALIZATION (tmpl);
	      DECL_INITIAL (DECL_TEMPLATE_RESULT (tmpl)) = NULL_TREE;
	      return tmpl;
	    }

	  /* If we thought that the DECL was a member function, but it
	     turns out to be specializing a static member function,
	     make DECL a static member function as well.  */
	  if (DECL_STATIC_FUNCTION_P (tmpl)
	      && DECL_NONSTATIC_MEMBER_FUNCTION_P (decl))
	    {
	      revert_static_member_fn (decl);
	      last_function_parms = TREE_CHAIN (last_function_parms);
	    }

	  /* Set up the DECL_TEMPLATE_INFO for DECL.  */
	  DECL_TEMPLATE_INFO (decl) = tree_cons (tmpl, targs, NULL_TREE);

	  /* Inherit default function arguments from the template
	     DECL is specializing.  */
	  copy_default_args_to_explicit_spec (decl);

	  /* This specialization has the same protection as the
	     template it specializes.  */
	  TREE_PRIVATE (decl) = TREE_PRIVATE (gen_tmpl);
	  TREE_PROTECTED (decl) = TREE_PROTECTED (gen_tmpl);

	  if (is_friend && !have_def)
	    /* This is not really a declaration of a specialization.
	       It