/* Output variables, constants and external declarations, for GNU compiler.
   Copyright (C) 1987, 1988, 1989, 1992, 1993, 1994, 1995, 1996, 1997,
   1998, 1999, 2000, 2001, 2002 Free Software Foundation, Inc.

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


/* This file handles generation of all the assembler code
   *except* the instructions of a function.
   This includes declarations of variables and their initial values.

   We also output the assembler code for constants stored in memory
   and are responsible for combining constants with the same value.  */

/* !kawai! */
#include "config.h"
#include "system.h"
#include "rtl.h"
#include "tree.h"
#include "flags.h"
#include "function.h"
#include "expr.h"
#include "hard-reg-set.h"
#include "regs.h"
#include "output.h"
#include "real.h"
#include "toplev.h"
#include "../include/obstack.h"
#include "../include/hashtab.h"
#include "c-pragma.h"
#include "ggc.h"
#include "langhooks.h"
#include "tm_p.h"
#include "debug.h"
#include "target.h"
/* end of !kawai! */

#ifdef XCOFF_DEBUGGING_INFO
#include "xcoffout.h"		/* Needed for external data
				   declarations for e.g. AIX 4.x.  */
#endif

#ifndef TRAMPOLINE_ALIGNMENT
#define TRAMPOLINE_ALIGNMENT FUNCTION_BOUNDARY
#endif

#ifndef ASM_STABS_OP
#define ASM_STABS_OP "¥t.stabs¥t"
#endif

/* The (assembler) name of the first globally-visible object output.  */
const char *first_global_object_name;
const char *weak_global_object_name;

extern struct obstack permanent_obstack;
#define obstack_chunk_alloc xmalloc

struct addr_const;
struct constant_descriptor;
struct rtx_const;
struct pool_constant;

#define MAX_RTX_HASH_TABLE 61

struct varasm_status
{
  /* Hash facility for making memory-constants
     from constant rtl-expressions.  It is used on RISC machines
     where immediate integer arguments and constant addresses are restricted
     so that such constants must be stored in memory.

     This pool of constants is reinitialized for each function
     so each function gets its own constants-pool that comes right before
     it.  */
  struct constant_descriptor **x_const_rtx_hash_table;
  struct pool_constant **x_const_rtx_sym_hash_table;

  /* Pointers to first and last constant in pool.  */
  struct pool_constant *x_first_pool, *x_last_pool;

  /* Current offset in constant pool (does not include any machine-specific
     header).  */
  HOST_WIDE_INT x_pool_offset;

  /* Chain of all CONST_DOUBLE rtx's constructed for the current function.
     They are chained through the CONST_DOUBLE_CHAIN.  */
  rtx x_const_double_chain;
};

#define const_rtx_hash_table (cfun->varasm->x_const_rtx_hash_table)
#define const_rtx_sym_hash_table (cfun->varasm->x_const_rtx_sym_hash_table)
#define first_pool (cfun->varasm->x_first_pool)
#define last_pool (cfun->varasm->x_last_pool)
#define pool_offset (cfun->varasm->x_pool_offset)
#define const_double_chain (cfun->varasm->x_const_double_chain)

/* Number for making the label on the next
   constant that is stored in memory.  */

int const_labelno;

/* Number for making the label on the next
   static variable internal to a function.  */

int var_labelno;

/* Carry information from ASM_DECLARE_OBJECT_NAME
   to ASM_FINISH_DECLARE_OBJECT.  */

int size_directive_output;

/* The last decl for which assemble_variable was called,
   if it did ASM_DECLARE_OBJECT_NAME.
   If the last call to assemble_variable didn't do that,
   this holds 0.  */

tree last_assemble_variable_decl;

/* RTX_UNCHANGING_P in a MEM can mean it is stored into, for initialization.
   So giving constant the alias set for the type will allow such
   initializations to appear to conflict with the load of the constant.  We
   avoid this by giving all constants an alias set for just constants.
   Since there will be no stores to that alias set, nothing will ever
   conflict with them.  */

static HOST_WIDE_INT const_alias_set;

static const char *strip_reg_name	PARAMS ((const char *));
static int contains_pointers_p		PARAMS ((tree));
static void decode_addr_const		PARAMS ((tree, struct addr_const *));
static int const_hash			PARAMS ((tree));
static int compare_constant		PARAMS ((tree,
					       struct constant_descriptor *));
static const unsigned char *compare_constant_1  PARAMS ((tree, const unsigned char *));
static struct constant_descriptor *record_constant PARAMS ((tree));
static void record_constant_1		PARAMS ((tree));
static tree copy_constant		PARAMS ((tree));
static void output_constant_def_contents  PARAMS ((tree, int, int));
static void decode_rtx_const		PARAMS ((enum machine_mode, rtx,
					       struct rtx_const *));
static int const_hash_rtx		PARAMS ((enum machine_mode, rtx));
static int compare_constant_rtx		PARAMS ((enum machine_mode, rtx,
					       struct constant_descriptor *));
static struct constant_descriptor *record_constant_rtx PARAMS ((enum machine_mode,
							      rtx));
static struct pool_constant *find_pool_constant PARAMS ((struct function *, rtx));
static void mark_constant_pool		PARAMS ((void));
static void mark_constants		PARAMS ((rtx));
static int mark_constant		PARAMS ((rtx *current_rtx, void *data));
static int output_addressed_constants	PARAMS ((tree));
static void output_after_function_constants PARAMS ((void));
static unsigned HOST_WIDE_INT array_size_for_constructor PARAMS ((tree));
static unsigned min_align		PARAMS ((unsigned, unsigned));
static void output_constructor		PARAMS ((tree, HOST_WIDE_INT,
						 unsigned int));
static void globalize_decl		PARAMS ((tree));
static int in_named_entry_eq		PARAMS ((const PTR, const PTR));
static hashval_t in_named_entry_hash	PARAMS ((const PTR));
#ifdef ASM_OUTPUT_BSS
static void asm_output_bss		PARAMS ((FILE *, tree, const char *, int, int));
#endif
#ifdef BSS_SECTION_ASM_OP
#ifdef ASM_OUTPUT_ALIGNED_BSS
static void asm_output_aligned_bss	PARAMS ((FILE *, tree, const char *,
						 int, int));
#endif
#endif /* BSS_SECTION_ASM_OP */
static void mark_pool_constant          PARAMS ((struct pool_constant *));
static void mark_const_hash_entry	PARAMS ((void *));
static int mark_const_str_htab_1	PARAMS ((void **, void *));
static void mark_const_str_htab		PARAMS ((void *));
static hashval_t const_str_htab_hash	PARAMS ((const void *x));
static int const_str_htab_eq		PARAMS ((const void *x, const void *y));
static void const_str_htab_del		PARAMS ((void *));
static void asm_emit_uninitialised	PARAMS ((tree, const char*, int, int));
static void resolve_unique_section	PARAMS ((tree, int, int));
static void mark_weak                   PARAMS ((tree));

static enum in_section { no_section, in_text, in_data, in_named
#ifdef BSS_SECTION_ASM_OP
  , in_bss
#endif
#ifdef CTORS_SECTION_ASM_OP
  , in_ctors
#endif
#ifdef DTORS_SECTION_ASM_OP
  , in_dtors
#endif
#ifdef EXTRA_SECTIONS
  , EXTRA_SECTIONS
#endif
} in_section = no_section;

/* Return a non-zero value if DECL has a section attribute.  */
#ifndef IN_NAMED_SECTION
#define IN_NAMED_SECTION(DECL) ¥
  ((TREE_CODE (DECL) == FUNCTION_DECL || TREE_CODE (DECL) == VAR_DECL) ¥
   && DECL_SECTION_NAME (DECL) != NULL_TREE)
#endif

/* Text of section name when in_section == in_named.  */
static const char *in_named_name;

/* Hash table of flags that have been used for a particular named section.  */

struct in_named_entry
{
  const char *name;
  unsigned int flags;
  bool declared;
};

static htab_t in_named_htab;

/* Define functions like text_section for any extra sections.  */
#ifdef EXTRA_SECTION_FUNCTIONS
EXTRA_SECTION_FUNCTIONS
#endif

/* Tell assembler to switch to text section.  */

void
text_section ()
{
  if (in_section != in_text)
    {
#ifdef TEXT_SECTION
      TEXT_SECTION ();
#else
      fprintf (asm_out_file, "%s¥n", TEXT_SECTION_ASM_OP);
#endif
      in_section = in_text;
    }
}

/* Tell assembler to switch to data section.  */

void
data_section ()
{
  if (in_section != in_data)
    {
      if (flag_shared_data)
	{
#ifdef SHARED_SECTION_ASM_OP
	  fprintf (asm_out_file, "%s¥n", SHARED_SECTION_ASM_OP);
#else
	  fprintf (asm_out_file, "%s¥n", DATA_SECTION_ASM_OP);
#endif
	}
      else
	fprintf (asm_out_file, "%s¥n", DATA_SECTION_ASM_OP);

      in_section = in_data;
    }
}
/* Tell assembler to ALWAYS switch to data section, in case
   it's not sure where it is.  */

void
force_data_section ()
{
  in_section = no_section;
  data_section ();
}

/* Tell assembler to switch to read-only data section.  This is normally
   the text section.  */

void
readonly_data_section ()
{
#ifdef READONLY_DATA_SECTION
  READONLY_DATA_SECTION ();  /* Note this can call data_section.  */
#else
  text_section ();
#endif
}

/* Determine if we're in the text section.  */

int
in_text_section ()
{
  return in_section == in_text;
}

/* Determine if we're in the data section.  */

int
in_data_section ()
{
  return in_section == in_data;
}

/* Helper routines for maintaining in_named_htab.  */

static int
in_named_entry_eq (p1, p2)
     const PTR p1;
     const PTR p2;
{
  const struct in_named_entry *old = p1;
  const char *new = p2;

  return strcmp (old->name, new) == 0;
}

static hashval_t
in_named_entry_hash (p)
     const PTR p;
{
  const struct in_named_entry *old = p;
  return htab_hash_string (old->name);
}

/* If SECTION has been seen before as a named section, return the flags
   that were used.  Otherwise, return 0.  Note, that 0 is a perfectly valid
   set of flags for a section to have, so 0 does not mean that the section
   has not been seen.  */

unsigned int
get_named_section_flags (section)
     const char *section;
{
  struct in_named_entry **slot;

  slot = (struct in_named_entry**)
    htab_find_slot_with_hash (in_named_htab, section,
			      htab_hash_string (section), NO_INSERT);

  return slot ? (*slot)->flags : 0;
}

/* Returns true if the section has been declared before.   Sets internal
   flag on this section in in_named_hash so subsequent calls on this 
   section will return false.  */

bool
named_section_first_declaration (name)
     const char *name;
{
  struct in_named_entry **slot;

  slot = (struct in_named_entry**)
    htab_find_slot_with_hash (in_named_htab, name, 
			      htab_hash_string (name), NO_INSERT);
  if (! (*slot)->declared)
    {
      (*slot)->declared = true;
      return true;
    }
  else 
    {
      return false;
    }
}


/* Record FLAGS for SECTION.  If SECTION was previously recorded with a
   different set of flags, return false.  */

bool
set_named_section_flags (section, flags)
     const char *section;
     unsigned int flags;
{
  struct in_named_entry **slot, *entry;

  slot = (struct in_named_entry**)
    htab_find_slot_with_hash (in_named_htab, section,
			      htab_hash_string (section), INSERT);
  entry = *slot;

  if (!entry)
    {
      entry = (struct in_named_entry *) xmalloc (sizeof (*entry));
      *slot = entry;
      entry->name = ggc_strdup (section);
      entry->flags = flags;
      entry->declared = false;
    }
  else if (entry->flags != flags)
    return false;

  return true;
}

/* Tell assembler to change to section NAME with attributes FLAGS.  */

void
named_section_flags (name, flags)
     const char *name;
     unsigned int flags;
{
  if (in_section != in_named || strcmp (name, in_named_name) != 0)
    {
      if (! set_named_section_flags (name, flags))
	abort ();

      (* targetm.asm_out.named_section) (name, flags);

      if (flags & SECTION_FORGET)
	in_section = no_section;
      else
	{
	  in_named_name = ggc_strdup (name);
	  in_section = in_named;
	}
    }
}

/* Tell assembler to change to section NAME for DECL.
   If DECL is NULL, just switch to section NAME.
   If NAME is NULL, get the name from DECL.
   If RELOC is 1, the initializer for DECL contains relocs.  */

void
named_section (decl, name, reloc)
     tree decl;
     const char *name;
     int reloc;
{
  unsigned int flags;

  if (decl != NULL_TREE && !DECL_P (decl))
    abort ();
  if (name == NULL)
    name = TREE_STRING_POINTER (DECL_SECTION_NAME (decl));

  flags = (* targetm.section_type_flags) (decl, name, reloc);

  /* Sanity check user variables for flag changes.  Non-user
     section flag changes will abort in named_section_flags.
     However, don't complain if SECTION_OVERRIDE is set.
     We trust that the setter knows that it is safe to ignore
     the default flags for this decl.  */
  if (decl && ! set_named_section_flags (name, flags))
    {
      flags = get_named_section_flags (name);
      if ((flags & SECTION_OVERRIDE) == 0)
	error_with_decl (decl, "%s causes a section type conflict");
    }

  named_section_flags (name, flags);
}

/* If required, set DECL_SECTION_NAME to a unique name.  */

static void
resolve_unique_section (decl, reloc, flag_function_or_data_sections)
     tree decl;
     int reloc ATTRIBUTE_UNUSED;
     int flag_function_or_data_sections;
{
  if (DECL_SECTION_NAME (decl) == NULL_TREE
      && (flag_function_or_data_sections
	  || (targetm.have_named_sections
	      && DECL_ONE_ONLY (decl))))
    UNIQUE_SECTION (decl, reloc);
}

#ifdef BSS_SECTION_ASM_OP

/* Tell the assembler to switch to the bss section.  */

void
bss_section ()
{
  if (in_section != in_bss)
    {
#ifdef SHARED_BSS_SECTION_ASM_OP
      if (flag_shared_data)
	fprintf (asm_out_file, "%s¥n", SHARED_BSS_SECTION_ASM_OP);
      else
#endif
	fprintf (asm_out_file, "%s¥n", BSS_SECTION_ASM_OP);

      in_section = in_bss;
    }
}

#ifdef ASM_OUTPUT_BSS

/* Utility function for ASM_OUTPUT_BSS for targets to use if
   they don't support alignments in .bss.
   ??? It is believed that this function will work in most cases so such
   support is localized here.  */

static void
asm_output_bss (file, decl, name, size, rounded)
     FILE *file;
     tree decl ATTRIBUTE_UNUSED;
     const char *name;
     int size ATTRIBUTE_UNUSED, rounded;
{
  ASM_GLOBALIZE_LABEL (file, name);
  bss_section ();
#ifdef ASM_DECLARE_OBJECT_NAME
  last_assemble_variable_decl = decl;
  ASM_DECLARE_OBJECT_NAME (file, name, decl);
#else
  /* Standard thing is just output label for the object.  */
  ASM_OUTPUT_LABEL (file, name);
#endif /* ASM_DECLARE_OBJECT_NAME */
  ASM_OUTPUT_SKIP (file, rounded ? rounded : 1);
}

#endif

#ifdef ASM_OUTPUT_ALIGNED_BSS

/* Utility function for targets to use in implementing
   ASM_OUTPUT_ALIGNED_BSS.
   ??? It is believed that this function will work in most cases so such
   support is localized here.  */

static void
asm_output_aligned_bss (file, decl, name, size, align)
     FILE *file;
     tree decl ATTRIBUTE_UNUSED;
     const char *name;
     int size, align;
{
  ASM_GLOBALIZE_LABEL (file, name);
  bss_section ();
  ASM_OUTPUT_ALIGN (file, floor_log2 (align / BITS_PER_UNIT));
#ifdef ASM_DECLARE_OBJECT_NAME
  last_assemble_variable_decl = decl;
  ASM_DECLARE_OBJECT_NAME (file, name, decl);
#else
  /* Standard thing is just output label for the object.  */
  ASM_OUTPUT_LABEL (file, name);
#endif /* ASM_DECLARE_OBJECT_NAME */
  ASM_OUTPUT_SKIP (file, size ? size : 1);
}

#endif

#endif /* BSS_SECTION_ASM_OP */

/* Switch to the section for function DECL.

   If DECL is NULL_TREE, switch to the text section.
   ??? It's not clear that we will ever be passed NULL_TREE, but it's
   safer to handle it.  */

void
function_section (decl)
    