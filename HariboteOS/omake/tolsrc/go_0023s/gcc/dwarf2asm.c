/* Dwarf2 assembler output helper routines.
   Copyright (C) 2001, 2002 Free Software Foundation, Inc.

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
#include "flags.h"
#include "tree.h"
#include "rtl.h"
#include "output.h"
#include "target.h"
#include "dwarf2asm.h"
#include "dwarf2.h"
#include "../include/splay-tree.h"
#include "ggc.h"
#include "tm_p.h"
/* end of !kawai! */

/* How to start an assembler comment.  */
#ifndef ASM_COMMENT_START
#define ASM_COMMENT_START ";#"
#endif


/* Output an unaligned integer with the given value and size.  Prefer not
   to print a newline, since the caller may want to add a comment.  */

void
dw2_assemble_integer (size, x)
     int size;
     rtx x;
{
  const char *op = integer_asm_op (size, FALSE);

  if (op)
    {
      fputs (op, asm_out_file);
      if (GET_CODE (x) == CONST_INT)
	fprintf (asm_out_file, HOST_WIDE_INT_PRINT_HEX, INTVAL (x));
      else
	output_addr_const (asm_out_file, x);
    }
  else
    assemble_integer (x, size, BITS_PER_UNIT, 1);
}
     

/* Output an immediate constant in a given size.  */

void
dw2_asm_output_data VPARAMS ((int size, unsigned HOST_WIDE_INT value,
			      const char *comment, ...))
{
  VA_OPEN (ap, comment);
  VA_FIXEDARG (ap, int, size);
  VA_FIXEDARG (ap, unsigned HOST_WIDE_INT, value);
  VA_FIXEDARG (ap, const char *, comment);

  if (size * 8 < HOST_BITS_PER_WIDE_INT)
    value &= ‾(‾(unsigned HOST_WIDE_INT) 0 << (size * 8));

  dw2_assemble_integer (size, GEN_INT (value));

  if (flag_debug_asm && comment)
    {
      fprintf (asm_out_file, "¥t%s ", ASM_COMMENT_START);
      vfprintf (asm_out_file, comment, ap);
    }
  fputc ('¥n', asm_out_file);

  VA_CLOSE (ap);
}

/* Output the difference between two symbols in a given size.  */
/* ??? There appear to be assemblers that do not like such
   subtraction, but do support ASM_SET_OP.  It's unfortunately
   impossible to do here, since the ASM_SET_OP for the difference
   symbol must appear after both symbols are defined.  */

void
dw2_asm_output_delta VPARAMS ((int size, const char *lab1, const char *lab2,
			       const char *comment, ...))
{
  VA_OPEN (ap, comment);
  VA_FIXEDARG (ap, int, size);
  VA_FIXEDARG (ap, const char *, lab1);
  VA_FIXEDARG (ap, const char *, lab2);
  VA_FIXEDARG (ap, const char *, comment);

  dw2_assemble_integer (size,
			gen_rtx_MINUS (Pmode,
				       gen_rtx_SYMBOL_REF (Pmode, lab1),
				       gen_rtx_SYMBOL_REF (Pmode, lab2)));

  if (flag_debug_asm && comment)
    {
      fprintf (asm_out_file, "¥t%s ", ASM_COMMENT_START);
      vfprintf (asm_out_file, comment, ap);
    }
  fputc ('¥n', asm_out_file);

  VA_CLOSE (ap);
}

/* Output a section-relative reference to a label.  In general this
   can only be done for debugging symbols.  E.g. on most targets with
   the GNU linker, this is accomplished with a direct reference and
   the knowledge that the debugging section will be placed at VMA 0.
   Some targets have special relocations for this that we must use.  */

void
dw2_asm_output_offset VPARAMS ((int size, const char *label,
			       const char *comment, ...))
{
  VA_OPEN (ap, comment);
  VA_FIXEDARG (ap, int, size);
  VA_FIXEDARG (ap, const char *, label);
  VA_FIXEDARG (ap, const char *, comment);

#ifdef ASM_OUTPUT_DWARF_OFFSET
