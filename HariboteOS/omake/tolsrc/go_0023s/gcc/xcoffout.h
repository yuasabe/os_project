/* XCOFF definitions.  These are needed in dbxout.c, final.c,
   and xcoffout.h. 
   Copyright (C) 1998, 2000 Free Software Foundation, Inc.

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


#define ASM_STABS_OP "¥t.stabx¥t"

/* Tags and typedefs are C_DECL in XCOFF, not C_LSYM.  */

#define DBX_TYPE_DECL_STABS_CODE N_DECL

/* Use the XCOFF predefined type numbers.  */

/* ??? According to metin, typedef stabx must go in text control section,
   but he did not make this changes everywhere where such typedef stabx
   can be emitted, so it is really needed or not?  */

#define DBX_OUTPUT_STANDARD_TYPES(SYMS)		¥
{						¥
  text_section ();				¥
  xcoff_output_standard_types (SYMS);		¥
}

/* Any type with a negative type index has already been output.  */

#define DBX_TYPE_DEFINED(TYPE) (TYPE_SYMTAB_ADDRESS (TYPE) < 0)

/* Must use N_STSYM for static const variables (those in the text section)
   instead of N_FUN.  */

#define DBX_STATIC_CONST_VAR_CODE N_STSYM

/* For static variables, output code to define the start of a static block.

   ??? The IBM rs6000/AIX assembler has a bug that causes bss block debug
   info to be occasionally lost.  A simple example is this:
	int a; static int b;
   The commands `gcc -g -c tmp.c; dump -t tmp.o' gives
[10]	m   0x00000016         1     0    0x8f  0x0000            .bs
[11]	m   0x00000000         1     0    0x90  0x0000            .es
...
[21]	m   0x00000000        -2     0    0x85  0x0000            b:S-1
   which is wrong.  The `b:S-1' must be between the `.bs' and `.es'.
   We can apparently work around the problem by forcing the text section
   (even if we are already in the text section) immediately before outputting
   the `.bs'.  This should be fixed in the next major AIX release (3.3?).  */

#define DBX_STATIC_BLOCK_START(ASMFILE,CODE)				¥
{									¥
  if ((CODE) == N_STSYM)						¥
    fprintf ((ASMFILE), "¥t.bs¥t%s[RW]¥n", xcoff_private_data_section_name);¥
  else if ((CODE) == N_LCSYM)						¥
    {									¥
      fprintf ((ASMFILE), "%s¥n", TEXT_SECTION_ASM_OP);			¥
      fprintf ((ASMFILE), "¥t.bs¥t%s¥n", xcoff_bss_section_name);	¥
    }									¥
}

/* For static variables, output code to define the end of a static block.  */

#define DBX_STATIC_BLOCK_END(ASMFILE,CODE)				¥
{									¥
  if ((CODE) == N_STSYM || (CODE) == N_LCSYM)				¥
    fputs ("¥t.es¥n", (ASMFILE));					¥
}

/* We must use N_RPYSM instead of N_RSYM for register parameters.  */

#define DBX_REGPARM_STABS_CODE N_RPSYM

/* We must use 'R' instead of 'P' for register parameters.  */

#define DBX_REGPARM_STABS_LETTER 'R'

/* Define our own finish symbol function, since xcoff stabs have their
   own different format.  */

#define DBX_FINISH_SYMBOL(SYM)					¥
{								¥
  if (current_sym_addr && current_sym_code == N_FUN)		¥
    fprintf (asmfile, "¥",.");					¥
  else								¥
    fprintf (asmfile, "¥",");					¥
  /* If we are writing a function name, we must ensure that	¥
     there is no storage-class suffix on the name.  */		¥
  if (current_sym_addr && current_sym_code == N_FUN		¥
      && GET_CODE (current_sym_addr) == SYMBOL_REF)		¥
    {								¥
      const char *_p = XSTR (current_sym_addr, 0);		¥
      if (*_p == '*')						¥
	fprintf (asmfile, "%s", _p+1);				¥
      else							¥
        for (; *_p != '[' && *_p; _p++)				¥
	  fprintf (asmfile, "%c", *_p);				¥
    