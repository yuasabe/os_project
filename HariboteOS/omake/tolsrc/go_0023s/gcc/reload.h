/* Communication between reload.c and reload1.c.
   Copyright (C) 1987, 1991, 1992, 1993, 1994, 1995, 1997, 1998,
   1999, 2000, 2001 Free Software Foundation, Inc.

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


/* If secondary reloads are the same for inputs and outputs, define those
   macros here.  */

#ifdef SECONDARY_RELOAD_CLASS
#define SECONDARY_INPUT_RELOAD_CLASS(CLASS, MODE, X) ¥
  SECONDARY_RELOAD_CLASS (CLASS, MODE, X)
#define SECONDARY_OUTPUT_RELOAD_CLASS(CLASS, MODE, X) ¥
  SECONDARY_RELOAD_CLASS (CLASS, MODE, X)
#endif

/* If either macro is defined, show that we need secondary reloads.  */
#if defined(SECONDARY_INPUT_RELOAD_CLASS) || defined(SECONDARY_OUTPUT_RELOAD_CLASS)
#define HAVE_SECONDARY_RELOADS
#endif

/* If MEMORY_MOVE_COST isn't defined, give it a default here.  */
#ifndef MEMORY_MOVE_COST
#ifdef HAVE_SECONDARY_RELOADS
#define MEMORY_MOVE_COST(MODE,CLASS,IN) ¥
  (4 + memory_move_secondary_cost ((MODE), (CLASS), (IN)))
#else
#define MEMORY_MOVE_COST(MODE,CLASS,IN) 4
#endif
#endif
extern int memory_move_secondary_cost PARAMS ((enum machine_mode, enum reg_class, int));

/* Maximum number of reloads we can need.  */
#define MAX_RELOADS (2 * MAX_RECOG_OPERANDS * (MAX_REGS_PER_ADDRESS + 1))

/* Encode the usage of a reload.  The following codes are supported:

   RELOAD_FOR_INPUT		reload of an input operand
   RELOAD_FOR_OUTPUT		likewise, for output
   RELOAD_FOR_INSN		a reload that must not conflict with anything
				used in the insn, but may conflict with
				something used before or after the insn
   RELOAD_FOR_INPUT_ADDRESS	reload for parts of the address of an object
				that is an input reload
   RELOAD_FOR_INPADDR_ADDRESS	reload needed for RELOAD_FOR_INPUT_ADDRESS
   RELOAD_FOR_OUTPUT_ADDRESS	like RELOAD_FOR INPUT_ADDRESS, for output
   RELOAD_FOR_OUTADDR_ADDRESS	reload needed for RELOAD_FOR_OUTPUT_ADDRESS
   RELOAD_FOR_OPERAND_ADDRESS	reload for the address of a non-reloaded
				operand; these don't conflict with
				any other addresses.
   RELOAD_FOR_OPADDR_ADDR	reload needed for RELOAD_FOR_OPERAND_ADDRESS
                                reloads; usually secondary reloads
   RELOAD_OTHER			none of the above, usually multiple uses
   RELOAD_FOR_OTHER_ADDRESS     reload for part of the address of an input
   				that is marked RELOAD_OTHER.

   This used to be "enum reload_when_needed" but some debuggers have trouble
   with an enum tag and variable of the same name.  */

enum reload_type
{
  RELOAD_FOR_INPUT, RELOAD_FOR_OUTPUT, RELOAD_FOR_INSN, 
  RELOAD_FOR_INPUT_ADDRESS, RELOAD_FOR_INPADDR_ADDRESS,
  RELOAD_FOR_OUTPUT_ADDRESS, RELOAD_FOR_OUTADDR_ADDRESS,
  RELOAD_FOR_OPERAND_ADDRESS, RELOAD_FOR_OPADDR_ADDR,
  RELOAD_OTHER, RELOAD_FOR_OTHER_ADDRESS
};

#ifdef GCC_INSN_CODES_H
/* Each reload is recorded with a structure like this.  */
struct reload
{
  /* The value to reload from */
  rtx in;
  /* Where to store reload-reg afterward if nec (often the same as
     reload_in)  */
  rtx out;

  /* The class of registers to reload into.  */
  enum reg_class class;

  /* The mode this operand should have when reloaded, on input.  */
  enum machine_mode inmode;
  /* The mode this operand should have when reloaded, on output.  */
  enum machine_mode outmode;

  /* The mode of the reload register.  */
  enum machine_mode mode;

  /* the largest number of registers this rel