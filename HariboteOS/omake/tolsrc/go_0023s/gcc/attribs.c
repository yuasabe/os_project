/* Functions dealing with attribute handling, used by most front ends.
   Copyright (C) 1992, 1993, 1994, 1995, 1996, 1997, 1998, 1999, 2000, 2001,
   2002 Free Software Foundation, Inc.

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
#include "flags.h"
#include "toplev.h"
#include "output.h"
#include "rtl.h"
#include "ggc.h"
#include "expr.h"
#include "tm_p.h"
#include "../include/obstack.h"
#include "cpplib.h"
#include "target.h"
/* end of !kawai! */

static void init_attributes		PARAMS ((void));

/* Table of the tables of attributes (common, format, language, machine)
   searched.  */
static const struct attribute_spec *attribute_tables[4];

static bool attributes_initialized = false;

static tree handle_packed_attribute	PARAMS ((tree *, tree, tree, int,
						 bool *));
static tree handle_nocommon_attribute	PARAMS ((tree *, tree, tree, int,
						 bool *));
static tree handle_common_attribute	PARAMS ((tree *, tree, tree, int,
						 bool *));
static tree handle_noreturn_attribute	PARAMS ((tree *, tree, tree, int,
						 bool *));
static tree handle_noinline_attribute	PARAMS ((tree *, tree, tree, int,
						 bool *));
static tree handle_always_inline_attribute PARAMS ((tree *, tree, tree, int,
						    bool *));
static tree handle_used_attribute	PARAMS ((tree *, tree, tree, int,
						 bool *));
static tree handle_unused_attribute	PARAMS ((tree *, tree, tree, int,
						 bool *));
static tree handle_const_attribute	PARAMS ((tree *, tree, tree, int,
						 bool *));
static tree handle_transparent_union_attribute PARAMS ((tree *, tree, tree,
							int, bool *));
static tree handle_constructor_attribute PARAMS ((tree *, tree, tree, int,
						  bool *));
static tree handle_destructor_attribute PARAMS ((tree *, tree, tree, int,
						 bool *));
static tree handle_mode_attribute	PARAMS ((tree *, tree, tree, int,
						 bool *));
static tree handle_section_attribute	PARAMS ((tree *, tree, tree, int,
						 bool *));
static tree handle_aligned_attribute	PARAMS ((tree *, tree, tree, int,
						 bool *));
static tree handle_weak_attribute	PARAMS ((tree *, tree, tree, int,
						 bool *));
static tree handle_alias_attribute	PARAMS ((tree *, tree, tree, int,
						 bool *));
static tree handle_no_instrument_function_attribute PARAMS ((tree *, tree,
							     tree, int,
							     bool *));
static tree handle_malloc_attribute	PARAMS ((tree *, tree, tree, int,
						 bool *));
static tree handle_no_limit_stack_attribute PARAMS ((tree *, tree, tree, int,
						     bool *));
static tree handle_pure_attribute	PARAMS ((tree *, tree, tree, int,
						 bool *));
static tree handle_deprecated_attribute	PARAMS ((tree *, tree, tree, int,
						 bool *));
static tree handle_vector_size_attribute PARAMS ((tree *, tree, tree, int,
						  bool *));
static tree vector_size_helper PARAMS ((tree, tree));

/* Table of machine-independent attributes common to all C-like languages.  */
static const struct attribute_spec c_common_attribute_table[] =
{
  /* { name, min_len, max_len, decl_req, type_req, fn_type_req, handler } */
  { "packed",                 0, 0, false, false, false,
      			      handle_packed_attribute },
  { "nocommon",               0, 0, true,  false, false,
			      handle_nocommon_attribute },
  { "common",                 0, 0, true,  false, false,
			      handle_common_attribute },
  /* FIXME: logically, noreturn attributes should be listed as
     "false, true, true" and apply to function types.  But implementing this
     would require all the places in the compiler that use TREE_THIS_VOLATILE
     on a decl to identify non-returning functions to be located and fixed
     to check the function type instead.  */
  { "noreturn",               0, 0, true,  false, false,
			      handle_noreturn_attribute },
  { "volatile",               0, 0, true,  false, false,
			      handle_noreturn_attribute },
  { "noinline",               0, 0, true,  false, false,
			      handle_noinline_attribute },
  { "always_inline",          0, 0, true,  false, false,
			      handle_always_inline_attribute },
  { "used",                   0, 0, true,  false, false,
			      handle_used_attribute },
  { "unused",                 0, 0, false, false, false,
			      handle_unused_attribute },
  /* The same comments as for noreturn attributes apply to const ones.  */
  { "const",                  0, 0, true,  false, false,
			      handle_const_attribute },
  { "transparent_union",      0, 0, false, false, false,
			      handle_transparent_union_attribute },
  { "constructor",            0, 0, true,  false, false,
			      handle_constructor_attribute },
  { "destructor",             0, 0, true,  false, false,
			      handle_destructor_attribute },
  { "mode",                   1, 1, false,  true, false,
			      handle_mode_attribute },
  { "section",                1, 1, true,  false, false,
			      handle_section_attribute },
  { "aligned",                0, 1, false, false, false,
			      handle_aligned_attribute },
  { "weak",                   0, 0, true,  false, false,
			      handle_weak_attribute },
  { "alias",                  1, 1, true,  false, false,
			      handle_alias_attribute },
  { "no_instrument_function", 0, 0, true,  false, false,
			      handle_no_instrument_function_attribute },
  { "malloc",                 0, 0, true,  false, false,
			      handle_malloc_attribute },
  { "no_stack_limit",         0, 0, true,  false, false,
			      handle_no_limit_stack_attribute },
  { "pure",                   0, 0, true,  false, false,
			      handle_pure_attribute },
  { "deprecated",             0, 0, false, false, false,
			      handle_deprecated_attribute },
  { "vector_size",	      1, 1, false, true, false,
			      handle_vector_size_attribute },
  { NULL,                     0, 0, false, false, false, NULL }
};

/* Default empty table of attributes.  */
static const struct attribute_spec empty_attribute_table[] =
{
  { NULL, 0, 0, false, false, false, NULL }
};

/* Table of machine-independent attributes for checking formats, if used.  */
const struct attribute_spec *format_attribute_table = empty_attribute_table;

/* Table of machine-independent attributes for a particular language.  */
const struct attribute_spec *lang_attribute_table = empty_attribute_table;

/* Flag saying whether common language attributes are to be supported.  */
int lang_attribute_common = 1;

/* Initialize attribute tables, and make some sanity checks
   if --enable-checking.  */

static void
init_attributes ()
{
#ifdef ENABLE_CHECKING
  int i;
#endif

  attribute_tables[0]
    = lang_attribute_common ? c_common_attribute_table : empty_attribute_table;
  attribute_tables[1] = lang_attribute_table;
  attribute_tables[2] = format_attribute_table;
  attribute_tables[3] = targetm.attribute_table;

#ifdef ENABLE_CHECKING
  /* Make some sanity checks on the attribute tables.  */
  for (i = 0;
       i < (int) (sizeof (attribute_tables) / sizeof (attribute_tables[0]));
       i++)
    {
      int j;

      for (j = 0; attribute_tables[i][j].name != NULL; j++)
	{
	  /* The name must not begin and end with __.  */
	  const char *name = attribute_tables[i][j].name;
	  int len = strlen (name);
	  if (name[0] == '_' && name[1] == '_'
	      && name[len - 1] == '_' && name[len - 2] == '_')
	    abort ();
	  /* The minimum and maximum lengths must be consistent.  */
	  if (attribute_tables[i][j].min_length < 0)
	    abort ();
	  if (attribute_tables[i][j].max_length != -1
	      && (attribute_tables[i][j].max_length
		  < attribute_tables[i][j].min_length))
	    abort ();
	  /* An attribute cannot require both a DECL and a TYPE.  */
	  if (attribute_tables[i][j].decl_required
	      && attribute_tables[i][j].type_required)
	    abort ();
	  /* If an attribute requires a function type, in particular
	     it requires a type.  */
	  if (attribute_tables[i][j].function_type_required
	      && !attribute_tables[i][j].type_required)
	    abort ();
	}
    }

  /* Check that each name occurs just once in each table.  */
  for (i = 0;
       i < (int) (sizeof (attribute_tables) / sizeof (attribute_tables[0]));
       i++)
    {
      int j, k;
      for (j = 0; attribute_tables[i][j].name != NULL; j++)
	for (k = j + 1; attribute_tables[i][k].name != NULL; k++)
	  if (!strcmp (attribute_tables[i][j].name,
		       attribute_tables[i][k].name))
	    abort ();
    }
  /* Check that no name occurs in more than one table.  */
  for (i = 0;
       i < (int) (sizeof (attribute_tables) / sizeof (attribute_tables[0]));
       i++)
    {
      int j, k, l;

      for (j = i + 1;
	   j < ((int) (sizeof (attribute_tables)
		       / sizeof (attribute_tables[0])));
	   j++)
	for (k = 0; attribute_tables[i][k].name != NULL; k++)
	  for (l = 0; attribute_tables[j][l].name != NULL; l++)
	    if (!strcmp (attribute_tables[i][k].name,
			 attribute_tables[j][l].name))
	      abort ();
    }
#endif

  attributes_initialized = true;
}

/* Process the attributes listed in ATTRIBUTES and install them in *NODE,
   which is either a DECL (including a TYPE_DECL) or a TYPE.  If a DECL,
   it should be modified in place; if a TYPE, a copy should be created
   unless ATTR_FLAG_TYPE_IN_PLACE is set in FLAGS.  FLAGS gives further
   information, in the form of a bitwise OR of flags in enum attribute_flags
   from tree.h.  Depending on these flags, some attributes may be
   returned to be applied at a later stage (for example, to apply
   a decl attribute to the declaration rather than to its type).  If
   ATTR_FLAG_BUILT_IN is not set and *NODE is a DECL, then also consider
   whether there might be some default attributes to apply to this DECL;
   if so, decl_attributes will be called recursively with those attributes
   and ATTR_FLAG_BUILT_IN set.  */

tree
decl_attributes (node, attributes, flags)
     tree *node, attributes;
     int flags;
{
  tree a;
  tree returned_attrs = NULL_TREE;

  if (!attributes_initialized)
    init_attributes ();

  (*targetm.insert_attributes) (*node, &attributes);

  if (DECL_P (*node) && TREE_CODE (*node) == FUNCTION_DECL
      && !(flags & (int) ATTR_FLAG_BUILT_IN))
    insert_default_attributes (*node);

  for (a = attributes; a; a = TREE_CHAIN (a))
    {
      tree name = TREE_PURPOSE (a);
      tree args = TREE_VALUE (a);
      tree *anode = node;
      const struct attribute_spec *spec = NULL;
      bool no_add_attrs = 0;
      int i;

      for (i = 0;
	   i < ((int) (sizeof (attribute_tables)
		       / sizeof (attribute_tables[0])));
	   i++)
	{
	  int j;

	  for (j = 0; attribute_tables[i][j].name != NULL; j++)
	    {
	      if (is_attribute_p (attribute_tables[i][j].name, name))
		{
		  spec = &attribute_tables[i][j];
		  break;
		}
	    }
	  if (spec != NULL)
	    break;
	}

      if (spec == NULL)
	{
	  warning ("`%s' attribute directive ignored",
		   IDENTIFIER_POINTER (name));
	  continue;
	}
      else if (list_length (args) < spec->min_length
	       || (spec->max_length >= 0
		   && list_length (args) > spec->max_length))
	{
	  error ("wrong number of arguments specified for `%s' attribute",
		 IDENTIFIER_POINTER (name));
	  continue;
	}

      if (spec->decl_required && !DECL_P (*anode))
	{
	  if (flags & ((int) ATTR_FLAG_DECL_NEXT
		       | (