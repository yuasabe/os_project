/* Process declarations and variables for C compiler.
   Copyright (C) 1988, 1992, 1993, 1994, 1995, 1996, 1997, 1998,
   1999, 2000, 2001, 2002 Free Software Foundation, Inc.
   Hacked by Michael Tiemann (tiemann@cygnus.com)

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


/* Process declarations and symbol lookup for C front end.
   Also constructs types; the standard scalar types at initialization,
   and structure, union, array and enum types when they are declared.  */

/* ??? not all decl nodes are given the most useful possible
   line numbers.  For example, the CONST_DECLs for enum values.  */

/* !kawai! */
#include "../gcc/config.h"
#include "../gcc/system.h"
#include "../gcc/tree.h"
#include "../gcc/rtl.h"
#include "../gcc/expr.h"
#include "../gcc/flags.h"
#include "cp-tree.h"
#include "decl.h"
#include "lex.h"
#include "../gcc/output.h"
#include "../gcc/except.h"
#include "../gcc/toplev.h"
#include "../gcc/ggc.h"
#include "../gcc/timevar.h"
#include "../gcc/cpplib.h"
#include "../gcc/target.h"
extern cpp_reader *parse_in;
/* end of !kawai! */

/* This structure contains information about the initializations
   and/or destructions required for a particular priority level.  */
typedef struct priority_info_s {
  /* Non-zero if there have been any initializations at this priority
     throughout the translation unit.  */
  int initializations_p;
  /* Non-zero if there have been any destructions at this priority
     throughout the translation unit.  */
  int destructions_p;
} *priority_info;

static void mark_vtable_entries PARAMS ((tree));
static void grok_function_init PARAMS ((tree, tree));
static int finish_vtable_vardecl PARAMS ((tree *, void *));
static int prune_vtable_vardecl PARAMS ((tree *, void *));
static int is_namespace_ancestor PARAMS ((tree, tree));
static void add_using_namespace PARAMS ((tree, tree, int));
static tree ambiguous_decl PARAMS ((tree, tree, tree,int));
static tree build_anon_union_vars PARAMS ((tree, tree*, int, int));
static int acceptable_java_type PARAMS ((tree));
static void output_vtable_inherit PARAMS ((tree));
static tree start_objects PARAMS ((int, int));
static void finish_objects PARAMS ((int, int, tree));
static tree merge_functions PARAMS ((tree, tree));
static tree decl_namespace PARAMS ((tree));
static tree validate_nonmember_using_decl PARAMS ((tree, tree *, tree *));
static void do_nonmember_using_decl PARAMS ((tree, tree, tree, tree,
					   tree *, tree *));
static tree start_static_storage_duration_function PARAMS ((void));
static void finish_static_storage_duration_function PARAMS ((tree));
static priority_info get_priority_info PARAMS ((int));
static void do_static_initialization PARAMS ((tree, tree));
static void do_static_destruction PARAMS ((tree));
static tree start_static_initialization_or_destruction PARAMS ((tree, int));
static void finish_static_initialization_or_destruction PARAMS ((tree));
static void generate_ctor_or_dtor_function PARAMS ((int, int));
static int generate_ctor_and_dtor_functions_for_priority
                                  PARAMS ((splay_tree_node, void *));
static tree prune_vars_needing_no_initialization PARAMS ((tree));
static void write_out_vars PARAMS ((tree));
static void import_export_class	PARAMS ((tree));
static tree key_method PARAMS ((tree));
static int compare_options PARAMS ((const PTR, const PTR));
static tree get_guard_bits PARAMS ((tree));

/* A list of static class variables.  This is needed, because a
   static class variable can be declared inside the class without
   an initializer, and then initialized, statically, outside the class.  */
static varray_type pending_statics;
#define pending_statics_used ¥
  (pending_statics ? pending_statics->elements_used : 0)

/* A list of functions which were declared inline, but which we
   may need to emit outline anyway.  */
static varray_type deferred_fns;
#define deferred_fns_used ¥
  (deferred_fns ? deferred_fns->elements_used : 0)

/* Flag used when debugging spew.c */

extern int spew_debug;

/* Nonzero if we're done parsing and into end-of-file activities.  */

int at_eof;

/* Functions called along with real static constructors and destructors.  */

tree static_ctors;
tree static_dtors;

/* The :: namespace. */

tree global_namespace;

/* C (and C++) language-specific option variables.  */

/* Nonzero means don't recognize the keyword `asm'.  */

int flag_no_asm;

/* Nonzero means don't recognize any extension keywords.  */

int flag_no_gnu_keywords;

/* Nonzero means do some things the same way PCC does.  Only provided so
   the compiler will link.  */

int flag_traditional;

/* Nonzero means to treat bitfields as unsigned unless they say `signed'.  */

int flag_signed_bitfields = 1;

/* Nonzero means enable obscure standard features and disable GNU
   extensions that might cause standard-compliant code to be
   miscompiled.  */

int flag_ansi;

/* Nonzero means do emit exported implementations of functions even if
   they can be inlined.  */

int flag_implement_inlines = 1;

/* Nonzero means do emit exported implementations of templates, instead of
   multiple static copies in each file that needs a definition.  */

int flag_external_templates;

/* Nonzero means that the decision to emit or not emit the implementation of a
   template depends on where the template is instantiated, rather than where
   it is defined.  */

int flag_alt_external_templates;

/* Nonzero means that implicit instantiations will be emitted if needed.  */

int flag_implicit_templates = 1;

/* Nonzero means that implicit instantiations of inline templates will be
   emitted if needed, even if instantiations of non-inline templates
   aren't.  */

int flag_implicit_inline_templates = 1;

/* Nonzero means warn about implicit declarations.  */

int warn_implicit = 1;

/* Nonzero means warn about usage of long long when `-pedantic'.  */

int warn_long_long = 1;

/* Nonzero means warn when all ctors or dtors are private, and the class
   has no friends.  */

int warn_ctor_dtor_privacy = 1;

/* Nonzero means generate separate instantiation control files and juggle
   them at link time.  */

int flag_use_repository;

/* Nonzero if we want to issue diagnostics that the standard says are not
   required.  */

int flag_optional_diags = 1;

/* Nonzero means give string constants the type `const char *', as mandated
   by the standard.  */

int flag_const_strings = 1;

/* Nonzero means warn about deprecated conversion from string constant to
   `char *'.  */

int warn_write_strings;

/* Nonzero means warn about pointer casts that can drop a type qualifier
   from the pointer target type.  */

int warn_cast_qual;

/* Nonzero means warn about sizeof(function) or addition/subtraction
   of function pointers.  */

int warn_pointer_arith = 1;

/* Nonzero means warn for any function def without prototype decl.  */

int warn_missing_prototypes;

/* Nonzero means warn about multiple (redundant) decls for the same single
   variable or function.  */

int warn_redundant_decls;

/* Warn if initializer is not completely bracketed.  */

int warn_missing_braces;

/* Warn about comparison of signed and unsigned values.  */

int warn_sign_compare;

/* Warn about testing equality of floating point numbers. */

int warn_float_equal = 0;

/* Warn about functions which might be candidates for format attributes.  */

int warn_missing_format_attribute;

/* Warn about a subscript that has type char.  */

int warn_char_subscripts;

/* Warn if a type conversion is done that might have confusing results.  */

int warn_conversion;

/* Warn if adding () is suggested.  */

int warn_parentheses;

/* Non-zero means warn in function declared in derived class has the
   same name as a virtual in the base class, but fails to match the
   type signature of any virtual function in the base class.  */

int warn_overloaded_virtual;

/* Non-zero means warn when declaring a class that has a non virtual
   destructor, when it really ought to have a virtual one.  */

int warn_nonvdtor;

/* Non-zero means warn when the compiler will reorder code.  */

int warn_reorder;

/* Non-zero means warn when synthesis behavior differs from Cfront's.  */

int warn_synth;

/* Non-zero means warn when we convert a pointer to member function
   into a pointer to (void or function).  */

int warn_pmf2ptr = 1;

/* Nonzero means warn about violation of some Effective C++ style rules.  */

int warn_ecpp;

/* Nonzero means warn where overload resolution chooses a promotion from
   unsigned to signed over a conversion to an unsigned of the same size.  */

int warn_sign_promo;

/* Nonzero means warn when an old-style cast is used.  */

int warn_old_style_cast;

/* Warn about #pragma directives that are not recognised.  */      

int warn_unknown_pragmas; /* Tri state variable.  */  

/* Nonzero means warn about use of multicharacter literals.  */

int warn_multichar = 1;

/* Nonzero means warn when non-templatized friend functions are
   declared within a template */

int warn_nontemplate_friend = 1;

/* Nonzero means complain about deprecated features.  */

int warn_deprecated = 1;

/* Nonzero means `$' can be in an identifier.  */

#ifndef DOLLARS_IN_IDENTIFIERS
#define DOLLARS_IN_IDENTIFIERS 1
#endif
int dollars_in_ident = DOLLARS_IN_IDENTIFIERS;

/* Nonzero means allow Microsoft extensions without a pedwarn.  */

int flag_ms_extensions;

/* C++ specific flags.  */   

/* Nonzero means we should attempt to elide constructors when possible.  */

int flag_elide_constructors = 1;

/* Nonzero means that member functions defined in class scope are
   inline by default.  */

int flag_default_inline = 1;

/* Controls whether compiler generates 'type descriptor' that give
   run-time type information.  */

int flag_rtti = 1;

/* Nonzero if we want to support huge (> 2^(sizeof(short)*8-1) bytes)
   objects.  */

int flag_huge_objects;

/* Nonzero if we want to conserve space in the .o files.  We do this
   by putting uninitialized data and runtime initialized data into
   .common instead of .data at the expense of not flagging multiple
   definitions.  */

int flag_conserve_space;

/* Nonzero if we want to obey access control semantics.  */

int flag_access_control = 1;

/* Nonzero if we want to understand the operator names, i.e. 'bitand'.  */

int flag_operator_names = 1;

/* Nonzero if we want to check the return value of new and avoid calling
   constructors if it is a null pointer.  */

int flag_check_new;

/* Nonzero if we want the new ISO rules for pushing a new scope for `for'
   initialization variables.
   0: Old rules, set by -fno-for-scope.
   2: New ISO rules, set by -ffor-scope.
   1: Try to implement new ISO rules, but with backup compatibility
   (and warnings).  This is the default, for now.  */

int flag_new_for_scope = 1;

/* Nonzero if we want to emit defined symbols with common-like linkage as
   weak symbols where possible, in order to conform to C++ semantics.
   Otherwise, emit them as local symbols.  */

int flag_weak = 1;

/* Nonzero to use __cxa_atexit, rather than atexit, to register
   destructors for local statics and global objects.  */

int flag_use_cxa_atexit = DEFAULT_USE_CXA_ATEXIT;

/* Maximum template instantiation depth.  This limit is rather
   arbitrary, but it exists to limit the time it takes to notice
   infinite template instantiations.  */

int max_tinst_depth = 500;

/* Nonzero means output .vtable_{entry,inherit} for use in doing vtable gc.  */

int flag_vtable_gc;

/* Nonzero means make the default pedwarns warnings instead of errors.
   The value of this flag is ignored if -pedantic is specified.  */

int flag_permissive;

/* Nonzero means to implement standard semantics for exception
   specifications, calling unexpected if an exception is thrown that
   doesn't match the specification.  Zero means to treat them as
   assertions and optimize accordingly, but not check them.  */

int flag_enforce_eh_specs = 1;

/* Table of language-dependent -f options.
   STRING is the option name.  VARIABLE is the address of the variable.
   ON_VALUE is the value to store in VARIABLE
    if `-fSTRING' is seen as an option.
   (If `-fno-STRING' is seen as an option, the opposite value is stored.)  */

static const struct { const char *const string; int *const variable; const int on_value;}
lang_f_options[] =
{
  /* C/C++ options.  */
  {"signed-char", &flag_signed_char, 1},
  {"unsigned-char", &flag_signed_char, 0},
  {"signed-bitfields", &flag_signed_bitfields, 1},
  {"unsigned-bitfields", &flag_signed_bitfields, 0},
  {"short-enums", &flag_short_enums, 1},
  {"short-double", &flag_short_double, 1},
  {"short-wchar", &flag_short_wchar, 1},
  {"asm", &flag_no_asm, 0},
  {"builtin", &flag_no_builtin, 0},

  /* C++-only options.  */
  {"access-control", &flag_access_control, 1},
  {"check-new", &flag_check_new, 1},
  {"conserve-space", &flag_conserve_space, 1},
  {"const-strings", &flag_const_strings, 1},
  {"default-inline", &flag_default_inline, 1},
  {"dollars-in-identifiers", &dollars_in_ident, 1},
  {"elide-constructors", &flag_elide_constructors, 1},
  {"enforce-eh-specs", &flag_enforce_eh_specs, 1},
  {"external-templates", &flag_external_templates, 1},
  {"for-scope", &flag_new_for_scope, 2},
  {"gnu-keywords", &flag_no_gnu_keywords, 0},
  {"handle-exceptions", &flag_exceptions, 1},
  {"implement-inlines", &flag_implement_inlines, 1},
  {"implicit-inline-templates", &flag_implicit_inline_templates, 1},
  {"implicit-templates", &flag_implicit_templates, 1},
  {"ms-extensions", &flag_ms_extensions, 1},
  {"nonansi-builtins", &flag_no_nonansi_builtin, 0},
  {"operator-names", &flag_operator_names, 1},
  {"optional-diags", &flag_optional_diags, 1},
  {"permissive", &flag_permissive, 1},
  {"repo", &flag_use_repository, 1},
  {"rtti", &flag_rtti, 1},
  {"stats", &flag_detailed_statistics, 1},
  {"vtable-gc", &flag_vtable_gc, 1},
  {"use-cxa-atexit", &flag_use_cxa_atexit, 1},
  {"weak", &flag_weak, 1}
};

/* The list of `-f' options that we no longer support.  The `-f'
   prefix is not given in this table.  The `-fno-' variants are not
   listed here.  This table must be kept in alphabetical order.  */
static const char * const unsupported_options[] = {
  "all-virtual",
  "cond-mismatch",
  "enum-int-equiv",
  "guiding-decls",
  "honor-std",
  "huge-objects",
  "labels-ok",
  "new-abi",
  "nonnull-objects",
  "squangle",
  "strict-prototype",
  "this-is-variable",
  "vtable-thunks",
  "xref"
};

/* Compare two option strings, pointed two by P1 and P2, for use with
   bsearch.  */

static int
compare_options (p1, p2)
     const PTR p1;
     const PTR p2;
{
  return strcmp (*((const char *const *) p1), *((const char *const *) p2));
}

/* Decode the string P as a language-specific option.
   Return the number of strings consumed for a valid option.
   Otherwise return 0.  Should not complain if it does not
   recognise the option.  */

int   
cxx_decode_option (argc, argv)
     int argc;
     char **argv;
{
  int strings_processed;
  const char *p = argv[0];

  strings_processed = cpp_handle_option (parse_in, argc, argv, 0);

  if (!strcmp (p, "-ftraditional") || !strcmp (p, "-traditional"))
    /* ignore */;
  else if (p[0] == '-' && p[1] == 'f')
    {
      /* 