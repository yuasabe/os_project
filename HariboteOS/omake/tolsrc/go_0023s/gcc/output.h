/* Declarations for insn-output.c.  These functions are defined in recog.c,
   final.c, and varasm.c.
   Copyright (C) 1987, 1991, 1994, 1997, 1998,
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

/* Compute branch alignments based on frequency information in the CFG.  */
extern void compute_alignments  PARAMS ((void));

/* Initialize data in final at the beginning of a compilation.  */
extern void init_final		PARAMS ((const char *));

/* Called at end of source file,
   to output the block-profiling table for this entire compilation.  */
extern void end_final		PARAMS ((const char *));

/* Enable APP processing of subsequent output.
   Used before the output from an `asm' statement.  */
extern void app_enable		PARAMS ((void));

/* Disable APP processing of subsequent output.
   Called from varasm.c before most kinds of output.  */
extern void app_disable		PARAMS ((void));

/* Return the number of slots filled in the current
   delayed branch sequence (we don't count the insn needing the
   delay slot).   Zero if not in a delayed branch sequence.  */
extern int dbr_sequence_length	PARAMS ((void));

/* Indicate that branch shortening hasn't yet been done.  */
extern void init_insn_lengths	PARAMS ((void));

#ifdef RTX_CODE
/* Obtain the current length of an insn.  If branch shortening has been done,
   get its actual length.  Otherwise, get its maximum length.  */
extern int get_attr_length	PARAMS ((rtx));

/* Make a pass over all insns and compute their actual lengths by shortening
   any branches of variable length if possible.  */
extern void shorten_branches	PARAMS ((rtx));

/* Output assembler code for the start of a function,
   and initialize some of the variables in this file
   for the new function.  The label for the function and associated
   assembler pseudo-ops have already been output in
   `assemble_start_function'.  */
extern void final_start_function  PARAMS ((rtx, FILE *, int));

/* Output assembler code for the end of a function.
   For clarity, args are same as those of `final_start_function'
   even though not all of them are needed.  */
extern void final_end_function  PARAMS ((void));

/* Output assembler code for some insns: all or part of a function.  */
extern void final		PARAMS ((rtx, FILE *, int, int));

/* The final scan for one insn, INSN.  Args are same as in `final', except
   that INSN is the insn being scanned.  Value returned is the next insn to
   be scanned.  */
extern rtx final_scan_insn	PARAMS ((rtx, FILE *, int, int, int));

/* Replace a SUBREG with a REG or a MEM, based on the thing it is a
   subreg of.  */
extern rtx alter_subreg PARAMS ((rtx *));

/* Report inconsistency between the assembler template and the operands.
   In an `asm', it's the user's fault; otherwise, the compiler's fault.  */
extern void output_operand_lossage  PARAMS ((const char *, ...)) ATTRIBUTE_PRINTF_1;

/* Output a string of assembler code, substituting insn operands.
   Defined in final.c.  */
extern void output_asm_insn	PARAMS ((const char *, rtx *));

/* Compute a worst-case reference address of a branch so that it
   can be safely used in the presence of aligned labels.
   Defined in final.c.  */
extern int insn_current_reference_address	PARAMS ((rtx));

/* Find the alignment associated with a CODE_LABEL.
   Defined in final.c.  