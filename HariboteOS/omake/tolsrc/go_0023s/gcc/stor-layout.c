/* C-compiler utilities for types and variables storage layout
   Copyright (C) 1987, 1988, 1992, 1993, 1994, 1995, 1996, 1996, 1998,
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


#include "config.h"
#include "system.h"
#include "tree.h"
#include "rtl.h"
#include "tm_p.h"
#include "flags.h"
#include "function.h"
#include "expr.h"
#include "toplev.h"
#include "ggc.h"
#include "target.h"

/* Set to one when set_sizetype has been called.  */
static int sizetype_set;

/* List of types created before set_sizetype has been called.  We do not
   make this a GGC root since we want these nodes to be reclaimed.  */
static tree early_type_list;

/* Data type for the expressions representing sizes of data types.
   It is the first integer type laid out.  */
tree sizetype_tab[(int) TYPE_KIND_LAST];

/* If nonzero, this is an upper limit on alignment of structure fields.
   The value is measured in bits.  */
unsigned int maximum_field_alignment;

/* If non-zero, the alignment of a bitstring or (power-)set value, in bits.
   May be overridden by front-ends.  */
unsigned int set_alignment = 0;

/* Nonzero if all REFERENCE_TYPEs are internal and hence should be
   allocated in Pmode, not ptr_mode.   Set only by internal_reference_types
   called only by a front end.  */
static int reference_types_internal = 0;

static void finalize_record_size	PARAMS ((record_layout_info));
static void finalize_type_size		PARAMS ((tree));
static void place_union_field		PARAMS ((record_layout_info, tree));
extern void debug_rli			PARAMS ((record_layout_info));

/* SAVE_EXPRs for sizes of types and decls, waiting to be expanded.  */

static tree pending_sizes;

/* Nonzero means cannot safely call expand_expr now,
   so put variable sizes onto `pending_sizes' instead.  */

int immediate_size_expand;

/* Show that REFERENCE_TYPES are internal and should be Pmode.  Called only
   by front end.  */

void
internal_reference_types ()
{
  reference_types_internal = 1;
}

/* Get a list of all the objects put on the pending sizes list.  */

tree
get_pending_sizes ()
{
  tree chain = pending_sizes;
  tree t;

  /* Put each SAVE_EXPR into the current function.  */
  for (t = chain; t; t = TREE_CHAIN (t))
    SAVE_EXPR_CONTEXT (TREE_VALUE (t)) = current_function_decl;

  pending_sizes = 0;
  return chain;
}

/* Return non-zero if EXPR is present on the pending sizes list.  */

int
is_pending_size (expr)
     tree expr;
{
  tree t;

  for (t = pending_sizes; t; t = TREE_CHAIN (t))
    if (TREE_VALUE (t) == expr)
      return 1;
  return 0;
}

/* Add EXPR to the pending sizes list.  */

void
put_pending_size (expr)
     tree expr;
{
  /* Strip any simple arithmetic from EXPR to see if it has an underlying
     SAVE_EXPR.  */
  while (TREE_CODE_CLASS (TREE_CODE (expr)) == '1'
	 || (TREE_CODE_CLASS (TREE_CODE (expr)) == '2'
	    && TREE_CONSTANT (TREE_OPERAND (expr, 1))))
    expr = TREE_OPERAND (expr, 0);

  if (TREE_CODE (expr) == SAVE_EXPR)
    pending_sizes = tree_cons (NULL_TREE, expr, pending_sizes);
}

/* Put a chain of objects into the pending sizes list, which must be
   empty.  */

void
put_pending_sizes (chain)
     tree chain;
{
  if (pending_sizes)
    abort ();

  pending_sizes = chain;
}

/* Given a size SIZE that may not be a constant, return a SAVE_EXPR
   to serve as the actual size-expression for a type or decl.  */

tree
variable_size (size)
     tree size;
{
  /* If the language-processor is to take responsibility for variable-sized
     items (e.g., languages which have elaboration procedures like Ada),
     just return SIZE unchanged.  Likewise for self-referential sizes and
     constant sizes.  */
  if (TREE_CONSTANT (size)
      || global_bindings_p () < 0 || contains_placeholder_p (size))
    return size;

  size = save_expr (size);

  /* If an array with a variable number of elements is declared, and
     the elements require destruction, we will emit a cleanup for the
     array.  That cleanup is run both on normal exit from the block
     and in the exception-handler for the block.  Normally, when code
     is used in both ordinary code and in an exception handler it is
     `unsaved', i.e., all SAVE_EXPRs are recalculated.  However, we do
     not wish to do that here; the array-size is the same in both
     places.  */
  if (TREE_CODE (size) == SAVE_EXPR)
    SAVE_EXPR_PERSISTENT_P (size) = 1;

  if (global_bindings_p ())
    {
      if (TREE_CONSTANT (size))
	error ("type size can't be explicitly evaluated");
      else
	error ("variable-size type declared outside of any function");

      return size_one_node;
    }

  if (immediate_size_expand)
    /* NULL_RTX is not defined; neither is the rtx type. 
       Also, we would like to pass const0_rtx here, but don't have it.  */
    expand_expr (size, expand_expr (integer_zero_node, NULL_RTX, VOIDmode, 0),
		 VOIDmode, 0);
  else if (cfun != 0 && cfun->x_dont_save_pending_sizes_p)
    /* The front-end doesn't want us to keep a list of the expressions
       that determine sizes for variable size objects.  */
    ;
  else
    put_pending_size (size);

  return size;
}

#ifndef MAX_FIXED_MODE_SIZE
#define MAX_FIXED_MODE_SIZE GET_MODE_BITSIZE (DImode)
#endif

/* Return the machine mode to use for a nonscalar of SIZE bits.
   The mode must be in class CLASS, and have exactly that many bits.
   If LIMIT is nonzero, modes of wider than MAX_FIXED_MODE_SIZE will not
   be used.  */

enum machine_mode
mode_for_size (size, class, limit)
     unsigned int size;
     enum mode_class class;
     int limit;
{
  enum machine_mode mode;

  if (limit && size > MAX_FIXED_MODE_SIZE)
    return BLKmode;

  /* Get the first mode which has this size, in the specified class.  */
  for (mode = GET_CLASS_NARROWEST_MODE (class); mode != VOIDmode;
       mode = GET_MODE_WIDER_MODE (mode))
    if (GET_MODE_BITSIZE (mode) == size)
      return mode;

  return BLKmode;
}

/* Similar, except passed a tree node.  */

enum machine_mode
mode_for_size_tree (size, class, limit)
     tree size;
     enum mode_class class;
     int limit;
{
  if (TREE_CODE (size) != INTEGER_CST
      /* What we really want to say here is that the size can fit in a
	 host integer, but we know there's no way we'd find a mode for
	 this many bits, so there's no point in doing the precise test.  */
      || compare_tree_int (size, 1000) > 0)
    return BLKmode;
  else
    return mode_for_size (TREE_INT_CST_LOW (size), class, limit);
}

/* Similar, but never return BLKmode; return the narrowest mode that
   contains at least the requested number of bits.  */

enum machine_mode
smallest_mode_for_size (size, class)
     unsigned int size;
     enum mode_class class;
{
  enum machine_mode mode;

  /* Get the first mode which has at least this size, in the
     specified class.  */
  for (mode = GET_CLASS_NARROWEST_MODE (class); mode != VOIDmode;
       mode = GET_MODE_WIDER_MODE (mode))
    if (GET_MODE_BITSIZE (mode) >= size)
      return mode;

  abort ();
}

/* Find an integer mode of the exact same size, or BLKmode on failure.  */

enum machine_mode
int_mode_for_mode (mode)
     enum machine_mode mode;
{
  switch (GET_MODE_CLASS (mode))
    {
    case MODE_INT:
    case MODE_PARTIAL_INT:
      break;

    case MODE_COMPLEX_INT:
    case MODE_COMPLEX_FLOAT:
    case MODE_FLOAT:
    case MODE_VECTOR_INT:
    case MODE_VECTOR_FLOAT:
      mode = mode_for_size (GET_MODE_BITSIZE (mode), MODE_INT, 0);
      break;

    case MODE_RANDOM:
      if (mode == BLKmode)
        break;

      /* ... fall through ...  */

    case MODE_CC:
    default:
      abort ();
    }

  return mode;
}

/* Return the value of VALUE, rounded up to a multiple of DIVISOR.
   This can only be applied to objects of a sizetype.  */

tree
round_up (value, divisor)
     tree value;
     int divisor;
{
  tree arg = size_int_type (divisor, TREE_TYPE (value));

  return size_binop (MULT_EXPR, size_binop (CEIL_DIV_EXPR, value, arg), arg);
}

/* Likewise, but round down.  */

tree
round_down (value, divisor)
     tree value;
     int divisor;
{
  tree arg = size_int_type (divisor, TREE_TYPE (value));

  return size_binop (MULT_EXPR, size_binop (FLOOR_DIV_EXPR, value, arg), arg);
}

/* Set the size, mode and alignment of a ..._DECL node.
   TYPE_DECL does need this for C++.
   Note that LABEL_DECL and CONST_DECL nodes do not need this,
   and FUNCTION_DECL nodes have them set up in a special (and simple) way.
   Don't call layout_decl for them.

   KNOWN_ALIGN is the amount of alignment we can assume this
   decl has with no special effort.  It is relevant only for FIELD_DECLs
   and depends on the previous fields.
   All that matters about KNOWN_ALIGN is which powers of 2 divide it.
   If KNOWN_ALIGN is 0, it means, "as much alignment as you like":
   the record will be aligned to suit.  */

void
layout_decl (decl, known_align)
     tree decl;
     unsigned int known_align;
{
  tree type = TREE_TYPE (decl);
  enum tree_code code = TREE_CODE (decl);

  if (code == CONST_DECL)
    return;
  else if (code != VAR_DECL && code != PARM_DECL && code != RESULT_DECL
	   && code != TYPE_DECL && code != FIELD_DECL)
    abort ();

  if (type == error_mark_node)
    type = void_type_node;

  /* Usually the size and mode come from the data type without change,
     however, the front-end may set the explicit width of the field, so its
     size may not be the same as the size of its type.  This happens with
     bitfields, of course (an `int' bitfield may be only 2 bits, say), but it
     also happens with other fields.  For example, the C++ front-end creates
     zero-sized fields corresponding to empty base classes, and depends on
     layout_type setting DECL_FIELD_BITPOS correctly for the field.  Set the
     size in bytes from the size in bits.  If we have already set the mode,
     don't set it again since we can be called twice for FIELD_DECLs.  */

  TREE_UNSIGNED (decl) = TREE_UNSIGNED (type);
  if (DECL_MODE (decl) == VOIDmode)
    DECL_MODE (decl) = TYPE_MODE (type);

  if (DECL_SIZE (decl) == 0)
    {
      DECL_SIZE (decl) = TYPE_SIZE (type);
      DECL_SIZE_UNIT (decl) = TYPE_SIZE_UNIT (type);
    }
  else
    DECL_SIZE_UNIT (decl)
      = convert (sizetype, size_binop (CEIL_DIV_EXPR, DECL_SIZE (decl),
				       bitsize_unit_node));

  /* Force alignment required for the data type.
     But if the decl itself wants greater alignment, don't override that.
     Likewise, if the decl is packed, don't override it.  */
  if (! (code == FIELD_DECL && DECL_BIT_FIELD (decl))
      && (DECL_ALIGN (decl) == 0
	  || (! (code == FIELD_DECL && DECL_PACKED (decl))
	      && TYPE_ALIGN (type) > DECL_ALIGN (decl))))
    {	      
      DECL_ALIGN (decl) = TYPE_ALIGN (type);
      DECL_USER_ALIGN (decl) = 0;
    }

  /* For fields, set the bit field type and update the alignment.  */
  if (code == FIELD_DECL)
    {
      DECL_BIT_FIELD_TYPE (decl) = DECL_BIT_FIELD (decl) ? type : 0;
      if (maximum_field_alignment != 0)
	DECL_ALIGN (decl) = MIN (DECL_ALIGN (decl), maximum_field_alignment);

      /* If the field is of variable size, we can't misalign it since we
	 have no way to make a temporary to align the result.  But this
	 isn't an issue if the decl is not addressable.  Likewise if it
	 is of unknown size.  */
      else if (DECL_PACKED (decl)
	       && (DECL_NONADDRESSABLE_P (decl)
		   || DECL_SIZE_UNIT (decl) == 0
		   || TREE_CODE (DECL_SIZE_UNIT (decl)) == INTEGER_CST))
	{
	  DECL_ALIGN (decl) = MIN (DECL_ALIGN (decl), BITS_PER_UNIT);
	  DECL_USER_ALIGN (decl) = 0;
	}
    }

  /* See if we can use an ordinary integer mode for a bit-field. 
     Conditions are: a fixed size that is correct for another mode
     and occupying a complete byte or bytes on proper boundary.  */
  if (code == FIELD_DECL && DECL_BIT_FIELD (decl)
      && TYPE_SIZE (type) != 0
      && TREE_CODE (TYPE_SIZE (type)) == INTEGER_CST
      && GET_MODE_CLASS (TYPE_MODE (type)) == MODE_INT)
    {
      enum machine_mode xmode
	= mode_for_size_tree (DECL_SIZE (decl), MODE_INT, 1);

      if (xmode != BLKmode && known_align >= GET_MODE_ALIGNMENT (xmode))
	{
	  DECL_ALIGN (decl) = MAX (GET_MODE_ALIGNMENT (xmode),
				   DECL_ALIGN (decl));
	  DECL_MODE (decl) = xmode;
	  DECL_BIT_FIELD (decl) = 0;
	}
    }

  /* Turn off DECL_BIT_FIELD if we won't need it set.  */
  if (code == FIELD_DECL && DECL_BIT_FIELD (decl)
      && TYPE_MODE (type) == BLKmode && DECL_MODE (decl) == BLKmode
      && known_align >= TYPE_ALIGN (type)
      && DECL_ALIGN (decl) >= TYPE_ALIGN (type)
      && DECL_SIZE_UNIT (decl) != 0)
    DECL_BIT_FIELD (decl) = 0;

  /* Evaluate nonconstant size only once, either now or as soon as safe.  */
  if (DECL_SIZE (decl) != 0 && TREE_CODE (DECL_SIZE (decl)) != INTEGER_CST)
    DECL_SIZE (decl) = variable_size (DECL_SIZE (decl));
  if (DECL_SIZE_UNIT (decl) != 0
      && TREE_CODE (DECL_SIZE_UNIT (decl)) != INTEGER_CST)
    DECL_SIZE_UNIT (decl) = variable_size (DECL_SIZE_UNIT (decl));

  /* If requested, warn about definitions of large data objects.  */
  if (warn_larger_than
      && (code == VAR_DECL || code == PARM_DECL)
      && ! DECL_EXTERNAL (decl))
    {
      tree size = DECL_SIZE_UNIT (decl);

      if (size != 0 && TREE_CODE (size) == INTEGER_CST
	  && compare_tree_int (size, larger_than_size) > 0)
	{
	  unsigned int size_as_int = TREE_INT_CST_LOW (size);

	  if (compare_tree_int (size, size_as_int) == 0)
	    warning_with_decl (decl, "size of `%s' is %d bytes", size_as_int);
	  else
	    warning_with_decl (decl, "size of `%s' is larger than %d bytes",
			       larger_than_size);
	}
    }
}

/* Hook for a front-end function that can modify the record layout as needed
   immediately before it is finalized.  */

void (*lang_adjust_rli) PARAMS ((record_layout_info)) = 0;

void
set_lang_adjust_rli (f)
     void (*f) PARAMS ((record_layout_info));
{
  lang_adjust_rli = f;
}

/* Begin laying out type T, which may be a RECORD_TYPE, UNION_TYPE, or
   QUAL_UNION_TYPE.  Return a pointer to a struct record_layout_info which
   is to be passed to all other layout functions for this record.  It is the
   responsibility of the caller to call `free' for the storage returned. 
   Note that garbage collection is not permitted until we finish laying
   out the record.  */

record_layout_info
start_record_layout (t)
     tree t;
{
  record_layout_info rli 
    = (record_layout_info) xmalloc (sizeof (struct record_layout_info_s));

  rli->t = t;

  /* If the type has a minimum specified alignment (via an attribute
     declaration, for example) use it -- otherwise, start with a
     one-byte alignment.  */
  rli->record_align = MAX (BITS_PER_UNIT, TYPE_ALIGN (t));
  rli->unpacked_align = rli->unpadded_align = rli->record_align;
  rli->offset_align = MAX (rli->record_align, BIGGEST_ALIGNMENT);

#ifdef STRUCTURE_SIZE_BOUNDARY
  /* Packed structures don't need to have minimum size.  */
  if (! TYPE_PACKED (t))
    rli->record_align = MAX (rli->record_align, STRUCTURE_SIZE_BOUNDARY);
#endif

  rli->offset = size_zero_node;
  rli->bitpos = bitsize_zero_node;
  rli->prev_field = 0;
  rli->pending_statics = 0;
  rli->packed_maybe_necessary = 0;

  return rli;
}

/* These four routines perform computations that convert between
   the offset/bitpos forms and byte and bit offsets.  */

tree
bit_from_pos (offset, bitpos)
     tree offset, bitpos;
{
  return size_binop (PLUS_EXPR, bitpos,
		     size_binop (MULT_EXPR, convert (bitsizetype, offset),
				 bitsize_unit_node));
}

tree
byte_from_pos (offset, bitpos)
     tree offset, bitpos;
{
  return size_binop (PLUS_EXPR, offset,
		     convert (sizetype,
			      size_binop (TRUNC_DIV_EXPR, bitpos,
					  bitsize_unit_node)));
}

void
pos_from_byte (poffset, pbitpos, off_align, pos)
     tree *poffset, *pbitpos;
     unsigned int off_align;
     tree pos;
{
  *poffset
    = size_binop (MULT_EXPR,
		  convert (sizetype,
			   size_binop (FLOOR_DIV_EXPR, pos,
				       bitsize_int (off_align
						    / BITS_PER_UNIT))),
		  size_int (off_align / BITS_PER_UNIT));
  *pbitpos = size_binop (MULT_EXPR,
			 size_binop (FLOOR_MOD_EXPR, pos,
				     bitsize_int (off_align / BITS_PER_UNIT)),
			 bitsize_unit_node);
}

void
pos_from_bit (poffset, pbitpos, off_align, pos)
     tree *poffset, *pbitpos;
     unsigned int off_align;
     tree pos;
{
  *poffset = size_binop (MULT_EXPR,
			 convert (sizetype,
				  size_binop (FLOOR_DIV_EXPR, pos,
					      bitsize_int (off_align))),
			 size_int (off_align / BITS_PER_UNIT));
  *pbitpos = size_binop (FLOOR_MOD_EXPR, pos, bitsize_int (off_align));
}

/* Given a pointer to bit and byte offsets and an offset alignment,
   normalize the offsets so they are within the alignment.  */

void
normalize_offset (poffset, pbitpos, off_align)
     tree *poffset, *pbitpos;
     unsigned int off_align;
{
  /* If the bit position is now larger than it should be, adjust it
     downwards.  */
  if (compare_tree_int (*pbitpos, off_align) >= 0)
    {
      tree extra_aligns = size_binop (FLOOR_DIV_EXPR, *pbitpos,
				      bitsize_int (off_align));

      *poffset
	= size_binop (PLUS_EXPR, *poffset,
		      size_binop (MULT_EXPR, convert (sizetype, extra_aligns),
				  size_int (off_align / BITS_PER_UNIT)));
				
      *pbitpos
	= size_binop (FLOOR_MOD_EXPR, *pbitpos, bitsize_int (off_align));
    }
}

/* Print debugging information about the information in RLI.  */

void
debug_rli (rli)
     record_layout_info rli;
{
  print_node_brief (stderr, "type", rli->t, 0);
  print_node_brief (stderr, "¥noffset", rli->offset, 0);
  print_node_brief (stderr, " bitpos", rli->bitpos, 0);

  fprintf (stderr, "¥naligns: rec = %u, unpack = %u, unpad = %u, off = %u¥n",
	   rli->record_align, rli->unpacked_align, rli->unpadded_align,
	   rli->offset_align);
  if (rli->packed_maybe_necessary)
    fprintf (stderr, "packed may be necessary¥n");

  if (rli->pending_statics)
    {
      fprintf (stderr, "pending statics:¥n");
      debug_tree (rli->pending_statics);
    }
}

/* Given an RLI with a possibly-incremented BITPOS, adjust OFFSET and
   BITPOS if necessary to keep BITPOS below OFFSET_ALIGN.  */

void
normalize_rli (rli)
     record_layout_info rli;
{
  normalize_offset (&rli->offset, &rli->bitpos, rli->offset_align);
}

/* Returns the size in bytes allocated so far.  */

tree
rli_size_unit_so_far (rli)
     record_layout_info rli;
{
  return byte_from_pos (rli->offset, rli->bitpos);
}

/* Returns the size in bits allocated so far.  */

tree
rli_size_so_far (rli)
     record_layout_info rli;
{
  return bit_from_pos (rli->offset, rli->bitpos);
}

/* Called from place_field to handle unions.  */

static void
place_union_field (rli, field)
     record_layout_info rli;
     tree field;
{
  unsigned int desired_align;

  layout_decl (field, 0);
  
  DECL_FIELD_OFFSET (field) = size_zero_node;
  DECL_FIELD_BIT_OFFSET (field) = bitsize_zero_node;
  SET_DECL_OFFSET_ALIGN (field, BIGGEST_ALIGNMENT);

  desired_align = DECL_ALIGN (field);

#ifdef BIGGEST_FIELD_ALIGNMENT
  /* Some targets (i.e. i386) limit union field alignment
     to a lower boundary than alignment of variables unless
     it was overridden by attri