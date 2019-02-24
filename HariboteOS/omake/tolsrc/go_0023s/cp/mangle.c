/* Name mangling for the 3.0 C++ ABI.
   Copyright (C) 2000, 2001, 2002 Free Software Foundation, Inc.
   Written by Alex Samuel <sameul@codesourcery.com>

   This file is part of GNU CC.

   GNU CC is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   GNU CC is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNU CC; see the file COPYING.  If not, write to the Free
   Software Foundation, 59 Temple Place - Suite 330, Boston, MA
   02111-1307, USA.  */

/* This file implements mangling of C++ names according to the IA64
   C++ ABI specification.  A mangled name encodes a function or
   variable's name, scope, type, and/or template arguments into a text
   identifier.  This identifier is used as the function's or
   variable's linkage name, to preserve compatibility between C++'s
   language features (templates, scoping, and overloading) and C
   linkers.

   Additionally, g++ uses mangled names internally.  To support this,
   mangling of types is allowed, even though the mangled name of a
   type should not appear by itself as an exported name.  Ditto for
   uninstantiated templates.

   The primary entry point for this module is mangle_decl, which
   returns an identifier containing the mangled name for a decl.
   Additional entry points are provided to build mangled names of
   particular constructs when the appropriate decl for that construct
   is not available.  These are:

     mangle_typeinfo_for_type:        typeinfo data
     mangle_typeinfo_string_for_type: typeinfo type name
     mangle_vtbl_for_type:            virtual table data
     mangle_vtt_for_type:             VTT data
     mangle_ctor_vtbl_for_type:       `C-in-B' constructor virtual table data
     mangle_thunk:                    thunk function or entry

*/

/* !kawai! */
#include "../gcc/config.h"
#include "../gcc/system.h"
#include "../gcc/tree.h"
#include "cp-tree.h"
#include "../include/obstack.h"
#include "../gcc/toplev.h"
#include "../gcc/varray.h"
/* end of !kawai! */

/* Debugging support.  */

/* Define DEBUG_MANGLE to enable very verbose trace messages.  */
#ifndef DEBUG_MANGLE
#define DEBUG_MANGLE 0
#endif

/* Macros for tracing the write_* functions.  */
#if DEBUG_MANGLE
# define MANGLE_TRACE(FN, INPUT) ¥
  fprintf (stderr, "  %-24s: %-24s¥n", (FN), (INPUT))
# define MANGLE_TRACE_TREE(FN, NODE) ¥
  fprintf (stderr, "  %-24s: %-24s (%p)¥n", ¥
           (FN), tree_code_name[TREE_CODE (NODE)], (void *) (NODE))
#else
# define MANGLE_TRACE(FN, INPUT)
# define MANGLE_TRACE_TREE(FN, NODE)
#endif

/* Non-zero if NODE is a class template-id.  We can't rely on
   CLASSTYPE_USE_TEMPLATE here because of tricky bugs in the parser
   that hard to distinguish A<T> from A, where A<T> is the type as
   instantiated outside of the template, and A is the type used
   without parameters inside the template.  */
#define CLASSTYPE_TEMPLATE_ID_P(NODE)				      ¥
  (TYPE_LANG_SPECIFIC (NODE) != NULL 				      ¥
   && CLASSTYPE_TEMPLATE_INFO (NODE) != NULL                          ¥
   && (PRIMARY_TEMPLATE_P (CLASSTYPE_TI_TEMPLATE (NODE))))

/* Things we only need one of.  This module is not reentrant.  */
static struct globals
{
  /* The name in which we're building the mangled name.  */
  struct obstack name_obstack;

  /* An array of the current substitution candidates, in the order
     we've seen them.  */
  varray_type substitutions;
} G;

/* Indices into subst_identifiers.  These are identifiers used in
   special substitution rules.  */
typedef enum
{
  SUBID_ALLOCATOR,
  SUBID_BASIC_STRING,
  SUBID_CHAR_TRAITS,
  SUBID_BASIC_ISTREAM,
  SUBID_BASIC_OSTREAM,
  SUBID_BASIC_IOSTREAM,
  SUBID_MAX
}
substitution_identifier_index_t;

/* For quick substitution checks, look up these common identifiers
   once only.  */
static tree subst_identifiers[SUBID_MAX];

/* Single-letter codes for builtin integer types, defined in
   <builtin-type>.  These are indexed by integer_type_kind values.  */
static char
integer_type_codes[itk_none] =
{
  'c',  /* itk_char */
  'a',  /* itk_signed_char */
  'h',  /* itk_unsigned_char */
  's',  /* itk_short */
  't',  /* itk_unsigned_short */
  'i',  /* itk_int */
  'j',  /* itk_unsigned_int */
  'l',  /* itk_long */
  'm',  /* itk_unsigned_long */
  'x',  /* itk_long_long */
  'y'   /* itk_unsigned_long_long */
};

static int decl_is_template_id PARAMS ((tree, tree*));

/* Functions for handling substitutions.  */

static inline tree canonicalize_for_substitution PARAMS ((tree));
static void add_substitution PARAMS ((tree));
static inline int is_std_substitution PARAMS ((tree, substitution_identifier_index_t));
static inline int is_std_substitution_char PARAMS ((tree, substitution_identifier_index_t));
static int find_substitution PARAMS ((tree));

/* Functions for emitting mangled representations of things.  */

static void write_mangled_name PARAMS ((tree));
static void write_encoding PARAMS ((tree));
static void write_name PARAMS ((tree, int));
static void write_unscoped_name PARAMS ((tree));
static void write_unscoped_template_name PARAMS ((tree));
static void write_nested_name PARAMS ((tree));
static void write_prefix PARAMS ((tree));
static void write_template_prefix PARAMS ((tree));
static void write_unqualified_name PARAMS ((tree));
static void write_source_name PARAMS ((tree));
static int hwint_to_ascii PARAMS ((unsigned HOST_WIDE_INT, unsigned int, char *, unsigned));
static void write_number PARAMS ((unsigned HOST_WIDE_INT, int,
				  unsigned int));
static void write_integer_cst PARAMS ((tree));
static void write_identifier PARAMS ((const char *));
static void write_special_name_constructor PARAMS ((tree));
static void write_special_name_destructor PARAMS ((tree));
static void write_type PARAMS ((tree));
static int write_CV_qualifiers_for_type PARAMS ((tree));
static void write_builtin_type PARAMS ((tree));
static void write_function_type PARAMS ((tree));
static void write_bare_function_type PARAMS ((tree, int, tree));
static void write_method_parms PARAMS ((tree, int, tree));
static void write_class_enum_type PARAMS ((tree));
static void write_template_args PARAMS ((tree));
static void write_expression PARAMS ((tree));
static void write_template_arg_literal PARAMS ((tree));
static void write_template_arg PARAMS ((tree));
static void write_template_template_arg PARAMS ((tree));
static void write_array_type PARAMS ((tree));
static void write_pointer_to_member_type PARAMS ((tree));
static void write_template_param PARAMS ((tree));
static void write_template_template_param PARAMS ((tree));
static void write_substitution PARAMS ((int));
static int discriminator_for_local_entity PARAMS ((tree));
static int discriminator_for_string_literal PARAMS ((tree, tree));
static void write_discriminator PARAMS ((int));
static void write_local_name PARAMS ((tree, tree, tree));
static void dump_substitution_candidates PARAMS ((void));
static const char *mangle_decl_string PARAMS ((tree));

/* Control functions.  */

static inline void start_mangling PARAMS ((void));
static inline const char *finish_mangling PARAMS ((void));
static tree mangle_special_for_type PARAMS ((tree, const char *));

/* Foreign language functions. */

static void write_java_integer_type_codes PARAMS ((tree));

/* Append a single character to the end of the mangled
   representation.  */
#define write_char(CHAR)                                              ¥
  obstack_1grow (&G.name_obstack, (CHAR))

/* Append a sized buffer to the end of the mangled representation. */
#define write_chars(CHAR, LEN)                                        ¥
  obstack_grow (&G.name_obstack, (CHAR), (LEN))

/* Append a NUL-terminated string to the end of the mangled
   representation.  */
#define write_string(STRING)                                          ¥
  obstack_grow (&G.name_obstack, (STRING), strlen (STRING))

/* Return the position at which the next character will be appended to
   the mangled representation.  */
#define mangled_position()                                              ¥
  obstack_object_size (&G.name_obstack)

/* Non-zero if NODE1 and NODE2 are both TREE_LIST nodes and have the
   same purpose (context, which may be a type) and value (template
   decl).  See write_template_prefix for more information on what this
   is used for.  */
#define NESTED_TEMPLATE_MATCH(NODE1, NODE2)                         ¥
  (TREE_CODE (NODE1) == TREE_LIST                                     ¥
   && TREE_CODE (NODE2) == TREE_LIST                                  ¥
   && ((TYPE_P (TREE_PURPOSE (NODE1))                                 ¥
        && same_type_p (TREE_PURPOSE (NODE1), TREE_PURPOSE (NODE2)))¥
       || TREE_PURPOSE (NODE1) == TREE_PURPOSE (NODE2))             ¥
   && TREE_VALUE (NODE1) == TREE_VALUE (NODE2))

/* Write out a signed quantity in base 10.  */
#define write_signed_number(NUMBER) ¥
  write_number ((NUMBER), /*unsigned_p=*/0, 10)

/* Write out an unsigned quantity in base 10.  */
#define write_unsigned_number(NUMBER) ¥
  write_number ((NUMBER), /*unsigned_p=*/1, 10)

/* If DECL is a template instance, return non-zero and, if
   TEMPLATE_INFO is non-NULL, set *TEMPLATE_INFO to its template info.
   Otherwise return zero.  */

static int
decl_is_template_id (decl, template_info)
     tree decl;
     tree* template_info;
{
  if (TREE_CODE (decl) == TYPE_DECL)
    {
      /* TYPE_DECLs are handled specially.  Look at its type to decide
	 if this is a template instantiation.  */
      tree type = TREE_TYPE (decl);

      if (CLASS_TYPE_P (type) && CLASSTYPE_TEMPLATE_ID_P (type))
	{
	  if (template_info != NULL)
	    /* For a templated TYPE_DECL, the template info is hanging
	       off the type.  */
	    *template_info = CLASSTYPE_TEMPLATE_INFO (type);
	  return 1;
	}
    } 
  else
    {
      /* Check if this is a primary template.  */
      if (DECL_LANG_SPECIFIC (decl) != NULL
	  && DECL_USE_TEMPLATE (decl)
	  && PRIMARY_TEMPLATE_P (DECL_TI_TEMPLATE (decl))
	  && TREE_CODE (decl) != TEMPLATE_DECL)
	{
	  if (template_info != NULL)
	    /* For most templated decls, the template info is hanging
	       off the decl.  */
	    *template_info = DECL_TEMPLATE_INFO (decl);
	  return 1;
	}
    }

  /* It's not a template id.  */
  return 0;
}

/* Produce debugging output of current substitution candidates.  */

static void
dump_substitution_candidates ()
{
  unsigned i;

  fprintf (stderr, "  ++ substitutions  ");
  for (i = 0; i < VARRAY_ACTIVE_SIZE (G.substitutions); ++i)
    {
      tree el = VARRAY_TREE (G.substitutions, i);
      const char *name = "???";

      if (i > 0)
	fprintf (stderr, "                    ");
      if (DECL_P (el))
	name = IDENTIFIER_POINTER (DECL_NAME (el));
      else if (TREE_CODE (el) == TREE_LIST)
	name = IDENTIFIER_POINTER (DECL_NAME (TREE_VALUE (el)));
      else if (TYPE_NAME (el))
	name = IDENTIFIER_POINTER (DECL_NAME (TYPE_NAME (el)));
      fprintf (stderr, " S%d_ = ", i - 1);
      if (TYPE_P (el) && 
	  (CP_TYPE_RESTRICT_P (el) 
	   || CP_TYPE_VOLATILE_P (el) 
	   || CP_TYPE_CONST_P (el)))
	fprintf (stderr, "CV-");
      fprintf (stderr, "%s (%s at %p)¥n", 
	       name, tree_code_name[TREE_CODE (el)], (void *) el);
    }
}

/* Both decls and types can be substitution candidates, but sometimes
   they refer to the same thing.  For instance, a TYPE_DECL and
   RECORD_TYPE for the same class refer to the same thing, and should
   be treated accordinginly in substitutions.  This function returns a
   canonicalized tree node representing NODE that is used when adding
   and substitution candidates and finding matches.  */

