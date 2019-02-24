/* Operating system specific defines to be used when targeting GCC for
   hosting on Windows32, using a Unix style C library and tools.
   Copyright (C) 1995, 1996, 1997, 1998, 1999, 2000, 2001, 2002
   Free Software Foundation, Inc.

This file is part of GNU CC.

GNU CC is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

GNU CC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU CC; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */

#define YES_UNDERSCORES

#define DBX_DEBUGGING_INFO 
#define SDB_DEBUGGING_INFO 
#define PREFERRED_DEBUGGING_TYPE DBX_DEBUG

#define TARGET_EXECUTABLE_SUFFIX ".exe"

/* !kawai! */
#include "../include/stdio.h"
#include "gas.h"
#include "dbxcoff.h"
/* end of !kawai! */

/* Masks for subtarget switches used by other files.  */
#define MASK_NOP_FUN_DLLIMPORT 0x08000000 /* Ignore dllimport for functions */
#define MASK_MS_BITFIELD_LAYOUT 0x10000000 /* Use MS bitfield layout */

/* Used in winnt.c.  */
#define TARGET_NOP_FUN_DLLIMPORT (target_flags & MASK_NOP_FUN_DLLIMPORT)
/* Tell i386.c to put a target-specific specialization of
   ms_bitfield_layout_p in struct gcc_target targetm.  */
#define TARGET_USE_MS_BITFIELD_LAYOUT  ¥
  (target_flags & MASK_MS_BITFIELD_LAYOUT)	

#undef  SUBTARGET_SWITCHES
#define SUBTARGET_SWITCHES ¥
{ "no-cygwin",		  0, N_("Use the Mingw32 interface") },	¥
{ "cygwin",		  0, N_("Use the Cygwin interface") },	¥
{ "no-win32",		  0, N_("Don't set Windows defines") },	¥
{ "win32",		  0, N_("Set Windows defines") },	¥
{ "console",		  0, N_("Create console application") },¥
{ "windows",		  0, N_("Create GUI application") },	¥
{ "dll",		  0, N_("Generate code for a DLL") },	¥
{ "nop-fun-dllimport",	  MASK_NOP_FUN_DLLIMPORT,		¥
  N_("Ignore dllimport for functions") }, 			¥
{ "no-nop-fun-dllimport", -MASK_NOP_FUN_DLLIMPORT, "" },	¥
{ "threads",		  0, N_("Use Mingw-specific thread support") },	¥
{ "no-ms-bitfields",	  -MASK_MS_BITFIELD_LAYOUT,		¥
  N_("Don't use MS bitfield layout") },				¥
{ "ms-bitfields",	  MASK_MS_BITFIELD_LAYOUT,		¥
  N_("Use MS bitfield layout") },

/* Get tree.c to declare a target-specific specialization of
   merge_decl_attributes.  */
#define TARGET_DLLIMPORT_DECL_ATTRIBUTES

#undef ENDFILE_SPEC
#define ENDFILE_SPEC "crtend%O%s"

/* This macro defines names of additional specifications to put in the specs
   that can be used in various specifications like CC1_SPEC.  Its definition
   is an initializer with a subgrouping for each command option.

   Each subgrouping contains a string constant, that defines the
   specification name, and a string constant that used by the GNU CC driver
   program.

   Do not define this macro if it does not need to do anything.  */

#undef  SUBTARGET_EXTRA_SPECS
#define SUBTARGET_EXTRA_SPECS ¥
  { "mingw_include_path", DEFAULT_TARGET_MACHINE }

#undef MATH_LIBRARY
#define MATH_LIBRARY ""

#define SIZE_TYPE "unsigned int"
#define PTRDIFF_TYPE "int"
#define WCHAR_UNSIGNED 1
#define WCHAR_TYPE_SIZE 16
#define WCHAR_TYPE "short unsigned int"

/* Enable parsing of #pragma pack(push,<n>) and #pragma pack(pop).  */
#define HANDLE_PRAGMA_PACK_PUSH_POP 1

union tree_node;
#define TREE union tree_node *

/* Used to implement dllexport overriding dllimport semantics.  It's also used
   to handle vtables - the first pass won't do anything because
   DECL_CONTEXT (DECL) will be 0 so i386_pe_dll{ex,im}port_p will return 0.
   It's also used to handle dllimport override semantics.  */
#define REDO_SECTION_INFO_P(DECL) 1

#undef EXTRA_SECTIONS
#define EXTRA_SECTIONS in_drectve

#undef EXTRA_SECTION_FUNCTIONS
#define EXTRA_SECTION_FUNCTIONS					¥
  DRECTVE_SECTION_FUNCTION					¥
  SWITCH_TO_SECTION_FUNCTION

#define DRECTVE_SECTION_FUNCTION ¥
void									¥
drectve_section ()							¥
{									¥
  if (in_section != in_drectve)						¥
    {									¥
      fprintf (asm_out_file, "%s¥n", "¥t.section .drectve¥n");		¥
      in_section = in_drectve;						¥
    }									¥
}
void drectve_section PARAMS ((void));

/* Switch to SECTION (an `enum in_section').

   ??? This facility should be provided by GCC proper.
   The problem is that we want to temporarily switch sections in
   ASM_DECLARE_OBJECT_NAME and then switch back to the original section
   afterwards.  */
#define SWITCH_TO_SECTION_FUNCTION 				¥
void switch_to_section PARAMS ((enum in_section, tree));        ¥
void 								¥
switch_to_section (section, decl) 				¥
     enum in_section section; 					¥
     tree decl; 						¥
{ 								¥
  switch (section) 						¥
    { 								¥
      case in_text: text_section (); break; 			¥
      case in_data: data_section (); break; 			¥
      case in_named: named_section (decl, NULL, 0); break; 	¥
      case in_drectve: drectve_section (); break; 		¥
      default: abort (); break; 				¥
    } 								¥
}

/* Don't allow flag_pic to propagate since gas may produce invalid code
   otherwise.  */

#undef  SUBTARGET_OVERRIDE_OPTIONS
#define SUBTARGET_OVERRIDE_OPTIONS					¥
do {									¥
  if (flag_pic)								¥
    {									¥
      warning ("-f%s ignored for target (all code is position independent)",¥
	       (flag_pic > 1) ? "PIC" : "pic");				¥
      flag_pic = 0;							¥
    }									¥
} while (0)								¥

/* Define this macro if references to a symbol must be treated
   differently depending on something about the variable or
   function named by the symbol (such as what section it is in).

   On i386 running Windows NT, modify the assembler name with a suffix 
   consisting of an atsign (@) followed by string of digits that represents
   the number of bytes of arguments passed to the function, if it has the 
   attribute STDCALL.

   In addition, we must mark dll symbols specially. Definitions of 
   dllexport'd objects install some info in the .drectve section.  
   References to dllimport'd objects are fetched indirectly via
   _imp__.  If both are declared, dllexport overrides.  This is also 
   needed to implement one-only vtables: they go into their own
   section and we need to set DECL_SECTION_NAME so we do that here.
   Note that we can be called twice on the same decl.  */

extern void i386_pe_encode_section_info PARAMS ((TREE));

#ifdef ENCODE_SECTION_INFO
#undef ENCODE_SECTION_INFO
#endif
#define ENCODE_SECTION_INFO(DECL) i386_pe_encode_section_info (DECL)

/* Utility used only in this file.  */
#define I386_PE_STRIP_ENCODING(SYM_NAME) ¥
  ((SYM_NAME) + ((SYM_NAME)[0] == '@' ¥
		? (((SYM_NAME)[3] == '*' || (SYM_NAME)[3] == '+') ¥
		   ? 4 : 3 ) : 0 ) ¥
	      + (((SYM_NAME)[0] == '*' || (SYM_NAME)[0] == '+') ? 1 : 0))

/* This macro gets just the user-specified name
   out of the string in a SYMBOL_REF.  Discard
   trailing @[NUM] encoded by ENCODE_SECTION_INFO.  */
#undef  STRIP_NAME_ENCODING
#define STRIP_NAME_ENCODING(VAR,SYMBOL_NAME)				¥
do {									¥
  const char *_p;							¥
  const char *_name = I386_PE_STRIP_ENCODING (SYMBOL_NAME);		¥
  for (_p = _name; *_p && *_p != '@'; ++_p)				¥
    ;									¥
  if (*_p == '@')							¥
    {									¥
      int _len = _p - _name;						¥
      char *_new_name = (char *) alloca (_len + 1);			¥
      strncpy (_new_name, _name, _len);					¥
      _new_name[_len] = '¥0';						¥
      (VAR) = _new_name;						¥
    }									¥
  else									¥
    (VAR) = _name;							¥
} while (0)
      

/* Output a reference to a label.  */
#undef ASM_OUTPUT_LABELREF
#define ASM_OUTPUT_LABELREF(STREAM, NAME)		¥
do {							¥
  if (strncmp(NAME,"@i.",3) == 0)			¥
    {							¥
      if (NAME[3] == '+')				¥
       