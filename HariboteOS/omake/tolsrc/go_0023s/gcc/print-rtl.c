/* Print RTL for GNU C Compiler.
   Copyright (C) 1987, 1988, 1992, 1997, 1998, 1999, 2000
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

#define GENERATOR_FILE	1

#include "config.h"
#include "system.h"
#include "rtl.h"

/* We don't want the tree code checking code for the access to the
   DECL_NAME to be included in the gen* programs.  */
#undef ENABLE_TREE_CHECKING
#include "tree.h"
#include "real.h"
#include "flags.h"
#include "hard-reg-set.h"
#include "basic-block.h"

/* How to print out a register name.
   We don't use PRINT_REG because some definitions of PRINT_REG
   don't work here.  */
#ifndef DEBUG_PRINT_REG
#define DEBUG_PRINT_REG(RTX, CODE, FILE) ¥
  fprintf ((FILE), "%d %s", REGNO (RTX), reg_names[REGNO (RTX)])
#endif

/* Array containing all of the register names */

#ifdef DEBUG_REGISTER_NAMES
static const char * const debug_reg_names[] = DEBUG_REGISTER_NAMES;
#define reg_names debug_reg_names
#else
const char * reg_names[] = REGISTER_NAMES;
#endif

static FILE *outfile;

static int sawclose = 0;

static int indent;

static void print_rtx		PARAMS ((rtx));

/* String printed at beginning of each RTL when it is dumped.
   This string is set to ASM_COMMENT_START when the RTL is dumped in
   the assembly output file.  */
const char *print_rtx_head = "";

/* Nonzero means suppress output of instruction numbers and line number
   notes in debugging dumps.
   This must be defined here so that programs like gencodes can be linked.  */
int flag_dump_unnumbered = 0;

/* Nonzero means use simplified format without flags, modes, etc.  */
int flag_simple = 0;

/* Nonzero if we are dumping graphical description.  */
int dump_for_graph;

/* Nonzero to dump all call_placeholder alternatives.  */
static int debug_call_placeholder_verbose;

void
print_mem_expr (outfile, expr)
     FILE *outfile;
     tree expr;
{
  if (TREE_CODE (expr) == COMPONENT_REF)
    {
      if (TREE_OPERAND (expr, 0))
        print_mem_expr (outfile, TREE_OPERAND (expr, 0));
      else
	fputs (" <variable>", outfile);
      if (DECL_NAME (TREE_OPERAND (expr, 1)))
	fprintf (outfile, ".%s",
		 IDENTIFIER_POINTER (DECL_NAME (TREE_OPERAND (expr, 1))));
    }
  else if (DECL_NAME (expr))
    fprintf (outfile, " %s", IDENTIFIER_POINTER (DECL_NAME (expr)));
  else if (TREE_CODE (expr) == RESULT_DECL)
    fputs (" <result>", outfile);
  else
    fputs (" <anonymous>", outfile);
}

/* Print IN_RTX onto OUTFILE.  This is the recursive part of printing.  */

static void
print_rtx (in_rtx)
     rtx in_rtx;
{
  int i = 0;
  int j;
  const char *format_ptr;
  int is_insn;
  rtx tem;

  if (sawclose)
    {
      if (flag_simple)
	fputc (' ', outfile);
      else
	fprintf (outfile, "¥n%s%*s", print_rtx_head, indent * 2, "");
      sawclose = 0;
    }

  if (in_rtx == 0)
    {
      fputs ("(nil)", outfile);
      sawclose = 1;
      return;
    }
  else if (GET_CODE (in_rtx) > NUM_RTX_CODE)
    {
       fprintf (outfile, "(??? bad code %d¥n)", GET_CODE (in_rtx));
       sawclose = 1;
       return;
    }

  is_insn = INSN_P (in_rtx);

  /* When printing in VCG format we write INSNs, NOTE, LABEL, and BARRIER
     in separate nodes and therefore have to handle them special here.  */
  if (dump_for_graph
      && (is_insn || GET_CODE (in_rtx) == NOTE
	  || GET_CODE (i