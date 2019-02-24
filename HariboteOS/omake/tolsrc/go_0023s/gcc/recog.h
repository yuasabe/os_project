/* Declarations for interface to insn recognizer and insn-output.c.
   Copyright (C) 1987, 1996, 1997, 1998, 1999, 2000, 2001
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

/* Random number that should be large enough for all purposes.  */
#define MAX_RECOG_ALTERNATIVES 30
#define recog_memoized(I) (INSN_CODE (I) >= 0 ¥
			   ? INSN_CODE (I) : recog_memoized_1 (I))

/* Types of operands.  */
enum op_type {
  OP_IN,
  OP_OUT,
  OP_INOUT
};

struct operand_alternative
{
  /* Pointer to the beginning of the constraint string for this alternative,
     for easier access by alternative number.  */
  const char *constraint;

  /* The register class valid for this alternative (possibly NO_REGS).  */
  enum reg_class class;

  /* "Badness" of this alternative, computed from number of '?' and '!'
     characters in the constraint string.  */
  unsigned int reject;

  /* -1 if no matching constraint was found, or an operand number.  */
  int matches;
  /* The same information, but reversed: -1 if this operand is not
     matched by any other, or the operand number of the operand that
     matches this one.  */
  int matched;

  /* Nonzero if '&' was found in the constraint string.  */
  unsigned int earlyclobber:1;
  /* Nonzero if 'm' was found in the constraint string.  */
  unsigned int memory_ok:1;  
  /* Nonzero if 'o' was found in the constraint string.  */
  unsigned int offmem_ok:1;  
  /* Nonzero if 'V' was found in the constraint string.  */
  unsigned int nonoffmem_ok:1;
  /* Nonzero if '<' was found in the constraint string.  */
  unsigned int decmem_ok:1;
  /* Nonzero if '>' was found in the constraint string.  */
  unsigned int incmem_ok:1;
  /* Nonzero if 'p' was found in the constraint string.  */
  unsigned int is_address:1;
  /* Nonzero if 'X' was found in the constraint string, or if the constraint
     string for this alternative was empty.  */
  unsigned int anything_ok:1;
};


extern void init_recog			PARAMS ((void));
extern void init_recog_no_volatile	PARAMS ((void));
extern int recog_memoized_1		PARAMS ((rtx));
extern int check_asm_operands		PARAMS ((rtx));
extern int asm_operand_ok		PARAMS ((rtx, const char *));
extern int validate_change		PARAMS ((rtx, rtx *, rtx, int));
extern int insn_invalid_p		PARAMS ((rtx));
extern int apply_change_group		PARAMS ((void));
extern int num_validated_changes	PARAMS ((void));
extern void cancel_changes		PARAMS ((int));
extern int constrain_operands		PARAMS ((int));
extern int constrain_operands_cached	PARAMS ((int));
extern int memory_address_p		PARAMS ((enum machine_mode, rtx));
extern int strict_memory_address_p	PARAMS ((enum machine_mode, rtx));
extern int validate_replace_rtx_subexp	PARAMS ((rtx, rtx, rtx, rtx *));
extern int validate_replace_rtx		PARAMS ((rtx, rtx, rtx));
extern void validate_replace_rtx_group	PARAMS ((rtx, rtx, rtx));
extern int validate_replace_src		PARAMS ((rtx, rtx, rtx));
#ifdef HAVE_cc0
extern int next_insn_tests_no_inequality PARAMS ((rtx));
#endif
extern int reg_fits_class_p		PARAMS ((rtx, enum reg_class, int,
					       enum machine_mode));
extern rtx *find_single_use		PARAMS ((rtx, rtx, rtx *));

extern int general_operand		PARAMS ((rtx, enum machine_mode));
extern int address_operand		PARAMS ((rtx, enum machine_mode));
extern int register_operand		PARAMS ((rtx, enum machine_mode));