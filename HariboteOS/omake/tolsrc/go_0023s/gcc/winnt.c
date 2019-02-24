/* Subroutines for insn-output.c for Windows NT.
   Contributed by Douglas Rupp (drupp@cs.washington.edu)
   Copyright (C) 1995, 1997, 1998, 1999, 2000, 2001 Free Software Foundation, Inc.

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
#include "../gcc/config.h"
#include "../gcc/system.h"
#include "../gcc/rtl.h"
#include "../gcc/regs.h"
#include "../gcc/hard-reg-set.h"
#include "../gcc/output.h"
#include "../gcc/tree.h"
#include "../gcc/flags.h"
#include "../gcc/tm_p.h"
#include "../gcc/toplev.h"
#include "../include/hashtab.h"
/* end of !kawai! */

/* i386/PE specific attribute support.

   i386/PE has two new attributes:
   dllexport - for exporting a function/variable that will live in a dll
   dllimport - for importing a function/variable from a dll

   Microsoft allows multiple declspecs in one __declspec, separating
   them with spaces.  We do NOT support this.  Instead, use __declspec
   multiple times.
*/

static tree associated_type PARAMS ((tree));
const char * gen_stdcall_suffix PARAMS ((tree));
const char * gen_fastcall_suffix PARAMS ((tree));
static int i386_pe_dllexport_p PARAMS ((tree));
static int i386_pe_dllimport_p PARAMS ((tree));
static void i386_pe_mark_dllexport PARAMS ((tree));
static void i386_pe_mark_dllimport PARAMS ((tree));
static void i386_pe_unmark_dllimport PARAMS ((tree));
static int i386_pe_fastcall_name_p PARAMS ((const char *));

/* Handle a "dllimport" or "dllexport" attribute;
   arguments as in struct attribute_spec.handler.  */
tree
ix86_handle_dll_attribute (node, name, args, flags, no_add_attrs)
     tree *node;
     tree name;
     tree args;
     int flags;
     bool *no_add_attrs;
{
  /* These attributes may apply to structure and union types being created,
     but otherwise should pass to the declaration involved.  */
  if (!DECL_P (*node))
    {
      if (flags & ((int) ATTR_FLAG_DECL_NEXT | (int) ATTR_FLAG_FUNCTION_NEXT
		   | (int) ATTR_FLAG_ARRAY_NEXT))
	{
	  *no_add_attrs = true;
	  return tree_cons (name, args, NULL_TREE);
	}
      if (TREE_CODE (*node) != RECORD_TYPE && TREE_CODE (*node) != UNION_TYPE)
	{
	  warning ("`%s' attribute ignored", IDENTIFIER_POINTER (name));
	  *no_add_attrs = true;
	}
    }

  return NULL_TREE;
}

/* Handle a "shared" attribute;
   arguments as in struct attribute_spec.handler.  */
tree
ix86_handle_shared_attribute (node, name, args, flags, no_add_attrs)
     tree *node;
     tree name;
     tree args ATTRIBUTE_UNUSED;
     int flags ATTRIBUTE_UNUSED;
     bool *no_add_attrs;
{
  if (TREE_CODE (*node) != VAR_DECL)
    {
      warning ("`%s' attribute only applies to variables",
	       IDENTIFIER_POINTER (name));
      *no_add_attrs = true;
    }

  return NULL_TREE;
}

/* Return the type that we should use to determine if DECL is
   imported or exported.  */

static tree
associated_type (decl)
     tree decl;
{
  tree t = NULL_TREE;

  /* In the C++ frontend, DECL_CONTEXT for a method doesn't actually refer
     to the containing class.  So we look at the 'this' arg.  */
  if (TREE_CODE (TREE_TYPE (decl)) == METHOD_TYPE)
    {
      /* Artificial methods are not affected by the import/export status of
	 their class unless they are virtual . Why?  
         This test does not appear necessary, and causes problems with
	 non-virtual thunks generated for methods in deriv