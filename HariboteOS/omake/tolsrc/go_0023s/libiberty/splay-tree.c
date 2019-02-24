/* A splay-tree datatype.  
   Copyright (C) 1998, 1999, 2000, 2001 Free Software Foundation, Inc.
   Contributed by Mark Mitchell (mark@markmitchell.com).

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

/* For an easily readable description of splay-trees, see:

     Lewis, Harry R. and Denenberg, Larry.  Data Structures and Their
     Algorithms.  Harper-Collins, Inc.  1991.  */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_STDLIB_H
#include "../include/stdlib.h"
#endif

#include "../include/stdio.h"

/* !kawai! */
#include "../include/libiberty.h"
#include "../include/splay-tree.h"
/* end of !kawai! */

static void splay_tree_delete_helper    PARAMS((splay_tree, 
						splay_tree_node));
static void splay_tree_splay            PARAMS((splay_tree,
						splay_tree_key));
static splay_tree_node splay_tree_splay_helper     
                                        PARAMS((splay_tree,
						splay_tree_key,
						splay_tree_node*,
						splay_tree_node*,
						splay_tree_node*));
static int splay_tree_foreach_helper    PARAMS((splay_tree,
					        splay_tree_node,
						splay_tree_foreach_fn,
						void*));

/* Deallocate NODE (a member of SP), and all its sub-trees.  */

static void 
splay_tree_delete_helper (sp, node)
     splay_tree sp;
     splay_tree_node node;
{
  if (!node)
    return;

  splay_tree_delete_helper (sp, node->left);
  splay_tree_delete_helper (sp, node->right);

  if (sp->delete_key)
    (*sp->delete_key)(node->key);
  if (sp->delete_value)
    (*sp->delete_value)(node->value);

  (*sp->deallocate) ((char*) node, sp->allocate_data);
}

/* Help splay SP around KEY.  PARENT and GRANDPARENT are the parent
   and grandparent, respectively, of NODE.  */

static splay_tree_node
splay_tree_splay_helper (sp, key, node, parent, grandparent)
     splay_tree sp;
     splay_tree_key key;
     splay_tree_node *node;
     splay_tree_node *parent;
     splay_tree_node *grandparent;
{
  splay_tree_node *next;
  splay_tree_node n;
  int comparison;
  
  n = *node;

  if (!n)
    return *parent;

  comparison = (*sp->comp) (key, n->key);

  if (comparison == 0)
    /* We've found the target.  */
    next = 0;
  else if (comparison < 0)
    /* The target is to the left.  */
    next = &n->left;
  else 
    /* The target is to the right.  */
    next = &n->right;

  if (next)
    {
      /* Continue down the tree.  */
      n = splay_tree_splay_helper (sp, key, next, node, parent);

      /* The recursive call will change the place to which NODE
	 points.  */
      if (*node != n)
	return n;
    }

  if (!parent)
    /* NODE is the root.  We are done.  */
    return n;

  /* First, handle the case where there is no grandparent (i.e.,
     *PARENT is the root of the tree.)  */
  if (!grandparent) 
    {
      if (n == (*parent)->left)
	{
	  *node = n->right;
	  n->right = *parent;
	}
      else
	{
	  *node = n->left;
	  n->left = *parent;
	}
      *parent = n;
      return n;
    }

  /* Next handle the cases where both N and *PARENT are left children,
     or where both are right children.  */
  if (n == (*parent)->left && *parent == (*grandparent)->left)
    {
      splay_tree_node p = *parent;

      (*grandparent)->left = p->right;
      p->right = *grandparent;
      p->left = n->right;
      n->right = p;
   