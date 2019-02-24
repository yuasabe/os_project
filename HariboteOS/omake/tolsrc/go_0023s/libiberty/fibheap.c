/* A Fibonacci heap datatype.
   Copyright 1998, 1999, 2000, 2001 Free Software Foundation, Inc.
   Contributed by Daniel Berlin (dan@cgsoftware.com).
   
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
along with GNU CC; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#ifdef HAVE_LIMITS_H
#include "../include/limits.h"
#endif
#ifdef HAVE_STDLIB_H
#include "../include/stdlib.h"
#endif
#ifdef HAVE_STRING_H
#include "../include/string.h"
#endif
/* !kawai! */
#include "../include/libiberty.h"
#include "../include/fibheap.h"
/* end of !kawai! */

#define FIBHEAPKEY_MIN	LONG_MIN

static void fibheap_ins_root PARAMS ((fibheap_t, fibnode_t));
static void fibheap_rem_root PARAMS ((fibheap_t, fibnode_t));
static void fibheap_consolidate PARAMS ((fibheap_t));
static void fibheap_link PARAMS ((fibheap_t, fibnode_t, fibnode_t));
static void fibheap_cut PARAMS ((fibheap_t, fibnode_t, fibnode_t));
static void fibheap_cascading_cut PARAMS ((fibheap_t, fibnode_t));
static fibnode_t fibheap_extr_min_node PARAMS ((fibheap_t));
static int fibheap_compare PARAMS ((fibheap_t, fibnode_t, fibnode_t));
static int fibheap_comp_data PARAMS ((fibheap_t, fibheapkey_t, void *,
				      fibnode_t));
static fibnode_t fibnode_new PARAMS ((void));
static void fibnode_insert_after PARAMS ((fibnode_t, fibnode_t));
#define fibnode_insert_before(a, b) fibnode_insert_after (a->left, b)
static fibnode_t fibnode_remove PARAMS ((fibnode_t));


/* Create a new fibonacci heap.  */
fibheap_t
fibheap_new ()
{
  return (fibheap_t) xcalloc (1, sizeof (struct fibheap));
}

/* Create a new fibonacci heap node.  */
static fibnode_t
fibnode_new ()
{
  fibnode_t node;

  node = xcalloc (1, sizeof *node);
  node->left = node;
  node->right = node;

  return node;
}

static inline int
fibheap_compare (heap, a, b)
     fibheap_t heap ATTRIBUTE_UNUSED;
     fibnode_t a;
     fibnode_t b;
{
  if (a->key < b->key)
    return -1;
  if (a->key > b->key)
    return 1;
  return 0;
}

static inline int
fibheap_comp_data (heap, key, data, b)
     fibheap_t heap;
     fibheapkey_t key;
     void *data;
     fibnode_t b;
{
  struct fibnode a;

  a.key = key;
  a.data = data;

  return fibheap_compare (heap, &a, b);
}

/* Insert DATA, with priority KEY, into HEAP.  */
fibnode_t
fibheap_insert (heap, key, data)
     fibheap_t heap;
     fibheapkey_t key;
     void *data;
{
  fibnode_t node;

  /* Create the new node.  */
  node = fibnode_new ();

  /* Set the node's data.  */
  node->data = data;
  node->key = key;

  /* Insert it into the root list.  */
  fibheap_ins_root (heap, node);

  /* If their was no minimum, or this key is less than the min,
     it's the new min.  */
  if (heap->min == NULL || node->key < heap->min->key)
    heap->min = node;

  heap->nodes++;

  return node;
}

/* Return the data of the minimum node (if we know it).  */
void *
fibheap_min (heap)
     fibheap_t heap;
{
  /* If there is no min, we can't easily return it.  */
  if (heap->min == NULL)
    return NULL;
  return heap->min->data;
}

/* Return the key of the minimum node (if we know it).  */
fibheapkey_t
fibheap_min_key (heap)
     fibheap_t heap;
{
  /* If there is no min, we can't easily return it.  */
  if (heap->min == NULL)
    return 0;
  return heap->min->key;
}

/* Union HEAPA and HEAPB into a new heap.  */
fibheap_t
f