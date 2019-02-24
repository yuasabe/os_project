/* Perform optimizations on tree structure.
   Copyright (C) 1998, 1999, 2000, 2001 Free Software Foundation, Inc.
   Written by Mark Michell (mark@codesourcery.com).

This file is part of GNU CC.

GNU CC is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

GNU CC is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU CC; see the file COPYING.  If not, write to the Free
Software Foundation, 59 Temple Place - Suite 330, Boston, MA
02111-1307, USA.  */

/* !kawai! */
#include "../gcc/config.h"
#include "../gcc/system.h"
#include "../gcc/tree.h"
#include "cp-tree.h"
#include "../gcc/rtl.h"
#include "../gcc/insn-config.h"
#include "../gcc/input.h"
#include "../gcc/integrate.h"
#include "../gcc/toplev.h"
#include "../gcc/varray.h"
#include "../gcc/ggc.h"
#include "../gcc/params.h"
#include "../include/hashtab.h"
#include "../gcc/debug.h"
#include "../gcc/tree-inline.h"
/* end of !kawai! */

/* Prototypes.  */

static tree calls_setjmp_r PARAMS ((tree *, int *, void *));
static void update_cloned_parm PARAMS ((tree, tree));
static void dump_function PARAMS ((enum tree_dump_index, tree));

/* Optimize the body of FN. */

void
optimize_function (fn)
     tree fn;
{
  dump_function (TDI_original, fn);

  /* While in this function, we may choose to go off and compile
     another function.  For example, we might instantiate a function
     in the hopes of inlining it.  Normally, that wouldn't trigger any
     actual RTL code-generation -- but it will if the template is
     actually needed.  (For example, if it's address is taken, or if
     some other function already refers to the template.)  If
     code-generation occurs, then garbage collection will occur, so we
     must protect ourselves, just as we do while building up the body
     of the function.  */
  ++function_depth;

  if (flag_inline_trees
      /* We do not inline thunks, as (a) the backend tries to optimize
         the call to the thunkee, (b) tree based inlining breaks that
         optimization, (c) virtual functions are rarely inlineable,
         and (d) ASM_OUTPUT_MI_THUNK is there to DTRT anyway.  */
      && !DECL_THUNK_P (fn))
    {
      optimize_inline_calls (fn);

      dump_function (TDI_inlined, fn);
    }
  
  /* Undo the call to ggc_push_context above.  */
  --function_depth;
  
  dump_function (TDI_optimized, fn);
}

/* Called from calls_setjmp_p via walk_tree.  */

static tree
calls_setjmp_r (tp, walk_subtrees, data)
     tree *tp;
     int *walk_subtrees ATTRIBUTE_UNUSED;
     void *data ATTRIBUTE_UNUSED;
{
  /* We're only interested in FUNCTION_DECLS.  */
  if (TREE_CODE (*tp) != FUNCTION_DECL)
    return NULL_TREE;

  return setjmp_call_p (*tp) ? *tp : NULL_TREE;
}

/* Returns non-zero if FN calls `setjmp' or some other function that
   can return more than once.  This function is conservative; it may
   occasionally return a non-zero value even when FN does not actually
   call `setjmp'.  */

int
calls_setjmp_p (fn)
     tree fn;
{
  return walk_tree_without_duplicates (&DECL_SAVED_TREE (fn),
				       calls_setjmp_r,
				       NULL) != NULL_TREE;
}

/* CLONED_PARM is a copy of CLONE, generated for a cloned constructor
   or destructor.  Update it to ensure that the source-position for
   the cloned parameter matches that for the original, and that the
   debugging generation code will be able to find the original PARM.  */

static void
update_cloned_parm (parm, cloned_parm)
     tree parm;
     tree cloned_parm;
{
  DECL_ABSTRACT_ORIGIN (cloned_parm) = parm;

  /* We may have taken its address. */
  TREE_ADDRESSABLE 