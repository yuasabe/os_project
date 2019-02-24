/* Definitions for CPP library.
   Copyright (C) 1995, 1996, 1997, 1998, 1999, 2000, 2001, 2002
   Free Software Foundation, Inc.
   Written by Per Bothner, 1994-95.

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
Foundation, 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

 In other words, you are welcome to use, share and improve this program.
 You are forbidden to forbid anyone else to use, share and improve
 what you give them.   Help stamp out software-hoarding!  */
#ifndef GCC_CPPLIB_H
#define GCC_CPPLIB_H

#include "hashtable.h"
#include "line-map.h"

#ifdef __cplusplus
extern "C" {
#endif

/* For complex reasons, cpp_reader is also typedefed in c-pragma.h.  */
#ifndef GCC_C_PRAGMA_H
typedef struct cpp_reader cpp_reader;
#endif
typedef struct cpp_buffer cpp_buffer;
typedef struct cpp_options cpp_options;
typedef struct cpp_token cpp_token;
typedef struct cpp_string cpp_string;
typedef struct cpp_hashnode cpp_hashnode;
typedef struct cpp_macro cpp_macro;
typedef struct cpp_callbacks cpp_callbacks;

struct answer;
struct file_name_map_list;

/* The first two groups, apart from '=', can appear in preprocessor
   expressions.  This allows a lookup table to be implemented in
   _cpp_parse_expr.

   The first group, to CPP_LAST_EQ, can be immediately followed by an
   '='.  The lexer needs operators ending in '=', like ">>=", to be in
   the same order as their counterparts without the '=', like ">>".  */

/* Positions in the table.  */
#define CPP_LAST_EQ CPP_MAX
#define CPP_FIRST_DIGRAPH CPP_HASH
#define CPP_LAST_PUNCTUATOR CPP_DOT_STAR

#define TTYPE_TABLE				¥
  OP(CPP_EQ = 0,	"=")			¥
  OP(CPP_NOT,		"!")			¥
  OP(CPP_GREATER,	">")	/* compare */	¥
  OP(CPP_LESS,		"<")			¥
  OP(CPP_PLUS,		"+")	/* math */	¥
  OP(CPP_MINUS,		"-")			¥
  OP(CPP_MULT,		"*")			¥
  OP(CPP_DIV,		"/")			¥
  OP(CPP_MOD,		"%")			¥
  OP(CPP_AND,		"&")	/* bit ops */	¥
  OP(CPP_OR,		"|")			¥
  OP(CPP_XOR,		"^")			¥
  OP(CPP_RSHIFT,	">>")			¥
  OP(CPP_LSHIFT,	"<<")			¥
  OP(CPP_MIN,		"<?")	/* extension */	¥
  OP(CPP_MAX,		">?")			¥
¥
  OP(CPP_COMPL,		"‾")			¥
  OP(CPP_AND_AND,	"&&")	/* logical */	¥
  OP(CPP_OR_OR,		"||")			¥
  OP(CPP_QUERY,		"?")			¥
  OP(CPP_COLON,		":")			¥
  OP(CPP_COMMA,		",")	/* grouping */	¥
  OP(CPP_OPEN_PAREN,	"(")			¥
  OP(CPP_CLOSE_PAREN,	")")			¥
  OP(CPP_EQ_EQ,		"==")	/* compare */	¥
  OP(CPP_NOT_EQ,	"!=")			¥
  OP(CPP_GREATER_EQ,	">=")			¥
  OP(CPP_LESS_EQ,	"<=")			¥
¥
  OP(CPP_PLUS_EQ,	"+=")	/* math */	¥
  OP(CPP_MINUS_EQ,	"-=")			¥
  OP(CPP_MULT_EQ,	"*=")			¥
  OP(CPP_DIV_EQ,	"/=")			¥
  OP(CPP_MOD_EQ,	"%=")			¥
  OP(CPP_AND_EQ,	"&=")	/* bit ops */	¥
  OP(CPP_OR_EQ,		"|=")			¥
  OP(CPP_XOR_EQ,	"^=")			¥
  OP(CPP_RSHIFT_EQ,	">>=")			¥
  OP(CPP_LSHIFT_EQ,	"<<=")			¥
  OP(CPP_MIN_EQ,	"<?=")	/* extension */	¥
  OP(CPP_MAX_EQ,	">?=")			¥
  /* Digraphs together, beginning with CPP_FIRST_DIGRAPH.  */	¥
  OP(CPP_HASH,		"#")	/* digraphs */	¥
  OP(CPP_PASTE,		"##")			¥
  OP(CPP_OPEN_SQUARE,	"[")			¥
  OP(CPP_CLOSE_SQUARE,	"]")			¥
  OP(CPP_OPEN_BRACE,	"{")			¥
  OP(CPP_CLOSE_BRACE,	"}")			¥
  /* The remainder of the punctuation.  Order is not significant.  */	¥
  OP(CPP_SEMICOLON,	";")	/* structure */	¥
  OP(CPP_ELLIPSIS,	"...")			¥
  OP(CPP_PLUS_PLUS,	"++")	/* increment */	¥
  OP(CPP_MINUS_MINUS,	"--")			¥
  OP(CPP_DEREF,		"->")	/* accessors */	¥
  OP(CPP_DOT,		".")			¥
  OP(CPP_SCOPE,		"::")			¥
  OP(CPP_DEREF_STAR,	"->*")			¥
  OP(CPP_DOT_STAR,	".*")			¥
  OP(CPP_ATSI