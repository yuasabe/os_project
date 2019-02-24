/* Tree-dumping functionality for intermediate representation.
   Copyright (C) 1999, 2000, 2002 Free Software Foundation, Inc.
   Written by Mark Mitchell <mark@codesourcery.com>

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

/* !kawai! */
#include "config.h"
#include "system.h"
#include "tree.h"
#include "c-tree.h"
#include "../include/splay-tree.h"
#include "diagnostic.h"
#include "toplev.h"
#include "tree-dump.h"
#include "langhooks.h"
/* end of !kawai! */

static unsigned int queue PARAMS ((dump_info_p, tree, int));
static void dump_index PARAMS ((dump_info_p, unsigned int));
static void dequeue_and_dump PARAMS ((dump_info_p));
static void dump_new_line PARAMS ((dump_info_p));
static void dump_maybe_newline PARAMS ((dump_info_p));
static void dump_string_field PARAMS ((dump_info_p, const char *, const char *));

/* Add T to the end of the queue of nodes to dump.  Returns the index
   assigned to T.  */

static unsigned int
queue (di, t, flags)
     dump_info_p di;
     tree t;
     int flags;
{
  dump_queue_p dq;
  dump_node_info_p dni;
  unsigned int index;

  /* Assign the next available index to T.  */
  index = ++di->index;

  /* Obtain a new queue node.  */
  if (di->free_list)
    {
      dq = di->free_list;
      di->free_list = dq->next;
    }
  else
    dq = (dump_queue_p) xmalloc (sizeof (struct dump_queue));

  /* Create a new entry in the splay-tree.  */
  dni = (dump_node_info_p) xmalloc (sizeof (struct dump_node_info));
  dni->index = index;
  dni->binfo_p = ((flags & DUMP_BINFO) != 0);
  dq->node = splay_tree_insert (di->nodes, (splay_tree_key) t, 
				(splay_tree_value) dni);

  /* Add it to the end of the queue.  */
  dq->next = 0;
  if (!di->queue_end)
    di->queue = dq;
  else
    di->queue_end->next = dq;
  di->queue_end = dq;

  /* Return the index.  */
  return index;
}

static void
dump_index (di, index)
     dump_info_p di;
     unsigned int index;
{
  fprintf (di->stream, "@%-6u ", index);
  di->column += 8;
}

/* If T has not already been output, queue it for subsequent output.
   FIELD is a string to print before printing the index.  Then, the
   index of T is printed.  */

void
queue_and_dump_index (di, field, t, flags)
     dump_info_p di;
     const char *field;
     tree t;
     int flags;
{
  unsigned int index;
  splay_tree_node n;

  /* If there's no node, just return.  This makes for fewer checks in
     our callers.  */
  if (!t)
    return;

  /* See if we've already queued or dumped this node.  */
  n = splay_tree_lookup (di->nodes, (splay_tree_key) t);
  if (n)
    index = ((dump_node_info_p) n->value)->index;
  else
    /* If we haven't, add it to the queue.  */
    index = queue (di, t, flags);

  /* Print the index of the node.  */
  dump_maybe_newline (di);
  fprintf (di->stream, "%-4s: ", field);
  di->column += 6;
  dump_index (di, index);
}

/* Dump the type of T.  */

void
queue_and_dump_type (di, t)
     dump_info_p di;
     tree t;
{
  queue_and_dump_index (di, "type", TREE_TYPE (t), DUMP_NONE);
}

/* Dump column control */
#define SOL_COLUMN 25		/* Start of line column.  */
#define EOL_COLUMN 55		/* End of line column.  */
#define COLUMN_ALIGNMENT 15	/* Alignment.  */

/* Insert a new line in the dump output, and indent to an appropriate
   place to start printing more fields.  */

static void
dum