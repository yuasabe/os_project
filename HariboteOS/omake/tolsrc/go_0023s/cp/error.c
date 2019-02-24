/* Call-backs for C++ error reporting.
   This code is non-reentrant.
   Copyright (C) 1993, 1994, 1995, 1996, 1997, 1998, 1999, 2000, 2002
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

/* !kawai! */
#include "../gcc/config.h"
#include "../gcc/system.h"
#include "../gcc/tree.h"
#include "cp-tree.h"
#include "../gcc/real.h"
#include "../include/obstack.h"
#include "../gcc/toplev.h"
#include "../gcc/flags.h"
#include "../gcc/diagnostic.h"
/* end of !kawai! */

enum pad { none, before, after };

#define sorry_for_unsupported_tree(T)                                      ¥
   sorry ("`%s' not supported by %s", tree_code_name[(int) TREE_CODE (T)], ¥
             __FUNCTION__)

#define print_scope_operator(BUFFER)  output_add_string ((BUFFER), "::")
#define print_left_paren(BUFFER)      output_add_character ((BUFFER), '(')
#define print_right_paren(BUFFER)     output_add_character ((BUFFER), ')')
#define print_left_bracket(BUFFER)    output_add_character ((BUFFER), '[')
#define print_right_bracket(BUFFER)   output_add_character ((BUFFER), ']')
#define print_template_argument_list_start(BUFFER) ¥
   print_non_consecutive_character ((BUFFER), '<')
#define print_template_argument_list_end(BUFFER)  ¥
   print_non_consecutive_character ((BUFFER), '>')
#define print_whitespace(BUFFER, TFI)        ¥
   do {                                      ¥
     output_add_space (BUFFER);              ¥
     put_whitespace (TFI) = none;            ¥
   } while (0)
#define print_tree_identifier(BUFFER, TID) ¥
   output_add_string ((BUFFER), IDENTIFIER_POINTER (TID))
#define print_identifier(BUFFER, ID) output_add_string ((BUFFER), (ID))
#define separate_with_comma(BUFFER) output_add_string ((BUFFER), ", ")

/* The global buffer where we dump everything.  It is there only for
   transitional purpose.  It is expected, in the near future, to be
   completely removed.  */
static output_buffer scratch_buffer_rec;
static output_buffer *scratch_buffer = &scratch_buffer_rec;

# define NEXT_CODE(T) (TREE_CODE (TREE_TYPE (T)))

#define reinit_global_formatting_buffer() ¥
   output_clear_message_text (scratch_buffer)

static const char *args_to_string		PARAMS ((tree, int));
static const char *assop_to_string		PARAMS ((enum tree_code, int));
static const char *code_to_string		PARAMS ((enum tree_code, int));
static const char *cv_to_string			PARAMS ((tree, int));
static const char *decl_to_string		PARAMS ((tree, int));
static const char *expr_to_string		PARAMS ((tree, int));
static const char *fndecl_to_string		PARAMS ((tree, int));
static const char *op_to_string			PARAMS ((enum tree_code, int));
static const char *parm_to_string		PARAMS ((int, int));
static const char *type_to_string		PARAMS ((tree, int));

static void dump_type PARAMS ((tree, int));
static void dump_typename PARAMS ((tree, int));
static void dump_simple_decl PARAMS ((tree, tree, int));
static void dump_decl PARAMS ((tree, int));
static void dump_template_decl PARAMS ((tree, int));
static void dump_function_decl PARAMS ((tree, int));
static void dump_expr PARAMS ((tree, int));
static void dump_unary_op PARAMS ((const char *, tree, int));
static void dump_binary_op PARAMS ((const char *, tree, int));
static void dump_aggr_type PARAMS ((tree, int));
static enum pad dump_type_prefix PARAMS ((tree, int));
static void dump_type_suffix PARAMS ((tree, int));
static void dump_function_name PARAMS ((tree, int));
static void dump_expr_list PARAMS ((tree, int));
static void dump_global_iord PARAMS ((tree));
static enum pad dump_qualifiers PARAMS ((tree, enum pad));
static void dump_char PARAMS ((int));
static void dump_parameters PARAMS ((tree, int));
static void dump_exception_spec PARAMS ((tree, int));
static const char *class_key_or_enum PARAMS ((tree));
static void dump_template_argument PARAMS ((tree, int));
static void dump_template_argument_list PARAMS ((tree, int));
static void dump_template_parameter PARAMS ((tree, int));
static void dump_template_bindings PARAMS ((tree, tree));
static void dump_scope PARAMS ((tree, int));
static void dump_template_parms PARAMS ((tree, int, int));

static const char *function_category PARAMS ((tree));
static void lang_print_error_function PARAMS ((diagnostic_context *,
                                               const char *));
static void maybe_print_instantiation_context PARAMS ((output_buffer *));
static void print_instantiation_full_context PARAMS ((output_buffer *));
static void print_instantiation_partial_context PARAMS ((output_buffer *, tree,
                                                         const char *, int));
static void cp_diagnostic_starter PARAMS ((output_buffer *,
                                           diagnostic_context *));
static void cp_diagnostic_finalizer PARAMS ((output_buffer *,
                                             diagnostic_context *));
static void cp_print_error_function PARAMS ((output_buffer *,
                                             diagnostic_context *));

static int cp_printer PARAMS ((output_buffer *));
static void print_non_consecutive_character PARAMS ((output_buffer *, int));
static void print_integer PARAMS ((output_buffer *, HOST_WIDE_INT));
static tree locate_error PARAMS ((const char *, va_list));

void
init_error ()
{
  print_error_function = lang_print_error_function;
  diagnostic_starter (global_dc) = cp_diagnostic_starter;
  diagnostic_finalizer (global_dc) = cp_diagnostic_finalizer;
  diagnostic_format_decoder (global_dc) = cp_printer;

  init_output_buffer (scratch_buffer, /* prefix */NULL, /* line-width */0);
}

/* Dump a scope, if deemed necessary.  */

static void
dump_scope (scope, flags)
     tree scope;
     int flags;
{
  int f = ‾TFF_RETURN_TYPE & (flags & (TFF_SCOPE | TFF_CHASE_TYPEDEF));

  if (scope == NULL_TREE)
    return;

  if (TREE_CODE (scope) == NAMESPACE_DECL)
    {
      if (scope != global_namespace)
        {
          dump_decl (scope, f);
          print_scope_operator (scratch_buffer);
        }
    }
  else if (AGGREGATE_TYPE_P (scope))
    {
      dump_type (scope, f);
      print_scope_operator (scratch_buffer);
    }
  else if ((flags & TFF_SCOPE) && TREE_CODE (scope) == FUNCTION_DECL)
    {
      dump_function_decl (scope, f);
      print_scope_operator (scratch_buffer);
    }
}

/* Dump type qualifiers, providing padding as requested. Return an
   indication of whether we dumped something.  */

static enum pad
dump_qualifiers (t, p)
     tree t;
     enum pad p;
{
  static const int masks[] =
    {TYPE_QUAL_CONST, TYPE_QUAL_VOLATILE, TYPE_QUAL_RESTRICT};
  static const char *const names[] =
    {"const", "volatile", "__restrict"};
  int ix;
  int quals = TYPE_QUALS (t);
  int do_after = p == after;

  if (quals)
    {
      for (ix = 0; ix != 3; ix++)
        if (masks[ix] & quals)
          {
            if (p == before)
              output_add_space (scratch_buffer);
            p = before;
            print_identifier (scratch_buffer, names[ix]);
          }
      if (do_after)
        output_add_space (scratch_buffer);
    }
  else
    p = none;
  return p;
}

/* This must be large enough to hold any printed integer or floating-point
   value.  */
static char digit_buffer[128];

/* Dump the template ARGument under control of FLAGS.  */

static void
dump_template_argument (arg, flags)
     tree arg;
     int flags;
{
  if (TYPE_P (arg) || TREE_CODE (arg) == TEMPLATE_DECL)
    dump_type (arg, flags & ‾TFF_CLASS_KEY_OR_ENUM);
  else
    dump_expr (arg, (flags | TFF_EXPR_IN_PARENS) & ‾TFF_CLASS_KEY_OR_ENUM);
}

/* Dump a template-argument-list ARGS (always a TREE_VEC) under control
   of FLAGS.  */

static void
dump_template_argument_list (args, flags)
     tree args;
     int flags;
{
  int n = TREE_VEC_LENGTH (args);
  int need_comma = 0;
  int i;

  for (i = 0; i< n; ++i)
    {
      if (need_comma)
        separate_with_comma (scratch_buffer);
      dump_template_argument (TREE_VEC_ELT (args, i), flags);
      need_comma = 1;
    }
}

/* Dump a template parameter PARM (a TREE_LIST) under control of FLAGS.  */

static void
dump_template_parameter (parm, flags)
     tree parm;
     int flags;
{
  tree p = TREE_VALUE (parm);
  tree a = TREE_PURPOSE (parm);

  if (TREE_CODE (p) == TYPE_DECL)
    {
      if (flags & TFF_DECL_SPECIFIERS)
        {
          print_identifier (scratch_buffer, "class");
          if (DECL_NAME (p))
            {
              output_add_space (scratch_buffer);
              print_tree_identifier (scratch_buffer, DECL_NAME (p));
            }
        }
      else if (DECL_NAME (p))
        print_tree_identifier (scratch_buffer, DECL_NAME (p));
      else
        print_identifier (scratch_buffer, "{template default argument error}");
    }
  else
    dump_decl (p, flags | TFF_DECL_SPECIFIERS);

  if ((flags & TFF_FUNCTION_DEFAULT_ARGUMENTS) && a != NULL_TREE)
    {
      output_add_string (scratch_buffer, " = ");
      if (TREE_CODE (p) == TYPE_DECL || TREE_CODE (p) == TEMPLATE_DECL)
        dump_type (a, flags & ‾TFF_CHASE_TYPEDEF);
      else
        dump_expr (a, flags | TFF_EXPR_IN_PARENS);
    }
}

/* Dump, under control of FLAGS, a template-parameter-list binding.
   PARMS is a TREE_LIST of TREE_VEC of TREE_LIST and ARGS is a
   TREE_VEC.  */

static void
dump_template_bindings (parms, args)
     tree parms, args;
{
  int need_comma = 0;

  while (parms)
    {
      tree p = TREE_VALUE (parms);
      int lvl = TMPL_PARMS_DEPTH (parms);
      int arg_idx = 0;
      int i;

      for (i = 0; i < TREE_VEC_LENGTH (p); ++i)
	{
	  tree arg = NULL_TREE;

	  /* Don't crash if we had an invalid argument list.  */
	  if (TMPL_ARGS_DEPTH (args) >= lvl)
	    {
	      tree lvl_args = TMPL_ARGS_LEVEL (args, lvl);
	      if (NUM_TMPL_ARGS (lvl_args) > arg_idx)
		arg = TREE_VEC_ELT (lvl_args, arg_idx);
	    }

	  if (need_comma)
	    separate_with_comma (scratch_buffer);
	  dump_template_parameter (TREE_VEC_ELT (p, i), TFF_PLAIN_IDENTIFIER);
	  output_add_string (scratch_buffer, " = ");
	  if (arg)
	    dump_template_argument (arg, TFF_PLAIN_IDENTIFIER);
	  else
	    print_identifier (scratch_buffer, "<missing>");

	  ++arg_idx;
	  need_comma = 1;
	}

      parms = TREE_CHAIN (parms);
    }
}

/* Dump into the obstack a human-readable equivalent of TYPE.  FLAGS
   controls the format.  */

static void
dump_type (t, flags)
     tree t;
     int flags;
{
  if (t == NULL_TREE)
    return;

  if (TYPE_PTRMEMFUNC_P (t))
    goto offset_type;

  switch (TREE_CODE (t))
    {
    case UNKNOWN_TYPE:
      print_identifier (scratch_buffer, "<unknown type>");
      break;

    case TREE_LIST:
      /* A list of function parms.  */
      dump_parameters (t, flags);
      break;

    case IDENTIFIER_NODE:
      print_tree_identifier (scratch_buffer, t);
      break;

    case TREE_VEC:
      dump_type (BINFO_TYPE (t), flags);
      break;

    case RECORD_TYPE:
    case UNION_TYPE:
    case ENUMERAL_TYPE:
      dump_aggr_type (t, flags);
      break;

    case TYPE_DECL:
      if (flags & TFF_CHASE_TYPEDEF)
        {
          dump_type (DECL_ORIGINAL_TYPE (t)
                     ? DECL_ORIGINAL_TYPE (t) : TREE_TYPE (t), flags);
          break;
        }
      /* else fallthrough */

    case TEMPLATE_DECL:
    case NAMESPACE_DECL:
      dum