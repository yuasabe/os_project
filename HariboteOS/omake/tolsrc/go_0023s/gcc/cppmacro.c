/* Part of CPP library.  (Macro and #define handling.)
   Copyright (C) 1986, 1987, 1989, 1992, 1993, 1994, 1995, 1996, 1998,
   1999, 2000, 2001, 2002 Free Software Foundation, Inc.
   Written by Per Bothner, 1994.
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
Foundation, 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

 In other words, you are welcome to use, share and improve this program.
 You are forbidden to forbid anyone else to use, share and improve
 what you give them.   Help stamp out software-hoarding!  */

#include "config.h"
#include "system.h"
#include "cpplib.h"
#include "cpphash.h"

struct cpp_macro
{
  cpp_hashnode **params;	/* Parameters, if any.  */
  cpp_token *expansion;		/* First token of replacement list.  */
  unsigned int line;		/* Starting line number.  */
  unsigned int count;		/* Number of tokens in expansion.  */
  unsigned short paramc;	/* Number of parameters.  */
  unsigned int fun_like : 1;	/* If a function-like macro.  */
  unsigned int variadic : 1;	/* If a variadic macro.  */
  unsigned int syshdr   : 1;	/* If macro defined in system header.  */
};

typedef struct macro_arg macro_arg;
struct macro_arg
{
  const cpp_token **first;	/* First token in unexpanded argument.  */
  const cpp_token **expanded;	/* Macro-expanded argument.  */
  const cpp_token *stringified;	/* Stringified argument.  */
  unsigned int count;		/* # of tokens in argument.  */
  unsigned int expanded_count;	/* # of tokens in expanded argument.  */
};

/* Macro expansion.  */

static int enter_macro_context PARAMS ((cpp_reader *, cpp_hashnode *));
static int builtin_macro PARAMS ((cpp_reader *, cpp_hashnode *));
static void push_token_context
  PARAMS ((cpp_reader *, cpp_hashnode *, const cpp_token *, unsigned int));
static void push_ptoken_context
  PARAMS ((cpp_reader *, cpp_hashnode *, _cpp_buff *,
	   const cpp_token **, unsigned int));
static _cpp_buff *collect_args PARAMS ((cpp_reader *, const cpp_hashnode *));
static cpp_context *next_context PARAMS ((cpp_reader *));
static const cpp_token *padding_token
  PARAMS ((cpp_reader *, const cpp_token *));
static void expand_arg PARAMS ((cpp_reader *, macro_arg *));
static const cpp_token *new_string_token PARAMS ((cpp_reader *, U_CHAR *,
						  unsigned int));
static const cpp_token *new_number_token PARAMS ((cpp_reader *, unsigned int));
static const cpp_token *stringify_arg PARAMS ((cpp_reader *, macro_arg *));
static void paste_all_tokens PARAMS ((cpp_reader *, const cpp_token *));
static bool paste_tokens PARAMS ((cpp_reader *, const cpp_token **,
				  const cpp_token *));
static void replace_args PARAMS ((cpp_reader *, cpp_hashnode *, macro_arg *));
static _cpp_buff *funlike_invocation_p PARAMS ((cpp_reader *, cpp_hashnode *));

/* #define directive parsing and handling.  */

static cpp_token *alloc_expansion_token PARAMS ((cpp_reader *, cpp_macro *));
static cpp_token *lex_expansion_token PARAMS ((cpp_reader *, cpp_macro *));
static int warn_of_redefinition PARAMS ((const cpp_hashnode *,
					 const cpp_macro *));
static int save_parameter PARAMS ((cpp_reader *, cpp_macro *, cpp_hashnode *));
static int parse_params PARAMS ((cpp_reader *, cpp_macro *));
static void check_trad_stringification PARAMS ((cpp_reader *,
						const cpp_macro *,
						const cpp_string *));

/* Allocates and returns a CPP_STRING token, containing TEXT of length
   LEN, after null-terminating it.  TEXT must be in permanent storage.  */
static const cpp_token *
new_string_token (pfile, text, len)
     cpp_reader *pfile;
     unsigned char *text;
     unsigned int len;
{
  cpp_token *token = _cpp_temp_token (pfile);

  text[len] = '¥0';
  token->type = CPP_STRING;
  token->val.str.len = len;
  token->val.str.text = text;
  token->flags = 0;
  return token;
}

/* Allocates and returns a CPP_NUMBER token evaluating to NUMBER.  */
static const cpp_token *
new_number_token (pfile, number)
     cpp_reader *pfile;
     unsigned int number;
{
  cpp_token *token = _cpp_temp_token (pfile);
  /* 21 bytes holds all NUL-terminated unsigned 64-bit numbers.  */
  unsigned char *buf = _cpp_unaligned_alloc (pfile, 21);

  sprintf ((char *) buf, "%u", number);
  token->type = CPP_NUMBER;
  token->val.str.text = buf;
  token->val.str.len = ustrlen (buf);
  token->flags = 0;
  return token;
}

static const char * const monthnames[] =
{
  "Jan", "Feb", "Mar", "Apr", "May", "Jun",
  "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

/* Handle builtin macros like __FILE__, and push the resulting token
   on the context stack.  Also handles _Pragma, for which no new token
   is created.  Returns 1 if it generates a new token context, 0 to
   return the token to the caller.  */
static int
builtin_macro (pfile, node)
     cpp_reader *pfile;
     cpp_hashnode *node;
{
  const cpp_token *result;

  switch (node->value.builtin)
    {
    default:
      cpp_ice (pfile, "invalid built-in macro ¥"%s¥"", NODE_NAME (node));
      return 0;

    case BT_FILE:
    case BT_BASE_FILE:
      {
	unsigned int len;
	const char *name;
	U_CHAR *buf;
	const struct line_map *map = pfile->map;

	if (node->value.builtin == BT_BASE_FILE)
	  while (! MAIN_FILE_P (map))
	    map = INCLUDED_FROM (&pfile->line_maps, map);

	name = map->to_file;
	len = strlen (name);
	buf = _cpp_unaligned_alloc (pfile, len * 4 + 1);
	len = cpp_quote_string (buf, (const unsigned char *) name, len) - buf;

	result = new_string_token (pfile, buf, len);
      }
      break;

    case BT_INCLUDE_LEVEL:
      /* The line map depth counts the primary source as level 1, but
	 historically __INCLUDE_DEPTH__ has called the primary source
	 level 0.  */
      result = new_number_token (pfile, pfile->line_maps.depth - 1);
      break;

    case BT_SPECLINE:
      /* If __LINE__ is embedded in a macro, it must expand to the
	 line of the macro's invocation, not its definition.
	 Otherwise things like assert() will not work properly.  */
      result = new_number_token (pfile,
				 SOURCE_LINE (pfile->map,
					      pfile->cur_token[-1].line));
      break;

    case BT_STDC:
      {
	int stdc = (!CPP_IN_SYSTEM_HEADER (pfile)
		    || pfile->spec_nodes.n__STRICT_ANSI__->type != NT_VOID);
	result = new_number_token (pfile, stdc);
      }
      break;

    case BT_DATE:
    case BT_TIME:
      if (pfile->date.type == CPP_EOF)
	{
	  /* Allocate __DATE__ and __TIME__ strings from permanent
	     storage.  We only do this once, and don't generate them
	     at init time, because time() and localtime() are very
	     slow on some systems.  */
#if 0
	  time_t tt = time (NULL);
	  struct tm *tb = localtime (&tt);
#endif
	  pfile->date.val.str.text =
	    _cpp_unaligned_alloc (pfile, sizeof ("Oct 11 1347"));
	  pfile->date.val.str.len = sizeof ("Oct 11 1347") - 1;
	  pfile->date.type = CPP_STRING;
	  pfile->date.flags = 0;
#if 0
	  sprintf ((char *) pfile->date.val.str.text, "%s %2d %4d",
		   monthnames[tb->tm_mon], tb->tm_mday, tb->tm_year + 1900);
#endif
	  sprintf ((char *) pfile->date.val.str.text, "%s %2d %4d",
		   "Oct", 11, 1347);

	  pfile->time.val.str.text =
	    _cpp_unaligned_alloc (pfile, sizeof ("12:34:56"));
	  pfile->time.val.str.len = sizeof ("12:34:56") - 1;
	  pfile->time.type = CPP_STRING;
	  pfile->time.flags = 0;
#if 0
	  sprintf ((char *) pfile->time.val.str.text, "%02d:%02d:%02d",
		   tb->tm_hour, tb->tm_min, tb->tm_sec);
#endif
	  sprintf ((char *) pfile->time.val.str.text, "%02d:%02d:%02d",
		   12, 34, 56);
	}

      if (node->value.builtin == BT_DATE)
	result = &pfile->date;
      else
	result = &pfile->time;
      break;

    case BT_PRAGMA:
      /* Don't interpret _Pragma within directives.  The standard is
         not clear on this, but to me this makes most sense.  */
      if (pfile->state.in_directive)
	return 0;

      _cpp_do__Pragma (pfile);
      return 1;
    }

  push_token_context (pfile, NULL, result, 1);
  return 1;
}

/* Copies SRC, of length LEN, to DEST, adding backslashes before all
   backslashes and double quotes.  Non-printable characters are
   converted to octal.  DEST must be of sufficient size.  Returns
   a pointer to the end of the string.  */
U_CHAR *
cpp_quote_string (dest, src, len)
     U_CHAR *dest;
     const U_CHAR *src;
     unsigned int len;
{
  while (len--)
    {
      U_CHAR c = *src++;

      if (c == '¥¥' || c == '"')
	{
	  *dest++ = '¥¥';
	  *dest++ = c;
	}
      else
	{
	  if (ISPRINT (c))
	    *dest++ = c;
	  else
	    {
	      sprintf ((char *) dest, "¥¥%03o", c);
	      dest += 4;
	    }
	}
    }

  return dest;
}

/* Convert a token sequence ARG to a single string token according to
   the rules of the ISO C #-operator.  */
static const cpp_token *
stringify_arg (pfile, arg)
     cpp_reader *pfile;
     macro_arg *arg;
{
  unsigned char *dest = BUFF_FRONT (pfile->u_buff);
  unsigned int i, escape_it, backslash_count = 0;
  const cpp_token *source = NULL;
  size_t len;

  /* Loop, reading in the argument's tokens.  */
  for (i = 0; i < arg->count; i++)
    {
      const cpp_token *token = arg->first[i];

      if (token->type == CPP_PADDING)
	{
	  if (source == NULL)
	    source = token->val.source;
	  continue;
	}

      escape_it = (token->type == CPP_STRING || token->type == CPP_WSTRING
		   || token->type == CPP_CHAR || token->type == CPP_WCHAR);

      /* Room for each char being written in octal, initial space and
	 final NUL.  */
      len = cpp_token_len (token);
      if (escape_it)
	len *= 4;
      len += 2;

      if ((size_t) (BUFF_LIMIT (pfile->u_buff) - dest) < len)
	{
	  size_t len_so_far = dest - BUFF_FRONT (pfile->u_buff);
	  _cpp_extend_buff (pfile, &pfile->u_buff, len);
	  dest = BUFF_FRONT (pfile->u_buff) + len_so_far;
	}

      /* Leading white space?  */
      if (dest != BUFF_FRONT (pfile->u_buff))
	{
	  if (source == NULL)
	    source = token;
	  if (source->flags & PREV_WHITE)
	    *dest++ = ' ';
	}
      source = NULL;

      if (escape_it)
	{
	  _cpp_buff *buff = _cpp_get_buff (pfile, len);
	  unsigned char *buf = BUFF_FRONT (buff);
	  len = cpp_spell_token (pfile, token, buf) - buf;
	  dest = cpp_quote_string (dest, buf, len);
	  _cpp_release_buff (pfile, buff);
	}
      else
	dest = cpp_spell_token (pfile, token, dest);

      if (token->type == CPP_OTHER && token->val.c == '¥¥')
	backslash_count++;
      else
	backslash_count = 0;
    }

  /* Ignore the final ¥ of invalid string literals.  */
  if (backslash_count & 1)
    {
      cpp_warning (pfile, "invalid string literal, ignoring final '¥¥'");
      dest--;
    }

  /* Commit the memory, including NUL, and return the token.  */
  len = dest - BUFF_FRONT (pfile->u_buff);
  BUFF_FRONT (pfile->u_buff) = dest + 1;
  return new_string_token (pfile, dest - len, len);
}

/* Try to paste two tokens.  On success, return non-zero.  In any
   case, PLHS is updated to point to the pasted token, which is
   guaranteed to not have the PASTE_LEFT flag set.  */
static bool
paste_tokens (pfile, plhs, rhs)
     cpp_reader *pfile;
     const cpp_token **plhs, *rhs;
{
  unsigned char *buf, *end;
  const cpp_token *lhs;
  unsigned int len;
  bool valid;

  lhs = *plhs;
  len = cpp_token_len (lhs) + cpp_token_len (rhs) + 1;
  buf = (unsigned char *) alloca (len);
  end = cpp_spell_token (pfile, lhs, buf);

  /* Avoid comment headers, since they are still processed in stage 3.
     It is simpler to insert a space here, rather than modifying the
     lexer to ignore comments in some circumstances.  Simply returning
     false doesn't work, since we want to clear the PASTE_LEFT flag.  */
  if (lhs->type == CPP_DIV
      && (rhs->type == CPP_MULT || rhs->type == CPP_DIV))
    *end++ = ' ';
  end = cpp_spell_token (pfile, rhs, end);
  *end = '¥0';

  cpp_push_buffer (pfile, buf, end - buf, /* from_stage3 */ true, 1);

  /* Tweak the column number the lexer will report.  */
  pfile->buffer->col_adjust = pfile->cur_token[-1].col - 1;

  /* We don't want a leading # to be interpreted as a directive.  */
  pfile->buffer->saved_flags = 0;

  /* Set pfile->cur_token as required by _cpp_lex_direct.  */
  pfile->cur_token = _cpp_temp_token (pfile);
  *plhs = _cpp_lex_direct (pfile);
  valid = pfile->buffer->cur == pfile->buffer->rlimit;
  _cpp_pop_buffer (pfile);

  return valid;
}

/* Handles an arbitrarily long sequence of ## operators, with initial
   operand LHS.  This implementation is left-associative,
   non-recursive, and finishes a paste before handling succeeding
   ones.  If a paste fails, we back up to the RHS of the failing ##
   operator before pushing the context containing the result of prior
   successful pastes, with the effect that the RHS appears in the
   output stream after the pasted LHS normally.  */
static void
paste_all_tokens (pfile, lhs)
     cpp_reader *pfile;
     const cpp_token *lhs;
{
  const cpp_token *rhs;
  cpp_context *context = pfile->context;

  do
    {
      /* Take the token directly from the current context.  We can do
	 this, because we are in the replacement list of either an
	 object-like macro, or a function-like macro with arguments
	 inserted.  In either case, the constraints to #define
	 guarantee we have at least one more token.  */
      if (context->direct_p)
	rhs = context->first.token++;
      else
	rhs = *context->first.ptoken++;

      if (rhs->type == CPP_PADDING)
	abort ();

      if (!paste_tokens (pfile, &lhs, rhs))
	{
	  _cpp_backup_tokens (pfile, 1);

	  /* Mandatory warning for all apart from assembler.  */
	  if (CPP_OPTION (pfile, lang) != CLK_ASM)
	    cpp_warning (pfile,
	 "pasting ¥"%s¥" and ¥"%s¥" does not give a valid preprocessing token",
			 cpp_token_as_text (pfile, lhs),
			 cpp_token_as_text (pfile, rhs));
	  break;
	}
    }
  while (rhs->flags & PASTE_LEFT);

  /* Put the resulting token in its own context.  */
  push_token_context (pfile, NULL, lhs, 1);
}

/* Reads and returns the arguments to a function-like macro
   invocation.  Assumes the opening parenthesis has been processed.
   If there is an error, emits an appropriate diagnostic and returns
   NULL.  Each argument is terminated by a CPP_EOF token, for the
   future benefit of expand_arg().  */
static _cpp_buff *
collect_args (pfile, node)
     cpp_reader *pfile;
     const cpp_hashnode *node;
{
  _cpp_buff *buff, *base_buff;
  cpp_macro *macro;
  macro_arg *args, *arg;
  const cpp_token *token;
  unsigned int argc;
  bool error = false;

  macro = node->value.macro;
  if (macro->paramc)
    argc = macro->paramc;
  else
    argc = 1;
  buff = _cpp_get_buff (pfile, argc * (50 * sizeof (cpp_token *)
				       + sizeof (macro_arg)));
  base_buff = buff;
  args = (macro_arg *) buff->base;
  memset (args, 0, argc * sizeof (macro_arg));
  buff->cur = (unsigned char *) &args[argc];
  arg = args, argc = 0;

  /* Collect the tokens making up each argument.  We don't yet know
     how many arguments have been supplied, whether too many or too
     few.  Hence the slightly bizarre usage of "argc" and "arg".  */
  do
    {
      unsigned int paren_depth = 0;
      unsigned int ntokens = 0;

      argc++;
      arg->first = (const cpp_token **) buff->cur;

      for (;;)
	{
	  /* Require space for 2 new tokens (including a CPP_EOF).  */
	  if ((unsigned char *) &arg->first[ntokens + 2] > buff->limit)
	    {
	      buff = _cpp_append_extend_buff (pfile, buff,
					      1000 * sizeof (cpp_token *));
	      arg->first = (const cpp_token **) buff->cur;
	    }

	  token = cpp_get_token (pfile);

	  if (token->type == CPP_PADDING)
	    {
	      /* Drop leading padding.  */
	      if (ntokens == 0)
		continue;
	    }
	  else if (token->type == CPP_OPEN_PAREN)
	    paren_depth++;
	  else if (token->type == CPP_CLOSE_PAREN)
	    {
	      if (paren_depth-- == 0)
		break;
	    }
	  else if (token->type == CPP_COMMA)
	    {
	      /* A comma does not terminate an argument within
		 parentheses or as part of a variable argument.  */
	      if (paren_depth == 0
		  && ! (macro->variadic && argc == macro->paramc))
		break;
	    }
	  else if (token->type == CPP_EOF
		   || (token->type == CPP_HASH && token->flags & BOL))
	    break;

	  arg->first[ntokens++] = token;
	}

      /* Drop trailing padding.  */
      while (ntokens > 0 && arg->first[ntokens - 1]->type == CPP_PADDING)
	ntokens--;

      arg->count = ntokens;
      arg->first[ntokens] = &pfile->eof;

      /* Terminate the argument.  Excess arguments loop back and
	 overwrite the final legitimate argument, before failing.  */
      if (argc <= macro->paramc)
	{
	  buff->cur = (unsigned char *) &arg->first[ntokens + 1];
	  if (argc != macro->paramc)
	    arg++;
	}
    }
  while (token->type != CPP_CLOSE_PAREN
	 && token->type != CPP_EOF
	 && token->type != CPP_HASH);

  if (token->type == CPP_EOF || token->type == CPP_HASH)
    {
      bool step_back = false;

      /* 6.10.3 paragraph 11: If there are sequences of preprocessing
	 tokens within the list of arguments that would otherwise act
	 as preprocessing directives, the behavior is undefined.

	 This implementation will report a hard error, terminate the
	 macro invocation, and proceed to process the directive.  */
      if (token->type == CPP_HASH)
	{
	  cpp_error (pfile,
		     "directives may not be used inside a macro argument");
	  step_back = true;
	}
      else
	step_back = (pfile->context->prev || pfile->state.in_directive);

      /* We still need the CPP_EOF to end directives, and to end
	 pre-expansion of a macro argument.  Step back is not
	 unconditional, since we don't want to return a CPP_EOF to our
	 callers at the end of an -include-d file.  */
      if (step_back)
	_cpp_backup_tokens (pfile, 1);
      cpp_error (pfile, "unterminated argument list invoking macro ¥"%s¥"",
		 NODE_NAME (node));
      error = true;
    }
  else if (argc < macro->paramc)
    {
      /* As an extension, a rest argument is allowed to not appear in
	 the invocation at all.
	 e.g. #define debug(format, args...) something
	 debug("string");
	 
	 This is exactly the same as if there had been an empty rest
	 argument - debug("string", ).  */

      if (argc + 1 == macro->paramc && macro->variadic)
	{
	  if (CPP_PEDANTIC (pfile) && ! macro->syshdr)
	    cpp_pedwarn (pfile, "ISO C99 requires rest arguments to be used");
	}
      else
	{
	  cpp_error (pfile,
		     "macro ¥"%s¥" requires %u arguments, but only %u given",
		     NODE_NAME (node), macro->paramc, argc);
	  error = true;
	}
    }
  else if (argc > macro->paramc)
    {
      /* Empty argument to a macro taking no arguments is OK.  */
      if (argc != 1 || arg->count)
	{
	  cpp_error (pfile,
		     "macro ¥"%s¥" passed %u arguments, but takes just %u",
		     NODE_NAME (node), argc, macro->paramc);
	  error = true;
	}
    }

  if (!error)
    return base_buff;

  _cpp_release_buff (pfile, base_buff);
  return NULL;
}

/* Search for an opening parenthesis to the macro of NODE, in such a
   way that, if none is found, we don't lose the information in any
   intervening padding tokens.  If we find the parenthesis, collect
   the arguments and return the buffer containing them.  */
static _cpp_buff *
funlike_invocation_p (pfile, node)
     cpp_reader *pfile;
     cpp_hashnode *node;
{
  const cpp_token *token, *padding = NULL;

  for (;;)
    {
      token = cpp_get_token (pfile);
