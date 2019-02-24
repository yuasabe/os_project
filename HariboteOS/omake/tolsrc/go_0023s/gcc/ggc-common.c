/* Simple garbage collection for the GNU compiler.
   Copyright (C) 1999, 2000, 2001, 2002 Free Software Foundation, Inc.

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

/* Generic garbage collection (GC) functions and data, not specific to
   any particular GC implementation.  */

/* !kawai! */
#include "config.h"
#include "system.h"
#include "rtl.h"
#include "tree.h"
#include "tm_p.h"
#include "hash.h"
#include "../include/hashtab.h"
#include "varray.h"
#include "ggc.h"
/* end of !kawai! */

/* Statistics about the allocation.  */
static ggc_statistics *ggc_stats;

/* The FALSE_LABEL_STACK, declared in except.h, has language-dependent
   semantics.  If a front-end needs to mark the false label stack, it
   should set this pointer to a non-NULL value.  Otherwise, no marking
   will be done.  */
void (*lang_mark_false_label_stack) PARAMS ((struct label_node *));

/* Trees that have been marked, but whose children still need marking.  */
varray_type ggc_pending_trees;

static void ggc_mark_rtx_children_1 PARAMS ((rtx));
static void ggc_mark_rtx_ptr PARAMS ((void *));
static void ggc_mark_tree_ptr PARAMS ((void *));
static void ggc_mark_rtx_varray_ptr PARAMS ((void *));
static void ggc_mark_tree_varray_ptr PARAMS ((void *));
static void ggc_mark_tree_hash_table_ptr PARAMS ((void *));
static int ggc_htab_delete PARAMS ((void **, void *));
static void ggc_mark_trees PARAMS ((void));
static bool ggc_mark_tree_hash_table_entry PARAMS ((struct hash_entry *,
						    hash_table_key));

/* Maintain global roots that are preserved during GC.  */

/* Global roots that are preserved during calls to gc.  */

struct ggc_root
{
  struct ggc_root *next;
  void *base;
  int nelt;
  int size;
  void (*cb) PARAMS ((void *));
};

static struct ggc_root *roots;

/* Add BASE as a new garbage collection root.  It is an array of
   length NELT with each element SIZE bytes long.  CB is a 
   function that will be called with a pointer to each element
   of the array; it is the intention that CB call the appropriate
   routine to mark gc-able memory for that element.  */

void
ggc_add_root (base, nelt, size, cb)
     void *base;
     int nelt, size;
     void (*cb) PARAMS ((void *));
{
  struct ggc_root *x = (struct ggc_root *) xmalloc (sizeof (*x));

  x->next = roots;
  x->base = base;
  x->nelt = nelt;
  x->size = size;
  x->cb = cb;

  roots = x;
}

/* Register an array of rtx as a GC root.  */

void
ggc_add_rtx_root (base, nelt)
     rtx *base;
     int nelt;
{
  ggc_add_root (base, nelt, sizeof (rtx), ggc_mark_rtx_ptr);
}

/* Register an array of trees as a GC root.  */

void
ggc_add_tree_root (base, nelt)
     tree *base;
     int nelt;
{
  ggc_add_root (base, nelt, sizeof (tree), ggc_mark_tree_ptr);
}

/* Register a varray of rtxs as a GC root.  */

void
ggc_add_rtx_varray_root (base, nelt)
     varray_type *base;
     int nelt;
{
  ggc_add_root (base, nelt, sizeof (varray_type), 
		ggc_mark_rtx_varray_ptr);
}

/* Register a varray of trees as a GC root.  */

void
ggc_add_tree_varray_root (base, nelt)
     varray_type *base;
     int nelt;
{
  ggc_add_root (base, nelt, sizeof (varray_type), 
		ggc_mark_tree_varray_ptr);
}

/* Register a hash table of trees as a GC root.  */

void
ggc_add_tree_hash_table_root (base, nelt)
     struct hash_table **base;
     int nelt;
{
  