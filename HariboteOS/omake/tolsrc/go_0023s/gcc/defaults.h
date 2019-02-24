/* Definitions of various defaults for tm.h macros.
   Copyright (C) 1992, 1996, 1997, 1998, 1999, 2000, 2001, 2002
   Free Software Foundation, Inc.
   Contributed by Ron Guilmette (rfg@monkeys.com)

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

#ifndef GCC_DEFAULTS_H
#define GCC_DEFAULTS_H

/* Define default standard character escape sequences.  */
#ifndef TARGET_BELL
#  define TARGET_BELL 007
#  define TARGET_BS 010
#  define TARGET_TAB 011
#  define TARGET_NEWLINE 012
#  define TARGET_VT 013
#  define TARGET_FF 014
#  define TARGET_CR 015
#  define TARGET_ESC 033
#endif

/* Store in OUTPUT a string (made with alloca) containing
   an assembler-name for a local static variable or function named NAME.
   LABELNO is an integer which is different for each call.  */

#ifndef ASM_FORMAT_PRIVATE_NAME
#define ASM_FORMAT_PRIVATE_NAME(OUTPUT, NAME, LABELNO)			¥
  do {									¥
    int len = strlen (NAME);						¥
    char *temp = (char *) alloca (len + 3);				¥
    temp[0] = 'L';							¥
    strcpy (&temp[1], (NAME));						¥
    temp[len + 1] = '.';						¥
    temp[len + 2] = 0;							¥
    (OUTPUT) = (char *) alloca (strlen (NAME) + 11);			¥
    ASM_GENERATE_INTERNAL_LABEL (OUTPUT, temp, LABELNO);		¥
  } while (0)
#endif

#ifndef ASM_STABD_OP
#define ASM_STABD_OP "¥t.stabd¥t"
#endif

/* This is how to output an element of a case-vector that is absolute.
   Some targets don't use this, but we have to define it anyway.  */

#ifndef ASM_OUTPUT_ADDR_VEC_ELT
#define ASM_OUTPUT_ADDR_VEC_ELT(FILE, VALUE)  ¥
do { fputs (integer_asm_op (POINTER_SIZE / UNITS_PER_WORD, TRUE), FILE); ¥
     ASM_OUTPUT_INTERNAL_LABEL (FILE, "L", (VALUE));			¥
     fputc ('¥n', FILE);						¥
   } while (0)
#endif

/* Provide default for ASM_OUTPUT_ALTERNATE_LABEL_NAME.  */
#ifndef ASM_OUTPUT_ALTERNATE_LABEL_NAME
#define ASM_OUTPUT_ALTERNATE_LABEL_NAME(FILE,INSN) ¥
do { ASM_OUTPUT_LABEL(FILE,LABEL_ALTERNATE_NAME (INSN)); } while (0)
#endif

/* choose a reasonable default for ASM_OUTPUT_ASCII.  */

#ifndef ASM_OUTPUT_ASCII
#define ASM_OUTPUT_ASCII(MYFILE, MYSTRING, MYLENGTH) ¥
  do {									      ¥
    FILE *_hide_asm_out_file = (MYFILE);				      ¥
    const unsigned char *_hide_p = (const unsigned char *) (MYSTRING);	      ¥
    int _hide_thissize = (MYLENGTH);					      ¥
    {									      ¥
      FILE *asm_out_file = _hide_asm_out_file;				      ¥
      const unsigned char *p = _hide_p;					      ¥
      int thissize = _hide_thissize;					      ¥
      int i;								      ¥
      fprintf (asm_out_file, "¥t.ascii ¥"");				      ¥
									      ¥
      for (i = 0; i < thissize; i++)					      ¥
	{								      ¥
	  int c = p[i];			   				      ¥
	  if (c == '¥"' || c == '¥¥')					      ¥
	    putc ('¥¥', asm_out_file);					      ¥
	  if (ISPRINT(c))						      ¥
	    putc (c, asm_out_file);					      ¥
	  else								      ¥
	    {								      ¥
	      fprintf (asm_out_file, "¥¥%o", c);			      ¥
	      /* After an octal-escape, if a digit follows,		      ¥
		 terminate one string constant and start another.	      ¥
		 The VAX assembler fails to stop reading the escape	      ¥
		 after three digits, so this is the only way we		      ¥
		 can get it to parse the data properly.  */		      ¥
	      if (i < thissize - 1 && ISDIGIT(p[i + 1]))		      ¥
		fprintf (asm_out_file, "¥"¥n¥t.ascii ¥"");		      ¥
	  }								      ¥