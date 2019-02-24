/* Definitions of floating-point access for GNU compiler.
   Copyright (C) 1989, 1991, 1994, 1996, 1997, 1998,
   1999, 2000, 2002 Free Software Foundation, Inc.

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

#ifndef GCC_REAL_H
#define GCC_REAL_H

/* Define codes for all the float formats that we know of.  */
#define UNKNOWN_FLOAT_FORMAT 0
#define IEEE_FLOAT_FORMAT 1
#define VAX_FLOAT_FORMAT 2
#define IBM_FLOAT_FORMAT 3
#define C4X_FLOAT_FORMAT 4

/* Default to IEEE float if not specified.  Nearly all machines use it.  */

#ifndef TARGET_FLOAT_FORMAT
#define	TARGET_FLOAT_FORMAT	IEEE_FLOAT_FORMAT
#endif

#ifndef HOST_FLOAT_FORMAT
#define	HOST_FLOAT_FORMAT	IEEE_FLOAT_FORMAT
#endif

#ifndef INTEL_EXTENDED_IEEE_FORMAT
#define INTEL_EXTENDED_IEEE_FORMAT 0
#endif

#if TARGET_FLOAT_FORMAT == IEEE_FLOAT_FORMAT
#define REAL_INFINITY
#endif

/* If FLOAT_WORDS_BIG_ENDIAN and HOST_FLOAT_WORDS_BIG_ENDIAN are not defined
   in the header files, then this implies the word-endianness is the same as
   for integers.  */

/* This is defined 0 or 1, like WORDS_BIG_ENDIAN.  */
#ifndef FLOAT_WORDS_BIG_ENDIAN
#define FLOAT_WORDS_BIG_ENDIAN WORDS_BIG_ENDIAN
#endif

/* This is defined 0 or 1, unlike HOST_WORDS_BIG_ENDIAN.  */
#ifndef HOST_FLOAT_WORDS_BIG_ENDIAN
#ifdef HOST_WORDS_BIG_ENDIAN
#define HOST_FLOAT_WORDS_BIG_ENDIAN 1
#else
#define HOST_FLOAT_WORDS_BIG_ENDIAN 0
#endif
#endif

/* Defining REAL_ARITHMETIC invokes a floating point emulator
   that can produce a target machine format differing by more
   than just endian-ness from the host's format.  The emulator
   is also used to support extended real XFmode.  */
#ifndef LONG_DOUBLE_TYPE_SIZE
#define LONG_DOUBLE_TYPE_SIZE 64
#endif
/* MAX_LONG_DOUBLE_TYPE_SIZE is a constant tested by #if.
   LONG_DOUBLE_TYPE_SIZE can vary at compiler run time.
   So long as macros like REAL_VALUE_TO_TARGET_LONG_DOUBLE cannot
   vary too, however, then XFmode and TFmode long double
   cannot both be supported at the same time.  */
#ifndef MAX_LONG_DOUBLE_TYPE_SIZE
#define MAX_LONG_DOUBLE_TYPE_SIZE LONG_DOUBLE_TYPE_SIZE
#endif
#if (MAX_LONG_DOUBLE_TYPE_SIZE == 96) || (MAX_LONG_DOUBLE_TYPE_SIZE == 128)
#ifndef REAL_ARITHMETIC
#define REAL_ARITHMETIC
#endif
#endif
#ifdef REAL_ARITHMETIC
/* **** Start of software floating point emulator interface macros **** */

/* Support 80-bit extended real XFmode if LONG_DOUBLE_TYPE_SIZE
   has been defined to be 96 in the tm.h machine file.  */
#if (MAX_LONG_DOUBLE_TYPE_SIZE == 96)
#define REAL_IS_NOT_DOUBLE
#define REAL_ARITHMETIC
typedef struct {
  HOST_WIDE_INT r[(11 + sizeof (HOST_WIDE_INT))/(sizeof (HOST_WIDE_INT))];
} realvaluetype;
#define REAL_VALUE_TYPE realvaluetype

#else /* no XFmode support */

#if (MAX_LONG_DOUBLE_TYPE_SIZE == 128)

#define REAL_IS_NOT_DOUBLE
#define REAL_ARITHMETIC
typedef struct {
  HOST_WIDE_INT r[(19 + sizeof (HOST_WIDE_INT))/(sizeof (HOST_WIDE_INT))];
} realvaluetype;
#define REAL_VALUE_TYPE realvaluetype

#else /* not TFmode */

#if HOST_FLOAT_FORMAT != TARGET_FLOAT_FORMAT
/* If no XFmode support, then a REAL_VALUE_TYPE is 64 bits wide
   but it is not necessarily a host machine double.  */
#define REAL_IS_NOT_DOUBLE
typedef struct {
  HOST_WIDE_INT r[(7 + sizeof (HOST_WIDE_INT))/(sizeof (HOST_WIDE_INT))];
} realvaluetype;
#define REAL_VALUE_TYPE realvaluetype
#else
/* If host and target f