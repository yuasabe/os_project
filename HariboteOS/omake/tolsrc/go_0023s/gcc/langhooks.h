/* The lang_hooks data structure.
   Copyright 2001 Free Software Foundation, Inc.

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

#ifndef GCC_LANG_HOOKS_H
#define GCC_LANG_HOOKS_H

/* A print hook for print_tree ().  */
typedef void (*lang_print_tree_hook) PARAMS ((FILE *, tree, int indent));

/* The following hooks are documented in langhooks.c.  Must not be
   NULL.  */

struct lang_hooks_for_tree_inlining
{
  union tree_node *(*walk_subtrees) PARAMS ((union tree_node **, int *,
					     union tree_node *(*)
					     (union tree_node **,
					      int *, void *),
					     void *, void *));
  int (*cannot_inline_tree_fn) PARAMS ((union tree_node **));
  int (*disregard_inline_limits) PARAMS ((union tree_node *));
  union tree_node *(*add_pending_fn_decls) PARAMS ((void *,
						    union tree_node *));
  int (*tree_chain_matters_p) PARAMS ((union tree_node *));
  int (*auto_var_in_fn_p) PARAMS ((union tree_node *, union tree_node *));
  union tree_node *(*copy_res_decl_for_inlining) PARAMS ((union tree_node *,
							  union tree_node *,
							  union tree_node *,
							  void *, int *,
							  void *));
  int (*anon_aggr_type_p) PARAMS ((union tree_node *));
  int (*start_inlining) PARAMS ((union tree_node *));
  void (*end_inlining) PARAMS ((union tree_node *));
  union tree_node *(*convert_parm_for_inlining) PARAMS ((union tree_node *,
							 union tree_node *,
							 union tree_node *));
};

/* The following hooks are used by tree-dump.c.  */

struct lang_hooks_for_tree_dump
{
  /* Dump language-specific parts of tree nodes.  Returns non-zero if it 
     does not want the usual dumping of the second argument.  */
  int (*dump_tree) PARAMS ((void *, tree));

  /* Determine type qualifiers in a language-specific way.  */
  int (*type_quals) PARAMS ((tree));
};

/* Language-specific hooks.  See langhooks-def.h for defaults.  */

struct lang_hooks
{
  /* String identifying the front end.  e.g. "GNU C++".  */
  const char *name;

  /* sizeof (struct lang_identifier), so make_node () creates
     identifier nodes long enough for the language-specific slots.  */
  size_t identifier_size;

  /* The first callback made to the front end, for simple
     initialization needed before any calls to decode_option.  */
  void (*init_options) PARAMS ((void));

  /* Function called with an option vector as argument, to decode a
     single option (typically starting with -f or -W or +).  It should
     return the number of command-line arguments it uses if it handles
     the option, or 0 and not complain if it does not recognise the
     option.  If this function returns a negative number, then its
     absolute value is the number of command-line arguments used, but,
     in addition, no language-independent option processing should be
     done for this option.  */
  int (*decode_option) PARAMS ((int, char **));

  /* Called when all command line options have been parsed.  Should do
     any required consistency checks, modifications etc.  Complex
     initialization should be left to the "init" callback, since GC
     and the identifier hashes are set up between now and then.

     If errorcount is non-zero after this call the compiler exits
     immediately and the finish hook is not called.  */
  void (*post_options) PARAMS ((void));

  /* Called after post_options, to initialize th