/* Sets (bit vectors) of hard registers, and operations on them.
   Copyright (C) 1987, 1992, 1994, 2000 Free Software Foundation, Inc.

This file is part of GCC

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

#ifndef GCC_HARD_REG_SET_H
#define GCC_HARD_REG_SET_H 

/* Define the type of a set of hard registers.  */

/* HARD_REG_ELT_TYPE is a typedef of the unsigned integral type which
   will be used for hard reg sets, either alone or in an array.

   If HARD_REG_SET is a macro, its definition is HARD_REG_ELT_TYPE,
   and it has enough bits to represent all the target machine's hard
   registers.  Otherwise, it is a typedef for a suitably sized array
   of HARD_REG_ELT_TYPEs.  HARD_REG_SET_LONGS is defined as how many.

   Note that lots of code assumes that the first part of a regset is
   the same format as a HARD_REG_SET.  To help make sure this is true,
   we only try the widest integer mode (HOST_WIDE_INT) instead of all the
   smaller types.  This approach loses only if there are a very few
   registers and then only in the few cases where we have an array of
   HARD_REG_SETs, so it needn't be as complex as it used to be.  */

typedef unsigned HOST_WIDE_INT HARD_REG_ELT_TYPE;

#if FIRST_PSEUDO_REGISTER <= HOST_BITS_PER_WIDE_INT

#define HARD_REG_SET HARD_REG_ELT_TYPE

#else

#define HARD_REG_SET_LONGS ¥
 ((FIRST_PSEUDO_REGISTER + HOST_BITS_PER_WIDE_INT - 1)	¥
  / HOST_BITS_PER_WIDE_INT)
typedef HARD_REG_ELT_TYPE HARD_REG_SET[HARD_REG_SET_LONGS];

#endif

/* HARD_CONST is used to cast a constant to the appropriate type
   for use with a HARD_REG_SET.  */

#define HARD_CONST(X) ((HARD_REG_ELT_TYPE) (X))

/* Define macros SET_HARD_REG_BIT, CLEAR_HARD_REG_BIT and TEST_HARD_REG_BIT
   to set, clear or test one bit in a hard reg set of type HARD_REG_SET.
   All three take two arguments: the set and the register number.

   In the case where sets are arrays of longs, the first argument
   is actually a pointer to a long.

   Define two macros for initializing a set:
   CLEAR_HARD_REG_SET and SET_HARD_REG_SET.
   These take just one argument.

   Also define macros for copying hard reg sets:
   COPY_HARD_REG_SET and COMPL_HARD_REG_SET.
   These take two arguments TO and FROM; they read from FROM
   and store into TO.  COMPL_HARD_REG_SET complements each bit.

   Also define macros for combining hard reg sets:
   IOR_HARD_REG_SET and AND_HARD_REG_SET.
   These take two arguments TO and FROM; they read from FROM
   and combine bitwise into TO.  Define also two variants
   IOR_COMPL_HARD_REG_SET and AND_COMPL_HARD_REG_SET
   which use the complement of the set FROM.

   Also define GO_IF_HARD_REG_SUBSET (X, Y, TO):
   if X is a subset of Y, go to TO.
*/

#ifdef HARD_REG_SET

#define SET_HARD_REG_BIT(SET, BIT)  ¥
 ((SET) |= HARD_CONST (1) << (BIT))
#define CLEAR_HARD_REG_BIT(SET, BIT)  ¥
 ((SET) &= ‾(HARD_CONST (1) << (BIT)))
#define TEST_HARD_REG_BIT(SET, BIT)  ¥
 ((SET) & (HARD_CONST (1) << (BIT)))

#define CLEAR_HARD_REG_SET(TO) ((TO) = HARD_CONST (0))
#define SET_HARD_REG_SET(TO) ((TO) = ‾ HARD_CONST (0))

#define COPY_HARD_REG_SET(TO, FROM) ((TO) = (FROM))
#define COMPL_HARD_REG_SET(TO, FROM) ((TO) = ‾(FROM))

#define IOR_HARD_REG_SET(TO, FROM) ((TO) |= (FROM))
#define IOR_COMPL_HARD_REG_SET(TO, FROM) ((TO) |= ‾ (FROM))
#define AND_HARD_REG_SET(TO, FROM) ((TO) &= (FROM))
#define AND_COMPL_HARD_REG_SET(TO, FROM) ((TO) &= 