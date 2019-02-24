/* Separate lexical analyzer for GNU C++.
   Copyright (C) 1987, 1989, 1992, 1993, 1994, 1995, 1996, 1997, 1998,
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


/* This file is the lexical analyzer for GNU C++.  */

/* Cause the `yydebug' variable to be defined.  */
#define YYDEBUG 1

/* !kawai! */
#include "../gcc/config.h"
#include "../gcc/system.h"
#include "../gcc/input.h"
#include "../gcc/tree.h"
#include "cp-tree.h"
#include "../gcc/cpplib.h"
#include "../gcc/c-lex.h"
#include "lex.h"
#include "parse.h"
#include "../gcc/flags.h"
#include "../gcc/c-pragma.h"
#include "../gcc/toplev.h"
#include "../gcc/output.h"
#include "../gcc/ggc.h"
#include "../gcc/tm_p.h"
#include "../gcc/timevar.h"
#include "../gcc/diagnostic.h"
/* end of !kawai! */

#ifdef MULTIBYTE_CHARS
#include "mbchar.h"
#include <locale.h>
#endif

extern void yyprint PARAMS ((FILE *, int, YYSTYPE));

static int interface_strcmp PARAMS ((const char *));
static int *init_cpp_parse PARAMS ((void));
static void init_cp_pragma PARAMS ((void));

static tree parse_strconst_pragma PARAMS ((const char *, int));
static void handle_pragma_vtable PARAMS ((cpp_reader *));
static void handle_pragma_unit PARAMS ((cpp_reader *));
static void handle_pragma_interface PARAMS ((cpp_reader *));
static void handle_pragma_implementation PARAMS ((cpp_reader *));
static void handle_pragma_java_exceptions PARAMS ((cpp_reader *));

#ifdef GATHER_STATISTICS
#ifdef REDUCE_LENGTH
static int reduce_cmp PARAMS ((int *, int *));
static int token_cmp PARAMS ((int *, int *));
#endif
#endif
static int is_global PARAMS ((tree));
static void init_operators PARAMS ((void));
static void copy_lang_type PARAMS ((tree));

/* A constraint that can be tested at compile time.  */
#ifdef __STDC__
#define CONSTRAINT(name, expr) extern int constraint_##name [(expr) ? 1 : -1]
#else
#define CONSTRAINT(name, expr) extern int constraint_/**/name [(expr) ? 1 : -1]
#endif

/* !kawai! */
/* #include "cpplib.h" */
/* end of !kawai! */

extern int yychar;		/*  the lookahead symbol		*/
extern YYSTYPE yylval;		/*  the semantic value of the		*/
				/*  lookahead symbol			*/

/* These flags are used by c-lex.c.  In C++, they're always off and on,
   respectively.  */
int warn_traditional = 0;
int flag_digraphs = 1;

/* the declaration found for the last IDENTIFIER token read in.
   yylex must look this up to detect typedefs, which get token type TYPENAME,
   so it is left around in case the identifier is not a typedef but is
   used in a context which makes it a reference to a variable.  */
tree lastiddecl;

/* Array for holding counts of the numbers of tokens seen.  */
extern int *token_count;

/* Functions and data structures for #pragma interface.

   `#pragma implementation' means that the main file being compiled
   is considered to implement (provide) the classes that appear in
   its main body.  I.e., if this is file "foo.cc", and class `bar'
   is defined in "foo.cc", then we say that "foo.cc implements bar".

   All main input files "implement" themselves automagically.

   `#pragma interface' means that unless this file (of the form "foo.h"
   is not presently being included by file "foo.cc", the
   CLASSTYPE_INTERFACE_ONLY bit gets set.  The effect is that none
   of the vtables nor any of the inline functions defined in foo.h
   will ever be output.

   There are cases when we want to link files such as "defs.h" and
   "main.cc".  In this case, we give "defs.h" a `#pragma interface',
   and "main.cc" has `#pragma implementation "defs.h"'.  */

struct impl_files
{
  const char *filename;
  struct impl_files *next;
};

static struct impl_files *impl_file_chain;


/* Return something to represent absolute declarators containing a *.
   TARGET is the absolute declarator that the * contains.
   CV_QUALIFIERS is a list of modifiers such as const or volatile
   to apply to the pointer type, represented as identifiers.

   We return an INDIRECT_REF whose "contents" are TARGET
   and whose type is the modifier list.  */

tree
make_pointer_declarator (cv_qualifiers, target)
     tree cv_qualifiers, target;
{
  if (target && TREE_CODE (target) == IDENTIFIER_NODE
      && ANON_AGGRNAME_P (target))
    error ("type name expected before `*'");
  target = build_nt (INDIRECT_REF, target);
  TREE_TYPE (target) = cv_qualifiers;
  return target;
}

/* Return something to represent absolute declarators containing a &.
   TARGET is the absolute declarator that the & contains.
   CV_QUALIFIERS is a list of modifiers such as const or volatile
   to apply to the reference type, represented as identifiers.

   We return an ADDR_EXPR whose "contents" are TARGET
   and whose type is the modifier list.  */

tree
make_reference_declarator (cv_qualifiers, target)
     tree cv_qualifiers, target;
{
  if (target)
    {
      if (TREE_CODE (target) == ADDR_EXPR)
	{
	  error ("cannot declare references to references");
	  return target;
	}
      if (TREE_CODE (target) == INDIRECT_REF)
	{
	  error ("cannot declare pointers to references");
	  return target;
	}
      if (TREE_CODE (target) == IDENTIFIER_NODE && ANON_AGGRNAME_P (target))
	  error ("type name expected before `&'");
    }
  target = build_nt (ADDR_EXPR, target);
  TREE_TYPE (target) = cv_qualifiers;
  return target;
}

tree
make_call_declarator (target, parms, cv_qualifiers, exception_specification)
     tree target, parms, cv_qualifiers, exception_specification;
{
  target = build_nt (CALL_EXPR, target,
		     tree_cons (parms, cv_qualifiers, NULL_TREE),
		     /* The third operand is really RTL.  We
			shouldn't put anything there.  */
		     NULL_TREE);
  CALL_DECLARATOR_EXCEPTION_SPEC (target) = exception_specification;
  return target;
}

void
set_quals_and_spec (call_declarator, cv_qualifiers, exception_specification)
     tree call_declarator, cv_qualifiers, exception_specification;
{
  CALL_DECLARATOR_QUALS (call_declarator) = cv_qualifiers;
  CALL_DECLARATOR_EXCEPTION_SPEC (call_declarator) = exception_specification;
}

int interface_only;		/* whether or not current file is only for
				   interface definitions.  */
int interface_unknown;		/* whether or not we know this class
				   to behave according to #pragma interface.  */

/* Tree code classes. */

#define DEFTREECODE(SYM, NAME, TYPE, LENGTH) TYPE,

static const char cplus_tree_code_type[] = {
  'x',
#include "cp-tree.def"
};
#undef DEFTREECODE

/* Table indexed by tree code giving number of expression
   operands beyond the fixed part of the node structure.
   Not used for types or decls.  */

#define DEFTREECODE(SYM, NAME, TYPE, LENGTH) LENGTH,

static const int cplus_tree_code_length[] = {
  0,
#include "cp-tree.def"
};
#undef DEFTREECODE

/* Names of tree components.
   Used for printing out the tree and error messages.  */
#define DEFTREECODE(SYM, NAME, TYPE, LEN) NAME,

static const char *const cplus_tree_code_name[] = {
  "@@dummy",
#include "cp-tree.def"
};
#undef DEFTREECODE

/* Initialization before switch parsing.  */
void
cxx_init_options ()
{
  c_common_init_options (clk_cplusplus);

  /* Default exceptions on.  */
  flag_exceptions = 1;
  /* By default wrap lines at 80 characters.  Is getenv ("COLUMNS")
     preferable?  */
  diagnostic_line_cutoff (global_dc) = 80;
  /* By default, emit location information once for every
     diagnostic message.  */
  diagnostic_prefixing_rule (global_dc) = DIAGNOSTICS_SHOW_PREFIX_ONCE;
}

void
cxx_finish ()
{
  c_common_finish ();
}

static int *
init_cpp_parse ()
{
#ifdef GATHER_STATISTICS
#ifdef REDUCE_LENGTH
  reduce_count = (int *) xcalloc (sizeof (int), (REDUCE_LENGTH + 1));
  reduce_count += 1;
  token_count = (int *) xcalloc (sizeof (int), (TOKEN_LENGTH + 1));
  token_count += 1;
#endif
#endif
  return token_count;
}

/* A mapping from tree codes to operator name information.  */
operator_name_info_t operator_name_info[(int) LAST_CPLUS_TREE_CODE];
/* Similar, but for assignment operators.  */
operator_name_info_t assignment_operator_name_info[(int) LAST_CPLUS_TREE_CODE];

/* Initialize data structures that keep track of operator names.  */

#define DEF_OPERATOR(NAME, C, M, AR, AP) ¥
 CONSTRAINT (C, sizeof "operator " + sizeof NAME <= 256);
#include "operators.def"
#undef DEF_OPERATOR

static void
init_operators ()
{
  tree identifier;
  char buffer[256];
  struct operator_name_info_t *oni;

#define DEF_OPERATOR(NAME, CODE, MANGLING, ARITY, ASSN_P)		    ¥
  sprintf (buffer, ISALPHA (NAME[0]) ? "operator %s" : "operator%s", NAME); ¥
  identifier = get_identifier (buffer);					    ¥
  IDENTIFIER_OPNAME_P (identifier) = 1;					    ¥
									    ¥
  oni = (ASSN_P								    ¥
	 ? &assignment_operator_name_info[(int) CODE]			    ¥
	 : &operator_name_info[(int) CODE]);				    ¥
  oni->identifier = identifier;						    ¥
  oni->name = NAME;							    ¥
  oni->mangled_name = MANGLING;

#include "operators.def"
#undef DEF_OPERATOR

  operator_name_info[(int) ERROR_MARK].identifier
    = get_identifier ("<invalid operator>");

  /* Handle some special cases.  These operators are not defined in
     the language, but can be produced internally.  We may need them
     for error-reporting.  (Eventually, we should ensure that this
     does not happen.  Error messages involving these operators will
     be confusing to users.)  */

  operator_name_info [(int) INIT_EXPR].name
    = operator_name_info [(int) MODIFY_EXPR].name;
  operator_name_info [(int) EXACT_DIV_EXPR].name = "(ceiling /)";
  operator_name_info [(int) CEIL_DIV_EXPR].name = "(ceiling /)";
  operator_name_info [(int) FLOOR_DIV_EXPR].name = "(floor /)";
  operator_name_info [(int) ROUND_DIV_EXPR].name = "(round /)";
  operator_name_info [(int) CEIL_MOD_EXPR].name = "(ceiling %)";
  operator_name_info [(int) FLOOR_MOD_EXPR].name = "(floor %)";
  operator_name_info [(int) ROUND_MOD_EXPR].name = "(round %)";
  operator_name_info [(int) ABS_EXPR].name = "abs";
  operator_name_info [(int) FFS_EXPR].name = "ffs";
  operator_name_info [(int) BIT_ANDTC_EXPR].name = "&‾";
  operator_name_info [(int) TRUTH_AND_EXPR].name = "strict &&";
  operator_name_info [(int) TRUTH_OR_EXPR].name = "strict ||";
  operator_name_info [(int) IN_EXPR].name = "in";
  operator_name_info [(int) RANGE_EXPR].name = "...";
  operator_name_info [(int) CONVERT_EXPR].name = "+";

  assignment_operator_name_info [(int) EXACT_DIV_EXPR].name
    = "(exact /=)";
  assignment_operator_name_info [(int) CEIL_DIV_EXPR].name
    = "(ceiling /=)";
  assignment_operator_name_info [(int) FLOOR_DIV_EXPR].name
    = "(floor /=)";
  assignment_operator_name_info [(int) ROUND_DIV_EXPR].name
    = "(round /=)";
  assignment_operator_name_info [(int) CEIL_MOD_EXPR].name
    = "(ceiling %=)";
  assignment_operator_name_info [(int) FLOOR_MOD_EXPR].name
    = "(floor %=)";
  assignment_operator_name_info [(int) ROUND_MOD_EXPR].name
    = "(round %=)";
}

/* The reserved keyword table.  */
struct resword
{
  const char *const word;
  const ENUM_BITFIELD(rid) rid : 16;
  const unsigned int disable   : 16;
};

/* Disable mask.  Keywords are disabled if (reswords[i].disable & mask) is
   _true_.  */
#define D_EXT		0x01	/* GCC extension */
#define D_ASM		0x02	/* in C99, but has a switch to turn i