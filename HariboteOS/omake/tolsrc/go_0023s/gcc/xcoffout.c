/* Output xcoff-format symbol table information from GNU compiler.
   Copyright (C) 1992, 1994, 1995, 1997, 1998, 1999, 2000, 2002
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

/* Output xcoff-format symbol table data.  The main functionality is contained
   in dbxout.c.  This file implements the sdbout-like parts of the xcoff
   interface.  Many functions are very similar to their counterparts in
   sdbout.c.  */

#include "config.h"
#include "system.h"
#include "tree.h"
#include "rtl.h"
#include "flags.h"
#include "toplev.h"
#include "output.h"
#include "ggc.h"

#ifdef XCOFF_DEBUGGING_INFO

/* This defines the C_* storage classes.  */
#include "dbxstclass.h"
#include "xcoffout.h"
#include "dbxout.h"
#include "gstab.h"

/* Line number of beginning of current function, minus one.
   Negative means not in a function or not using xcoff.  */

static int xcoff_begin_function_line = -1;
static int xcoff_inlining = 0;

/* Name of the current include file.  */

const char *xcoff_current_include_file;

/* Name of the current function file.  This is the file the `.bf' is
   emitted from.  In case a line is emitted from a different file,
   (by including that file of course), then the line number will be
   absolute.  */

static const char *xcoff_current_function_file;

/* Names of bss and data sections.  These should be unique names for each
   compilation unit.  */

char *xcoff_bss_section_name;
char *xcoff_private_data_section_name;
char *xcoff_read_only_section_name;

/* Last source file name mentioned in a NOTE insn.  */

const char *xcoff_lastfile;

/* Macro definitions used below.  */

#define ABS_OR_RELATIVE_LINENO(LINENO)		¥
((xcoff_inlining) ? (LINENO) : (LINENO) - xcoff_begin_function_line)

/* Output source line numbers via ".line" rather than ".stabd".  */
#define ASM_OUTPUT_SOURCE_LINE(FILE,LINENUM) 				   ¥
  do									   ¥
    {									   ¥
      if (xcoff_begin_function_line >= 0)				   ¥
	fprintf (FILE, "¥t.line¥t%d¥n", ABS_OR_RELATIVE_LINENO (LINENUM)); ¥
    }									   ¥
  while (0)

#define ASM_OUTPUT_LFB(FILE,LINENUM) ¥
{						¥
  if (xcoff_begin_function_line == -1)		¥
    {						¥
      xcoff_begin_function_line = (LINENUM) - 1;¥
      fprintf (FILE, "¥t.bf¥t%d¥n", (LINENUM));	¥
    }						¥
  xcoff_current_function_file			¥
    = (xcoff_current_include_file		¥
       ? xcoff_current_include_file : main_input_filename); ¥
}

#define ASM_OUTPUT_LFE(FILE,LINENUM) 		¥
  do						¥
    {						¥
      fprintf (FILE, "¥t.ef¥t%d¥n", (LINENUM));	¥
      xcoff_begin_function_line = -1;		¥
    }						¥
  while (0)

#define ASM_OUTPUT_LBB(FILE,LINENUM,BLOCKNUM) ¥
  fprintf (FILE, "¥t.bb¥t%d¥n", ABS_OR_RELATIVE_LINENO (LINENUM))

#define ASM_OUTPUT_LBE(FILE,LINENUM,BLOCKNUM) ¥
  fprintf (FILE, "¥t.eb¥t%d¥n", ABS_OR_RELATIVE_LINENO (LINENUM))

static void assign_type_number		PARAMS ((tree, const char *, int));
static void xcoffout_block		PARAMS ((tree, int, tree));
static void xcoffout_source_file	PARAMS ((FILE *, const char *, int));

/* Support routines for XCOFF debugging info.  */

/* Assign NUMBER as the stabx type number for the type described by NAME.
   Search all decls in the list SYMS to find the type NAME.  */

static void
assign_type_number (syms, name, number)
     tree syms;
     const char *name;
     int number;
{
  tree decl;

  for (decl = syms; decl; decl = TREE_CHAIN (decl))
    if (DECL_NAME (decl)
	&& strcmp (IDENTIFIER_POINTER (DECL_NAME (decl)), name) == 0)
      {
	TREE_ASM_WRITTEN (decl) = 1;
	TYPE_SYMTAB_ADDRESS (TREE_TYPE (decl)) = number;
      }
}

/* Setup gcc primitive types to use the XCOFF built-in type numbers where
   possible.  */

void
xcoff_output_standard_types (syms)
     tree syms;
{
  /* Handle built-in C types here.  */

  assign_type_number (syms, "int", -1);
  assign_type_number (syms, "char", -2);
  assign_type_number (syms, "short int", -3);
  assign_type_number (syms, "long int", (TARGET_64BIT ? -31 : -4));
  assign_type_number (syms, "unsigned char", -5);
  assign_type_number (syms, "signed char", -6);
  assign_type_number (syms, "short unsigned int", -7);
  assign_type_number (syms, "unsigned int", -8);
  /* No such type "unsigned".  */
  assign_type_number (syms, "long unsigned int", (TARGET_64BIT ? -32 : -10));
  assign_type_number (syms, "void", -11);
  assign_type_number (syms, "float", -12);
  assign_type_number (syms, "double", -13);
  assign_type_number (syms, "long double", -14);
  /* Pascal and Fortran types run from -15 to -29.  */
  assign_type_number (syms, "wchar", -30);
  assign_type_number (syms, "long long int", -31);
  assign_type_number (syms, "long long unsigned int", -32);
  /* Additional Fortran types run from -33 to -37.  */

  /* ??? Should also handle built-in C++ and Obj-C types.  There perhaps
     aren't any that C doesn't already have.  */
}

/* Print an error message for unrecognized stab codes.  */

#define UNKNOWN_STAB(STR)	¥
  internal_error ("no sclass for %s stab (0x%x)¥n", STR, stab)

/* Conversion routine from BSD stabs to AIX storage classes.  */

int
stab_to_sclass (stab)
     int stab;
{
  switch (stab)
    {
    case N_GSYM:
      return C_GSYM;

    case N_FNAME:
      UNKNOWN_STAB ("N_FNAME");

    case N_FUN:
      return C_FUN;

    case N_STSYM:
    case N_LCSYM:
      return C_STSYM;

    case N_MAIN:
      UNKNOWN_STAB ("N_MAIN");

    case N_RSYM:
      return C_RSYM;

    case N_SSYM:
      UNKNOWN_STAB ("N_SSYM");

    case N_RPSYM:
      return C_RPSYM;

    case N_PSYM:
      return C_PSYM;
    case N_LSYM:
      return C_LSYM;
    case N_DECL:
      return C_DECL;
    case N_ENTRY:
      return C_ENTRY;

    case N_SO:
      UNKNOWN_STAB ("N_SO");

    case N_SOL:
      UNKNOWN_STAB ("N_SOL");

    case N_SLINE:
      UNKNOWN_STAB ("N_SLINE");

    case N_DSLINE:
      UNKNOWN_STAB ("N_DSLINE");

    case N_BSLINE:
      UNKNOWN_STAB ("N_BSLINE");

    case N_BINCL:
      UNKNOWN_STAB ("N_BINCL");

    case N_EINCL:
      UNKNOWN_STAB ("N_EINCL");

    case N_EXCL:
      UNKNOWN_STAB ("N_EXCL");

    case N_LBRAC:
      UNKNOWN_STAB ("N_LBRAC");

    case N_RBRAC:
      UNKNOWN_STAB ("N_RBRAC");

    case N_BCOMM:
      return C_BCOMM;
    case N_ECOMM:
      return C_ECOMM;
    case N_ECOML:
      return C_ECOML;

    case N_LENG:
      UNKNOWN_STAB ("N_LENG");

    case N_PC:
      UNKNOWN_STAB ("N_PC");

    case N_M2C:
      UNKNOWN_STAB ("N_M2C");

    case N_SCOPE:
      UNKNOWN_STAB ("N_SCOPE");

    case N_CATCH:
      UNKNOWN_STAB ("N_CATCH");

    case N_OPT:
      UNKNOWN_STAB ("N_OPT");

    default:
      UNKNOWN_STAB ("?");
    }
}

/* Output debugging info to FILE to switch to sourcefile FILENAME.
   INLINE_P is true if this is from an inlined function.  */

static void
xcoffout_source_file (file, filename, inline_p)
     FILE *file;
     const char *filename;
     int inline_p;
{
  if (filename
      && (xcoff_lastfile == 0 || strcmp (filename, xcoff_lastfile)
	  || (inline_p && ! xcoff_inlining)
	  || (! inline_p && xcoff_inlining)))
    {
      if (xcoff_current_include_file)
	{
	  fprintf (file, "¥t.ei¥t");
	  output_quoted_string (file, xcoff_current_include_file);
	  fprintf (file, "¥n");
	  xcoff_current_include_file = NULL;
	}
      xcoff_inlining = inline