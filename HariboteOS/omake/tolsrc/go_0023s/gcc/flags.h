/* Compilation switch flag definitions for GCC.
   Copyright (C) 1987, 1988, 1994, 1995, 1996, 1997, 1998, 1999, 2000
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

#ifndef GCC_FLAGS_H
#define GCC_FLAGS_H

/* Name of the input .c file being compiled.  */
extern const char *main_input_filename;

enum debug_info_type
{
  NO_DEBUG,	    /* Write no debug info.  */
  DBX_DEBUG,	    /* Write BSD .stabs for DBX (using dbxout.c).  */
  SDB_DEBUG,	    /* Write COFF for (old) SDB (using sdbout.c).  */
  DWARF_DEBUG,	    /* Write Dwarf debug info (using dwarfout.c).  */
  DWARF2_DEBUG,	    /* Write Dwarf v2 debug info (using dwarf2out.c).  */
  XCOFF_DEBUG,	    /* Write IBM/Xcoff debug info (using dbxout.c).  */
  VMS_DEBUG,        /* Write VMS debug info (using vmsdbgout.c).  */
  VMS_AND_DWARF2_DEBUG /* Write VMS debug info (using vmsdbgout.c).
                          and DWARF v2 debug info (using dwarf2out.c).  */
};

/* Specify which kind of debugging info to generate.  */
extern enum debug_info_type write_symbols;

enum debug_info_level
{
  DINFO_LEVEL_NONE,	/* Write no debugging info.  */
  DINFO_LEVEL_TERSE,	/* Write minimal info to support tracebacks only.  */
  DINFO_LEVEL_NORMAL,	/* Write info for all declarations (and line table).  */
  DINFO_LEVEL_VERBOSE	/* Write normal info plus #define/#undef info.  */
};

/* Specify how much debugging info to generate.  */
extern enum debug_info_level debug_info_level;

/* Nonzero means use GNU-only extensions in the generated symbolic
   debugging information.  */
extern int use_gnu_debug_info_extensions;

/* Nonzero means do optimizations.  -opt.  */

extern int optimize;

/* Nonzero means optimize for size.  -Os.  */

extern int optimize_size;

/* Don't print functions as they are compiled and don't print
   times taken by the various passes.  -quiet.  */

extern int quiet_flag;

/* Print times taken by the various passes.  -ftime-report.  */

extern int time_report;

/* Print memory still in use at end of compilation (which may have little
   to do with peak memory consumption).  -fmem-report.  */

extern int mem_report;

/* Don't print warning messages.  -w.  */

extern int inhibit_warnings;

/* Don't suppress warnings from system headers.  -Wsystem-headers.  */

extern int warn_system_headers;

/* Do print extra warnings (such as for uninitialized variables).  -W.  */

extern int extra_warnings;

/* Nonzero to warn about unused variables, functions et.al.  Use
   set_Wunused() to update the -Wunused-* flags that correspond to the
   -Wunused option.  */

extern void set_Wunused PARAMS ((int setting));

extern int warn_unused_function;
extern int warn_unused_label;
extern int warn_unused_parameter;
extern int warn_unused_variable;
extern int warn_unused_value;

/* Nonzero to warn about code which is never reached.  */

extern int warn_notreached;

/* Nonzero means warn if inline function is too large.  */

extern int warn_inline;

/* Nonzero to warn about variables used before they are initialized.  */

extern int warn_uninitialized;

/* Zero if unknown pragmas are ignored
   One if the compiler should warn about an unknown pragma not in
   a system include file.
   Greater than one if the compiler should warn for all unknown
   pragmas.  */

extern int warn_unknown_pragmas;

/* Nonzero means w