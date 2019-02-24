/* Control and data flow functions for trees.
   Copyright 2001, 2002 Free Software Foundation, Inc.
   Contributed by Alexandre Oliva <aoliva@redhat.com>

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
#include "config.h"
#include "system.h"
#include "toplev.h"
#include "tree.h"
#include "tree-inline.h"
#include "rtl.h"
#include "expr.h"
#include "flags.h"
#include "params.h"
#include "input.h"
#include "insn-config.h"
#include "integrate.h"
#include "varray.h"
#include "../include/hashtab.h"
#include "../include/splay-tree.h"
#include "langhooks.h"
/* end of !kawai! */

/* This should be eventually be generalized to other languages, but
   this would require a shared function-as-trees infrastructure.  */
#include "c-common.h" 

/* 0 if we should not perform inlining.
   1 if we should expand functions calls inline at the tree level.  
   2 if we should consider *all* functions to be inline 
   candidates.  */

int flag_inline_trees = 0;

/* To Do:

   o In order to make inlining-on-trees work, we pessimized
     function-local static constants.  In particular, they are now
     always output, even when not addressed.  Fix this by treating
     function-local static constants just like global static
     constants; the back-end already knows not to output them if they
     are not needed.

   o Provide heuristics to clamp inlining of recursive template
     calls?  */

/* Data required for function inlining.  */

typedef struct inline_data
{
  /* A stack of the functions we are inlining.  For example, if we are
     compiling `f', which calls `g', which calls `h', and we are
     inlining the body of `h', the stack will contain, `h', followed
     by `g', followed by `f'.  The first few elements of the stack may
     contain other functions that we know we should not recurse into,
     even though they are not directly being inlined.  */
  varray_type fns;
  /* The index of the first element of FNS that really represents an
     inlined function.  */
  unsigned first_inlined_fn;
  /* The label to jump to when a return statement is encountered.  If
     this value is NULL, then return statements will simply be
     remapped as return statements, rather than as jumps.  */
  tree ret_label;
  /* The map from local declarations in the inlined function to
     equivalents in the function into which it is being inlined.  */
  splay_tree decl_map;
  /* Nonzero if we are currently within the cleanup for a
     TARGET_EXPR.  */
  int in_target_cleanup_p;
  /* A stack of the TARGET_EXPRs that we are currently processing.  */
  varray_type target_exprs;
  /* A list of the functions current function has inlined.  */
  varray_type inlined_fns;
  /* The approximate number of statements we have inlined in the
     current call stack.  */
  int inlined_stmts;
  /* We use the same mechanism to build clones that we do to perform
     inlining.  However, there are a few places where we need to
     distinguish between those two situations.  This flag is true if
     we are cloning, rather than inlining.  */
  bool cloning_p;
  /* Hash table used to prevent walk_tree from visiting the same node
     umpteen million times.  */
  htab_t tree_pruner;
} inline_data;

/* Prototypes.  */

static tree initialize_inlined_parameters PARAMS ((inline_data *, tree, tree));
static tree declare_return_variable PARAMS ((inline_data *, tree *));
static tree copy_body_r PARAMS ((tree *, int *, void *));
static tree copy_body PARAMS ((inline_data *));
static tree expand_call_inline PARAMS ((tree *, int *, void *));
static void expand_calls_inline PARAMS ((tree *, inline_data *));
static int inlinable_function_p PARAMS ((tree, inline_data *));
static tree remap_decl PARAMS ((tree, inline_data *));
static void remap_block PARAMS ((tree, tree, inline_data *));
static void copy_scope_stmt PARAMS ((tree *, int *, inline_data *));

/* The approximate number of instructions per statement.  This number
   need not be particularly accurate; it is used only to make
   decisions about when a function is too big to inline.  */
#define INSNS_PER_STMT (10)

/* Remap DECL during the copying of the BLOCK tree for the function.  */

static tree
remap_decl (decl, id)
     tree decl;
     inline_data *id;
{
  splay_tree_node n;
  tree fn;

  /* We only remap local variables in the current function.  */
  fn = VARRAY_TOP_TREE (id->fns);
  if (! (*lang_hooks.tree_inlining.auto_var_in_fn_p) (decl, fn))
    return NULL_TREE;

  /* See if we have remapped this declaration.  */
  n = splay_tree_lookup (id->decl_map, (splay_tree_key) decl);
  /* If we didn't already have an equivalent for this declaration,
     create one now.  */
  if (!n)
    {
      tree t;

      /* Make a copy of the variable or label.  */
      t = copy_decl_for_inlining (decl, fn,
				  VARRAY_TREE (id->fns, 0));

      /* The decl T could be a dynamic array or other variable size type,
	 in which case some fields need to be remapped because they may
	 contain SAVE_EXPRs.  */
      if (TREE_TYPE (t) && TREE_CODE (TREE_TYPE (t)) == ARRAY_TYPE
	  && TYPE_DOMAIN (TREE_TYPE (t)))
	{
	  TREE_TYPE (t) = copy_node (TREE_TYPE (t));
	  TYPE_DOMAIN (TREE_TYPE (t))
	    = copy_node (TYPE_DOMAIN (TREE_TYPE (t)));
	  walk_tree (&TYPE_MAX_VALUE (TYPE_DOMAIN (TREE_TYPE (t))),
		     copy_body_r, id, NULL);
	}

      if (! DECL_NAME (t) && TREE_TYPE (t)
	  && (*lang_hooks.tree_inlining.anon_aggr_type_p) (TREE_TYPE (t)))
	{
	  /* For a VAR_DECL of anonymous type, we must also copy the
	     member VAR_DECLS here and rechain the
	     DECL_ANON_UNION_ELEMS.  */
	  tree members = NULL;
	  tree src;
	  
	  for (src = DECL_ANON_UNION_ELEMS (t); src;
	       src = TREE_CHAIN (src))
	    {
	      tree member = remap_decl (TREE_VALUE (src), id);

	      if (TREE_PURPOSE (src))
		abort ();
	      members = tree_cons (NULL, member, members);
	    }
	  DECL_ANON_UNION_ELEMS (t) = nreverse (members);
	}
      
      /* Remember it, so that if we encounter this local entity
	 again we can reuse this copy.  */
      n = splay_tree_insert (id->decl_map,
			     (splay_tree_key) decl,
			     (splay_tree_value) t);
    }

  return (tree) n->value;
}

/* Copy the SCOPE_STMT_BLOCK associated with SCOPE_STMT to contain
   remapped versions of the variables therein.  And hook the new block
   into the block-tree.  If non-NULL, the DECLS are declarations to
   add to use instead of the BLOCK_VARS in the old block.  */

static void
remap_block (scope_stmt, decls, id)
     tree scope_stmt;
     tree decls;
     inline_data *id;
{
  /* We cannot do this in the cleanup for a TARGET_EXPR since we do
     not know whether or not expand_expr will actually write out the
     code we put there.  If it does not, then we'll have more BLOCKs
     than block-notes, and things will go awry.  At some point, we
     should make the back-end handle BLOCK notes in a tidier way,
     without requiring a strict correspondence to the block-tree; then
     this check can go.  */
  if (id->in_target_cleanup_p)
    {
      SCOPE_STMT_BLOCK (scope_stmt) = NULL_TREE;
      return;
    }

  /* If this is the beginning of a scope, remap the associated BLOCK.  */
  if (SCOPE_BEGIN_P (scope_stmt) && SCOPE_STMT_BLOCK (scope_stmt))
    {
      tree old_block;
      tree new_block;
      tree old_var;
      tree fn;

      /* Make the new block.  */
      old_block = SCOPE_STMT_BLOCK (scope_stmt);
      new_block = make_node (BLOCK);
      TREE_USED (new_block) = TREE_USED (old_block);
      BLOCK_ABSTRACT_ORIGIN (new_block) = old_block;
      SCOPE_STMT_BLOCK (scope_stmt) = new_block;

      /* Remap its variables.  */
      for (old_var = decls ? decls : BLOCK_VARS (old_block);
	   old_var;
	   old_var = TREE_CHAIN (old_var))
	{
	  tree new_var;

	  /* Remap the variable.  */
	  new_var = remap_decl (old_var, id);
	  /* If we didn't remap this variable, so we can't mess with
	     its TREE_CHAIN.  If we remapped this variable to
	     something other than a declaration (say, if we mapped it
	     to a constant), then we must similarly omit any mention
	     of it here.  */
	  if (!new_var || !DECL_P (new_var))
	    ;
	  else
	    {
	      TREE_CHAIN (new_var) = BLOCK_VARS (new_block);
	      BLOCK_VARS (new_block) = new_var;
	    }
	}
      /* We put the BLOCK_VARS in reverse order; fix that now.  */
      BLOCK_VARS (new_block) = nreverse (BLOCK_VARS (new_block));
      fn = VARRAY_TREE (id->fns, 0);
      if (id->cloning_p)
	/* We're building a clone; DECL_INITIAL is still
	   error_mark_node, and current_binding_level is the parm
	   binding level.  */
	insert_block (new_block);
      else
	{
	  /* Attach this new block after the DECL_INITIAL block for the
	     function into which this block is being inlined.  In
	     rest_of_compilation we will straighten out the BLOCK tree.  */
	  tree *first_block;
	  if (DECL_INITIAL (fn))
	    first_block = &BLOCK_CHAIN (DECL_INITIAL (fn));
	  else
	    first_block = &DECL_INITIAL (fn);
	  BLOCK_CHAIN (new_block) = *first_block;
	  *first_block = new_block;
	}
      /* Remember the remapped block.  */
      splay_tree_insert (id->decl_map,
			 (splay_tree_key) old_block,
			 (splay_tree_value) new_block);
    }
  /* If this is the end of a scope, set the SCOPE_STMT_BLOCK to be the
     remapped block.  */
  else if (SCOPE_END_P (scope_stmt) && SCOPE_STMT_BLOCK (scope_stmt))
    {
      splay_tree_node n;

      /* Find this block in the table of remapped things.  */
      n = splay_tree_lookup (id->decl_map,
			     (splay_tree_key) SCOPE_STMT_BLOCK (scope_stmt));
      if (! n)
	abort ();
      SCOPE_STMT_BLOCK (scope_stmt) = (tree) n->value;
    }
}

/* Copy the SCOPE_STMT pointed to by TP.  */

static void
copy_scope_stmt (tp, walk_subtrees, id)
     tree *tp;
     int *walk_subtrees;
     inline_data *id;
{
  tree block;

  /* Remember whether or not this statement was nullified.  When
     making a copy, copy_tree_r always sets SCOPE_NULLIFIED_P (and
     doesn't copy the SCOPE_STMT_BLOCK) to free callers from having to
     deal with copying BLOCKs if they do not wish to do so.  */
  block = SCOPE_STMT_BLOCK (*tp);
  /* Copy (and replace) the statement.  */
  copy_tree_r (tp, walk_subtrees, NULL);
  /* Restore the SCOPE_STMT_BLOCK.  */
  SCOPE_STMT_BLOCK (*tp) = block;

  /* Remap the associated block.  */
  remap_block (*tp, NULL_TREE, id);
}

/* Called from copy_body via walk_tree.  DATA is really an
   `inline_data *'.  */

static tree
copy_body_r (tp, walk_subtrees, data)
     tree *tp;
     int *walk_subtrees;
     void *data;
{
  inline_data* id;
  tree fn;

  /* Set up.  */
  id = (inline_data *) data;
  fn = VARRAY_TOP_TREE (id->fns);

#if 0
  /* All automatic variables should have a DECL_CONTEXT indicating
     what function they come from.  */
  if ((TREE_CODE (*tp) == VAR_DECL || TREE_CODE (*tp) == LABEL_DECL)
      && DECL_NAMESPACE_SCOPE_P (*tp))
    if (! DECL_EXTERNAL (*tp) && ! TREE_STATIC (*tp))
      abort ();
#endif

  /* If this is a RETURN_STMT, change it into an EXPR_STMT and a
     GOTO_STMT with the RET_LABEL as its target.  */
  if (TREE_CODE (*tp) == RETURN_STMT && id->ret_label)
    {
      tree return_stmt = *tp;
      tree goto_stmt;

      /* Build the GOTO_STMT.  */
      goto_stmt = build_stmt (GOTO_STMT, id->ret_label);
      TRE