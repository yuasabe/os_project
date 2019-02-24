/* Alias analysis for GNU C
   Copyright (C) 1997, 1998, 1999, 2000, 2001 Free Software Foundation, Inc.
   Contributed by John Carr (jfc@mit.edu).

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
#include "rtl.h"
#include "tree.h"
#include "tm_p.h"
#include "function.h"
#include "expr.h"
#include "regs.h"
#include "hard-reg-set.h"
#include "basic-block.h"
#include "flags.h"
#include "output.h"
#include "toplev.h"
#include "cselib.h"
#include "../include/splay-tree.h"
#include "ggc.h"
#include "langhooks.h"
/* end of !kawai! */

/* The alias sets assigned to MEMs assist the back-end in determining
   which MEMs can alias which other MEMs.  In general, two MEMs in
   different alias sets cannot alias each other, with one important
   exception.  Consider something like:

     struct S {int i; double d; };

   a store to an `S' can alias something of either type `int' or type
   `double'.  (However, a store to an `int' cannot alias a `double'
   and vice versa.)  We indicate this via a tree structure that looks
   like:
           struct S
            /   ¥
	   /     ¥
         |/_     _¥|
         int    double

   (The arrows are directed and point downwards.)
    In this situation we say the alias set for `struct S' is the
   `superset' and that those for `int' and `double' are `subsets'.

   To see whether two alias sets can point to the same memory, we must
   see if either alias set is a subset of the other. We need not trace
   past immediate descendents, however, since we propagate all
   grandchildren up one level.

   Alias set zero is implicitly a superset of all other alias sets.
   However, this is no actual entry for alias set zero.  It is an
   error to attempt to explicitly construct a subset of zero.  */

typedef struct alias_set_entry
{
  /* The alias set number, as stored in MEM_ALIAS_SET.  */
  HOST_WIDE_INT alias_set;

  /* The children of the alias set.  These are not just the immediate
     children, but, in fact, all descendents.  So, if we have:

       struct T { struct S s; float f; } 

     continuing our example above, the children here will be all of
     `int', `double', `float', and `struct S'.  */
  splay_tree children;

  /* Nonzero if would have a child of zero: this effectively makes this
     alias set the same as alias set zero.  */
  int has_zero_child;
} *alias_set_entry;

static int rtx_equal_for_memref_p	PARAMS ((rtx, rtx));
static rtx find_symbolic_term		PARAMS ((rtx));
rtx get_addr				PARAMS ((rtx));
static int memrefs_conflict_p		PARAMS ((int, rtx, int, rtx,
						 HOST_WIDE_INT));
static void record_set			PARAMS ((rtx, rtx, void *));
static rtx find_base_term		PARAMS ((rtx));
static int base_alias_check		PARAMS ((rtx, rtx, enum machine_mode,
						 enum machine_mode));
static rtx find_base_value		PARAMS ((rtx));
static int mems_in_disjoint_alias_sets_p PARAMS ((rtx, rtx));
static int insert_subset_children       PARAMS ((splay_tree_node, void*));
static tree find_base_decl		PARAMS ((tree));
static alias_set_entry get_alias_set_entry PARAMS ((HOST_WIDE_INT));
static rtx fixed_scalar_and_varying_struct_p PARAMS ((rtx, rtx, rtx, rtx,
						      int (*) (rtx, int)));
static int aliases_everything_p         PARAMS ((rtx));
static bool nonoverlapping_component_refs_p PARAMS ((tree, tree));
static tree decl_for_component_ref	PARAMS ((tree));
static rtx adjust_offset_for_component_ref PARAMS ((tree, rtx));
static int nonoverlapping_memrefs_p	PARAMS ((rtx, rtx));
static int write_dependence_p           PARAMS ((rtx, rtx, int));
static int nonlocal_mentioned_p         PARAMS ((rtx));

/* Set up all info needed to perform alias analysis on memory references.  */

/* Returns the size in bytes of the mode of X.  */
#define SIZE_FOR_MODE(X) (GET_MODE_SIZE (GET_MODE (X)))

/* Returns nonzero if MEM1 and MEM2 do not alias because they are in
   different alias sets.  We ignore alias sets in functions making use
   of variable arguments because the va_arg macros on some systems are
   not legal ANSI C.  */
#define DIFFERENT_ALIAS_SETS_P(MEM1, MEM2)			¥
  mems_in_disjoint_alias_sets_p (MEM1, MEM2)

/* Cap the number of passes we make over the insns propagating alias
   information through set chains.   10 is a completely arbitrary choice.  */
#define MAX_ALIAS_LOOP_PASSES 10
   
/* reg_base_value[N] gives an address to which register N is related.
   If all sets after the first add or subtract to the current value
   or otherwise modify it so it does not point to a different top level
   object, reg_base_value[N] is equal to the address part of the source
   of the first set.

   A base address can be an ADDRESS, SYMBOL_REF, or LABEL_REF.  ADDRESS
   expressions represent certain special values: function arguments and
   the stack, frame, and argument pointers.  

   The contents of an ADDRESS is not normally used, the mode of the
   ADDRESS determines whether the ADDRESS is a function argument or some
   other special value.  Pointer equality, not rtx_equal_p, determines whether
   two ADDRESS expressions refer to the same base address.

   The only use of the contents of an ADDRESS is for determining if the
   current function performs nonlocal memory memory references for the
   purposes of marking the function as a constant function.  */

static rtx *reg_base_value;
static rtx *new_reg_base_value;
static unsigned int reg_base_value_size; /* size of reg_base_value array */

#define REG_BASE_VALUE(X) ¥
  (REGNO (X) < reg_base_value_size ¥
   ? reg_base_value[REGNO (X)] : 0)

/* Vector of known invariant relationships between registers.  Set in
   loop unrolling.  Indexed by register number, if nonzero the value
   is an expression describing this register in terms of another.

   The length of this array is REG_BASE_VALUE_SIZE.

   Because this array contains only pseudo registers it has no effect
   after reload.  */
static rtx *alias_invariant;

/* Vector indexed by N giving the initial (unchanging) value known for
   pseudo-register N.  This array is initialized in
   init_alias_analysis, and does not change until end_alias_analysis
   is called.  */
rtx *reg_known_value;

/* Indicates number of valid entries in reg_known_value.  */
static unsigned int reg_known_value_size;

/* Vector recording for each reg_known_value whether it is due to a
   REG_EQUIV note.  Future passes (viz., reload) may replace the
   pseudo with the equivalent expression and so we account for the
   dependences that would be introduced if that happens.

   The REG_EQUIV notes created in assign_parms may mention the arg
   pointer, and there are explicit insns in the RTL that modify the
   arg pointer.  Thus we must ensure that such insns don't get
   scheduled across each other because that would invalidate the
   REG_EQUIV notes.  One could argue that the REG_EQUIV notes are
   wrong, but solving the problem in the scheduler will likely give
   better code, so we do it here.  */
char *reg_known_equiv_p;

/* True when scanning insns from the start of the rtl to the
   NOTE_INSN_FUNCTION_BEG note.  */
static int copying_arguments;

/* The splay-tree used to store the various alias set entries.  */
static splay_tree alias_sets;

/* Returns a pointer to the alias set entry for ALIAS_SET, if there is
   such an entry, or NULL otherwise.  */

static alias_set_entry
get_alias_set_entry (alias_set)
     HOST_WIDE_INT alias_set;
{
  splay_tree_node sn
    = splay_tree_lookup (alias_sets, (splay_tree_key) alias_set);

  return sn != 0 ? ((alias_set_entry) sn->value) : 0;
}

/* Returns nonzero if the alias sets for MEM1 and MEM2 are such that
   the two MEMs cannot alias each other.  */

static int 
mems_in_disjoint_alias_sets_p (mem1, mem2)
     rtx mem1;
     rtx mem2;
{
#ifdef ENABLE_CHECKING	
/* Perform a basic sanity check.  Namely, that there are no alias sets
   if we're not using strict aliasing.  This helps to catch bugs
   whereby someone uses PUT_CODE, but doesn't clear MEM_ALIAS_SET, or
   where a MEM is allocated in some way other than by the use of
   gen_rtx_MEM, and the MEM_ALIAS_SET is not cleared.  If we begin to
   use alias sets to indicate that spilled registers cannot alias each
   other, we might need to remove this check.  */
  if (! flag_strict_aliasing
      && (MEM_ALIAS_SET (mem1) != 0 || MEM_ALIAS_SET (mem2) != 0))
    abort ();
#endif

  return ! alias_sets_conflict_p (MEM_ALIAS_SET (mem1), MEM_ALIAS_SET (mem2));
}

/* Insert the NODE into the splay tree given by DATA.  Used by
   record_alias_subset via splay_tree_foreach.  */

static int
insert_subset_children (node, data)
     splay_tree_node node;
     void *data;
{
  splay_tree_insert ((splay_tree) data, node->key, node->value);

  return 0;
}

/* Return 1 if the two specified alias sets may conflict.  */

int
alias_sets_conflict_p (set1, set2)
     HOST_WIDE_INT set1, set2;
{
  alias_set_entry ase;

  /* If have no alias set information for one of the operands, we have
     to assume it can alias anything.  */
  if (set1 == 0 || set2 == 0
      /* If the two alias sets are the same, they may alias.  */
      || set1 == set2)
    return 1;

  /* See if the first alias set is a subset of the second.  */
  ase = get_alias_set_entry (set1);
  if (ase != 0
      && (ase->has_zero_child
	  || splay_tree_lookup (ase->children,
				(splay_tree_key) set2)))
    return 1;

  /* Now do the same, but with the alias sets reversed.  */
  ase = get_alias_set_entry (set2);
  if (ase != 0
      && (ase->has_zero_child
	  || splay_tree_lookup (ase->children,
				(splay_tree_key) set1)))
    return 1;

  /* The two alias sets are distinct and neither one is the
     child of the other.  Therefore, they cannot alias.  */
  return 0;
}

/* Return 1 if TYPE is a RECORD_TYPE, UNION_TYPE, or QUAL_UNION_TYPE and has
   has any readonly fields.  If any of the fields have types that
   contain readonly fields, return true as well.  */

int
readonly_fields_p (type)
     tree type;
{
  tree field;

  if (TREE_CODE (type) != RECORD_TYPE && TREE_CODE (type) != UNION_TYPE
      && TREE_CODE (type) != QUAL_UNION_TYPE)
    return 0;

  for (field = TYPE_FIELDS (type); field != 0; field = TREE_CHAIN (field))
    if (TREE_CODE (field) == FIELD_DECL
	&& (TREE_READONLY (field)
	    || readonly_fields_p (TREE_TYPE (field))))
      return 1;

  return 0;
}

/* Return 1 if any MEM object of type T1 will always conflict (using the
   dependency routines in this file) with any MEM object of type T2.
   This is used when allocating temporary storage.  If T1 and/or T2 are
   NULL_TREE, it means we know nothing about the storage.  */

int
objects_must_conflict_p (t1, t2)
     tree t1, t2;
{
  /* If neither has a type specified, we don't know if they'll conflict
     because we may be using them to store objects of various types, for
     example the argument and local variables areas of inlined functions.  */
  if (t1 == 0 && t2 == 0)
    return 0;

  /* If one or the other has readonly fields or is readonly,
     then they may not conflict.  */
  if ((t1 != 0 && readonly_fields_p (t1))
      || (t2 != 0 && readonly_fields_p (t2))
      || (t1 != 0 && TYPE_READONLY (t1))
      || (t2 != 0 && TYPE_READONLY (t2)))
    return 0;

  /* If they are the same type, they must conflict.  */
  if (t1 == t2
      /* Likewise 