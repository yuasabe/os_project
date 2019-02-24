/* Parse C expressions for cpplib.
   Copyright (C) 1987, 1992, 1994, 1995, 1997, 1998, 1999, 2000, 2001,
   2002 Free Software Foundation.
   Contributed by Per Bothner, 1994.

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
Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */

#include "config.h"
#include "system.h"
#include "cpplib.h"
#include "cpphash.h"

/* Yield nonzero if adding two numbers with A's and B's signs can yield a
   number with SUM's sign, where A, B, and SUM are all C integers.  */
#define possible_sum_sign(a, b, sum) ((((a) ^ (b)) | ‾ ((a) ^ (sum))) < 0)

static void integer_overflow PARAMS ((cpp_reader *));
static HOST_WIDEST_INT left_shift PARAMS ((cpp_reader *, HOST_WIDEST_INT,
					   unsigned int,
					   unsigned HOST_WIDEST_INT));
static HOST_WIDEST_INT right_shift PARAMS ((cpp_reader *, HOST_WIDEST_INT,
					    unsigned int,
					    unsigned HOST_WIDEST_INT));
static struct op parse_number PARAMS ((cpp_reader *, const cpp_token *));
static struct op parse_defined PARAMS ((cpp_reader *));
static struct op lex PARAMS ((cpp_reader *, int));
static const unsigned char *op_as_text PARAMS ((cpp_reader *, enum cpp_ttype));

struct op
{
  enum cpp_ttype op;
  U_CHAR prio;         /* Priority of op.  */
  U_CHAR flags;
  U_CHAR unsignedp;    /* True if value should be treated as unsigned.  */
  HOST_WIDEST_INT value; /* The value logically "right" of op.  */
};

/* There is no "error" token, but we can't get comments in #if, so we can
   abuse that token type.  */
#define CPP_ERROR CPP_COMMENT

/* With -O2, gcc appears to produce nice code, moving the error
   message load and subsequent jump completely out of the main path.  */
#define CPP_ICE(msgid) ¥
  do { cpp_ice (pfile, msgid); goto syntax_error; } while(0)
#define SYNTAX_ERROR(msgid) ¥
  do { cpp_error (pfile, msgid); goto syntax_error; } while(0)
#define SYNTAX_ERROR2(msgid, arg) ¥
  do { cpp_error (pfile, msgid, arg); goto syntax_error; } while(0)

struct suffix
{
  const unsigned char s[4];
  const unsigned char u;
  const unsigned char l;
};

static const struct suffix vsuf_1[] = {
  { "u", 1, 0 }, { "U", 1, 0 },
  { "l", 0, 1 }, { "L", 0, 1 }
};

static const struct suffix vsuf_2[] = {
  { "ul", 1, 1 }, { "UL", 1, 1 }, { "uL", 1, 1 }, { "Ul", 1, 1 },
  { "lu", 1, 1 }, { "LU", 1, 1 }, { "Lu", 1, 1 }, { "lU", 1, 1 },
  { "ll", 0, 2 }, { "LL", 0, 2 }
};

static const struct suffix vsuf_3[] = {
  { "ull", 1, 2 }, { "ULL", 1, 2 }, { "uLL", 1, 2 }, { "Ull", 1, 2 },
  { "llu", 1, 2 }, { "LLU", 1, 2 }, { "LLu", 1, 2 }, { "llU", 1, 2 }
};
#define Nsuff(tab) (sizeof tab / sizeof (struct suffix))

/* Parse and convert what is presumably an integer in TOK.  Accepts
   decimal, hex, or octal with or without size suffixes.  Returned op
   is CPP_ERROR on error, otherwise it is a CPP_NUMBER.  */
static struct op
parse_number (pfile, tok)
     cpp_reader *pfile;
     const cpp_token *tok;
{
  struct op op;
  const U_CHAR *start = tok->val.str.text;
  const U_CHAR *end = start + tok->val.str.len;
  const U_CHAR *p = start;
  int c = 0, i, nsuff;
  unsigned HOST_WIDEST_INT n = 0, nd, MAX_over_base;
  int base = 10;
  int overflow = 0;
  int digit, largest_digit = 0;
  const struct suffix *sufftab;

  op.unsignedp = 0;

  if (p[0] == '0')
    {
      if (end - start >= 3 && (p[1] == 'x' || p[1] == 'X'))
	{
	  p += 2;
	  base = 16;
	}
      else
	{
	  p += 1;
	  base = 8;
	}
    }

  /* So