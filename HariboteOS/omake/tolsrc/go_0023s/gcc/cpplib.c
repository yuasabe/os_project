/* CPP Library. (Directive handling.)
   Copyright (C) 1986, 1987, 1989, 1992, 1993, 1994, 1995, 1996, 1997, 1998,
   1999, 2000, 2001, 2002 Free Software Foundation, Inc.
   Contributed by Per Bothner, 1994-95.
   Based on CCCP program by Paul Rubin, June 1986
   Adapted to ANSI C, Richard Stallman, Jan 1987

This program is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 2, or (at your option) any
later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#include "config.h"
#include "system.h"

/* !kawai! */
#include "cpplib.h"
#include "cpphash.h"
#include "../include/obstack.h"
/* end of !kawai! */

/* Chained list of answers to an assertion.  */
struct answer
{
  struct answer *next;
  unsigned int count;
  cpp_token first[1];
};

/* Stack of conditionals currently in progress
   (including both successful and failing conditionals).  */
struct if_stack
{
  struct if_stack *next;
  unsigned int line;		/* Line where condition started.  */
  const cpp_hashnode *mi_cmacro;/* macro name for #ifndef around entire file */
  bool skip_elses;		/* Can future #else / #elif be skipped?  */
  bool was_skipping;		/* If were skipping on entry.  */
  int type;			/* Most recent conditional, for diagnostics.  */
};

/* Contains a registered pragma or pragma namespace.  */
typedef void (*pragma_cb) PARAMS ((cpp_reader *));
struct pragma_entry
{
  struct pragma_entry *next;
  const cpp_hashnode *pragma;	/* Name and length.  */
  int is_nspace;
  union {
    pragma_cb handler;
    struct pragma_entry *space;
  } u;
};

/* Values for the origin field of struct directive.  KANDR directives
   come from traditional (K&R) C.  STDC89 directives come from the
   1989 C standard.  EXTENSION directives are extensions.  */
#define KANDR		0
#define STDC89		1
#define EXTENSION	2

/* Values for the flags field of struct directive.  COND indicates a
   conditional; IF_COND an opening conditional.  INCL means to treat
   "..." and <...> as q-char and h-char sequences respectively.  IN_I
   means this directive should be handled even if -fpreprocessed is in
   effect (these are the directives with callback hooks).  */
#define COND		(1 << 0)
#define IF_COND		(1 << 1)
#define INCL		(1 << 2)
#define IN_I		(1 << 3)

/* Defines one #-directive, including how to handle it.  */
typedef void (*directive_handler) PARAMS ((cpp_reader *));
typedef struct directive directive;
struct directive
{
  directive_handler handler;	/* Function to handle directive.  */
  const U_CHAR *name;		/* Name of directive.  */
  unsigned short length;	/* Length of name.  */
  unsigned char origin;		/* Origin of directive.  */
  unsigned char flags;	        /* Flags describing this directive.  */
};

/* Forward declarations.  */

static void skip_rest_of_line	PARAMS ((cpp_reader *));
static void check_eol		PARAMS ((cpp_reader *));
static void start_directive	PARAMS ((cpp_reader *));
static void end_directive	PARAMS ((cpp_reader *, int));
static void directive_diagnostics
	PARAMS ((cpp_reader *, const directive *, int));
static void run_directive	PARAMS ((cpp_reader *, int,
					 const char *, size_t));
static const cpp_token *glue_header_name PARAMS ((cpp_reader *));
static const cpp_token *parse_include PARAMS ((cpp_reader *));
static void push_conditional	PARAMS ((cpp_reader *, int, int,
					 const cpp_hashnode *));
static unsigned int read_flag	PARAMS ((cpp_reader *, unsigned int));
static U_CHAR *dequote_string	PARAMS ((cpp_reader *, const U_CHAR *,
					 unsigned int));
static int  strtoul_for_line	PARAMS ((const U_CHAR *, unsigned int,
					 unsigned long *));
static void do_diagnostic	PARAMS ((cpp_reader *, enum error_type, int));
static cpp_hashnode *lex_macro_node	PARAMS ((cpp_reader *));
static void do_include_common	PARAMS ((cpp_reader *, enum include_type));
static struct pragma_entry *lookup_pragma_entry
  PARAMS ((struct pragma_entry *, const cpp_hashnode *pragma));
static struct pragma_entry *insert_pragma_entry
  PARAMS ((cpp_reader *, struct pragma_entry **, const cpp_hashnode *,
	   pragma_cb));
static void do_pragma_once	PARAMS ((cpp_reader *));
static void do_pragma_poison	PARAMS ((cpp_reader *));
static void do_pragma_system_header	PARAMS ((cpp_reader *));
static void do_pragma_dependency	PARAMS ((cpp_reader *));
static void do_linemarker		PARAMS ((cpp_reader *));
static const cpp_token *get_token_no_padding PARAMS ((cpp_reader *));
static const cpp_token *get__Pragma_string PARAMS ((cpp_reader *));
static void destringize_and_run PARAMS ((cpp_reader *, const cpp_string *));
static int parse_answer PARAMS ((cpp_reader *, struct answer **, int));
static cpp_hashnode *parse_assertion PARAMS ((cpp_reader *, struct answer **,
					      int));
static struct answer ** find_answer PARAMS ((cpp_hashnode *,
					     const struct answer *));
static void handle_assertion	PARAMS ((cpp_reader *, const char *, int));

/* This is the table of directive handlers.  It is ordered by
   frequency of occurrence; the numbers at the end are directive
   counts from all the source code I have lying around (egcs and libc
   CVS as of 1999-05-18, plus grub-0.5.91, linux-2.2.9, and
   pcmcia-cs-3.0.9).  This is no longer important as directive lookup
   is now O(1).  All extensions other than #warning and #include_next
   are deprecated.  The name is where the extension appears to have
   come from.  */

#define DIRECTIVE_TABLE							¥
D(define,	T_DEFINE = 0,	KANDR,     IN_I)	   /* 270554 */ ¥
D(include,	T_INCLUDE,	KANDR,     INCL)	   /*  52262 */ ¥
D(endif,	T_ENDIF,	KANDR,     COND)	   /*  45855 */ ¥
D(ifdef,	T_IFDEF,	KANDR,     COND | IF_COND) /*  22000 */ ¥
D(if,		T_IF,		KANDR,     COND | IF_COND) /*  18162 */ ¥
D(else,		T_ELSE,		KANDR,     COND)	   /*   9863 */ ¥
D(ifndef,	T_IFNDEF,	KANDR,     COND | IF_COND) /*   9675 */ ¥
D(undef,	T_UNDEF,	KANDR,     IN_I)	   /*   4837 */ ¥
D(line,		T_LINE,		KANDR,     0)		   /*   2465 */ ¥
D(elif,		T_ELIF,		STDC89,    COND)	   /*    610 */ ¥
D(error,	T_ERROR,	STDC89,    0)		   /*    475 */ ¥
D(pragma,	T_PRAGMA,	STDC89,    IN_I)	   /*    195 */ ¥
D(warning,	T_WARNING,	EXTENSION, 0)		   /*     22 */ ¥
D(include_next,	T_INCLUDE_NEXT,	EXTENSION, INCL)	   /*     19 */ ¥
D(ident,	T_IDENT,	EXTENSION, IN_I)	   /*     11 */ ¥
D(import,	T_IMPORT,	EXTENSION, INCL)	   /* 0 ObjC */	¥
D(assert,	T_ASSERT,	EXTENSION, 0)		   /* 0 SVR4 */	¥
D(unassert,	T_UNASSERT,	EXTENSION, 0)		   /* 0 SVR4 */	¥
SCCS_ENTRY						   /* 0 SVR4? */

/* #sccs is not always recognized.  */
#ifdef SCCS_DIRECTIVE
# define SCCS_ENTRY D(sccs, T_SCCS, EXTENSION, 0)
#else
# define SCCS_ENTRY /* nothing */
#endif

/* Use the table to generate a series of prototypes, an enum for the
   directive names, and an array of directive handlers.  */

/* Don't invoke CONCAT2 with any whitespace or K&R cc will fail.  */
#define D(name, t, o, f) static void CONCAT2(do_,name) PARAMS ((cpp_reader *));
DIRECTIVE_TABLE
#undef D

#define D(n, tag, o, f) tag,
enum
{
  DIRECTIVE_TABLE
  N_DIRECTIVES
};
#undef D

/* Don't invoke CONCAT2 with any whitespace or K&R cc will fail.  */
#define D(name, t, origin, flags) ¥
{ CONCAT2(do_,name), (const U_CHAR *) STRINGX(name), ¥
  sizeof STRINGX(name) - 1, origin, flags },
static const directive dtable[] =
{
DIRECTIVE_TABLE
};
#undef D
#undef DIRECTIVE_TABLE

/* Wrapper struct directive for linemarkers.
   The origin is more or less true - the original K+R cpp
   did use this notation in its preprocessed output.  */
static const directive linemarker_dir =
{
  do_linemarker, U"#", 1, K