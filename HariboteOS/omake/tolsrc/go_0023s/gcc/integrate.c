/* Procedure integration for GCC.
   Copyright (C) 1988, 1991, 1993, 1994, 1995, 1996, 1997, 1998,
   1999, 2000, 2001, 2002 Free Software Foundation, Inc.
   Contributed by Michael Tiemann (tiemann@cygnus.com)

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

#include "rtl.h"
#include "tree.h"
#include "tm_p.h"
#include "regs.h"
#include "flags.h"
#include "debug.h"
#include "insn-config.h"
#include "expr.h"
#include "output.h"
#include "recog.h"
#include "integrate.h"
#include "real.h"
#include "except.h"
#include "function.h"
#include "toplev.h"
#include "intl.h"
#include "loop.h"
#include "params.h"
#include "ggc.h"
#include "target.h"

/* !kawai! */
#include "../include/obstack.h"
#define	obstack_chunk_alloc	xmalloc
#define	obstack_chunk_free	free
/* end of !kawai! */

extern struct obstack *function_maybepermanent_obstack;

/* Similar, but round to the next highest integer that meets the
   alignment.  */
#define CEIL_ROUND(VALUE,ALIGN)	(((VALUE) + (ALIGN) - 1) & ‾((ALIGN)- 1))

/* Default max number of insns a function can have and still be inline.
   This is overridden on RISC machines.  */
#ifndef INTEGRATE_THRESHOLD
/* Inlining small functions might save more space then not inlining at
   all.  Assume 1 instruction for the call and 1.5 insns per argument.  */
#define INTEGRATE_THRESHOLD(DECL) ¥
  (optimize_size ¥
   ? (1 + (3 * list_length (DECL_ARGUMENTS (DECL))) / 2) ¥
   : (8 * (8 + list_length (DECL_ARGUMENTS (DECL)))))
#endif


/* Private type used by {get/has}_func_hard_reg_initial_val.  */
typedef struct initial_value_pair {
  rtx hard_reg;
  rtx pseudo;
} initial_value_pair;
typedef struct initial_value_struct {
  int num_entries;
  int max_entries;
  initial_value_pair *entries;
} initial_value_struct;

static void setup_initial_hard_reg_value_integration PARAMS ((struct function *, struct inline_remap *));

static rtvec initialize_for_inline	PARAMS ((tree));
static void note_modified_parmregs	PARAMS ((rtx, rtx, void *));
static void integrate_parm_decls	PARAMS ((tree, struct inline_remap *,
						 rtvec));
static tree integrate_decl_tree		PARAMS ((tree,
						 struct inline_remap *));
static void subst_constants		PARAMS ((rtx *, rtx,
						 struct inline_remap *, int));
static void set_block_origin_self	PARAMS ((tree));
static void set_block_abstract_flags	PARAMS ((tree, int));
static void process_reg_param		PARAMS ((struct inline_remap *, rtx,
						 rtx));
void set_decl_abstract_flags		PARAMS ((tree, int));
static void mark_stores                 PARAMS ((rtx, rtx, void *));
static void save_parm_insns		PARAMS ((rtx, rtx));
static void copy_insn_list              PARAMS ((rtx, struct inline_remap *,
						 rtx));
static void copy_insn_notes		PARAMS ((rtx, struct inline_remap *,
						 int));
static int compare_blocks               PARAMS ((const PTR, const PTR));
static int find_block                   PARAMS ((const PTR, const PTR));

/* Used by copy_rtx_and_substitute; this indicates whether the function is
   called for the purpose of inlining or some other purpose (i.e. loop
   unrolling).  This affects how constant pool references are handled.
   This variable contains the FUNCTION_DECL for the inlined function.  */
static struct function *inlining = 0;

/* Returns the Ith entry in the label_map contained in MAP.  If the
   Ith entry has not yet been set, return a fresh label.  This function
   performs a lazy initialization of label_map, thereby avoiding huge memory
   explosions when the label_map gets very large.  */

rtx
get_label_from_map (map, i)
     struct inline_remap *map;
     int i;
{
  rtx x = map->label_map[i];

  if (x == NULL_RTX)
    x = map->label_map[i] = gen_label_rtx ();

  return x;
}

/* Return false if the function FNDECL cannot be inlined on account of its
   attributes, true otherwise.  */
bool
function_attribute_inlinable_p (fndecl)
     tree fndecl;
{
  bool has_machine_attr = false;
  tree a;

  for (a = DECL_ATTRIBUTES (fndecl); a; a = TREE_CHAIN (a))
    {
      tree name = TREE_PURPOSE (a);
      int i;

      for (i = 0; targetm.attribute_table[i].name != NULL; i++)
	{
	  if (is_attribute_p (targetm.attribute_table[i].name, name))
	    {
	      has_machine_attr = true;
	      break;
	    }
	}
      if (has_machine_attr)
	break;
    }

  if (has_machine_attr)
    return (*targetm.function_attribute_inlinable_p) (fndecl);
  else
    return true;
}

/* Zero if the current function (whose FUNCTION_DECL is FNDECL)
   is safe and reasonable to integrate into other functions.
   Nonzero means value is a warning msgid with a single %s
   for the function's name.  */

const char *
function_cannot_inline_p (fndecl)
     tree fndecl;
{
  rtx insn;
  tree last = tree_last (TYPE_ARG_TYPES (TREE_TYPE (fndecl)));

  /* For functions marked as inline increase the maximum size to
     MAX_INLINE_INSNS (-finline-limit-<n>).  For regular functions
     use the limit given by INTEGRATE_THRESHOLD.  */

  int max_insns = (DECL_INLINE (fndecl))
		   ? (MAX_INLINE_INSNS
		      + 8 * list_length (DECL_ARGUMENTS (fndecl)))
		   : INTEGRATE_THRESHOLD (fndecl);

  int ninsns = 0;
  tree parms;

  if (DECL_UNINLINABLE (fndecl))
    return N_("function cannot be inline");

  /* No inlines with varargs.  */
  if ((last && TREE_VALUE (last) != void_type_node)
      || current_function_varargs)
    return N_("varargs function cannot be inline");

  if (current_function_calls_alloca)
    return N_("function using alloca cannot be inline");

  if (current_function_calls_setjmp)
    return N_("function using setjmp cannot be inline");

  if (current_function_calls_eh_return)
    return N_("function uses __builtin_eh_return");

  if (current_function_contains_functions)
    return N_("function with nested functions cannot be inline");

  if (forced_labels)
    return
      N_("function with label addresses used in initializers cannot inline");

  if (current_function_cannot_inline)
    return current_function_cannot_inline;

  /* If its not even close, don't even look.  */
  if (get_max_uid () > 3 * max_insns)
    return N_("function too large to be inline");

#if 0
  /* Don't inline functions which do not specify a function prototype and
     have BLKmode argument or take the address of a parameter.  */
  for (parms = DECL_ARGUMENTS (fndecl); parms; parms = TREE_CHAIN (parms))
    {
      if (TYPE_MODE (TREE_TYPE (parms)) == BLKmode)
	TREE_ADDRESSABLE (parms) = 1;
      if (last == NULL_TREE && TREE_ADDRESSABLE (parms))
	return N_("no prototype, and parameter address used; cannot be inline");
    }
#endif

  /* We can't inline functions that return structures
     the old-fashioned PCC way, copying into a static block.  */
  if (current_function_returns_pcc_struct)
    return N_("inline functions not supported for this return value type");

  /* We can't inline functions that return structures of varying size.  */
  if (TREE_CODE (TREE_TYPE (TREE_TYPE (fndecl))) != VOID_TYPE
      && int_size_in_bytes (TREE_TYPE (TREE_TYPE (fndecl))) < 0)
    return N_("function with varying-size return value cannot be inline");

  /* Cannot inline a function with a varying size argument or one that
     receives a transparent union.  */
  for (parms = DECL_ARGUMENTS (fndecl); parms; parms = TREE_CHAIN (parms))
    {
      if (int_size_in_bytes (TREE_TYPE (parms)) < 0)
	return N_("function with varying-size parameter cannot be inline");
      else if (TREE_CODE (TREE_TYPE (parms)) == UNION_TYPE
	       && TYPE_TRANSPARENT_UNION (TREE_TYPE (parms)))
	return N_("function with transparent unit parameter cannot be inline");
    }

  if (get_max_uid () > max_insns)
    {
      for (ninsns = 0, insn = get_first_nonparm_insn ();
	   insn && ninsns < max_insns;
	   insn = NEXT_INSN (insn))
	if (INSN_P (insn))
	  ninsns++;

      if (ninsns >= max_insns)
	return N_("function too large to be inline");
    }

  /* We will not inline a function which uses computed goto.  The addresses of
     its local labels, which may be tucked into global storage, are of course
     not constant across instantiations, which causes unexpected behaviour.  */
  if (current_function_has_computed_jump)
    return N_("function with computed jump cannot inline");

  /* We cannot inline a nested function that jumps to a nonlocal label.  */
  if (current_function_has_nonlocal_goto)
    return N_("function with nonlocal goto cannot be inline");

  /* We can't inline functions that return a PARALLEL rtx.  */
  if (DECL_RTL_SET_P (DECL_RESULT (fndecl)))
    {
      rtx result = DECL_RTL (DECL_RESULT (fndecl));
      if (GET_CODE (result) == PARALLEL)
	return N_("inline functions not supported for this return value type");
    }

  /* If the function has a target specific attribute attached to it,
     then we assume that we should not inline it.  This can be overriden
     by the target if it defines TARGET_FUNCTION_ATTRIBUTE_INLINABLE_P.  */
  if (!function_attribute_inlinable_p (fndecl))
    return N_("function with target specific attribute(s) cannot be inlined");

  return NULL;
}

/* Map pseudo reg number into the PARM_DECL for the parm living in the reg.
   Zero for a reg that isn't a parm's home.
   Only reg numbers less than max_parm_reg are mapped here.  */
static tree *parmdecl_map;

/* In save_for_inline, nonzero if past the parm-initialization insns.  */
static int in_nonparm_insns;

/* Subroutine for `save_for_inline'.  Performs initialization
   needed to save FNDECL's insns and info for future inline expansion.  */

static rtvec
initialize_for_inline (fndecl)
     tree fndecl;
{
  int i;
  rtvec arg_vector;
  tree parms;

  /* Clear out PARMDECL_MAP.  It was allocated in the caller's frame.  */
  memset ((char *) parmdecl_map, 0, max_parm_reg * sizeof (tree));
  arg_vector = rtvec_alloc (list_length (DECL_ARGUMENTS (fndecl)));

  for (parms = DECL_ARGUMENTS (fndecl), i = 0;
       parms;
       parms = TREE_CHAIN (parms), i++)
    {
      rtx p = DECL_RTL (parms);

      /* If we have (mem (addressof (mem ...))), use the inner MEM since
	 otherwise the copy_rtx call below will not unshare the MEM since
	 it shares ADDRESSOF.  */
      if (GET_CODE (p) == MEM && GET_CODE (XEXP (p, 0)) == ADDRESSOF
	  && GET_CODE (XEXP (XEXP (p, 0), 0)) == MEM)
	p = XEXP (XEXP (p, 0), 0);

      RTVEC_ELT (arg_vector, i) = p;

      if (GET_CODE (p) == REG)
	parmdecl_map[REGNO (p)] = parms;
      else if (GET_CODE (p) == CONCAT)
	{
	  rtx preal = gen_realpart (GET_MODE (XEXP (p, 0)), p);
	  rtx pimag = gen_imagpart (GET_MODE (preal), p);

	  if (GET_CODE (preal) == REG)
	    parmdecl_map[REGNO (preal)] = parms;
	  if (GET_CODE (pimag) == REG)
	    parmdecl_map[REGNO (pimag)] = parms;
	}

      /* This flag is cleared later
	 if the function ever modifies the value of the parm.  */
      TREE_READONLY (parms) = 1;
    }

  return arg_vector;
}

/* Copy NODE (which must be a DECL, but not a PARM_DECL).  The DECL
   originally was in the FROM_FN, but now it will be in the
   TO_FN.  */

tree
copy_decl_for_inlining (decl, from_fn, to_fn)
     tree decl;
     tree from_fn;
     tree to_fn;
{
  tree copy;

  /* Copy the declaration.  */
  if (TREE_CODE (decl) == PARM_DECL || TREE_CODE (decl) == RESULT_DECL)
    {
      /* For a parameter, we must make an equivalent VAR_DECL