/* Virtual array support.
   Copyright (C) 1998, 1999, 2000, 2002 Free Software Foundation, Inc.
   Contributed by Cygnus Solutions.

   This file is part of GCC.

   GCC is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   GCC is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License
   along with GCC; see the file COPYING.  If not, write to the Free
   the Free Software Foundation, 59 Temple Place - Suite 330, Boston,
   MA 02111-1307, USA.  */

#ifndef GCC_VARRAY_H
#define GCC_VARRAY_H

#ifndef HOST_WIDE_INT
#include "machmode.h"
#endif

#ifndef GCC_SYSTEM_H
#include "system.h"
#endif

/* Auxiliary structure used inside the varray structure, used for
   function integration data.  */

struct const_equiv_data {
  /* Map pseudo reg number in calling function to equivalent constant.  We
     cannot in general substitute constants into parameter pseudo registers,
     since some machine descriptions (many RISCs) won't always handle
     the resulting insns.  So if an incoming parameter has a constant
     equivalent, we record it here, and if the resulting insn is
     recognizable, we go with it.

     We also use this mechanism to convert references to incoming arguments
     and stacked variables.  copy_rtx_and_substitute will replace the virtual
     incoming argument and virtual stacked variables registers with new
     pseudos that contain pointers into the replacement area allocated for
     this inline instance.  These pseudos are then marked as being equivalent
     to the appropriate address and substituted if valid.  */
  struct rtx_def *rtx;

  /* Record the valid age for each entry.  The entry is invalid if its
     age is less than const_age.  */
  unsigned age;
};

/* Union of various array types that are used.  */
typedef union varray_data_tag {
  char			 c[1];
  unsigned char		 uc[1];
  short			 s[1];
  unsigned short	 us[1];
  int			 i[1];
  unsigned int		 u[1];
  long			 l[1];
  unsigned long		 ul[1];
  HOST_WIDE_INT		 hint[1];
  unsigned HOST_WIDE_INT uhint[1];
  PTR			 generic[1];
  char			 *cptr[1];
  struct rtx_def	 *rtx[1];
  struct rtvec_def	 *rtvec[1];
  union tree_node	 *tree[1];
  struct bitmap_head_def *bitmap[1];
  struct sched_info_tag	 *sched[1];
  struct reg_info_def	 *reg[1];
  struct const_equiv_data const_equiv[1];
  struct basic_block_def *bb[1];
  struct elt_list       *te[1];
} varray_data;

/* Virtual array of pointers header.  */
typedef struct varray_head_tag {
  size_t	num_elements;	/* maximum element number allocated */
  size_t        elements_used;  /* the number of elements used, if
				   using VARRAY_PUSH/VARRAY_POP.  */
  size_t	element_size;	/* size of each data element */
  const char   *name;		/* name of the varray for reporting errors */
  varray_data	data;		/* data elements follow, must be last */
} *varray_type;

/* Allocate a virtual array with NUM elements, each of which is SIZE bytes
   long, named NAME.  Array elements are zeroed.  */
extern varray_type varray_init	PARAMS ((size_t, size_t, const char *));

#define VARRAY_CHAR_INIT(va, num, name) ¥
  va = varray_init (num, sizeof (char), name)

#define VARRAY_UCHAR_INIT(va, num, name) ¥
  va = varray_init (num, sizeof (unsigned char), name)

#define VARRAY_SHORT_INIT(va, num, name) ¥
  va = varray_init (num, sizeof (short), name)

#define VARRAY_USHORT_INIT(va, num, name) ¥
  va = varray_init (num, sizeof (unsigned short), name)

#define VARRAY_INT_INIT(va, num, name) ¥
  va = varray_init (num, sizeof (int), name)

#define VARRAY_UINT_INIT(va, num, name) ¥
  va = varray_init (num, sizeof (unsigned i