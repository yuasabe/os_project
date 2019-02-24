/* Machine mode definitions for GNU C-Compiler; included by rtl.h and tree.h.
   Copyright (C) 1991, 1993, 1994, 1996, 1998, 1999, 2000, 2001
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

#ifndef HAVE_MACHINE_MODES
#define HAVE_MACHINE_MODES

/* Make an enum class that gives all the machine modes.  */

#define DEF_MACHMODE(SYM, NAME, TYPE, BITSIZE, SIZE, UNIT, WIDER, INNER)  SYM,

enum machine_mode {
#include "machmode.def"
MAX_MACHINE_MODE };

#undef DEF_MACHMODE

#ifndef NUM_MACHINE_MODES
#define NUM_MACHINE_MODES (int) MAX_MACHINE_MODE
#endif

/* Get the name of mode MODE as a string.  */

extern const char * const mode_name[NUM_MACHINE_MODES];
#define GET_MODE_NAME(MODE)		(mode_name[(int) (MODE)])

enum mode_class { MODE_RANDOM, MODE_INT, MODE_FLOAT, MODE_PARTIAL_INT, MODE_CC,
		  MODE_COMPLEX_INT, MODE_COMPLEX_FLOAT,
		  MODE_VECTOR_INT, MODE_VECTOR_FLOAT,
		  MAX_MODE_CLASS};

/* Get the general kind of object that mode MODE represents
   (integer, floating, complex, etc.)  */

extern const enum mode_class mode_class[NUM_MACHINE_MODES];
#define GET_MODE_CLASS(MODE)		(mode_class[(int) (MODE)])

/* Nonzero if MODE is an integral mode.  */
#define INTEGRAL_MODE_P(MODE)			¥
  (GET_MODE_CLASS (MODE) == MODE_INT		¥
   || GET_MODE_CLASS (MODE) == MODE_PARTIAL_INT ¥
   || GET_MODE_CLASS (MODE) == MODE_COMPLEX_INT ¥
   || GET_MODE_CLASS (MODE) == MODE_VECTOR_INT)

/* Nonzero if MODE is a floating-point mode.  */
#define FLOAT_MODE_P(MODE)		¥
  (GET_MODE_CLASS (MODE) == MODE_FLOAT	¥
   || GET_MODE_CLASS (MODE) == MODE_COMPLEX_FLOAT ¥
   || GET_MODE_CLASS (MODE) == MODE_VECTOR_FLOAT)

/* Nonzero if MODE is a complex mode.  */
#define COMPLEX_MODE_P(MODE)			¥
  (GET_MODE_CLASS (MODE) == MODE_COMPLEX_INT	¥
   || GET_MODE_CLASS (MODE) == MODE_COMPLEX_FLOAT)

/* Nonzero if MODE is a vector mode.  */
#define VECTOR_MODE_P(MODE)			¥
  (GET_MODE_CLASS (MODE) == MODE_VECTOR_INT	¥
   || GET_MODE_CLASS (MODE) == MODE_VECTOR_FLOAT)

/* Get the size in bytes of an object of mode MODE.  */

extern const unsigned char mode_size[NUM_MACHINE_MODES];
#define GET_MODE_SIZE(MODE)		(mode_size[(int) (MODE)])

/* Get the size in bytes of the basic parts of an object of mode MODE.  */

extern const unsigned char mode_unit_size[NUM_MACHINE_MODES];
#define GET_MODE_UNIT_SIZE(MODE)	(mode_unit_size[(int) (MODE)])

/* Get the number of units in the object.  */

#define GET_MODE_NUNITS(MODE)  ¥
  ((GET_MODE_UNIT_SIZE ((MODE)) == 0) ? 0 ¥
   : (GET_MODE_SIZE ((MODE)) / GET_MODE_UNIT_SIZE ((MODE))))

/* Get the size in bits of an object of mode MODE.  */

extern const unsigned short mode_bitsize[NUM_MACHINE_MODES];
#define GET_MODE_BITSIZE(MODE)  (mode_bitsize[(int) (MODE)])

#endif /* not HAVE_MACHINE_MODES */

#if defined HOST_WIDE_INT && ! defined GET_MODE_MASK

/* Get a bitmask containing 1 for all bits in a word
   that fit within mode MODE.  */

extern const unsigned HOST_WIDE_INT mode_mask_array[NUM_MACHINE_MODES];

#define GET_MODE_MASK(MODE) mode_mask_array[(int) (MODE)]

extern const enum machine_mode inner_mode_array[NUM_MACHINE_MODES];

/* Return the mode of the inner elements in a vector.  */

#define GET_MODE_INNER(MODE) inner_mode_array[(int) (MODE)]

#endif /* defined (HOST_WIDE_INT) && ! defined GET_MODE_MASK */

#if ! defined GET_MODE_WIDER_MODE || ! defined GET_M