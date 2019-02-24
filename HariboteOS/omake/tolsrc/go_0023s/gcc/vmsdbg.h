/* Definitions for the data structures and codes used in VMS debugging.
   Copyright (C) 2001 Free Software Foundation, Inc.

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

#ifndef GCC_VMSDBG_H
#define GCC_VMSDBG_H 1

/*  We define types and constants used in VMS Debug output.  Note that the
    structs only approximate the output that is written.  We write the output
    explicitly, field by field.  This output would only agree with the
    structs in this file if no padding were done.  The sizes after each
    struct are the size actually written, which is usually smaller than the
    size of the struct.  */

/* Header type codes.  */
typedef enum _DST_TYPE {DST_K_SOURCE = 155, DST_K_PROLOG = 162,
			DST_K_BLKBEG = 176, DST_K_BLKEND = 177,
			DST_K_LINE_NUM = 185, DST_K_MODBEG = 188,
			DST_K_MODEND = 189, DST_K_RTNBEG = 190,
			DST_K_RTNEND = 191} DST_DTYPE;

/* Header.  */

typedef struct _DST_HEADER
{
  union
    {
      unsigned short int dst_w_length;
      unsigned short int dst_x_length;
    } dst__header_length;
  union
    {
      ENUM_BITFIELD (_DST_TYPE) dst_w_type : 16;
      ENUM_BITFIELD (_DST_TYPE) dst_x_type : 16;
    } dst__header_type;
} DST_HEADER;
#define DST_K_DST_HEADER_SIZE sizeof 4

/* Language type codes.  */
typedef enum _DST_LANGUAGE {DST_K_FORTRAN = 1, DST_K_C = 7, DST_K_ADA = 9,
			    DST_K_UNKNOWN = 10, DST_K_CXX = 15} DST_LANGUAGE;

/* Module header (a module is the result of a single compilation).  */

typedef struct _DST_MODULE_BEGIN
{
  DST_HEADER dst_a_modbeg_header;
  struct
    {
      unsigned dst_v_modbeg_hide : 1;
      unsigned dst_v_modbeg_version : 1;
      unsigned dst_v_modbeg_unused : 6;
    } dst_b_modbeg_flags;
  unsigned char dst_b_modbeg_unused;
  DST_LANGUAGE dst_l_modbeg_language;
  unsigned short int dst_w_version_major;
  unsigned short int dst_w_version_minor;
  unsigned char dst_b_modbeg_name;
} DST_MODULE_BEGIN;
#define DST_K_MODBEG_SIZE 15

/* Module trailer.  */

typedef struct _DST_MB_TRLR
{
  unsigned char dst_b_compiler;
} DST_MB_TRLR;

#define DST_K_MB_TRLR_SIZE 1

#define DST_K_VERSION_MAJOR 1
#define DST_K_VERSION_MINOR 13

typedef struct _DST_MODULE_END
{
  DST_HEADER dst_a_modend_header;
} DST_MODULE_END;
#define DST_K_MODEND_SIZE sizeof 4

/* Routine header.  */

typedef struct _DST_ROUTINE_BEGIN
{
  DST_HEADER dst_a_rtnbeg_header;
  struct
    {
      unsigned dst_v_rtnbeg_unused : 4;
      unsigned dst_v_rtnbeg_unalloc : 1;
      unsigned dst_v_rtnbeg_prototype : 1;
      unsigned dst_v_rtnbeg_inlined : 1;
      unsigned dst_v_rtnbeg_no_call : 1;
    } dst_b_rtnbeg_flags;
  int *dst_l_rtnbeg_address;
  int *dst_l_rtnbeg_pd_address;
  unsigned char dst_b_rtnbeg_name;
} DST_ROUTINE_BEGIN;
#define DST_K_RTNBEG_SIZE 14

/* Routine trailer */

typedef struct _DST_ROUTINE_END
{
  DST_HEADER dst_a_rtnend_header;
  char dst_b_rtnend_unused;
  unsigned int dst_l_rtnend_size;
} DST_ROUTINE_END;
#define DST_K_RTNEND_SIZE 9

/* Block header.  */

typedef struct _DST_BLOCK_BEGIN
{
  DST_HEADER dst_a_blkbeg_header;
  unsigned char dst_b_blkbeg_unused;
  int *dst_l_blkbeg_address;
  unsigned char dst_b_blkbeg_name;
} DST_BLOCK_BEGIN;
#define DST_K_BLKBEG_SIZE 10

/* Block trailer.  */

typedef struct _DST_BLOCK_END
{
  DST_HEADER dst_a_blkend_header;
  unsigned char dst_b_blkend_unused;
  unsigned int