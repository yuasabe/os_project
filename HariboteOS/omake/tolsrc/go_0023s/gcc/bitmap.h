/* Functions to support general ended bitmaps.
   Copyright (C) 1997, 1998, 1999, 2000, 2001, 2002
   Free Software Foundation, Inc.

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

#ifndef GCC_BITMAP_H
#define GCC_BITMAP_H 

/* Number of words to use for each element in the linked list.  */

#ifndef BITMAP_ELEMENT_WORDS
#define BITMAP_ELEMENT_WORDS 2
#endif

/* Number of bits in each actual element of a bitmap.  We get slightly better
   code for bit % BITMAP_ELEMENT_ALL_BITS and bit / BITMAP_ELEMENT_ALL_BITS if
   bits is unsigned, assuming it is a power of 2.  */

#define BITMAP_ELEMENT_ALL_BITS ¥
  ((unsigned) (BITMAP_ELEMENT_WORDS * HOST_BITS_PER_WIDE_INT))

/* Bitmap set element.  We use a linked list to hold only the bits that
   are set.  This allows for use to grow the bitset dynamically without
   having to realloc and copy a giant bit array.  The `prev' field is
   undefined for an element on the free list.  */

typedef struct bitmap_element_def
{
  struct bitmap_element_def *next;		/* Next element.  */
  struct bitmap_element_def *prev;		/* Previous element.  */
  unsigned int indx;			/* regno/BITMAP_ELEMENT_ALL_BITS.  */
  unsigned HOST_WIDE_INT bits[BITMAP_ELEMENT_WORDS]; /* Bits that are set.  */
} bitmap_element;

/* Head of bitmap linked list.  */
typedef struct bitmap_head_def {
  bitmap_element *first;	/* First element in linked list.  */
  bitmap_element *current;	/* Last element looked at.  */
  unsigned int indx;		/* Index of last element looked at.  */

} bitmap_head, *bitmap;

/* Enumeration giving the various operations we support.  */
enum bitmap_bits {
  BITMAP_AND,			/* TO = FROM1 & FROM2 */
  BITMAP_AND_COMPL,		/* TO = FROM1 & ‾ FROM2 */
  BITMAP_IOR,			/* TO = FROM1 | FROM2 */
  BITMAP_XOR,			/* TO = FROM1 ^ FROM2 */
  BITMAP_IOR_COMPL			/* TO = FROM1 | ‾FROM2 */
};

/* Global data */
extern bitmap_element bitmap_zero_bits;	/* Zero bitmap element */

/* Clear a bitmap by freeing up the linked list.  */
extern void bitmap_clear PARAMS ((bitmap));

/* Copy a bitmap to another bitmap.  */
extern void bitmap_copy PARAMS ((bitmap, bitmap));

/* True if two bitmaps are identical.  */
extern int bitmap_equal_p PARAMS ((bitmap, bitmap));

/* Perform an operation on two bitmaps, yielding a third.  */
extern int bitmap_operation PARAMS ((bitmap, bitmap, bitmap, enum bitmap_bits));

/* `or' into one bitmap the `and' of a second bitmap witih the complement
   of a third.  */
extern void bitmap_ior_and_compl PARAMS ((bitmap, bitmap, bitmap));

/* Clear a single register in a register set.  */
extern void bitmap_clear_bit PARAMS ((bitmap, int));

/* Set a single register in a register set.  */
extern void bitmap_set_bit PARAMS ((bitmap, int));

/* Return true if a register is set in a register set.  */
extern int bitmap_bit_p PARAMS ((bitmap, int));

/* Debug functions to print a bitmap linked list.  */
extern void debug_bitmap PARAMS ((bitmap));
extern void debug_bitmap_file PARAMS ((FILE *, bitmap));

/* Print a bitmap */
extern void bitmap_print PARAMS ((FILE *, bitmap, const char *, const char *));

/* Initialize a bitmap header.  */
extern bitmap bitmap_initialize PARAMS ((bitmap));

/* Release all memory held by bitmaps.  */
extern void bitmap_release_memory PARAMS ((void));

/* A few compatibility/functions macros for compatibility with sbitmaps */
#define dump_bitmap