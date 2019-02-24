/* Type Analyzer for GNU C++.
   Copyright (C) 1987, 1989, 1992, 1993, 1994, 1995, 1996, 1997, 1998,
   1999, 2000, 2001, 2002 Free Software Foundation, Inc.
   Hacked... nay, bludgeoned... by Mark Eichin (eichin@cygnus.com)

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

/* This file is the type analyzer for GNU C++.  To debug it, define SPEW_DEBUG
   when compiling parse.c and spew.c.  */

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
#include "../include/obstack.h"
#include "../gcc/toplev.h"
#include "../gcc/ggc.h"
#include "../gcc/intl.h"
#include "../gcc/timevar.h"
/* end of !kawai! */

#ifdef SPEW_DEBUG
#define SPEW_INLINE
#else
#define SPEW_INLINE inline
#endif

/* This takes a token stream that hasn't decided much about types and
   tries to figure out as much as it can, with excessive lookahead and
   backtracking.  */

/* fifo of tokens recognized and available to parser.  */
struct token
{
  /* The values for YYCHAR will fit in a short.  */
  short		yychar;
  unsigned int	lineno;
  YYSTYPE	yylval;
};

/* Since inline methods can refer to text which has not yet been seen,
   we store the text of the method in a structure which is placed in the
   DECL_PENDING_INLINE_INFO field of the FUNCTION_DECL.
   After parsing the body of the class definition, the FUNCTION_DECL's are
   scanned to see which ones have this field set.  Those are then digested
   one at a time.

   This function's FUNCTION_DECL will have a bit set in its common so
   that we know to watch out for it.  */

struct unparsed_text
{
  struct unparsed_text *next;	/* process this one next */
  tree decl;		/* associated declaration */
  const char *filename;	/* name of file we were processing */
  int lineno;		/* line number we got the text from */
  int interface;	/* remembering interface_unknown and interface_only */

  struct token *pos;	/* current position, when rescanning */
  struct token *limit;	/* end of saved text */
};

/* Stack of state saved off when we return to an inline method or
   default argument that has been stored for later parsing.  */
struct feed
{
  struct unparsed_text *input;
  const char *filename;
  int lineno;
  int yychar;
  YYSTYPE yylval;
  int first_token;
  struct obstack token_obstack;
  struct feed *next;
};  

static struct obstack feed_obstack;
static struct feed *feed;

static SPEW_INLINE void do_aggr PARAMS ((void));
static SPEW_INLINE int identifier_type PARAMS ((tree));
static void scan_tokens PARAMS ((int));
static void feed_defarg PARAMS ((tree));
static void finish_defarg PARAMS ((void));
static int read_token PARAMS ((struct token *));

static SPEW_INLINE int num_tokens PARAMS ((void));
static SPEW_INLINE struct token *nth_token PARAMS ((int));
static SPEW_INLINE int add_token PARAMS ((struct token *));
static SPEW_INLINE int shift_token PARAMS ((void));
static SPEW_INLINE void push_token PARAMS ((struct token *));
static SPEW_INLINE void consume_token PARAMS ((void));
static SPEW_INLINE int read_process_identifier PARAMS ((YYSTYPE *));

static SPEW_INLINE void feed_input PARAMS ((struct unparsed_text *));
static SPEW_INLINE void snarf_block PARAMS ((const char *, int));
static tree snarf_defarg PARAMS ((void));
static int frob_id PARAMS ((int, int, tree *));

/* The list of inline functions being held off until we reach the end of
   the current class declaration.  */
static struct unparsed_text *pending_inlines;
static struct unparsed_text *pending_inlines_tail;

/* The list of previously-deferred inline functions currently being parsed.
   This exists solely to be a GC root.  */
static struct unparsed_text *processing_these_inlines;

static void begin_parsing_inclass_inline PARAMS ((struct unparsed_text *));

#ifdef SPEW_DEBUG
int spew_debug = 0;
static unsigned int yylex_ctr = 0;

static void debug_yychar PARAMS ((int));

/* In parse.y: */
extern char *debug_yytranslate PARAMS ((int));
#endif
static enum cpp_ttype last_token;
static tree last_token_id;

/* From lex.c: */
/* the declaration found for the last IDENTIFIER token read in.
   yylex must look this up to detect typedefs, which get token type TYPENAME,
   so it is left around in case the identifier is not a typedef but is
   used in a context which makes it a reference to a variable.  */
extern tree lastiddecl;		/* let our brains leak out here too */
extern int	yychar;		/*  the lookahead symbol		*/
extern YYSTYPE	yylval;		/*  the semantic value of the		*/
				/*  lookahead symbol			*/
/* The token fifo lives in this obstack.  */
struct obstack token_obstack;
int first_token;

/* Sometimes we need to save tokens for later parsing.  If so, they are
   stored on this obstack.  */
struct obstack inline_text_obstack;
char *inline_text_firstobj;

/* When we see a default argument in a method declaration, we snarf it as
   text using snarf_defarg.  When we get up to namespace scope, we then go
   through and parse all of them using do_pending_defargs.  Since yacc
   parsers are not reentrant, we retain defargs state in these two
   variables so that subsequent calls to do_pending_defargs can resume
   where the previous call left off. DEFARG_FNS is a tree_list where 
   the TREE_TYPE is the current_class_type, TREE_VALUE is the FUNCTION_DECL,
   and TREE_PURPOSE is the list unprocessed dependent functions.  */

static tree defarg_fns;     /* list of functions with unprocessed defargs */
static tree defarg_parm;    /* current default parameter */
static tree defarg_depfns;  /* list of unprocessed fns met during current fn. */
static tree defarg_fnsdone; /* list of fns with circular defargs */

/* Initialize obstacks. Called once, from cxx_init.  */

void
init_spew ()
{
  gcc_obstack_init (&inline_text_obstack);
  inline_text_firstobj = (char *) obstack_alloc (&inline_text_obstack, 0);
  gcc_obstack_init (&token_obstack);
  gcc_obstack_init (&feed_obstack);
  ggc_add_tree_root (&defarg_fns, 1);
  ggc_add_tree_root (&defarg_parm, 1);
  ggc_add_tree_root (&defarg_depfns, 1);
  ggc_add_tree_root (&defarg_fnsdone, 1);

  ggc_add_root (&pending_inlines, 1, sizeof (struct unparsed_text *),
		mark_pending_inlines);
  ggc_add_root (&processing_these_inlines, 1, sizeof (struct unparsed_text *),
		mark_pending_inlines);
}

void
clear_inline_text_obstack ()
{
  obstack_free (&inline_text_obstack, inline_text_firstobj);
}

/* Subroutine of read_token.  */
static SPEW_INLINE int
read_process_identifier (pyylval)
     YYSTYPE *pyylval;
{
  tree id = pyylval->ttype;

  if (C_IS_RESERVED_WORD (id))
    {
      /* Possibly replace the IDENTIFIER_NODE with a magic cookie.
	 Can't put yylval.code numbers in ridpointers[].  Bleah.  */

      switch (C_RID_CODE (id))
	{
	case RID_BITAND: pyylval->code = BIT_AND_EXPR;	return '&';
	case RID_AND_EQ: pyylval->code = BIT_AND_EXPR;	return ASSIGN;
	case RID_BITOR:	 pyylval->code = BIT_IOR_EXPR;	return '|';
	case RID_OR_EQ:	 pyylval->code = BIT_IOR_EXPR;	return ASSIGN;
	case RID_XOR:	 pyylval->code = BIT_XOR_EXPR;	return '^';
	case RID_XOR_EQ: pyylval->code = BIT_XOR_EXPR;	return ASSIGN;
	case RID_NOT_EQ: pyylval->code = NE_EXPR;	return EQCOMPARE;

	default:
	  pyylval->ttype = ridpointers[C_RID_CODE (id)];
	  return C_RID_YYCODE (id);
	}
    }

  /* Make sure that user does not collide with our internal naming
     scheme.  This is not necessary if '.' is used to remove them from
     the user's namespace, but is if '$' or double underscores are.  */

#if !defined(JOINER) || JOINER == '$'
  if (VPTR_NAME_P (id)
      || VTABLE_NAME_P (id)
      || TEMP_NAME_P (id)
      || ANON_AGGRNAME_P (id))
     warning (
"identifier name `%s' conflicts with GNU C++ internal naming strategy",
	      IDENTIFIER_POINTER (id));
#endif
  return IDENTIFIER;
}

/* Read the next token from the input file.  The token is written into
   T, and its type number is returned.  */
static int
read_token (t)
     struct token *t;
{
 retry:

  last_token = c_lex (&last_token_id);
  t->yylval.ttype = last_token_id;

  switch (last_token)
    {
#define YYCHAR(YY)	t->yychar = (YY); break;
#define YYCODE(C)	t->yylval.code = (C);

    case CPP_EQ:				YYCHAR('=');
    case CPP_NOT:				YYCHAR('!');
    case CPP_GREATER:	YYCODE(GT_EXPR);	YYCHAR('>');
    case CPP_LESS:	YYCODE(LT_EXPR);	YYCHAR('<');
    case CPP_PLUS:	YYCODE(PLUS_EXPR);	YYCHAR('+');
    case CPP_MINUS:	YYCODE(MINUS_EXPR);	YYCHAR('-');
    case CPP_MULT:	YYCODE(MULT_EXPR);	YYCHAR('*');
    case CPP_DIV:	YYCODE(TRUNC_DIV_EXPR);	YYCHAR('/');
    case CPP_MOD:	YYCODE(TRUNC_MOD_EXPR);	YYCHAR('%');
    case CPP_AND:	YYCODE(BIT_AND_EXPR);	YYCHAR('&');
    case CPP_OR:	YYCODE(BIT_IOR_EXPR);	YYCHAR('|');
    case CPP_XOR:	YYCODE(BIT_XOR_EXPR);	YYCHAR('^');
    case CPP_RSHIFT:	YYCODE(RSHIFT_EXPR);	YYCHAR(RSHIFT);
    case CPP_LSHIFT:	YYCODE(LSHIFT_EXPR);	YYCHAR(LSHIFT);

    case CPP_COMPL:				YYCHAR('‾');
    case CPP_AND_AND:				YYCHAR(ANDAND);
    case CPP_OR_OR:				YYCHAR(OROR);
    case CPP_QUERY:				YYCHAR('?');
    case CPP_COLON:				YYCHAR(':');
    case CPP_COMMA:				YYCHAR(',');
    case CPP_OPEN_PAREN:			YYCHAR('(');
    case CPP_CLOSE_PAREN:			YYCHAR(')');
    case CPP_EQ_EQ:	YYCODE(EQ_EXPR);	YYCHAR(EQCOMPARE);
    case CPP_NOT_EQ:	YYCODE(NE_EXPR);	YYCHAR(EQCOMPARE);
    case CPP_GREATER_EQ:YYCODE(GE_EXPR);	YYCHAR(ARITHCOMPARE);
    case CPP_LESS_EQ:	YYCODE(LE_EXPR);	YYCHAR(ARITHCOMPARE);

    case CPP_PLUS_EQ:	YYCODE(PLUS_EXPR);	YYCHAR(ASSIGN);
    case CPP_MINUS_EQ:	YYCODE(MINUS_EXPR);	YYCHAR(ASSIGN);
    case CPP_MULT_EQ:	YYCODE(MULT_EXPR);	YYCHAR(ASSIGN);
    case CPP_DIV_EQ:	YYCODE(TRUNC_DIV_EXPR);	YYCHAR(ASSIGN);
    case CPP_MOD_EQ:	YYCODE(TRUNC_MOD_EXPR);	YYCHAR(ASSIGN);
    case CPP_AND_EQ:	YYCODE(BIT_AND_EXPR);	YYCHAR(ASSIGN);
    case CPP_OR_EQ:	YYCODE(BIT_IOR_EXPR);	YYCHAR(ASSIGN);
    case CPP_XOR_EQ:	YYCODE(BIT_XOR_EXPR);	YYCHAR(ASSIGN);
    case CPP_RSHIFT_EQ:	YYCODE(RSHIFT_EXPR);	YYCHAR(ASSIGN);
    case CPP_LSHIFT_EQ:	YYCODE(LSHIFT_EXPR);	YYCHAR(ASSIGN);

    case CPP_OPEN_SQUARE:			YYCHAR('[');
    case CPP_CLOSE_SQUARE:			YYCHAR(']');
    case CPP_OPEN_BRACE:			YYCHAR('{');
    case CPP_CLOSE_BRACE:			YYCHAR('}');
    case CPP_SEMICOLON:				YYCHAR(';');
    case CPP_ELLIPSIS:				YYCHAR(ELLIPSIS);

    case CPP_PLUS_PLUS:				YYCHAR(PLUSPLUS);
    case CPP_MINUS_MINUS:			YYCHAR(MINUSMINUS);
    case CPP_DEREF:				YYCHAR(POINTSAT);
    case CPP_DOT:				YYCHAR('.');

    /* These tokens are C++ specific.  */
    case CPP_SCOPE:				YYCHAR(SCOPE);
    case CPP_DEREF_STAR: 			YYCHAR(POINTSAT_STAR);
    case CPP_DOT_STAR:				YYCHAR(DOT_STAR);
    case CPP_MIN_EQ:	YYCODE(MIN_EXPR);	YYCHAR(ASSIGN);
    case CPP_MAX_EQ:	YYCODE(MAX_EXPR);	YYCHAR(ASSIGN);
    case CPP_MIN:	YYCODE(MIN_EXPR);	YYCHAR(MIN_MAX);
    case CPP_MAX:	YYCODE(MAX_EXPR);	YYCHAR(MIN_MAX);
#undef YYCHAR
#undef YYCODE

    case CPP_EOF:
      t->yychar = 0;
      break;
      
    case CPP_NAME:
      t->yychar = read_process_identifier (&t->yylval);
      break;

    case CPP_NUMBER:
    case CPP_CHAR:
    case CPP_WCHAR:
      t->yychar = CONSTANT;
      break;

    case CPP_STRING:
    case CPP_WSTRING:
      t->yychar = STRING;
      break;

    default:
      yyerror ("pa