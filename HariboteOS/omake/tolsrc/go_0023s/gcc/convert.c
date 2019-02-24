/* Utility routines for data type conversion for GNU C.
   Copyright (C) 1987, 1988, 1991, 1992, 1993, 1994, 1995, 1997,
   1998 Free Software Foundation, Inc.

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


/* These routines are somewhat language-independent utility function
   intended to be called by the language-specific convert () functions.  */

#include "config.h"
#include "system.h"
#include "tree.h"
#include "flags.h"
#include "convert.h"
#include "toplev.h"

/* Convert EXPR to some pointer or reference type TYPE.

   EXPR must be pointer, reference, integer, enumeral, or literal zero;
   in other cases error is called.  */

tree
convert_to_pointer (type, expr)
     tree type, expr;
{
  if (integer_zerop (expr))
    {
      expr = build_int_2 (0, 0);
      TREE_TYPE (expr) = type;
      return expr;
    }

  switch (TREE_CODE (TREE_TYPE (expr)))
    {
    case POINTER_TYPE:
    case REFERENCE_TYPE:
      return build1 (NOP_EXPR, type, expr);

    case INTEGER_TYPE:
    case ENUMERAL_TYPE:
    case BOOLEAN_TYPE:
    case CHAR_TYPE:
      if (TYPE_PRECISION (TREE_TYPE (expr)) == POINTER_SIZE)
	return build1 (CONVERT_EXPR, type, expr);

      return
	convert_to_pointer (type,
			    convert (type_for_size (POINTER_SIZE, 0), expr));

    default:
      error ("cannot convert to a pointer type");
      return convert_to_pointer (type, integer_zero_node);
    }
}

/* Convert EXPR to some floating-point type TYPE.

   EXPR must be float, integer, or enumeral;
   in other cases error is called.  */

tree
convert_to_real (type, expr)
     tree type, expr;
{
  switch (TREE_CODE (TREE_TYPE (expr)))
    {
    case REAL_TYPE:
      return build1 (flag_float_store ? CONVERT_EXPR : NOP_EXPR,
		     type, expr);

    case INTEGER_TYPE:
    case ENUMERAL_TYPE:
    case BOOLEAN_TYPE:
    case CHAR_TYPE:
      return build1 (FLOAT_EXPR, type, expr);

    case COMPLEX_TYPE:
      return convert (type,
		      fold (build1 (REALPART_EXPR,
				    TREE_TYPE (TREE_TYPE (expr)), expr)));

    case POINTER_TYPE:
    case REFERENCE_TYPE:
      error ("pointer value used where a floating point value was expected");
      return convert_to_real (type, integer_zero_node);

    default:
      error ("aggregate value used where a float was expected");
      return convert_to_real (type, integer_zero_node);
    }
}

/* Convert EXPR to some integer (or enum) type TYPE.

   EXPR must be pointer, integer, discrete (enum, char, or bool), float, or
   vector; in other cases error is called.

   The result of this is always supposed to be a newly created tree node
   not in use in any existing structure.  */

tree
convert_to_integer (type, expr)
     tree type, expr;
{
  enum tree_code ex_form = TREE_CODE (expr);
  tree intype = TREE_TYPE (expr);
  unsigned int inprec = TYPE_PRECISION (intype);
  unsigned int outprec = TYPE_PRECISION (type);

  /* An INTEGER_TYPE cannot be incomplete, but an ENUMERAL_TYPE can
     be.  Consider `enum E = { a, b = (enum E) 3 };'.  */
  if (!COMPLETE_TYPE_P (type))
    {
      error ("conversion to incomplete type");
      return error_mark_node;
    }

  switch (TREE_CODE (intype))
    {
    case POINTER_TYPE:
    case REFERENCE_TYPE:
      if (integer_zerop (expr))
	expr = integer_zero_node;
      else
	expr = fold (build1 (CONVERT_EXPR,
			     type_f