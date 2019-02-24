
/*  A Bison parser, made from plural.y
    by GNU Bison version 1.28  */

#define YYBISON 1  /* Identify Bison output.  */

#define yyparse __gettextparse
#define yylex __gettextlex
#define yyerror __gettexterror
#define yylval __gettextlval
#define yychar __gettextchar
#define yydebug __gettextdebug
#define yynerrs __gettextnerrs
#define	EQUOP2	257
#define	CMPOP2	258
#define	ADDOP2	259
#define	MULOP2	260
#define	NUMBER	261

#line 1 "plural.y"

/* Expression parsing for plural form selection.
   Copyright (C) 2000, 2001 Free Software Foundation, Inc.
   Written by Ulrich Drepper <drepper@cygnus.com>, 2000.

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU Library General Public License as published
   by the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
   USA.  */

/* The bison generated parser uses alloca.  AIX 3 forces us to put this
   declaration at the beginning of the file.  The declaration in bison's
   skeleton file comes too late.  This must come before <config.h>
   because <config.h> may include arbitrary system headers.  */
#if defined _AIX && !defined __GNUC__
 #pragma alloca
#endif

/* !kawai! */
#ifdef HAVE_CONFIG_H
# include "../gcc/config.h"
#endif
/* end of !kawai! */

#include "../include/stdlib.h"
#include "gettextP.h"

/* Names for the libintl functions are a problem.  They must not clash
   with existing names and they should follow ANSI C.  But this source
   code is also used in GNU C Library where the names have a __
   prefix.  So we have to make a difference here.  */
#ifdef _LIBC
# define FREE_EXPRESSION __gettext_free_exp
#else
# define FREE_EXPRESSION gettext_free_exp__
# define __gettextparse gettextparse__
#endif

#define YYLEX_PARAM	&((struct parse_args *) arg)->cp
#define YYPARSE_PARAM	arg

#line 53 "plural.y"
typedef union {
  unsigned long int num;
  enum operator op;
  struct expression *exp;
} YYSTYPE;
#line 59 "plural.y"

/* Prototypes for local functions.  */
static struct expression *new_exp PARAMS ((int nargs, enum operator op,
					   struct expression * const *args));
static inline struct expression *new_exp_0 PARAMS ((enum operator op));
static inline struct expression *new_exp_1 PARAMS ((enum operator op,
						   struct expression *right));
static struct expression *new_exp_2 PARAMS ((enum operator op,
					     struct expression *left,
					     struct expression *right));
static inline struct expression *new_exp_3 PARAMS ((enum operator op,
						   struct expression *bexp,
						   struct expression *tbranch,
						   struct expression *fbranch));
static int yylex PARAMS ((YYSTYPE *lval, const char **pexp));
static void yyerror PARAMS ((const char *str));

/* Allocation of expressions.  */

static struct expression *
new_exp (nargs, op, args)
     int nargs;
     enum operator op;
     struct expression * const *args;
{
  int i;
  struct expression *newp;

  /* If any of the argument could not be malloc'ed, just return NULL.  */
  for (i = nargs - 1; i >= 0; i--)
    if (args[i] == NULL)
      goto fail;

  /* Allocate a new expression.  */
  newp = (struct expression *) malloc (sizeof (*newp));
  if (newp != NULL)
    {
      newp->nargs = nargs;
      newp->operation = op;
      for (i = nargs - 1; i >= 0; i--)
	newp->val.args[i] = args[i];
      return newp;
    }

 fail:
  for (i = nargs - 1; i >= 0; i--)
    FREE_EXPRESSION (args[i]);

  return NULL;
}

static inline struct expression *
new_exp_0 (op)
     enum operator op;
{
  return new_exp (0, op, NULL);
}

static inline struct expression *
new_exp_1 (op, right)
     enum operator op;
     struct expression *right;
{
  struct expression *args[1];

  args[0] = right;
  return new_exp (1, op, args);
}

static struct expression *
new_exp_2 (op, left, right)
     enum operator op;
     struct expression *left;
     struct expression *right;
{
  struct expression *args[2];

  args[0] = left;
  args[1] = right;
  return new_exp (2, op, args);
}

static inline struct expression *
new_exp_3 (op, bexp, tbranch, fbranch)
     enum operator op;
     struct expression *bexp;
     struct expression *tbranch;
     struct expression *fbranch;
{
  struct expression *args[3];

  args[0] = bexp;
  args[1] = tbranch;
  args[2] = fbranch;
  return new_exp (3, op, args);
}

#include "../include/stdio.h"




#define	YYFINAL		27
#define	YYFLAG		-32768
#define	YYNTBASE	16

#define YYTRANSLATE(x) ((unsigned)(x) <= 261 ? yytranslate[x] : 18)

static const char yytranslate[] = {     0,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,    10,     2,     2,     2,     2,     5,     2,    14,
    15,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,    12,     2,     2,
     2,     2,     3,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,    13,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     4,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     1,     6,     7,     8,     9,
    11
};

#if YYDEBUG != 0
static const short yyprhs[] = {     0,
     0,     2,     8,    12,    16,    20,    24,    28,    32,    35,
    37,    39
};

static const short yyrhs[] = {    17,
     0,    17,     3,    17,    12,    17,     0,    17,     4,    17,
     0,    17,     5,    17,     0,    17,     6,    17,     0,    17,
     7,    17,     0,    17,     8,    17,     0,    17,     9,    17,
     0,    10,    17,     0,    13,     0,    11,     0,    14,    17,
    15,     0
};

#endif

#if YYDEBUG != 0
static const short yyrline[] = { 0,
   178,   186,   190,   194,   198,   202,   206,   210,   214,   218,
   222,   227
};
#endif


#if YYDEBUG != 0 || defined (YYERROR_VERBOSE)

static const char * const yytname[] = {   "$","error","$undefined.","'?'","'|'",
"'&'","EQUOP2","CMPOP2","ADDOP2","MULOP2","'!'","NUMBER","':'","'n'","'('","')'",
"start","exp", NULL
};
#endif

static const short yyr1[] = {     0,
    16,    17,    17,    17,    17,    17,    17,    17,    17,    17,
    17,    17
};

static const short yyr2[] = {     0,
     1,     5,     3,     3,     3,     3,     3,     3,     2,     1,
     1,     3
};

static const short yydefact[] = {     0,
     0,    11,    10,     0,     1,     9,     0,     0,     0,     0,
     0,     0,     0,     0,    12,     0,     3,     4,     5,     6,
     7,     8,     0,     2,     0,     0,     0
};

static const short yydefgoto[] = {    25,
     5
};

static const short yypact[] = {    -9,
    -9,-32768,-32768,    -9,    34,-32768,    11,    -9,    -9,    -9,
    -9,    -9,    -9,    -9,-32768,    24,    39,    43,    16,    26,
    -3,-32768,    -9,    34,    21,    53,-32768
};

static const short yypgoto[] = {-32768,
    -1
};


#define	YYLAST		53


static const short yytable[] = {     6,
     1,     2,     7,     3,     4,    14,    16,    17,    18,    19,
    20,    21,    22,     8,     9,    10,    11,    12,    13,    14,
    26,    24,    12,    13,    14,    15,     8,     9,    10,    11,
    12,    13,    14,    13,    14,    23,     8,     9,    10,    11,
    12,    13,    14,    10,    11,    12,    13,    14,    11,    12,
    13,    14,    27
};

static const short yycheck[] = {     1,
    10,    11,     4,    13,    14,     9,     8,     9,    10,    11,
    12,    13,    14,     3,     4,     5,     6,     7,     8,     9,
     0,    23,     7,     8,     9,    15,     3,     4,     5,     6,
     7,     8,     9,     8,     9,    12,     3,     4,     5,     6,
     7,     8,     9,     5,     6,     7,     8,     9,     6,     7,
     8,     9,     0
};
#define YYPURE 1

/* -*-C-*-  Note some compilers choke on comments on `#line' lines.  */
#line 3 "/home/haible/gnu/arch/linuxlibc6/share/bison.simple"
/* This file comes from bison-1.28.  */

/* Skeleton output parser for bison,
   Copyright (C) 1984, 1989, 1990 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

/* As a special exception, when this file is copied by Bison into a
   Bison output file, you may use that output file without restriction.
   This special exception was added by the Free Software Foundation
   in version 1.24 of Bison.  */

/* This is the parser code that is written into each bison parser
  when the %semantic_parser declaration is not specified in the grammar.
  It was written by Richard Stallman by simplifying the hairy parser
  used when %semantic_parser is specified.  */

#ifndef YYSTACK_USE_ALLOCA
#ifdef alloca
#define YYSTACK_USE_ALLOCA
#else /* alloca not defined */
#ifdef __GNUC__
#define YYSTACK_USE_ALLOCA
#define alloca __builtin_alloca
#else /* not GNU C.  */
#if (!defined (__STDC__) && defined (sparc)) || defined (__sparc__) || defined (__sparc) || defined (__sgi) || (defined (__sun) && defined (__i386))
#define YYSTACK_USE_ALLOCA
#include <alloca.h>
#else /* not sparc */
/* We think this test detects Watcom and Microsoft C.  */
/* This used to test MSDOS, but that is a bad idea
   since that symbol is in the user namespace.  */
#if (defined (_MSDOS) || defined (_MSDOS_)) && !defined (__TURBOC__)
#if 0 /* No need for malloc.h, which pollutes the namespace;
	 instead, just don't use alloca.  */
#include <malloc.h>
#endif
#else /* not MSDOS, or __TURBOC__ */
#if defined(_AIX)
/* I don't know what this was needed for, but it pollutes the namespace.
   So I turned it off.   rms, 2 May 1997.  */
/* #include <malloc.h>  */
 #pragma alloca
#define YYSTACK_USE_ALLOCA
#else /* not MSDOS, or __TURBOC__, or _AIX */
#if 0
#ifdef __hpux /* haible@ilog.fr says this works for HPUX 9.05 and up,
		 and on HPUX 10.  Eventually we can turn this on.  */
#define YYSTACK_USE_ALLOCA
#define alloca __builtin_alloca
#endif /* __hpux */
#endif
#endif /* not _AIX */
#endif /* not MSDOS, or __TURBOC__ */
#endif /* not sparc */
#endif /* not GNU C */
#endif /* alloca not defined */
#endif /* YYSTACK_USE_ALLOCA not defined */

#ifdef YYSTACK_USE_ALLOCA
#define YYSTACK_ALLOC alloca
#else
#define YYSTACK_ALLOC malloc
#endif

/* Note: there must be only one dollar sign in this file.
   It is replaced by the list of actions, each action
   as one case of the switch.  */

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		-2
#define YYEOF		0
#define YYACCEPT	goto yyacceptlab
#define YYABORT 	goto yyabortlab
#define YYERROR		goto yyerrlab1
/* Like YYERROR except do call yyerror.
   This remains here temporarily to ease the
   transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */
#define YYFAIL		goto yyerrlab
#define YYRECOVERING()  (!!yyerrstatus)
#define YYBACKUP(token, value) ¥
do								¥
  if (yychar == YYEMPTY && yylen == 1)				¥
    { yychar = (token), yylval = (value);			¥
      yychar1 = YYTRANSLATE (yychar);				¥
      YYPOPSTACK;						¥
      goto yybackup;						¥
    }								¥
  else								¥
    { yyerror ("syntax error: cannot back up"); YYERROR; }	¥
while (0)

#define YYTERROR	1
#define YYERRCODE	256

#ifndef YYPURE
#define YYLEX		yylex()
#endif

#ifdef YYPURE
#ifdef YYLSP_NEEDED
#ifdef YYLEX_PARAM
#define YYLEX		yylex(&yylval, &yylloc, YYLEX_PARAM)
#else
#define YYLEX		yylex(&yylval, &yylloc)
#endif
#else /* not YYLSP_NEEDED */
#ifdef YYLEX_PARAM
#define YYLEX		yylex(&yylval, YYLEX_PARAM)
#else
#define YYLEX		yylex(&yylval)
#endif
#endif /* not YYLSP_NEEDED */
#endif

/* If nonreentrant, generate the variables here */

#ifndef YYPURE

int	yychar;			/*  the lookahead symbol		*/
YYSTYPE	yylval;			/*  the semantic value of the		*/
				/*  lookahead symbol			*/

#ifdef YYLSP_NEEDED
YYLTYPE yylloc;			/*  location data for the lookahead	*/
				/*  symbol				*/
#endif

int yynerrs;			/*  number of parse errors so far       */
#endif  /* not YYPURE */

#if YYDEBUG != 0
int yydebug;			/*  nonzero means print parse trace	*/
/* Since this is uninitialized, it does not stop multiple parsers
   from coexisting.  */
#endif

/*  YYINITDEPTH indicates the initial size of the parser's stacks	*/

#ifndef	YYINITDEPTH
#define YYINITDEPTH 200
#endif

/*  YYMAXDEPTH is the maximum size the stacks can grow to
    (effective only if the built-in stack extension method is used).  */

#if YYMAXDEPTH == 0
#undef YYMAXDEPTH
#endif

#ifndef YYMAXDEPTH
#define YYMAXDEPTH 10000
#endif

/* Define __yy_memcpy.  Note that the size argument
   should be passed with type unsigned int, because that is what the non-GCC
   definitions require.  With GCC, __builtin_memcpy takes an arg
   of type size_t, but it can handle unsigned int.  */

#if __GNUC__ > 1		/* GNU C and GNU C++ define this.  */
#define __yy_memcpy(TO,FROM,COUNT)	__builtin_memcpy(TO,FROM,COUNT)
#else				/* not GNU C or C++ */
#ifndef __cplusplus

/* This is the most reliable way to avoid incompatibilities
   in available built-in functions on various systems.  */
static void
__yy_memcpy (to, from, count)
     char *to;
     char *from;
     unsigned int count;
{
  register char *f = from;
  register char *t = to;
  register int i = count;

  while (i-- > 0)
    *t++ = *f++;
}

#else /* __cplusplus */

/* This is the most reliable way to avoid incompatibilities
   in available built-in functions on various systems.  */
static void
__yy_memcpy (char *to, char *from, unsigned int count)
{
  register char *t = to;
  register char *f = from;
  register int i = count;

  while (i-- > 0)
    *t++ = *f++