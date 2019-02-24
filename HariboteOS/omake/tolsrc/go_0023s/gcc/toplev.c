/* Top level of GNU C compiler
   Copyright (C) 1987, 1988, 1989, 1992, 1993, 1994, 1995, 1996, 1997, 1998,
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

/* This is the top level of cc1/c++.
   It parses command args, opens files, invokes the various passes
   in the proper order, and counts the time used by each.
   Error messages and low-level interface to malloc also handled here.  */

#define TARGET_NAME	"i586-pc-cygwin"

#include "config.h"
#undef FLOAT /* This is for hpux. They should change hpux.  */
#undef FFS  /* Some systems define this in param.h.  */
#include "system.h"
#include "../include/setjmp.h"

/* !kawai! */
#undef HAVE_SYS_RESOURCE_H
#undef HAVE_SYS_TIMES_H
#define SIGFPE		8 /* FPU error */
/* end of !kawai! */

#include "input.h"
#include "tree.h"
#include "rtl.h"
#include "tm_p.h"
#include "flags.h"
#include "insn-attr.h"
#include "insn-config.h"
#include "insn-flags.h"
#include "hard-reg-set.h"
#include "recog.h"
#include "output.h"
#include "except.h"
#include "function.h"
#include "toplev.h"
#include "expr.h"
#include "basic-block.h"
#include "intl.h"
#include "ggc.h"
#include "graph.h"
#include "loop.h"
#include "regs.h"
#include "timevar.h"
#include "diagnostic.h"
#include "ssa.h"
#include "params.h"
#include "reload.h"
#include "dwarf2asm.h"
#include "integrate.h"
#include "debug.h"
#include "target.h"
#include "langhooks.h"

#if defined (DWARF2_UNWIND_INFO) || defined (DWARF2_DEBUGGING_INFO)
#include "dwarf2out.h"
#endif

#if defined(DBX_DEBUGGING_INFO) || defined(XCOFF_DEBUGGING_INFO)
#include "dbxout.h"
#endif

#ifdef SDB_DEBUGGING_INFO
#include "sdbout.h"
#endif

#ifdef XCOFF_DEBUGGING_INFO
#include "xcoffout.h"		/* Needed for external data
				   declarations for e.g. AIX 4.x.  */
#endif

#ifdef HALF_PIC_DEBUG
#include "halfpic.h"
#endif

/* Carry information from ASM_DECLARE_OBJECT_NAME
   to ASM_FINISH_DECLARE_OBJECT.  */

extern int size_directive_output;
extern tree last_assemble_variable_decl;

static void general_init PARAMS ((char *));
static void parse_options_and_default_flags PARAMS ((int, char **));
static void do_compile PARAMS ((void));
static void process_options PARAMS ((void));
static void lang_independent_init PARAMS ((void));
static int lang_dependent_init PARAMS ((const char *));
static void init_asm_output PARAMS ((const char *));
static void finalize PARAMS ((void));

static void set_target_switch PARAMS ((const char *));
static const char *decl_name PARAMS ((tree, int));

static void float_signal PARAMS ((int)) ATTRIBUTE_NORETURN;
static void crash_signal PARAMS ((int)) ATTRIBUTE_NORETURN;
static void set_float_handler PARAMS ((jmp_buf));
static void compile_file PARAMS ((void));
static void display_help PARAMS ((void));
static void display_target_options PARAMS ((void));

static void decode_d_option PARAMS ((const char *));
static int decode_f_option PARAMS ((const char *));
static int decode_W_option PARAMS ((const char *));
static int decode_g_option PARAMS ((const char *));
static unsigned int independent_decode_option PARAMS ((int, char **));

static void print_version PARAMS ((FILE *, const char *));
static int print_single_switch PARAMS ((FILE *, int, int, const char *,
				      const char *, const char *,
				      const char *, const char *));
stati