/* Breadth-first and depth-first routines for
   searching multiple-inheritance lattice for GNU C++.
   Copyright (C) 1987, 1989, 1992, 1993, 1994, 1995, 1996, 1997, 1998,
   1999, 2000, 2002 Free Software Foundation, Inc.
   Contributed by Michael Tiemann (tiemann@cygnus.com)

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
#include "../include/obstack.h"
#include "../gcc/flags.h"
#include "../gcc/rtl.h"
#include "../gcc/output.h"
#include "../gcc/toplev.h"

#define obstack_chunk_alloc xmalloc
#define obstack_chunk_free free

#include "../gcc/stack.h"
/* end of !kawai! */

/* Obstack used for remembering decision points of breadth-first.  */

static struct obstack search_obstack;

/* Methods for pushing and popping objects to and from obstacks.  */

struct stack_level *
push_stack_level (obstack, tp, size)
     struct obstack *obstack;
     char *tp;  /* Sony NewsOS 5.0 compiler doesn't like void * here.  */
     int size;
{
  struct stack_level *stack;
  obstack_grow (obstack, tp, size);
  stack = (struct stack_level *) ((char*)obstack_next_free (obstack) - size);
  obstack_finish (obstack);
  stack->obstack = obstack;
  stack->first = (tree *) obstack_base (obstack);
  stack->limit = obstack_room (obstack) / sizeof (tree *);
  return stack;
}

struct stack_level *
pop_stack_level (stack)
     struct stack_level *stack;
{
  struct stack_level *tem = stack;
  struct obstack *obstack = tem->obstack;
  stack = tem->prev;
  obstack_free (obstack, tem);
  return stack;
}

#define search_level stack_level
static struct search_level *search_stack;

struct vbase_info 
{
  /* The class dominating the hierarchy.  */
  tree type;
  /* A pointer to a complete object of the indicated TYPE.  */
  tree decl_ptr;
  tree inits;
};

static tree lookup_field_1 PARAMS ((tree, tree));
static int is_subobject_of_p PARAMS ((tree, tree, tree));
static tree dfs_check_overlap PARAMS ((tree, void *));
static tree dfs_no_overlap_yet PARAMS ((tree, void *));
static base_kind lookup_base_r
	PARAMS ((tree, tree, base_access, int, int, int, tree *));
static int dynamic_cast_base_recurse PARAMS ((tree, tree, int, tree *));
static tree marked_pushdecls_p PARAMS ((tree, void *));
static tree unmarked_pushdecls_p PARAMS ((tree, void *));
static tree dfs_debug_unmarkedp PARAMS ((tree, void *));
static tree dfs_debug_mark PARAMS ((tree, void *));
static tree dfs_get_vbase_types PARAMS ((tree, void *));
static tree dfs_push_type_decls PARAMS ((tree, void *));
static tree dfs_push_decls PARAMS ((tree, void *));
static tree dfs_unuse_fields PARAMS ((tree, void *));
static tree add_conversions PARAMS ((tree, void *));
static int covariant_return_p PARAMS ((tree, tree));
static int check_final_overrider PARAMS ((tree, tree));
static int look_for_overrides_r PARAMS ((tree, tree));
static struct search_level *push_search_level
	PARAMS ((struct stack_level *, struct obstack *));
static struct search_level *pop_search_level
	PARAMS ((struct stack_level *));
static tree bfs_walk
	PARAMS ((tree, tree (*) (tree, void *), tree (*) (tree, void *),
	       void *));
static tree lookup_field_queue_p PARAMS ((tree, void *));
static int shared_member_p PARAMS ((tree));
static tree lookup_field_r PARAMS ((tree, void *));
static tree canonical_binfo PARAMS ((tree));
static tree shared_marked_p PARAMS ((tree, void *));
static tree shared_unmarked_p PARAMS ((tree, void *));
static int  dependent_base_p PARAMS ((tree));
static tree dfs_accessible_queue_p PARAMS ((tree, void *));
static tree dfs_accessible_p PARAMS ((tree, void *));
static tree dfs_access_in_type PARAMS ((tree, void *));
static access_kind access_in_type PARAMS ((tree, tree));
static tree dfs_canonical_queue PARAMS ((tree, void *));
static tree dfs_assert_unmarked_p PARAMS ((tree, void *));
static void assert_canonical_unmarked PARAMS ((tree));
static int protected_accessible_p PARAMS ((tree, tree, tree));
static int friend_accessible_p PARAMS ((tree, tree, tree));
static void setup_class_bindings PARAMS ((tree, int));
static int template_self_reference_p PARAMS ((tree, tree));
static tree dfs_find_vbase_instance PARAMS ((tree, void *));
static tree dfs_get_pure_virtuals PARAMS ((tree, void *));
static tree dfs_build_inheritance_graph_order PARAMS ((tree, void *));

/* Allocate a level of searching.  */

static struct search_level *
push_search_level (stack, obstack)
     struct stack_level *stack;
     struct obstack *obstack;
{
  struct search_level tem;

  tem.prev = stack;
  return push_stack_level (obstack, (char *)&tem, sizeof (tem));
}

/* Discard a level of search allocation.  */

static struct search_level *
pop_search_level (obstack)
     struct stack_level *obstack;
{
  register struct search_level *stack = pop_stack_level (obstack);

  return stack;
}

/* Variables for gathering statistics.  */
#ifdef GATHER_STATISTICS
static int n_fields_searched;
static int n_calls_lookup_field, n_calls_lookup_field_1;
static int n_calls_lookup_fnfields, n_calls_lookup_fnfields_1;
static int n_calls_get_base_type;
static int n_outer_fields_searched;
static int n_contexts_saved;
#endif /* GATHER_STATISTICS */


/* Worker for lookup_base.  BINFO is the binfo we are searching at,
   BASE is the RECORD_TYPE we are searching for.  ACCESS is the
   required access checks.  WITHIN_CURRENT_SCOPE, IS_NON_PUBLIC and
   IS_VIRTUAL indicate how BINFO was reached from the start of the
   search.  WITHIN_CURRENT_SCOPE is true if we met the current scope,
   or friend thereof (this allows us to determine whether a protected
   base is accessible or not).  IS_NON_PUBLIC indicates whether BINFO
   is accessible and IS_VIRTUAL indicates if it is morally virtual.

   If BINFO is of the required type, then *BINFO_PTR is examined to
   compare with any other instance of BASE we might have already
   discovered. *BINFO_PTR is initialized and a base_kind return value
   indicates what kind of base was located.

   Otherwise BINFO's bases are searched.  */

static base_kind
lookup_base_r (binfo, base, access, within_current_scope,
	       is_non_public, is_virtual, binfo_ptr)
     tree binfo, base;
     base_access access;
     int within_current_scope;
     int is_non_public;		/* inside a non-public part */
     int is_virtual;		/* inside a virtual part */
     tree *binfo_ptr;
{
  int i;
  tree bases;
  base_kind found = bk_not_base;
  
  if (access == ba_check
      && !within_current_scope
      && is_friend (BINFO_TYPE (binfo), current_scope ()))
    {
      /* Do not clear is_non_public here.  If A is a private base of B, A
	 is not allowed to convert a B* to an A*.  */
      within_current_scope = 1;
    }
  
  if (same_type_p (BINFO_TYPE (binfo), base))
    {
      /* We have found a base. Check against what we have found
         already. */
      found = bk_same_type;
      if (is_virtual)
	found = bk_via_virtual;
      if (is_non_public)
	found = bk_inaccessible;
      
      if (!*binfo_ptr)
	*binfo_ptr = binfo;
      else if (!is_virtual || !tree_int_cst_equal (BINFO_OFFSET (binfo),
						   BINFO_OFFSET (*binfo_ptr)))
	{
	  if (access != ba_any)
	    *binfo_ptr = NULL;
	  else if (!is_virtual)
	    /* Prefer a non-virtual base.  */
	    *binfo_ptr = binfo;
	  found = bk_ambig;
	}
      
      return found;
    }
  
  bases = BINFO_BASETYPES (binfo);
  if (!bases)
    return bk_not_base;
  
  for (i = TREE_VEC_LENGTH (bases); i--;)
    {
      tree base_binfo = TREE_VEC_ELT (bases, i);
      int this_non_public = is_non_public;
      int this_virtual = is_virtual;
      base_kind bk;

      if (access <= ba_ignore)
	; /* no change */
      else if (TREE_VIA_PUBLIC (base_binfo))
	; /* no change */
      else if (access == ba_not_special)
	this_non_public = 1;
      else if (TREE_VIA_PROTECTED (base_binfo) && within_current_scope)
	; /* no change */
      else if (is_friend (BINFO_TYPE (binfo), current_scope ()))
	; /* no change */
      else
	this_non_public = 1;
      
      if (TREE_VIA_VIRTUAL (base_binfo))
	this_virtual = 1;
      
      bk = lookup_base_r (base_binfo, base,
		    	  access, within_current_scope,
			  this_non_public, this_virtual,
			  binfo_ptr);

      switch (bk)
	{
	case bk_ambig:
	  if (access != ba_any)
	    return bk;
	  found = bk;
	  break;
	  
	case bk_inaccessible:
	  if (found == bk_not_base)
	    found = bk;
	  my_friendly_assert (found == bk_via_virtual
			      || found == bk_inaccessible, 20010723);
	  
	  break;
	  
	case bk_same_type:
	  bk = bk_proper_base;
	  /* FALLTHROUGH */
	case bk_proper_base:
	  my_friendly_assert (found == bk_not_base, 20010723);
	  found = bk;
	  break;
	  
	case bk_via_virtual:
	  if (found != bk_ambig)
	    found = bk;
	  break;
	  
	case bk_not_base:
	  break;
	}
    }
  return found;
}

/* Lookup BASE in the hierarchy dominated by T.  Do access checking as
   ACCESS specifies.  Return the binfo we discover (which might not be
   canonical).  If KIND_PTR is non-NULL, fill with information about
   what kind of base we discovered.

   If ba_quiet bit is set in ACCESS, then do not issue an error, and
   return NULL_TREE for failure.  */

tree
lookup_base (t, base, access, kind_ptr)
     tree t, base;
     base_access access;
     base_kind *kind_ptr;
{
  tree binfo = NULL;		/* The binfo we've found so far. */
  base_kind bk;
  
  if (t == error_mark_node || base == error_mark_node)
    {
      if (kind_ptr)
	*kind_ptr = bk_not_base;
      return error_mark_node;
    }
  my_friendly_assert (TYPE_P (t) && TYPE_P (base), 20011127);
  
  /* Ensure that the types are instantiated.  */
  t = complete_type (TYPE_MAIN_VARIANT (t));
  base = complete_type (TYPE_MAIN_VARIANT (base));
  
  bk = lookup_base_r (TYPE_BINFO (t), base, access & ‾ba_quiet,
		      0, 0, 0, &binfo);

  switch (bk)
    {
    case bk_inaccessible:
      binfo = NULL_TREE;
      if (!(access & ba_quiet))
	{
	  error ("`%T' is an inaccessible base of `%T'", base, t);
	  binfo = error_mark_node;
	}
      break;
    case bk_ambig:
      if (access != ba_any)
	{
	  binfo = NULL_TREE;
	  if (!(access & ba_quiet))
	    {
	      error ("`%T' is an ambiguous base of `%T'", base, t);
	      binfo = error_mark_node;
	    }
	}
      break;
    default:;
    }
  
  if (kind_ptr)
    *kind_ptr = bk;
  
  return binfo;
}

/* Worker function for get_dynamic_cast_base_type.  */

static int
dynamic_cast_base_recurse (subtype, binfo, via_virtual, offset_ptr)
     tree subtype;
     tree binfo;
     int via_virtual;
     tree *offset_ptr;
{
  tree binfos;
  int i, n_baselinks;
  int worst = -2;
  
  if (BINFO_TYPE (binfo) == subtype)
    {
      if (via_virtual)
        return -1;
      else
        {
          *offset_ptr = BINFO_OFFSET (binfo);
          return 0;
        }
    }
  
  binfos = BINFO_BASETYPES (binfo);
  n_baselinks = binfos ? TREE_VEC_LENGTH (binfos) : 0;
  for (i = 0; i < n_baselinks; i++)
    {
      tree base_binfo = TREE_VEC_ELT (binfos, i);
      int rval;
      
      if (!TREE_VIA_PUBLIC (base_binfo))
        continue;
      rval = dynamic_cast_base_recurse
             (subtype, base_binfo,
              via_virtual || TREE_VIA_VIRTUAL (base_binfo