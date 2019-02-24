/* Common subexpression elimination library for GNU compiler.
   Copyright (C) 1987, 1988, 1989, 1992, 1993, 1994, 1995, 1996, 1997, 1998,
   1999, 2000, 2001 Free Software Foundation, Inc.

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

/* !kawai! */
#include "rtl.h"
#include "tm_p.h"
#include "regs.h"
#include "hard-reg-set.h"
#include "flags.h"
#include "real.h"
#include "insn-config.h"
#include "recog.h"
#include "function.h"
#include "expr.h"
#include "toplev.h"
#include "output.h"
#include "ggc.h"
#include "../include/obstack.h"
#include "../include/hashtab.h"
#include "cselib.h"
/* end of !kawai! */

static int entry_and_rtx_equal_p	PARAMS ((const void *, const void *));
static unsigned int get_value_hash	PARAMS ((const void *));
static struct elt_list *new_elt_list	PARAMS ((struct elt_list *,
						 cselib_val *));
static struct elt_loc_list *new_elt_loc_list PARAMS ((struct elt_loc_list *,
						      rtx));
static void unchain_one_value		PARAMS ((cselib_val *));
static void unchain_one_elt_list	PARAMS ((struct elt_list **));
static void unchain_one_elt_loc_list	PARAMS ((struct elt_loc_list **));
static void clear_table			PARAMS ((int));
static int discard_useless_locs		PARAMS ((void **, void *));
static int discard_useless_values	PARAMS ((void **, void *));
static void remove_useless_values	PARAMS ((void));
static rtx wrap_constant		PARAMS ((enum machine_mode, rtx));
static unsigned int hash_rtx		PARAMS ((rtx, enum machine_mode, int));
static cselib_val *new_cselib_val	PARAMS ((unsigned int,
						 enum machine_mode));
static void add_mem_for_addr		PARAMS ((cselib_val *, cselib_val *,
						 rtx));
static cselib_val *cselib_lookup_mem	PARAMS ((rtx, int));
static void cselib_invalidate_regno	PARAMS ((unsigned int,
						 enum machine_mode));
static int cselib_mem_conflict_p	PARAMS ((rtx, rtx));
static int cselib_invalidate_mem_1	PARAMS ((void **, void *));
static void cselib_invalidate_mem	PARAMS ((rtx));
static void cselib_invalidate_rtx	PARAMS ((rtx, rtx, void *));
static void cselib_record_set		PARAMS ((rtx, cselib_val *,
						 cselib_val *));
static void cselib_record_sets		PARAMS ((rtx));

/* There are three ways in which cselib can look up an rtx:
   - for a REG, the reg_values table (which is indexed by regno) is used
   - for a MEM, we recursively look up its address and then follow the
     addr_list of that value
   - for everything else, we compute a hash value and go through the hash
     table.  Since different rtx's can still have the same hash value,
     this involves walking the table entries for a given value and comparing
     the locations of the entries with the rtx we are looking up.  */

/* A table that enables us to look up elts by their value.  */
static htab_t hash_table;

/* This is a global so we don't have to pass this through every function.
   It is used in new_elt_loc_list to set SETTING_INSN.  */
static rtx cselib_current_insn;

/* Every new unknown value gets a unique number.  */
static unsigned int next_unknown_value;

/* The number of registers we had when the varrays were last resized.  */
static unsigned int cselib_nregs;

/* Count values without known locations.  Whenever this grows too big, we
   remove these useless values from the table.  */
static int n_useless_values;

/* Number of useless values before we remove them from the hash table.  */
#define MAX_USELESS_VALUES 32

/* This table maps from register number to values.  It does not contain
   pointers to cselib_val structures, but rather elt_lists.  The purpose is
   to be able to refer to the same register in different modes.  */
static varray_type reg_values;
#define REG_VALUES(I) VARRAY_ELT_LIST (reg_values, (I))

/* The largest number of hard regs used by any entry added to the
   REG_VALUES table.  Cleared on each clear_table() invocation.   */
static unsigned int max_value_regs;

/* Here the set of indices I with REG_VALUES(I) != 0 is saved.  This is used
   in clear_table() for fast emptying.  */
static varray_type used_regs;

/* We pass this to cselib_invalidate_mem to invalidate all of
   memory for a non-const call instruction.  */
static rtx callmem;

/* Memory for our structures is allocated from this obstack.  */
static struct obstack cselib_obstack;

/* Used to quickly free all memory.  */
static char *cselib_startobj;

/* Caches for unused structures.  */
static cselib_val *empty_vals;
static struct elt_list *empty_elt_lists;
static struct elt_loc_list *empty_elt_loc_lists;

/* Set by discard_useless_locs if it deleted the last location of any
   value.  */
static int values_became_useless;


/* Allocate a struct elt_list and fill in its two elements with the
   arguments.  */

static struct elt_list *
new_elt_list (next, elt)
     struct elt_list *next;
     cselib_val *elt;
{
  struct elt_list *el = empty_elt_lists;

  if (el)
    empty_elt_lists = el->next;
  else
    el = (struct elt_list *) obstack_alloc (&cselib_obstack,
					    sizeof (struct elt_list));
  el->next = next;
  el->elt = elt;
  return el;
}

/* Allocate a struct elt_loc_list and fill in its two elements with the
   arguments.  */

static struct elt_loc_list *
new_elt_loc_list (next, loc)
     struct elt_loc_list *next;
     rtx loc;
{
  struct elt_loc_list *el = empty_elt_loc_lists;

  if (el)
    empty_elt_loc_lists = el->next;
  else
    el = (struct elt_loc_list *) obstack_alloc (&cselib_obstack,
						sizeof (struct elt_loc_list));
  el->next = next;
  el->loc = loc;
  el->setting_insn = cselib_current_insn;
  return el;
}

/* The elt_list at *PL is no longer needed.  Unchain it and free its
   storage.  */

static void
unchain_one_elt_list (pl)
     struct elt_list **pl;
{
  struct elt_list *l = *pl;

  *pl = l->next;
  l->next = empty_elt_lists;
  empty_elt_lists = l;
}

/* Likewise for elt_loc_lists.  */

static void
unchain_one_elt_loc_list (pl)
     struct elt_loc_list **pl;
{
  struct elt_loc_list *l = *pl;

  *pl = l->next;
  l->next = empty_elt_loc_lists;
  empty_elt_loc_lists = l;
}

/* Likewise for cselib_vals.  This also frees the addr_list associated with
   V.  */

static void
unchain_one_value (v)
     cselib_val *v;
{
  while (v->addr_list)
    unchain_one_elt_list (&v->addr_list);

  v->u.next_free = empty_vals;
  empty_vals = v;
}

/* Remove all entries from the hash table.  Also used during
   initialization.  If CLEAR_ALL isn't set, then only clear the entries
   which are known to have been used.  */

static void
clear_table (clear_all)
     int clear_all;
{
  unsigned int i;

  if (clear_all)
    for (i = 0; i < cselib_nregs; i++)
      REG_VALUES (i) = 0;
  else
    for (i = 0; i < VARRAY_ACTIVE_SIZE (used_regs); i++)
      REG_VALUES (VARRAY_UINT (used_regs, i)) = 0;

  max_value_regs = 0;

  VARRAY_POP_ALL (used_regs);

  htab_empty (hash_table);
  obstack_free (&cselib_obstack, cselib_startobj);

  empty_vals = 0;
  empty_elt_lists = 0;
  empty_elt_loc_lists = 0;
  n_useless_values = 0;

  next_unknown_value = 0;
}

/* The equality test for our hash table.  The first argument ENTRY is a table
   element (i.e. a cselib_val), while the second arg X is an rtx.  We know
   that all callers of htab_find_slot_with_hash will wrap CONST_INTs into a
   CONST of an appropriate mode.  */

static int
entry_and_rtx_equal_p (entry, x_arg)
     const void *entry, *x_arg;
{
  struct elt_loc_list *l;
  const cselib_val *v = (const cselib_val *) entry;
  rtx x = (rtx) x_arg;
  enum machine_mode mode = GET_MODE (x);

  if (GET_CODE (x) == CONST_INT
      || (mode == VOIDmode && GET_CODE (x) == CONST_DOUBLE))
    abort ();
  if (mode != GET_MODE (v->u.val_rtx))
    return 0;

  /* Unwrap X if necessary.  */
  if (GET_CODE (x) == CONST
      && (GET_CODE (XEXP (x, 0)) == CONST_INT
	  || GET_CODE (XEXP (x, 0)) == CONST_DOUBLE))
    x = XEXP (x, 0);
  
  /* We don't guarantee that distinct rtx's have different hash values,
     so we need to do a comparison.  */
  for (l = v->locs; l; l = l->next)
    if (rtx_equal_for_cselib_p (l->loc, x))
      return 1;

  return 0;
}

/* The hash function for our hash table.  The value is always computed with
   hash_rtx when adding an element; this function just extracts the hash
   value from a cselib_val structure.  */

static unsigned int
get_value_hash (entry)
     const void *entry;
{
  const cselib_val *v = (const cselib_val *) entry;
  return v->value;
}

/* Return true if X contains a VALUE rtx.  If ONLY_USELESS is set, we
   only return true for values which point to a cselib_val whose value
   element has been set to zero, which implies the cselib_val will be
   removed.  */

int
references_value_p (x, only_useless)
     rtx x;
     int only_useless;
{
  enum rtx_code code = GET_CODE (x);
  const char *fmt = GET_RTX_FORMAT (code);
  int i, j;

  if (GET_CODE (x) == VALUE
      && (! only_useless || CSELIB_VAL_PTR (x)->locs == 0))
    return 1;

  for (i = GET_RTX_LENGTH (code) - 1; i >= 0; i--)
    {
      if (fmt[i] == 'e' && references_value_p (XEXP (x, i), only_useless))
	return 1;
      else if (fmt[i] == 'E')
	for (j = 0; j < XVECLEN (x, i); j++)
	  if (references_value_p (XVECEXP (x, i, j), only_useless))
	    return 1;
    }

  return 0;
}

/* For all locations found in X, delete locations that reference useless
   values (i.e. values without any location).  Called through
   htab_traverse.  */

static int
discard_useless_locs (x, info)
     void **x;
     void *info ATTRIBUTE_UNUSED;
{
  cselib_val *v = (cselib_val *)*x;
  struct elt_loc_list **p = &v->locs;
  int had_locs = v->locs != 0;

  while (*p)
    {
      if (references_value_p ((*p)->loc, 1))
	unchain_one_elt_loc_list (p);
      else
	p = &(*p)->next;
    }

  if (had_locs && v->locs == 0)
    {
      n_useless_values++;
      values_became_useless = 1;
    }
  return 1;
}

/* If X is a value with no locations, remove it from the hashtable.  */

static int
discard_useless_values (x, info)
     void **x;
     void *info ATTRIBUTE_UNUSED;
{
  cselib_val *v = (cselib_val *)*x;

  if (v->locs == 0)
    {
      htab_clear_slot (hash_table, x);
      unchain_one_value (v);
      n_useless_values--;
    }

  return 1;
}

/* Clean out useless values (i.e. those which no longer have locations
   associated with them) from the hash table.  */

static void
remove_useless_values ()
{
  /* First pass: eliminate locations that reference the value.  That in
     turn can make more values useless.  */
  do
    {
      values_became_useless = 0;
      htab_traverse (hash_table, discard_useless_locs, 0);
    }
  while (values_became_useless);

  /* Second pass: actually remove the values.  */
  htab_traverse (hash_table, discard_useless_values, 0);

  if (n_useless_values != 0)
    abort ();
}

/* Return nonzero if we can prove that X and Y contain the same value, taking
   our gathered information into account.  */

int
rtx_equal_for_cselib_p (x, y)
     rtx x, y;
{
  enum rtx_code code;
  const char *fmt;
  int i;
  
  if (GET_CODE (x) == REG || GET_CODE (x) == MEM)
    {
      cselib_val *e = cselib_lookup (x, GET_MODE (x), 0);

      if (e)
	x = e->u.val_rtx;
    }

  if (GET_CODE (y) == REG || GET_CODE (y) == MEM)
    {
      cselib_val *e = cselib_lookup (y, GET_MODE (y), 0);

      if (e)
	y = e->u.val_rtx;
    }

  if (x == y)
    return 1;

  if (GET_CODE (x) == VALUE && GET_CODE (y) == VALUE)
    return CSELIB_VAL_PTR (x) == CSELIB_VAL_PTR (y);

  if (GET_CODE (x) == VALUE)
    {
      cselib_val *e = CSELIB_VAL_PTR (x);
      struct elt_loc_list *l;

      for (l = e->locs; l; l = l->next)
	{
	  rtx t = l->loc;

	  /* Avoid infinite recursion.  */
	  if (GET_CODE (t) == REG || GET_CODE (t) == MEM)
	    continue;
	  else if (rtx_equal_for_cselib_p (t, y))
	    return 1;
	}
      
      return 0;
    }

  if (GET_CODE (y) == VALUE)
    {
      cselib_val *e = CSELIB_VAL_PTR (y);
      struct elt_loc_list *l;

      for (l = e->locs; l; l = l->next)
	{
	  rtx t = l->loc;

	  if (GET_CODE (t) == REG || GET_CODE (t) == MEM)
	    continue;
	  else if (rtx_equal_for_cselib_p (x, t))
	    return 1;
	}
      
      return 0;
    }

  if (GET_CODE (x) != GET_CODE (y) || GET_MODE (x) != GET_MODE (y))
    return 0;

  /* This won't be handled correctly by the code below.  */
  if (GET_CODE (x) == LABEL_REF)
    return XEXP (x, 0) == XEXP (y, 0);
  
  code = GET_CODE (x);
  fmt = GET_RTX_FORMAT (code);

  for (i = GET_RTX_LENGTH (code) - 1; i >= 0; i--)
    {
      int j;

      switch (fmt[i])
	{
	case 'w':
	  if (XWINT (x, i) != XWINT (y, i))
	    return 0;
	  break;

	case 'n':
	case 'i':
	  if (XINT (x, i) != XINT (y, i))
	    return 0;
	  break;

	case 'V':
	case 'E':
	  /* Two vectors must have the same length.  */
	  if (XVECLEN (x, i) != XVECLEN (y, i))
	    return 0;

	  /* And the corresponding elements must match.  */
	  for (j = 0; j < XVECLEN (x, i); j++)
	    if (! rtx_equal_for_cselib_p (XVECEXP (x, i, j),
					  XVECEXP (y, i, j)))
	      return 0;
	  break;

	case 'e':
	  if (! rtx_equal_for_cselib_p (XEXP (x, i), XEXP (y, i)))
	    return 0;
	  break;

	case 'S':
	case 's':
	  if (strcmp (XSTR (x, i), XSTR (y, i)))
	    return 0;
	  break;

	case 'u':
	  /* These are just backpointers, so they don't matter.  */
	  break;

	case '0':
	case 't':
	  break;

	  /* It is believed that rtx's at this level will never
	     contain anything but integers and other rtx's,
	     except for within LABEL_REFs and SYMBOL_REFs.  */
	default:
	  abort ();
	}
    }
  return 1;
}

/* We need to pass down the mode of constants through the hash table
   functions.  For that purpose, wrap them in a CONST of the appropriate
   mode.  */
static rtx
wrap_constant (mode, x)
     enum machine_mode mode;
     rtx x;
{
  if (GET_CODE (x) != CONST_INT
      && (GET_CODE (x) != CONST_DOUBLE || GET_MODE (x) != VOIDmode))
    return x;
  if (mode == VOIDmode)
    abort ();
  return gen_rtx_CONST (mode, x);
}

/* Hash an rtx.  Return 0 if we couldn't hash the rtx.
   For registers and memory locations, we look up their cselib_val structure
   and return its VALUE element.
   Possible reasons for return 0 are: the object is volatile, or we couldn't
   find a register or memory location in the table and CREATE is zero.  If
   CREATE is nonzero, table elts are created for regs and mem.
   MODE is used in hashing for CONST_INTs only;
   otherwise the mode of X is used.  */

static unsigned int
hash_rtx (x, mode, create)
     rtx x;
     enum machine_mode mode;
     int create;
{
  cselib_val *e;
  int i, j;
  enum rtx_code code;
  const char *fmt;
  unsigned int hash = 0;

  code = GET_CODE (x);
  hash += (unsigned) code + (unsigned) GET_MODE (x);

  switch (code)
    {
    case MEM:
    case REG:
      e = cselib_lookup (x, GET_MODE (x), create);
      if (! e)
	return 0;

      return e->value;

    case CONST_INT:
      hash += ((unsigned) CONST_INT << 7) + (unsigned) mode + INTVAL (x);
      return hash ? hash : (unsigned int) CONST_INT;

    case CONST_DOUBLE:
      /* This is like the general case, except that it only counts
	 the integers representing the constant.  */
      hash += (unsigned) code + (unsigned) GET_MODE (x);
      if (GET_MODE (x) != VOIDmode)
	for (i = 2; i < GET_RTX_LENGTH (CONST_DOUBLE); i++)
	  hash += XWINT (x, i);
      else
	hash += ((unsigned) CONST_DOUBLE_LOW (x)
		 + (unsigned) CONST_DOUBLE_HIGH (x));
      return hash ? hash : (unsigned int) CONST_DOUBLE;

    case CONST_VECTOR:
      {
	int units;
	rtx elt;

	units = CONST_VECTOR_NUNITS (x);

	for (i = 0; i < units; ++i)
	  {
	    elt = CONST_VECTOR_ELT (x, i);
	    hash += hash_rtx (elt, GET_MODE (elt), 0);
	  }

	return hash;
      }

      /* Assume there is only one rtx object for any given label.  */
    case LABEL_REF:
      hash
	+= ((unsigned) LABEL_REF << 7) + (unsigned long) XEXP (x, 0);
      return hash ? hash : (unsigned int) LABEL_REF;

    case SYMBOL_REF:
      hash
	+= ((unsigned) SYMBOL_REF << 7) + (unsigned long) XSTR (x, 0);
      return hash ? hash : (unsigned int) SYMBOL_REF;

    case PRE_DEC:
    case PRE_INC:
    case POST_DEC:
    case POST_INC:
    case POST_MODIFY:
    case PRE_MODIFY:
    case PC:
    case CC0:
    case CALL:
    case UNSPEC_VOLATILE:
      return 0;

    case ASM_OPERANDS:
      if (MEM_VOLATILE_P (x))
	return 0;

      break;
      
    default:
      break;
    }

  i = GET_RTX_LENGTH (code) - 1;
  fmt = GET_RTX_FORMAT (code);
  for (; i >= 0; i--)
    {
      if (fmt[i] == 'e')
	{
	  rtx tem = XEXP (x, i);
	  unsigned int tem_hash = hash_rtx (tem, 0, create);

	  if (tem_hash == 0)
	    return 0;

	  hash += tem_hash;
	}
      else if (fmt[i] == 'E')
	for (j = 0; j < XVECLEN (x, i); j++)
	  {
	    unsigned int tem_hash = hash_rtx (XVECEXP (x, i, j), 0, create);

	    if (tem_hash == 0)
	      return 0;

	    hash += tem_hash;
	  }
      else if (fmt[i] == 's')
	{
	  const unsigned char *p = (const unsigned char *) XSTR (x, i);

	  if (p)
	    while (*p)
	      hash += *p++;
	}
      else if (fmt[i] == 'i')
	hash += XINT (x, i);
      else if (fmt[i] == '0' || fmt[i] == 't')
	/* unused */;
      else
	abort ();
    }

  return hash ? hash : 1 + (unsigned int) GET_CODE (x);
}

/* Create a new value structure for VALUE and initialize it.  The mode of the
   value is MODE.  */

static cselib_val *
new_cselib_val (value, mode)
     unsigned int value;
     enum machine_mode mode;
{
  cselib_val *e = empty_vals;

  if (e)
    empty_vals = e->u.next_free;
  else
    e = (cselib_val *) obstack_alloc (&cselib_obstack, sizeof (cselib_val));

  if (value == 0)
    abort ();

  e->value = value;
  e->u.val_rtx = gen_rtx_VALUE (mode);
  CSELIB_VAL_PTR (e->u.val_rtx) = e;
  e->addr_list = 0;
  e->locs = 0;
  return e;
}

/* ADDR_ELT is a value that is used as address.  MEM_ELT is the value that
   contains the data at this address.  X is a MEM that represents the
   value.  Update the two value structures to represent this situation.  */

static void
add_mem_for_addr (addr_elt, mem_elt, x)
     cselib_val *addr_elt, *mem_elt;
     rtx x;
{
  struct elt_loc_list *l;

  /* Avoid duplicates.  */
  for (l = mem_elt->locs; l; l = l->next)
    if (GET_CODE (l->loc) == MEM
	&& CSELIB_VAL_PTR (XEXP (l->loc, 0)) == addr_elt)
      return;

  addr_elt->addr_list = new_elt_list (addr_elt->addr_list, mem_elt);
  mem_elt->locs
    = new_elt_loc_list (mem_elt->locs,
			replace_equiv_address_nv (x, addr_elt->u.val_rtx));
}

/* Subroutine of cselib_lookup.  Return a value for X, which is a MEM rtx.
   If CREATE, make a new one if we haven't seen it before.  */

static cselib_val *
cselib_lookup_mem (x, create)
     rtx x;
     int create;
{
  enum machine_mode mode = GET_MODE (x);
  void **slot;
  cselib_val *addr;
  cselib_val *mem_elt;
  struct elt_list *l;

  if (MEM_VOLATILE_P (x) || mode == BLKmode
      || (FLOAT_MODE_P (mode) && flag_float_store))
    return 0;

  /* Look up the value for the address.  */
  addr = cselib_lookup (XEXP (x, 0), mode, create);
  if (! addr)
    return 0;

  /* Find a value that describes a value of our mode at that address.  */
  for (l = addr->add