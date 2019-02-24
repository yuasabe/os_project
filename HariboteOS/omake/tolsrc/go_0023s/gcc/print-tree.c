/* Prints out tree in human readable form - GNU C-compiler
   Copyright (C) 1990, 1991, 1993, 1994, 1995, 1996, 1997, 1998, 1999, 2000,
   2001, 2002 Free Software Foundation, Inc.

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


#include "config.h"
#include "system.h"
#include "tree.h"
#include "ggc.h"
#include "langhooks.h"

/* Define the hash table of nodes already seen.
   Such nodes are not repeated; brief cross-references are used.  */

#define HASH_SIZE 37

struct bucket
{
  tree node;
  struct bucket *next;
};

static struct bucket **table;

/* Print the node NODE on standard error, for debugging.
   Most nodes referred to by this one are printed recursively
   down to a depth of six.  */

void
debug_tree (node)
     tree node;
{
  table = (struct bucket **) permalloc (HASH_SIZE * sizeof (struct bucket *));
  memset ((char *) table, 0, HASH_SIZE * sizeof (struct bucket *));
  print_node (stderr, "", node, 0);
  table = 0;
  fprintf (stderr, "¥n");
}

/* Print a node in brief fashion, with just the code, address and name.  */

void
print_node_brief (file, prefix, node, indent)
     FILE *file;
     const char *prefix;
     tree node;
     int indent;
{
  char class;

  if (node == 0)
    return;

  class = TREE_CODE_CLASS (TREE_CODE (node));

  /* Always print the slot this node is in, and its code, address and
     name if any.  */
  if (indent > 0)
    fprintf (file, " ");
  fprintf (file, "%s <%s ", prefix, tree_code_name[(int) TREE_CODE (node)]);
  fprintf (file, HOST_PTR_PRINTF, (char *) node);

  if (class == 'd')
    {
      if (DECL_NAME (node))
	fprintf (file, " %s", IDENTIFIER_POINTER (DECL_NAME (node)));
    }
  else if (class == 't')
    {
      if (TYPE_NAME (node))
	{
	  if (TREE_CODE (TYPE_NAME (node)) == IDENTIFIER_NODE)
	    fprintf (file, " %s", IDENTIFIER_POINTER (TYPE_NAME (node)));
	  else if (TREE_CODE (TYPE_NAME (node)) == TYPE_DECL
		   && DECL_NAME (TYPE_NAME (node)))
	    fprintf (file, " %s",
		     IDENTIFIER_POINTER (DECL_NAME (TYPE_NAME (node))));
	}
    }
  if (TREE_CODE (node) == IDENTIFIER_NODE)
    fprintf (file, " %s", IDENTIFIER_POINTER (node));

  /* We might as well always print the value of an integer or real.  */
  if (TREE_CODE (node) == INTEGER_CST)
    {
      if (TREE_CONSTANT_OVERFLOW (node))
	fprintf (file, " overflow");

      fprintf (file, " ");
      if (TREE_INT_CST_HIGH (node) == 0)
	fprintf (file, HOST_WIDE_INT_PRINT_UNSIGNED, TREE_INT_CST_LOW (node));
      else if (TREE_INT_CST_HIGH (node) == -1
	       && TREE_INT_CST_LOW (node) != 0)
	{
	  fprintf (file, "-");
	  fprintf (file, HOST_WIDE_INT_PRINT_UNSIGNED,
		   -TREE_INT_CST_LOW (node));
	}
      else
	fprintf (file, HOST_WIDE_INT_PRINT_DOUBLE_HEX,
		 TREE_INT_CST_HIGH (node), TREE_INT_CST_LOW (node));
    }
  if (TREE_CODE (node) == REAL_CST)
    {
      REAL_VALUE_TYPE d;

      if (TREE_OVERFLOW (node))
	fprintf (file, " overflow");

#if !defined(REAL_IS_NOT_DOUBLE) || defined(REAL_ARITHMETIC)
      d = TREE_REAL_CST (node);
      if (REAL_VALUE_ISINF (d))
	fprintf (file, " Inf");
      else if (REAL_VALUE_ISNAN (d))
	fprintf (file, " Nan");
      else
	{
	  char string[100];

	  REAL_VALUE_TO_DECIMAL (d, "%e", string);
	  fprintf (file, " %s", string);
	}
#else
      {
	int i;
	unsigned char *p = (unsigned char *) &TREE_REAL_CST (node);
	