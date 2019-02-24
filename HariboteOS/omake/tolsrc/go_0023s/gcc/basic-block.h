/* Define control and data flow tables, and regsets.
   Copyright (C) 1987, 1997, 1998, 1999, 2000, 2001, 2002
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

#ifndef GCC_BASIC_BLOCK_H
#define GCC_BASIC_BLOCK_H

/* !kawai! */
#include "bitmap.h"
#include "sbitmap.h"
#include "varray.h"
#include "../include/partition.h"
/* end of !kawai! */

/* Head of register set linked list.  */
typedef bitmap_head regset_head;
/* A pointer to a regset_head.  */
typedef bitmap regset;

/* Initialize a new regset.  */
#define INIT_REG_SET(HEAD) bitmap_initialize (HEAD)

/* Clear a register set by freeing up the linked list.  */
#define CLEAR_REG_SET(HEAD) bitmap_clear (HEAD)

/* Copy a register set to another register set.  */
#define COPY_REG_SET(TO, FROM) bitmap_copy (TO, FROM)

/* Compare two register sets.  */
#define REG_SET_EQUAL_P(A, B) bitmap_equal_p (A, B)

/* `and' a register set with a second register set.  */
#define AND_REG_SET(TO, FROM) bitmap_operation (TO, TO, FROM, BITMAP_AND)

/* `and' the complement of a register set with a register set.  */
#define AND_COMPL_REG_SET(TO, FROM) ¥
  bitmap_operation (TO, TO, FROM, BITMAP_AND_COMPL)

/* Inclusive or a register set with a second register set.  */
#define IOR_REG_SET(TO, FROM) bitmap_operation (TO, TO, FROM, BITMAP_IOR)

/* Exclusive or a register set with a second register set.  */
#define XOR_REG_SET(TO, FROM) bitmap_operation (TO, TO, FROM, BITMAP_XOR)

/* Or into TO the register set FROM1 `and'ed with the complement of FROM2.  */
#define IOR_AND_COMPL_REG_SET(TO, FROM1, FROM2) ¥
  bitmap_ior_and_compl (TO, FROM1, FROM2)

/* Clear a single register in a register set.  */
#define CLEAR_REGNO_REG_SET(HEAD, REG) bitmap_clear_bit (HEAD, REG)

/* Set a single register in a register set.  */
#define SET_REGNO_REG_SET(HEAD, REG) bitmap_set_bit (HEAD, REG)

/* Return true if a register is set in a register set.  */
#define REGNO_REG_SET_P(TO, REG) bitmap_bit_p (TO, REG)

/* Copy the hard registers in a register set to the hard register set.  */
extern void reg_set_to_hard_reg_set PARAMS ((HARD_REG_SET *, bitmap));
#define REG_SET_TO_HARD_REG_SET(TO, FROM)				¥
do {									¥
  CLEAR_HARD_REG_SET (TO);						¥
  reg_set_to_hard_reg_set (&TO, FROM);					¥
} while (0)

/* Loop over all registers in REGSET, starting with MIN, setting REGNUM to the
   register number and executing CODE for all registers that are set.  */
#define EXECUTE_IF_SET_IN_REG_SET(REGSET, MIN, REGNUM, CODE)		¥
  EXECUTE_IF_SET_IN_BITMAP (REGSET, MIN, REGNUM, CODE)

/* Loop over all registers in REGSET1 and REGSET2, starting with MIN, setting
   REGNUM to the register number and executing CODE for all registers that are
   set in the first regset and not set in the second.  */
#define EXECUTE_IF_AND_COMPL_IN_REG_SET(REGSET1, REGSET2, MIN, REGNUM, CODE) ¥
  EXECUTE_IF_AND_COMPL_IN_BITMAP (REGSET1, REGSET2, MIN, REGNUM, CODE)

/* Loop over all registers in REGSET1 and REGSET2, starting with MIN, setting
   REGNUM to the register number and executing CODE for all registers that are
   set in both regsets.  */
#define EXECUTE_IF_AND_IN_REG_SET(REGSET1, REGSET2, MIN, REGNUM, CODE) ¥
  EXECUTE_IF_AND_IN_BITMAP (REGSET1, REGSET2, MIN, REGNUM, CODE)

/* Allocate a register set with oballoc.  */
#define OBSTACK_ALLOC_REG_SET(OBSTACK) BITMAP_OBSTACK_ALLOC (OBSTACK)