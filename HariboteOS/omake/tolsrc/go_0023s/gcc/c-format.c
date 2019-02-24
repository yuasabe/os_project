/* Check calls to formatted I/O functions (-Wformat).
   Copyright (C) 1992, 1993, 1994, 1995, 1996, 1997, 1998, 1999, 2000,
   2001, 2002 Free Software Foundation, Inc.

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

#include "config.h"
#include "system.h"
#include "tree.h"
#include "flags.h"
#include "toplev.h"
#include "c-common.h"
#include "intl.h"
#include "diagnostic.h"


/* Command line options and their associated flags.  */

/* Warn about format/argument anomalies in calls to formatted I/O functions
   (*printf, *scanf, strftime, strfmon, etc.).  */

int warn_format;

/* Warn about Y2K problems with strftime formats.  */

int warn_format_y2k;

/* Warn about excess arguments to formats.  */

int warn_format_extra_args;

/* Warn about non-literal format arguments.  */

int warn_format_nonliteral;

/* Warn about possible security problems with calls to format functions.  */

int warn_format_security;

/* Set format warning options according to a -Wformat=n option.  */

void
set_Wformat (setting)
     int setting;
{
  warn_format = setting;
  warn_format_y2k = setting;
  warn_format_extra_args = setting;
  if (setting != 1)
    {
      warn_format_nonliteral = setting;
      warn_format_security = setting;
    }
}


/* Handle attributes associated with format checking.  */

/* This must be in the same order as format_types, with format_type_error
   last.  */
enum format_type { printf_format_type, scanf_format_type,
		   strftime_format_type, strfmon_format_type,
		   format_type_error };

typedef struct function_format_info
{
  enum format_type format_type;	/* type of format (printf, scanf, etc.) */
  unsigned HOST_WIDE_INT format_num;	/* number of format argument */
  unsigned HOST_WIDE_INT first_arg_num;	/* number of first arg (zero for varargs) */
} function_format_info;

static bool decode_format_attr		PARAMS ((tree,
						 function_format_info *, int));
static enum format_type decode_format_type	PARAMS ((const char *));

/* Handle a "format" attribute; arguments as in
   struct attribute_spec.handler.  */
tree
handle_format_attribute (node, name, args, flags, no_add_attrs)
     tree *node;
     tree name ATTRIBUTE_UNUSED;
     tree args;
     int flags;
     bool *no_add_attrs;
{
  tree type = *node;
  function_format_info info;
  tree argument;
  unsigned HOST_WIDE_INT arg_num;

  if (!decode_format_attr (args, &info, 0))
    {
      *no_add_attrs = true;
      return NULL_TREE;
    }

  /* If a parameter list is specified, verify that the format_num
     argument is actually a string, in case the format attribute
     is in error.  */
  argument = TYPE_ARG_TYPES (type);
  if (argument)
    {
      for (arg_num = 1; argument != 0 && arg_num != info.format_num;
	   ++arg_num, argument = TREE_CHAIN (argument))
	;

      if (! argument
	  || TREE_CODE (TREE_VALUE (argument)) != POINTER_TYPE
	  || (TYPE_MAIN_VARIANT (TREE_TYPE (TREE_VALUE (argument)))
	      != char_type_node))
	{
	  if (!(flags & (int) ATTR_FLAG_BUILT_IN))
	    error ("format string arg not a string type");
	  *no_add_attrs = true;
	  return NULL_TREE;
	}

      else if (info.first_arg_num != 0)
	{
	  /* Verify that first_arg_num points to the last arg,
	     the ...  */
	  while (argument)
	    arg_num++, argument = TREE_CHAIN (argument);

	  if (arg_num != info.first_arg_num)
	    {
	      if (!(flags & (int) ATTR_FLAG_BUILT_IN))
		error ("args to be formatted is not '...'");
	      *no_add_attrs = true;
	      return NULL_TREE;
	    }
	}
    }

  if (info.format_type == strftime_format_type && info.first_arg_num != 0)
    {
      error ("strftime formats cannot format arguments");
      *no_add_attrs = true;
      return NULL_TREE;
    }

  return NULL_TREE;
}


/* Handle a "format_arg" attribute; arguments as in
   struct attribute_spec.handler.  */
tree
handle_format_arg_attribute (node, name, args, flags, no_add_attrs)
     tree *node;
     tree name ATTRIBUTE_UNUSED;
     tree args;
     int flags;
     bool *no_add_attrs;
{
  tree type = *node;
  tree format_num_expr = TREE_VALUE (args);
  unsigned HOST_WIDE_INT format_num;
  unsigned HOST_WIDE_INT arg_num;
  tree argument;

  /* Strip any conversions from the first arg number and verify it
     is a constant.  */
  while (TREE_CODE (format_num_expr) == NOP_EXPR
	 || TREE_CODE (format_num_expr) == CONVERT_EXPR
	 || TREE_CODE (format_num_expr) == NON_LVALUE_EXPR)
    format_num_expr = TREE_OPERAND (format_num_expr, 0);

  if (TREE_CODE (format_num_expr) != INTEGER_CST
      || TREE_INT_CST_HIGH (format_num_expr) != 0)
    {
      error ("format string has invalid operand number");
      *no_add_attrs = true;
      return NULL_TREE;
    }

  format_num = TREE_INT_CST_LOW (format_num_expr);

  /* If a parameter list is specified, verify that the format_num
     argument is actually a string, in case the format attribute
     is in error.  */
  argument = TYPE_ARG_TYPES (type);
  if (argument)
    {
      for (arg_num = 1; argument != 0 && arg_num != format_num;
	   ++arg_num, argument = TREE_CHAIN (argument))
	;

      if (! argument
	  || TREE_CODE (TREE_VALUE (argument)) != POINTER_TYPE
	  || (TYPE_MAIN_VARIANT (TREE_TYPE (TREE_VALUE (argument)))
	      != char_type_node))
	{
	  if (!(flags & (int) ATTR_FLAG_BUILT_IN))
	    error ("format string arg not a string type");
	  *no_add_attrs = true;
	  return NULL_TREE;
	}
    }

  if (TREE_CODE (TREE_TYPE (type)) != POINTER_TYPE
      || (TYPE_MAIN_VARIANT (TREE_TYPE (TREE_TYPE (type)))
	  != char_type_node))
    {
      if (!(flags & (int) ATTR_FLAG_BUILT_IN))
	error ("function does not return string type");
      *no_add_attrs = true;
      return NULL_TREE;
    }

  return NULL_TREE;
}


/* Decode the arguments to a "format" attribute into a function_format_info
   structure.  It is already known that the list is of the right length.
   If VALIDATED_P is true, then these attributes have already been validated
   and this function will abort if they are erroneous; if false, it
   will give an error message.  Returns true if the attributes are
   successfully decoded, false otherwise.  */

static bool
decode_format_attr (args, info, validated_p)
     tree args;
     function_format_info *info;
     int validated_p;
{
  tree format_type_id = TREE_VALUE (args);
  tree format_num_expr = TREE_VALUE (TREE_CHAIN (args));
  tree first_arg_num_expr
    = TREE_VALUE (TREE_CHAIN (TREE_CHAIN (args)));

  if (TREE_CODE (format_type_id) != IDENTIFIER_NODE)
    {
      if (validated_p)
	abort ();
      error ("unrecognized format specifier");
      return false;
    }
  else
    {
      const char *p = IDENTIFIER_POINTER (format_type_id);

      info->format_type = decode_format_type (p);

      if (info->format_type == format_type_error)
	{
	  if (validated_p)
	    abort ();
	  warning ("`%s' is an unrecognized format function type", p);
	  return false;
	}
    }

  /* Strip any conversions from the string index and first arg number
     and verify they are constants.  */
  while (TREE_CODE (format_num_expr) == NOP_EXPR
	 || TREE_CODE (format_num_expr) == CONVERT_EXPR
	 || TREE_CODE (format_num_expr) == NON_LVALUE_EXPR)
    format_num_expr = TREE_OPERAND (format_num_expr, 0);

  while (TREE_CODE (first_arg_num_expr) == NOP_EXPR
	 || TREE_CODE (first_arg_num_expr) == CONVERT_EXPR
	 || TREE_CODE (first_arg_num_expr) == NON_LVALUE_EXPR)
    first_arg_num_expr = TREE_OPERAND (first_arg_num_expr, 0);

  if (TREE_CODE (format_num_expr) != INTEGER_CST
      || TREE_INT_CST_HIGH (format_num_expr) != 0
      || TREE_CODE (first_arg_num_expr) != INTEGER_CST
      || TREE_INT_CST_HIGH (first_arg_num_expr) != 0)
    {
      if (validated_p)
	abort ();
      error ("format string has invalid operand number");
      return false;
    }

  info->format_num = TREE_INT_CST_LOW (format_num_expr);
  info->first_arg_num = TREE_INT_CST_LOW (first_arg_num_expr);
  if (info->first_arg_num != 0 && info->first_arg_num <= info->format_num)
    {
      if (validated_p)
	abort ();
      error ("format string arg follows the args to be formatted");
      return false;
    }

  return true;
}

/* Check a call to a format function against a parameter list.  */

/* The meaningfully distinct length modifiers for format checking recognised
   by GCC.  */
enum format_lengths
{
  FMT_LEN_none,
  FMT_LEN_hh,
  FMT_LEN_h,
  FMT_LEN_l,
  FMT_LEN_ll,
  FMT_LEN_L,
  FMT_LEN_z,
  FMT_LEN_t,
  FMT_LEN_j,
  FMT_LEN_MAX
};


/* The standard versions in which various format features appeared.  */
enum format_std_version
{
  STD_C89,
  STD_C94,
  STD_C9L, /* C99, but treat as C89 if -Wno-long-long.  */
  STD_C99,
  STD_EXT
};

/* The C standard version C++ is treated as equivalent to
   or inheriting from, for the purpose of format features supported.  */
#define CPLUSPLUS_STD_VER	STD_C94
/* The C standard version we are checking formats against when pedantic.  */
#define C_STD_VER		((int)(c_language == clk_cplusplus	  ¥
				 ? CPLUSPLUS_STD_VER			  ¥
				 : (flag_isoc99				  ¥
				    ? STD_C99				  ¥
				    : (flag_isoc94 ? STD_C94 : STD_C89))))
/* The name to give to the standard version we are warning about when
   pedantic.  FEATURE_VER is the version in which the feature warned out
   appeared, which is higher than C_STD_VER.  */
#define C_STD_NAME(FEATURE_VER) (c_language == clk_cplusplus	¥
				 ? "ISO C++"			¥
				 : ((FEATURE_VER) == STD_EXT	¥
				    ? "ISO C"			¥
				    : "ISO C89"))
/* Adjust a C standard version, which may be STD_C9L, to account for
   -Wno-long-long.  Returns other standard versions unchanged.  */
#define ADJ_STD(VER)		((int)((VER) == STD_C9L			      ¥
				       ? (warn_long_long ? STD_C99 : STD_C89) ¥
				       : (VER)))

/* Flags that may apply to a particular kind of format checked by GCC.  */
enum
{
  /* This format converts arguments of types determined by the
     format string.  */
  FMT_FLAG_ARG_CONVERT = 1,
  /* The scanf allocation 'a' kludge applies to this format kind.  */
  FMT_FLAG_SCANF_A_KLUDGE = 2,
  /* A % during parsing a specifier is allowed to be a modified % rather
     that indicating the format is broken and we are out-of-sync.  */
  FMT_FLAG_FANCY_PERCENT_OK = 4,
  /* With $ operand numbers, it is OK to reference the same argument more
     than once.  */
  FMT_FLAG_DOLLAR_MULTIPLE = 8,
  /* This format type uses $ operand numbers (strfmon doesn't).  */
  FMT_FLAG_USE_DOLLAR = 16,
  /* Zero width is bad in this type of format (scanf).  */
  FMT_FLAG_ZERO_WIDTH_BAD = 32,
  /* Empty precision specification is OK in this type of format (printf).  */
  FMT_FLAG_EMPTY_PREC_OK = 64,
  /* Gaps are allowed in the arguments with $ operand numbers if all
     arguments are pointers (scanf).  */
  FMT_FLAG_DOLLAR_GAP_POINTER_OK = 128
  /* Not included here: details of whether width or precision may occur
     (controlled by width_char and precision_char); details of whether
     '*' can be used for these (width_type and precision_type); details
     of whether length modifiers can occur (length_char_specs).  */
};


/* Structure describing a length modifier supported in format checking, and
   possibly a doubled version such as "hh".  */
typedef struct
{
  /* Name of the single-character length modifier.  */
  const char *const name;
  /* Index into a format_char_info.types array.  */
  const enum format_lengths index;
  /* Standard version this length appears in.  */
  const enum format_std_version std;
  /* Same, if the modifier can be repeated, or NULL if it can't.  */
  const char *const double_name;
  const enum format_lengths double_index;
  const enum format_std_version double_std;
} format_length_info;


/* Structure describing the combination of a conversion specifier
   (or a set of specifiers which act identically) and a length modifier.  */
typedef struct
{
  /* The standard version this combination of length and type appeared in.
     This is only relevant if greater than those for length and type
     individually; otherwise it is ignored.  */
  enum format_std_version std;
  /* The name to use for the type, if different from that generated internally
     (e.g., "signed size_t").  */
  const char *name;
  /* The type itself.  */
  tree *type;
} format_type_detail;


/* Macros to fill out tables of these.  */
#define BADLEN	{ 0, NULL, NULL }
#define NOLENGTHS	{ BADLEN, BADLEN, BADLEN, BADLEN, BADLEN, BADLEN, BADLEN, BADLEN, BADLEN }


/* Structure describing a format conversion specifier (or a set of specifiers
   which act identically), and the length modifiers used with it.  */
typedef struct
{
  const char *const format_chars;
  const int pointer_count;
  const enum format_std_version std;
  /* Types accepted for each length modifier.  */
  const format_type_detail types[FMT_LEN_MAX];
  /* List of other modifier characters allowed with these specifiers.
     This lists flags, and additionally "w" for width, "p" for precision
     (right precision, for strfmon), "#" for left precision (strfmon),
     "a" for scanf "a" allocation extension (not applicable in C99 mode),
     "*" for scanf suppression, and "E" and "O" for those strftime
     modifiers.  */
  const char *const flag_chars;
  /* List of additional flags describing these conversion specifiers.
     "c" for generic character pointers being allowed, "2" for strftime
     two digit year formats, "3" for strftime formats giving two digit
     years in some locales, "4" for "2" which becomes "3" with an "E" modifier,
     "o" if use of strftime "O" is a GNU extension beyond C99,
     "W" if the argument is a pointer which is dereferenced and written into,
     "R" if the argument is a pointer which is dereferenced and read from,
     "i" for printf integer formats where the '0' flag is ignored with
     precision, and "[" for the starting character of a scanf scanset.  */
  const char *const flags2;
} format_char_info;


/* Structure describing a flag accepted by some kind of format.  */
typedef struct
{
  /* The flag character in question (0 for end of array).  */
  const int flag_char;
  /* Zero if this entry describes the flag character in general, or a
     non-zero character that may be found in flags2 if it describes the
     flag when used with certain formats only.  If the latter, only
     the first such entry found that applies to the current conversion
     specifier is used; the values of `name' and `long_name' it supplies
     will be used, if non-NULL and the standard version is higher than
     the unpredicated one, for any pedantic warning.  For example, 'o'
     for strftime formats (meaning 'O' is an extension over C99).  */
  const int predicate;
  /* Nonzero if the next character after this flag in the format should
     be skipped ('=' in strfmon), zero otherwise.  */
  const int skip_next_char;
  /* The name to use for this flag in diagnostic messages.  For example,
     N_("`0' flag"), N_("field width").  */
  const char *const name;
  /* Long name for this flag in diagnostic messages; currently only used for
     "ISO C does not support ...".  For example, N_("the `I' printf flag").  */
  const char *const long_name;
  /* The standard version in which it appeared.  */
  const enum format_std_version std;
} format_flag_spec;


/* Structure describing a combination of flags that is bad for some kind
   of for