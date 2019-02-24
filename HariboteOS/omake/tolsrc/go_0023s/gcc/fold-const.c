/* Fold a constant sub-tree into a single node for C-compiler
   Copyright (C) 1987, 1988, 1992, 1993, 1994, 1995, 1996, 1997, 1998,
   1999, 2000, 2001, 2002 Free Software Foundation, Inc.

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

/*@@ This file should be rewritten to use an arbitrary precision
  @@ representation for "struct tree_int_cst" and "struct tree_real_cst".
  @@ Perhaps the routines could also be used for bc/dc, and made a lib.
  @@ The routines that translate from the ap rep should
  @@ warn if precision et. al. is lost.
  @@ This would also make life easier when this technology is used
  @@ for cross-compilers.  */

/* The entry points in this file are fold, size_int_wide, size_binop
   and force_fit_type.

   fold takes a tree as argument and returns a simplified tree.

   size_binop takes a tree code for an arithmetic operation
   and two operands that are trees, and produces a tree for the
   result, assuming the type comes from `sizetype'.

   size_int takes an integer value, and creates a tree constant
   with type from `sizetype'.

   force_fit_type takes a constant and prior overflow indicator, and
   forces the value to fit the type.  It returns an overflow indicator.  */

/* !kawai! */
#include "config.h"
#include "system.h"
#include "flags.h"
#include "tree.h"
#include "rtl.h"
#include "expr.h"
#include "tm_p.h"
#include "toplev.h"
#include "ggc.h"
#include "../include/hashtab.h"
/* end of !kawai! */

static void encode		PARAMS ((HOST_WIDE_INT *,
					 unsigned HOST_WIDE_INT,
					 HOST_WIDE_INT));
static void decode		PARAMS ((HOST_WIDE_INT *,
					 unsigned HOST_WIDE_INT *,
					 HOST_WIDE_INT *));
#ifndef REAL_ARITHMETIC
static void exact_real_inverse_1 PARAMS ((PTR));
#endif
static tree negate_expr		PARAMS ((tree));
static tree split_tree		PARAMS ((tree, enum tree_code, tree *, tree *,
					 tree *, int));
static tree associate_trees	PARAMS ((tree, tree, enum tree_code, tree));
static tree int_const_binop	PARAMS ((enum tree_code, tree, tree, int));
static void const_binop_1	PARAMS ((PTR));
static tree const_binop		PARAMS ((enum tree_code, tree, tree, int));
static hashval_t size_htab_hash	PARAMS ((const void *));
static int size_htab_eq		PARAMS ((const void *, const void *));
static void fold_convert_1	PARAMS ((PTR));
static tree fold_convert	PARAMS ((tree, tree));
static enum tree_code invert_tree_comparison PARAMS ((enum tree_code));
static enum tree_code swap_tree_comparison PARAMS ((enum tree_code));
static int truth_value_p	PARAMS ((enum tree_code));
static int operand_equal_for_comparison_p PARAMS ((tree, tree, tree));
static int twoval_comparison_p	PARAMS ((tree, tree *, tree *, int *));
static tree eval_subst		PARAMS ((tree, tree, tree, tree, tree));
static tree omit_one_operand	PARAMS ((tree, tree, tree));
static tree pedantic_omit_one_operand PARAMS ((tree, tree, tree));
static tree distribute_bit_expr PARAMS ((enum tree_code, tree, tree, tree));
static tree make_bit_field_ref	PARAMS ((tree, tree, int, int, int));
static tree optimize_bit_field_compare PARAMS ((enum tree_code, tree,
						tree, tree));
static tree decode_field_reference PARAMS ((tree, HOST_WIDE_INT *,
					    HOST_WIDE_INT *,
					    enum machine_mode *, int *,
					    int *, tree *, tree *));
static int all_ones_mask_p	PARAMS ((tree, int));
static int simple_operand_p	PARAMS ((tree));
static tree range_binop		PARAMS ((enum tree_code, tree, tree, int,
					 tree, int));
static tree make_range		PARAMS ((tree, int *, tree *, tree *));
static tree build_range_check	PARAMS ((tree, tree, int, tree, tree));
static int merge_ranges		PARAMS ((int *, tree *, tree *, int, tree, tree,
				       int, tree, tree));
static tree fold_range_test	PARAMS ((tree));
static tree unextend		PARAMS ((tree, int, int, tree));
static tree fold_truthop	PARAMS ((enum tree_code, tree, tree, tree));
static tree optimize_minmax_comparison PARAMS ((tree));
static tree extract_muldiv	PARAMS ((tree, tree, enum tree_code, tree));
static tree strip_compound_expr PARAMS ((tree, tree));
static int multiple_of_p	PARAMS ((tree, tree, tree));
static tree constant_boolean_node PARAMS ((int, tree));
static int count_cond		PARAMS ((tree, int));
static tree fold_binary_op_with_conditional_arg 
  PARAMS ((enum tree_code, tree, tree, tree, int));
							 
#if defined(HOST_EBCDIC)
/* bit 8 is significant in EBCDIC */
#define CHARMASK 0xff
#else
#define CHARMASK 0x7f
#endif

/* We know that A1 + B1 = SUM1, using 2's complement arithmetic and ignoring
   overflow.  Suppose A, B and SUM have the same respective signs as A1, B1,
   and SUM1.  Then this yields nonzero if overflow occurred during the
   addition.

   Overflow occurs if A and B have the same sign, but A and SUM differ in
   sign.  Use `^' to test whether signs differ, and `< 0' to isolate the
   sign.  */
#define OVERFLOW_SUM_SIGN(a, b, sum) ((‾((a) ^ (b)) & ((a) ^ (sum))) < 0)

/* To do constant folding on INTEGER_CST nodes requires two-word arithmetic.
   We do that by representing the two-word integer in 4 words, with only
   HOST_BITS_PER_WIDE_INT / 2 bits stored in each word, as a positive
   number.  The value of the word is LOWPART + HIGHPART * BASE.  */

#define LOWPART(x) ¥
  ((x) & (((unsigned HOST_WIDE_INT) 1 << (HOST_BITS_PER_WIDE_INT / 2)) - 1))
#define HIGHPART(x) ¥
  ((unsigned HOST_WIDE_INT) (x) >> HOST_BITS_PER_WIDE_INT / 2)
#define BASE ((unsigned HOST_WIDE_INT) 1 << HOST_BITS_PER_WIDE_INT / 2)

/* Unpack a two-word integer into 4 words.
   LOW and HI are the integer, as two `HOST_WIDE_INT' pieces.
   WORDS points to the array of HOST_WIDE_INTs.  */

static void
encode (words, low, hi)
     HOST_WIDE_INT *words;
     unsigned HOST_WIDE_INT low;
     HOST_WIDE_INT hi;
{
  words[0] = LOWPART (low);
  words[1] = HIGHPART (low);
  words[2] = LOWPART (hi);
  words[3] = HIGHPART (hi);
}

/* Pack an array of 4 words into a two-word integer.
   WORDS points to the array of words.
   The integer is stored into *LOW and *HI as two `HOST_WIDE_INT' pieces.  */

static void
decode (words, low, hi)
     HOST_WIDE_INT *words;
     unsigned HOST_WIDE_INT *low;
     HOST_WIDE_INT *hi;
{
  *low = words[0] + words[1] * BASE;
  *hi = words[2] + words[3] * BASE;
}

/* Make the integer constant T valid for its type by setting to 0 or 1 all
   the bits in the constant that don't belong in the type.

   Return 1 if a signed overflow occurs, 0 otherwise.  If OVERFLOW is
   nonzero, a signed overflow has already occurred in calculating T, so
   propagate it.

   Make the real constant T valid for its type by calling CHECK_FLOAT_VALUE,
   if it exists.  */

int
force_fit_type (t, overflow)
     tree t;
     int overflow;
{
  unsigned HOST_WIDE_INT low;
  HOST_WIDE_INT high;
  unsigned int prec;

  if (TREE_CODE (t) == REAL_CST)
    {
#ifdef CHECK_FLOAT_VALUE
      CHECK_FLOAT_VALUE (TYPE_MODE (TREE_TYPE (t)), TREE_REAL_CST (t),
			 overflow);
#endif
      return overflow;
    }

  else if (TREE_CODE (t) != INTEGER_CST)
    return overflow;

  low = TREE_INT_CST_LOW (t);
  high = TREE_INT_CST_HIGH (t);

  if (POINTER_TYPE_P (TREE_TYPE (t)))
    prec = POINTER_SIZE;
  else
    prec = TYPE_PRECISION (TREE_TYPE (t));

  /* First clear all bits that are beyond the type's precision.  */

  if (prec == 2 * HOST_BITS_PER_WIDE_INT)
    ;
  else if (prec > HOST_BITS_PER_WIDE_INT)
    TREE_INT_CST_HIGH (t)
      &= ‾((HOST_WIDE_INT) (-1) << (prec - HOST_BITS_PER_WIDE_INT));
  else
    {
      TREE_INT_CST_HIGH (t) = 0;
      if (prec < HOST_BITS_PER_WIDE_INT)
	TREE_INT_CST_LOW (t) &= ‾((unsigned HOST_WIDE_INT) (-1) << prec);
    }

  /* Unsigned types do not suffer sign extension or overflow unless they
     are a sizetype.  */
  if (TREE_UNSIGNED (TREE_TYPE (t))
      && ! (TREE_CODE (TREE_TYPE (t)) == INTEGER_TYPE
	    && TYPE_IS_SIZETYPE (TREE_TYPE (t))))
    return overflow;

  /* If the value's sign bit is set, extend the sign.  */
  if (prec != 2 * HOST_BITS_PER_WIDE_INT
      && (prec > HOST_BITS_PER_WIDE_INT
	  ? 0 != (TREE_INT_CST_HIGH (t)
		  & ((HOST_WIDE_INT) 1
		     << (prec - HOST_BITS_PER_WIDE_INT - 1)))
	  : 0 != (TREE_INT_CST_LOW (t)
		  & ((unsigned HOST_WIDE_INT) 1 << (prec - 1)))))
    {
      /* Value is negative:
	 set to 1 all the bits that are outside this type's precision.  */
      if (prec > HOST_BITS_PER_WIDE_INT)
	TREE_INT_CST_HIGH (t)
	  |= ((HOST_WIDE_INT) (-1) << (prec - HOST_BITS_PER_WIDE_INT));
      else
	{
	  TREE_INT_CST_HIGH (t) = -1;
	  if (prec < HOST_BITS_PER_WIDE_INT)
	    TREE_INT_CST_LOW (t) |= ((unsigned HOST_WIDE_INT) (-1) << prec);
	}
    }

  /* Return nonzero if signed overflow occurred.  */
  return
    ((overflow | (low ^ TREE_INT_CST_LOW (t)) | (high ^ TREE_INT_CST_HIGH (t)))
     != 0);
}

/* Add two doubleword integers with doubleword result.
   Each argument is given as two `HOST_WIDE_INT' pieces.
   One argument is L1 and H1; the other, L2 and H2.
   The value is stored as two `HOST_WIDE_INT' pieces in *LV and *HV.  */

int
add_double (l1, h1, l2, h2, lv, hv)
     unsigned HOST_WIDE_INT l1, l2;
     HOST_WIDE_INT h1, h2;
     unsigned HOST_WIDE_INT *lv;
     HOST_WIDE_INT *hv;
{
  unsigned HOST_WIDE_INT l;
  HOST_WIDE_INT h;

  l = l1 + l2;
  h = h1 + h2 + (l < l1);

  *lv = l;
  *hv = h;
  return OVERFLOW_SUM_SIGN (h1, h2, h);
}

/* Negate a doubleword integer with doubleword result.
   Return nonzero if the operation overflows, assuming it's signed.
   The argument is given as two `HOST_WIDE_INT' pieces in L1 and H1.
   The value is stored as two `HOST_WIDE_INT' pieces in *LV and *HV.  */

int
neg_double (l1, h1, lv, hv)
     unsigned HOST_WIDE_INT l1;
     HOST_WIDE_INT h1;
     unsigned HOST_WIDE_INT *lv;
     HOST_WIDE_INT *hv;
{
  if (l1 == 0)
    {
      *lv = 0;
      *hv = - h1;
      return (*hv & h1) < 0;
    }
  else
    {
      *lv = -l1;
      *hv = ‾h1;
      return 0;
    }
}

/* Multiply two doubleword integers with doubleword result.
   Return nonzero if the operation overflows, assuming it's signed.
   Each argument is given as two `HOST_WIDE_INT' pieces.
   One argument is L1 and H1; the other, L2 and H2.
   The value is stored as two `HOST_WIDE_INT' pieces in *LV and *HV.  */

int
mul_double (l1, h1, l2, h2, lv, hv)
     unsigned HOST_WIDE_INT l1, l2;
     HOST_WIDE_INT h1, h2;
     unsigned HOST_WIDE_INT *lv;
     HOST_WIDE_INT *hv;
{
  HOST_WIDE_INT arg1[4];
  HOST_WIDE_INT arg2[4];
  HOST_WIDE_INT prod[4 * 2];
  unsigned HOST_WIDE_INT carry;
  int i, j, k;
  unsigned HOST_WIDE_INT toplow, neglow;
  HOST_WIDE_INT tophigh, neghigh;

  encode (arg1, l1, h1);
  encode (arg2, l2, h2);

  memset ((char *) prod, 0, sizeof prod);

  for (i = 0; i < 4; i++)
    {
      carry = 0;
      for (j = 0; j < 4; j++)
	{
	  k = i + j;
	  /* This product is <= 0xFFFE0001, the sum <= 0xFFFF0000.  */
	  carry += arg1[i] * arg2[j];
	  /* Since prod[p] < 0xFFFF, this sum <= 0xFFFFFFFF.  */
	  carry += prod[k];
	  prod[k] = LOWPART (carry);
	  carry = HIGHPART (carry);
	}
      prod[i + 4] = carry;
    }

  decode (prod, lv, hv);	/* This ignores prod[4] through prod[4*2-1] */

  /* Check for overflow by calculating the top half of the answer in full;
     it should agree with the low half's sign bit.  */
  decode (prod + 4, &toplow, &tophigh);
  if (h1 < 0)
    {
      neg_double (l2, h2, &neglow, &neghigh);
      ad