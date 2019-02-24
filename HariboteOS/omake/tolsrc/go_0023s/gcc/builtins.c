/* Expand builtin functions.
   Copyright (C) 1988, 1992, 1993, 1994, 1995, 1996, 1997, 1998,
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

/* !kawai! */
#include "config.h"
#include "system.h"
#include "machmode.h"
#include "rtl.h"
#include "tree.h"
#include "../include/obstack.h"
#include "flags.h"
#include "regs.h"
#include "hard-reg-set.h"
#include "except.h"
#include "function.h"
#include "insn-config.h"
#include "expr.h"
#include "optabs.h"
#include "libfuncs.h"
#include "recog.h"
#include "output.h"
#include "typeclass.h"
#include "toplev.h"
#include "predict.h"
#include "tm_p.h"
#include "target.h"
/* end of !kawai! */

#define CALLED_AS_BUILT_IN(NODE) ¥
   (!strncmp (IDENTIFIER_POINTER (DECL_NAME (NODE)), "__builtin_", 10))

/* Register mappings for target machines without register windows.  */
#ifndef INCOMING_REGNO
#define INCOMING_REGNO(OUT) (OUT)
#endif
#ifndef OUTGOING_REGNO
#define OUTGOING_REGNO(IN) (IN)
#endif

#ifndef PAD_VARARGS_DOWN
#define PAD_VARARGS_DOWN BYTES_BIG_ENDIAN
#endif

/* Define the names of the builtin function types and codes.  */
const char *const built_in_class_names[4]
  = {"NOT_BUILT_IN", "BUILT_IN_FRONTEND", "BUILT_IN_MD", "BUILT_IN_NORMAL"};

#define DEF_BUILTIN(X, N, C, T, LT, B, F, NA) STRINGX(X),
const char *const built_in_names[(int) END_BUILTINS] =
{
#include "builtins.def"
};
#undef DEF_BUILTIN

/* Setup an array of _DECL trees, make sure each element is
   initialized to NULL_TREE.  */
tree built_in_decls[(int) END_BUILTINS];

tree (*lang_type_promotes_to) PARAMS ((tree));

static int get_pointer_alignment	PARAMS ((tree, unsigned int));
static tree c_strlen			PARAMS ((tree));
static const char *c_getstr		PARAMS ((tree));
static rtx c_readstr			PARAMS ((const char *,
						 enum machine_mode));
static int target_char_cast		PARAMS ((tree, char *));
static rtx get_memory_rtx		PARAMS ((tree));
static int apply_args_size		PARAMS ((void));
static int apply_result_size		PARAMS ((void));
#if defined (HAVE_untyped_call) || defined (HAVE_untyped_return)
static rtx result_vector		PARAMS ((int, rtx));
#endif
static rtx expand_builtin_setjmp	PARAMS ((tree, rtx));
static void expand_builtin_prefetch	PARAMS ((tree));
static rtx expand_builtin_apply_args	PARAMS ((void));
static rtx expand_builtin_apply_args_1	PARAMS ((void));
static rtx expand_builtin_apply		PARAMS ((rtx, rtx, rtx));
static void expand_builtin_return	PARAMS ((rtx));
static enum type_class type_to_class	PARAMS ((tree));
static rtx expand_builtin_classify_type	PARAMS ((tree));
static rtx expand_builtin_mathfn	PARAMS ((tree, rtx, rtx));
static rtx expand_builtin_constant_p	PARAMS ((tree));
static rtx expand_builtin_args_info	PARAMS ((tree));
static rtx expand_builtin_next_arg	PARAMS ((tree));
static rtx expand_builtin_va_start	PARAMS ((int, tree));
static rtx expand_builtin_va_end	PARAMS ((tree));
static rtx expand_builtin_va_copy	PARAMS ((tree));
static rtx expand_builtin_memcmp	PARAMS ((tree, tree, rtx,
                                                 enum machine_mode));
static rtx expand_builtin_strcmp	PARAMS ((tree, rtx,
						 enum machine_mode));
static rtx expand_builtin_strncmp	PARAMS ((tree, rtx,
						 enum machine_mode));
static rtx builtin_memcpy_read_str	PARAMS ((PTR, HOST_WIDE_INT,
						 enum machine_mode));
static rtx expand_builtin_strcat	PARAMS ((tree, rtx,
						 enum machine_mode));
static rtx expand_builtin_strncat	PARAMS ((tree, rtx,
						 enum machine_mode));
static rtx expand_builtin_strspn	PARAMS ((tree, rtx,
						 enum machine_mode));
static rtx expand_builtin_strcspn	PARAMS ((tree, rtx,
						 enum machine_mode));
static rtx expand_builtin_memcpy	PARAMS ((tree, rtx,
                                                 enum machine_mode));
static rtx expand_builtin_strcpy	PARAMS ((tree, rtx,
                                                 enum machine_mode));
static rtx builtin_strncpy_read_str	PARAMS ((PTR, HOST_WIDE_INT,
						 enum machine_mode));
static rtx expand_builtin_strncpy	PARAMS ((tree, rtx,
						 enum machine_mode));
static rtx builtin_memset_read_str	PARAMS ((PTR, HOST_WIDE_INT,
						 enum machine_mode));
static rtx expand_builtin_memset	PARAMS ((tree, rtx,
                                                 enum machine_mode));
static rtx expand_builtin_bzero		PARAMS ((tree));
static rtx expand_builtin_strlen	PARAMS ((tree, rtx));
static rtx expand_builtin_strstr	PARAMS ((tree, rtx,
						 enum machine_mode));
static rtx expand_builtin_strpbrk	PARAMS ((tree, rtx,
						 enum machine_mode));
static rtx expand_builtin_strchr	PARAMS ((tree, rtx,
						 enum machine_mode));
static rtx expand_builtin_strrchr	PARAMS ((tree, rtx,
						 enum machine_mode));
static rtx expand_builtin_alloca	PARAMS ((tree, rtx));
static rtx expand_builtin_ffs		PARAMS ((tree, rtx, rtx));
static rtx expand_builtin_frame_address	PARAMS ((tree));
static rtx expand_builtin_fputs		PARAMS ((tree, int, int));
static tree stabilize_va_list		PARAMS ((tree, int));
static rtx expand_builtin_expect	PARAMS ((tree, rtx));
static tree fold_builtin_constant_p	PARAMS ((tree));
static tree fold_builtin_classify_type	PARAMS ((tree));
static tree build_function_call_expr	PARAMS ((tree, tree));
static int validate_arglist		PARAMS ((tree, ...));

/* Return the alignment in bits of EXP, a pointer valued expression.
   But don't return more than MAX_ALIGN no matter what.
   The alignment returned is, by default, the alignment of the thing that
   EXP points to.  If it is not a POINTER_TYPE, 0 is returned.

   Otherwise, look at the expression to see if we can do better, i.e., if the
   expression is actually pointing at an object whose alignment is tighter.  */

static int
get_pointer_alignment (exp, max_align)
     tree exp;
     unsigned int max_align;
{
  unsigned int align, inner;

  if (TREE_CODE (TREE_TYPE (exp)) != POINTER_TYPE)
    return 0;

  align = TYPE_ALIGN (TREE_TYPE (TREE_TYPE (exp)));
  align = MIN (align, max_align);

  while (1)
    {
      switch (TREE_CODE (exp))
	{
	case NOP_EXPR:
	case CONVERT_EXPR:
	case NON_LVALUE_EXPR:
	  exp = TREE_OPERAND (exp, 0);
	  if (TREE_CODE (TREE_TYPE (exp)) != POINTER_TYPE)
	    return align;

	  inner = TYPE_ALIGN (TREE_TYPE (TREE_TYPE (exp)));
	  align = MIN (inner, max_align);
	  break;

	case PLUS_EXPR:
	  /* If sum of pointer + int, restrict our maximum alignment to that
	     imposed by the integer.  If not, we can't do any better than
	     ALIGN.  */
	  if (! host_integerp (TREE_OPERAND (exp, 1), 1))
	    return align;

	  while (((tree_low_cst (TREE_OPERAND (exp, 1), 1))
		  & (max_align / BITS_PER_UNIT - 1))
		 != 0)
	    max_align >>= 1;

	  exp = TREE_OPERAND (exp, 0);
	  break;

	case ADDR_EXPR:
	  /* See what we are pointing at and look at its alignment.  */
	  exp = TREE_OPERAND (exp, 0);
	  if (TREE_CODE (exp) == FUNCTION_DECL)
	    align = FUNCTION_BOUNDARY;
	  else if (DECL_P (exp))
	    align = DECL_ALIGN (exp);
#ifdef CONSTANT_ALIGNMENT
	  else if (TREE_CODE_CLASS (TREE_CODE (exp)) == 'c')
	    align = CONSTANT_ALIGNMENT (exp, align);
#endif
	  return MIN (align, max_align);

	default:
	  return align;
	}
    }
}

/* Compute the length of a C string.  TREE_STRING_LENGTH is not the right
   way, because it could contain a zero byte in the middle.
   TREE_STRING_LENGTH is the size of the character array, not the string.

   The value returned is of type `ssizetype'.

   Unfortunately, string_constant can't access the values of const char
   arrays with initializers, so neither can we do so here.  */

static tree
c_strlen (src)
     tree src;
{
  tree offset_node;
  HOST_WIDE_INT offset;
  int max;
  const char *ptr;

  src = string_constant (src, &offset_node);
  if (src == 0)
    return 0;

  max = TREE_STRING_LENGTH (src) - 1;
  ptr = TREE_STRING_POINTER (src);

  if (offset_node && TREE_CODE (offset_node) != INTEGER_CST)
    {
      /* If the string has an internal zero byte (e.g., "foo¥0bar"), we can't
	 compute the offset to the following null if we don't know where to
	 start searching for it.  */
      int i;

      for (i = 0; i < max; i++)
	if (ptr[i] == 0)
	  return 0;

      /* We don't know the starting offset, but we do know that the string
	 has no internal zero bytes.  We can assume that the offset falls
	 within the bounds of the string; otherwise, the programmer deserves
	 what he gets.  Subtract the offset from the length of the string,
	 and return that.  This would perhaps not be valid if we were dealing
	 with named arrays in addition to literal string constants.  */

      return size_diffop (size_int (max), offset_node);
    }

  /* We have a known offset into the string.  Start searching there for
     a null character if we can represent it as a single HOST_WIDE_INT.  */
  if (offset_node == 0)
    offset = 0;
  else if (! host_integerp (offset_node, 0))
    offset = -1;
  else
    offset = tree_low_cst (offset_node, 0);

  /* If the offset is known to be out of bounds, warn, and call strlen at
     runtime.  */
  if (offset < 0 || offset > max)
    {
      warning ("offset outside bounds of constant string");
      return 0;
    }

  /* Use strlen to search for the first zero byte.  Since any strings
     constructed with build_string will have nulls appended, we win even
     if we get handed something like (char[4])"abcd".

     Since OFFSET is our starting index into the string, no further
     calculation is needed.  */
  return ssize_int (strlen (ptr + offset));
}

/* Return a char pointer for a C string if it is a string constant
   or sum of string constant and integer constant.  */

static const char *
c_getstr (src)
     tree src;
{
  tree offset_node;

  src = string_constant (src, &offset_node);
  if (src == 0)
    return 0;

  if (offset_node == 0)
    return TREE_STRING_POINTER (src);
  else if (!host_integerp (offset_node, 1)
	   || compare_tree_int (offset_node, TREE_STRING_LENGTH (src) - 1) > 0)
    return 0;

  return TREE_STRING_POINTER (src) + tree_low_cst (offset_node, 1);
}

/* Return a CONST_INT or CONST_DOUBLE corresponding to target reading
   GET_MODE_BITSIZE (MODE) bits from string constant STR.  */

static rtx
c_readstr (str, mode)
     const char *str;
     enum machine_mode mode;
{
  HOST_WIDE_INT c[2];
  HOST_WIDE_INT ch;
  unsigned int i, j;

  if (GET_MODE_CLASS (mode) != MODE_INT)
    abort ();
  c[0] = 0;
  c[1] = 0;
  ch = 1;
  for (i = 0; i < GET_MODE_SIZE (mode); i++)
    {
      j = i;
      if (WORDS_BIG_ENDIAN)
	j = GET_MODE_SIZE (mode) - i - 1;
      if (BYTES_BIG_ENDIAN != WORDS_BIG_ENDIAN
	  && GET_MODE_SIZE (mode) > UNITS_PER_WORD)
	j = j + UNITS_PER_WORD - 2 * (j % UNITS_PER_WORD) - 1;
      j *= BITS_PER_UNIT;
      if (j > 2 * HOST_BITS_PER_WIDE_INT)
	abort ();
      if (ch)
	ch = (unsigned char) str[i];
      c[j / HOST_BITS_PER_WIDE_INT] |= ch << (j % HOST_BITS_PER_WIDE_INT);
    }
  return immed_double_const (c[0], c[1], mode);
}

/* Cast a target constant CST to target CHAR and if that value fits into
   host char type, return zero and put that value into variable pointed by
   P.  */

static int
target_char_cast (cst, p)
     tree cst;
     char *p;
{
  unsigned HOST_WIDE_INT val, hostval;

  if (!host_integerp (cst, 1)
      || CHAR_TYPE_SIZE > HOST_BITS_PER_WIDE_INT)
    return