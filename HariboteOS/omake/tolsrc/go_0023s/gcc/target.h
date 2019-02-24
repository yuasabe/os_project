/* Data structure definitions for a generic GCC target.
   Copyright (C) 2001, 2002 Free Software Foundation, Inc.

This program is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 2, or (at your option) any
later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

 In other words, you are welcome to use, share and improve this program.
 You are forbidden to forbid anyone else to use, share and improve
 what you give them.   Help stamp out software-hoarding!  */

/* This file contains a data structure that describes a GCC target.
   At present it is incomplete, but in future it should grow to
   contain most or all target machine and target O/S specific
   information.

   This structure has its initializer declared in target-def.h in the
   form of large macro TARGET_INITIALIZER that expands to many smaller
   macros.

   The smaller macros each initialize one component of the structure,
   and each has a default.  Each target should have a file that
   includes target.h and target-def.h, and overrides any inappropriate
   defaults by undefining the relevant macro and defining a suitable
   replacement.  That file should then contain the definition of
   "targetm" like so:

   struct gcc_target targetm = TARGET_INITIALIZER;

   Doing things this way allows us to bring together everything that
   defines a GCC target.  By supplying a default that is appropriate
   to most targets, we can easily add new items without needing to
   edit dozens of target configuration files.  It should also allow us
   to gradually reduce the amount of conditional compilation that is
   scattered throughout GCC.  */

struct gcc_target
{
  /* Functions that output assembler for the target.  */
  struct asm_out
  {
    /* Opening and closing parentheses for asm expression grouping.  */
    const char *open_paren, *close_paren;

    /* Assembler instructions for creating various kinds of integer object.  */
    const char *byte_op;
    struct asm_int_op
    {
      const char *hi;
      const char *si;
      const char *di;
      const char *ti;
    } aligned_op, unaligned_op;

    /* Try to output the assembler code for an integer object whose
       value is given by X.  SIZE is the size of the object in bytes and
       ALIGNED_P indicates whether it is aligned.  Return true if
       successful.  Only handles cases for which BYTE_OP, ALIGNED_OP
       and UNALIGNED_OP are NULL.  */
    bool (* integer) PARAMS ((rtx x, unsigned int size, int aligned_p));

    /* Output the assembler code for entry to a function.  */
    void (* function_prologue) PARAMS ((FILE *, HOST_WIDE_INT));

    /* Output the assembler code for end of prologue.  */
    void (* function_end_prologue) PARAMS ((FILE *));

    /* Output the assembler code for start of epilogue.  */
    void (* function_begin_epilogue) PARAMS ((FILE *));

    /* Output the assembler code for function exit.  */
    void (* function_epilogue) PARAMS ((FILE *, HOST_WIDE_INT));

    /* Switch to an arbitrary section NAME with attributes as
       specified by FLAGS.  */
    void (* named_section) PARAMS ((const char *, unsigned int));

    /* Switch to the section that holds the exception table.  */
    void (* exception_section) PARAMS ((void));

    /* Switch to the section that holds the exception frames.  */
    void (* eh_frame_section) PARAMS ((void));

    /* Output a constructor for a symbol with a given priority.  */
    void (* constructor) PARAMS ((rtx, int));

    /* Output a destructor for