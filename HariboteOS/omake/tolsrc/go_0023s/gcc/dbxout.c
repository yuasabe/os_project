/* Output dbx-format symbol table information from GNU compiler.
   Copyright (C) 1987, 1988, 1992, 1993, 1994, 1995, 1996, 1997, 1998,
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


/* Output dbx-format symbol table data.
   This consists of many symbol table entries, each of them
   a .stabs assembler pseudo-op with four operands:
   a "name" which is really a description of one symbol and its type,
   a "code", which is a symbol defined in stab.h whose name starts with N_,
   an unused operand always 0,
   and a "value" which is an address or an offset.
   The name is enclosed in doublequote characters.

   Each function, variable, typedef, and structure tag
   has a symbol table entry to define it.
   The beginning and end of each level of name scoping within
   a function are also marked by special symbol table entries.

   The "name" consists of the symbol name, a colon, a kind-of-symbol letter,
   and a data type number.  The data type number may be followed by
   "=" and a type definition; normally this will happen the first time
   the type number is mentioned.  The type definition may refer to
   other types by number, and those type numbers may be followed
   by "=" and nested definitions.

   This can make the "name" quite long.
   When a name is more than 80 characters, we split the .stabs pseudo-op
   into two .stabs pseudo-ops, both sharing the same "code" and "value".
   The first one is marked as continued with a double-backslash at the
   end of its "name".

   The kind-of-symbol letter distinguished function names from global
   variables from file-scope variables from parameters from auto
   variables in memory from typedef names from register variables.
   See `dbxout_symbol'.

   The "code" is mostly redundant with the kind-of-symbol letter
   that goes in the "name", but not entirely: for symbols located
   in static storage, the "code" says which segment the address is in,
   which controls how it is relocated.

   The "value" for a symbol in static storage
   is the core address of the symbol (actually, the assembler
   label for the symbol).  For a symbol located in a stack slot
   it is the stack offset; for one in a register, the register number.
   For a typedef symbol, it is zero.

   If DEBUG_SYMS_TEXT is defined, all debugging symbols must be
   output while in the text section.

   For more on data type definitions, see `dbxout_type'.  */

#include "config.h"
#include "system.h"

#include "tree.h"
#include "rtl.h"
#include "flags.h"
#include "regs.h"
#include "insn-config.h"
#include "reload.h"
#include "output.h" /* ASM_OUTPUT_SOURCE_LINE may refer to sdb functions.  */
#include "dbxout.h"
#include "toplev.h"
#include "tm_p.h"
#include "ggc.h"
#include "debug.h"
#include "function.h"
#include "target.h"
#include "langhooks.h"

#ifdef XCOFF_DEBUGGING_INFO
#include "xcoffout.h"
#endif

#ifndef ASM_STABS_OP
#define ASM_STABS_OP "¥t.stabs¥t"
#endif

#ifndef ASM_STABN_OP
#define ASM_STABN_OP "¥t.stabn¥t"
#endif

#ifndef DBX_TYPE_DECL_STABS_CODE
#define DBX_TYPE_DECL_STABS_CODE N_LSYM
#endif

#ifndef DBX_STATIC_CONST_VAR_CODE
#define DBX_STATIC_CONST_VAR_CODE N_FUN
#endif

#ifndef DBX_REGPARM_STABS_CODE
#define DBX_REGPARM_STABS_CODE N_RSYM
#endif

#ifndef DBX_REGPARM_STABS_LETTER
#define DBX_REGPARM_STAB