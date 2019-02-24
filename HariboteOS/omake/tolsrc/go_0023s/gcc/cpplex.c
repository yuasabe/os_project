/* CPP Library - lexical analysis.
   Copyright (C) 2000, 2001, 2002 Free Software Foundation, Inc.
   Contributed by Per Bothner, 1994-95.
   Based on CCCP program by Paul Rubin, June 1986
   Adapted to ANSI C, Richard Stallman, Jan 1987
   Broken out to separate file, Zack Weinberg, Mar 2000
   Single-pass line tokenization by Neil Booth, April 2000

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
#include "cpplib.h"
#include "cpphash.h"

/* MULTIBYTE_CHARS support only works for native compilers.
   ??? Ideally what we want is to model widechar support after
   the current floating point support.  */
#ifdef CROSS_COMPILE
#undef MULTIBYTE_CHARS
#endif

#ifdef MULTIBYTE_CHARS
#include "mbchar.h"
#include <locale.h>
#endif

/* Tokens with SPELL_STRING store their spelling in the token list,
   and it's length in the token->val.name.len.  */
enum spell_type
{
  SPELL_OPERATOR = 0,
  SPELL_CHAR,
  SPELL_IDENT,
  SPELL_NUMBER,
  SPELL_STRING,
  SPELL_NONE
};

struct token_spelling
{
  enum spell_type category;
  const unsigned char *name;
};

static const unsigned char *const digraph_spellings[] =
{ U"%:", U"%:%:", U"<:", U":>", U"<%", U"%>" };

#define OP(e, s) { SPELL_OPERATOR, U s           },
#define TK(e, s) { s,              U STRINGX (e) },
static const struct token_spelling token_spellings[N_TTYPES] = { TTYPE_TABLE };
#undef OP
#undef TK

#define TOKEN_SPELL(token) (token_spellings[(token)->type].category)
#define TOKEN_NAME(token) (token_spellings[(token)->type].name)
#define BACKUP() do {buffer->cur = buffer->backup_to;} while (0)

static void handle_newline PARAMS ((cpp_reader *));
static cppchar_t skip_escaped_newlines PARAMS ((cpp_reader *));
static cppchar_t get_effective_char PARAMS ((cpp_reader *));

static int skip_block_comment PARAMS ((cpp_reader *));
static int skip_line_comment PARAMS ((cpp_reader *));
static void adjust_column PARAMS ((cpp_reader *));
static int skip_whitespace PARAMS ((cpp_reader *, cppchar_t));
static cpp_hashnode *parse_identifier PARAMS ((cpp_reader *));
static cpp_hashnode *parse_identifier_slow PARAMS ((cpp_reader *,
						    const U_CHAR *));
static void parse_number PARAMS ((cpp_reader *, cpp_string *, cppchar_t, int));
static int unescaped_terminator_p PARAMS ((cpp_reader *, const U_CHAR *));
static void parse_string PARAMS ((cpp_reader *, cpp_token *, cppchar_t));
static void unterminated PARAMS ((cpp_reader *, int));
static bool trigraph_p PARAMS ((cpp_reader *));
static void save_comment PARAMS ((cpp_reader *, cpp_token *, const U_CHAR *));
static int name_p PARAMS ((cpp_reader *, const cpp_string *));
static int maybe_read_ucs PARAMS ((cpp_reader *, const unsigned char **,
				   const unsigned char *, unsigned int *));
static tokenrun *next_tokenrun PARAMS ((tokenrun *));

static unsigned int hex_digit_value PARAMS ((unsigned int));
static _cpp_buff *new_buff PARAMS ((size_t));

/* Utility routine:

   Compares, the token TOKEN to the NUL-terminated string STRING.
   TOKEN must be a CPP_NAME.  Returns 1 for equal, 0 for unequal.  */
int
cpp_ideq (token, string)
     const cpp_token *token;
     const char *string;
{
  if (token->type != CPP_NAME)
    return 0;

  return !ustrcmp (NODE_NAME (token->val.node), (const U_CHAR *) string);
}

/* Call when meeting a newline, assumed to be in buffer->cur[-1].
   Returns with buffer->cur pointing to the character immediately
   following the newline (combination).  */
static void
handle_newline (pfile)
     cpp_reader *pfile;
{
  cpp_buffer *buffer = pfile->buffer;

  /* Handle CR-LF and LF-CR.  Most other implementations (e.g. java)
     only accept CR-LF; maybe we should fall back to that behaviour?  */
  if (buffer->cur[-1] + buffer->cur[0] == '¥r' + '¥n')
    buffer->cur++;

  buffer->line_base = buffer->cur;
  buffer->col_adjust = 0;
  pfile->line++;
}

/* Subroutine of skip_escaped_newlines; called when a 3-character
   sequence beginning with "??" is encountered.  buffer->cur points to
   the second '?'.

   Warn if necessary, and returns true if the sequence forms a
   trigraph and the trigraph should be honoured.  */
static bool
trigraph_p (pfile)
     cpp_reader *pfile;
{
  cpp_buffer *buffer = pfile->buffer;
  cppchar_t from_char = buffer->cur[1];
  bool accept;

  if (!_cpp_trigraph_map[from_char])
    return false;

  accept = CPP_OPTION (pfile, trigraphs);

  /* Don't warn about trigraphs in comments.  */
  if (CPP_OPTION (pfile, warn_trigraphs) && !pfile->state.lexing_comment)
    {
      if (accept)
	cpp_warning_with_line (pfile, pfile->line, CPP_BUF_COL (buffer) - 1,
			       "trigraph ??%c converted to %c",
			       (int) from_char,
			       (int) _cpp_trigraph_map[from_char]);
      else if (buffer->cur != buffer->last_Wtrigraphs)
	{
	  buffer->last_Wtrigraphs = buffer->cur;
	  cpp_warning_with_line (pfile, pfile->line,
				 CPP_BUF_COL (buffer) - 1,
				 "trigraph ??%c ignored", (int) from_char);
	}
    }

  return accept;
}

/* Skips any escaped newlines introduced by '?' or a '¥¥', assumed to
   lie in buffer->cur[-1].  Returns the next byte, which will be in
   buffer->cur[-1].  This routine performs preprocessing stages 1 and
   2 of the ISO C standard.  */
static cppchar_t
skip_escaped_newlines (pfile)
     cpp_reader *pfile;
{
  cpp_buffer *buffer = pfile->buffer;
  cppchar_t next = buffer->cur[-1];

  /* Only do this if we apply stages 1 and 2.  */
  if (!buffer->from_stage3)
    {
      const unsigned char *saved_cur;
      cppchar_t next1;

      do
	{
	  if (next == '?')
	    {
	      if (buffer->cur[0] != '?' || !trigraph_p (pfile))
		break;

	      /* Translate the trigraph.  */
	      next = _cpp_trigraph_map[buffer->cur[1]];
	      buffer->cur += 2;
	      if (next != '¥¥')
		break;
	    }

	  if (buffer->cur == buffer->rlimit)
	    break;

	  /* We have a backslash, and room for at least one more
	     character.  Skip horizontal whitespace.  */
	  saved_cur = buffer->cur;
	  do
	    next1 = *buffer->cur++;
	  while (is_nvspace (next1) && buffer->cur < buffer->rlimit);

	  if (!is_vspace (next1))
	    {
	      buffer->cur = saved_cur;
	      break;
	    }

	  if (saved_cur != buffer->cur - 1
	      && !pfile->state.lexing_comment)
	    cpp_warning (pfile, "backslash and newline separated by space");

	  handle_newline (pfile);
	  buffer->backup_to = buffer->cur;
	  if (buffer->cur == buffer->rlimit)
	    {
	      cpp_pedwarn (pfile, "backslash-newline at end of file");
	      next = EOF;
	    }
	  else
	    next = *buffer->cur++;
	}
      while (next == '¥¥' || next == '?');
    }

  return next;
}

/* Obtain the next character, after trigraph conversion and skipping
   an arbitrarily long string of escaped newlines.  The common case of
   no trigraphs or escaped newlines falls through quickly.  On return,
   buffer->backup_to points to where to return to if the character is
   not to be processed.  */
static cppchar_t
get_effective_char (pfile)
     cpp_reader *pfile;
{
  cppchar_t next;
  cpp_buffer *buffer = pfile->buffer;

  buffer->backup_to = buffer->cur;
  next = *buffer->cur++;
  if (__builtin_expect (next == '?' || next == '¥¥', 0))
    next = skip_escaped_newlines (pfile);

   return next;
}

/* Skip a C-style block comment.  We find the end of the comment by
   seeing if an asterisk is before every '/' we encounter.  Returns
   non-zero if comment terminated by EOF, zero otherwise.  */
static int
skip_block_comment (pfile)
     cpp_reader *pfile;
{
  cpp_buffer *buffer = pfile->buffer;
  cppchar_t c = EOF, prevc = EOF;

  pfile->state.lexing_comment = 1;
  while (buffer->cur != buffer->rlimit)
    {
      prevc = c, c = *buffer->cur++;

      /* FIXME: For speed, create a new character class of characters
	 of interest inside block comments.  */
      if (c == '?' || c == '¥¥')
	c = skip_escaped_newlines (pfile);

      /* People like decorating comments with '*', so check for '/'
	 instead for efficiency.  */
      if (c == '/')
	{
	  if (prevc == '*')
	    break;

	  /* Warn about potential nested comments, but not if the '/'
	     comes immediately before the true comment delimiter.
	     Don't bother to get it right across escaped newlines.  */
	  if (CPP_OPTION (pfile, warn_comments)
	      && buffer->cur[0] == '*' && buffer->cur[1] != '/')
	    cpp_warning_with_line (pfile,
				   pfile->line, CPP_BUF_COL (buffer),
				   "¥"/*¥" within comment");
	}
      else if (is_vspace (c))
	handle_newline (pfile);
      else if (c == '¥t')
	adjust_column (pfile);
    }

  pfile->state.lexing_comment = 0;
  return c != '/' || prevc != '*';
}

/* Skip a C++ line comment, leaving buffer->cur pointing to the
   terminating newline.  Handles escaped newlines.  Returns non-zero
   if a multiline comment.  */
static int
skip_line_comment (pfile)
     cpp_reader *pfile;
{
  cpp_buffer *buffer = pfile->buffer;
  unsigned int orig_line = pfile->line;
  cppchar_t c;

  pfile->state.lexing_comment = 1;
  do
    {
      if (buffer->cur == buffer->rlimit)
	goto at_eof;

      c = *buffer->cur++;
      if (c == '?' || c == '¥¥')
	c = skip_escaped_newlines (pfile);
    }
  while (!is_vspace (c));

  /* Step back over the newline, except at EOF.  */
  buffer->cur--;
 at_eof:

  pfile->state.lexing_comment = 0;
  return orig_line != pfile->line;
}

/* pfile->buffer->cur is one beyond the ¥t character.  Update
   col_adjust so we track the column correctly.  */
static void
adjust_column (pfile)
     cpp_reader *pfile;
{
  cpp_buffer *buffer = pfile->buffer;
  unsigned int col = CPP_BUF_COL (buffer) - 1; /* Zero-based column.  */

  /* Round it up to multiple of the tabstop, but subtract 1 since the
     tab itself occupies a character position.  */
  buffer->col_adjust += (CPP_OPTION (pfile, tabstop)
			 - col % CPP_OPTION (pfile, tabstop)) - 1;
}

/* Skips whitespace, saving the next non-whitespace character.
   Adjusts pfile->col_adjust to account for tabs.  Without this,
   tokens might be assigned an incorrect column.  */
static int
skip_whitespace (pfile, c)
     cpp_reader *pfile;
     cppchar_t c;
{
  cpp_buffer *buffer = pfile->buffer;
  unsigned int warned = 0;

  do
    {
      /* Horizontal space always OK.  */
      if (c == ' ')
	;
      else if (c == '¥t')
	adjust_column (pfile);
      /* Just ¥f ¥v or ¥0 left.  */
      else if (c == '¥0')
	{
	  if (buffer->cur - 1 == buffer->rlimit)
	    return 0;
	  if (!warned)
	    {
	      cpp_warning (pfile, "null character(s) ignored");
	      warned = 1;
	    }
	}
      else if (pfile->state.in_directive && CPP_PEDANTIC (pfile))
	cpp_pedwarn_with_line (pfile, pfile->line,
			       CPP_BUF_COL (buffer),
			       "%s in preprocessing directive",
			       c == '¥f' ? "form feed" : "vertical tab");

      c = *buffer->cur++;
    }
  /* We only want non-vertical space, i.e. ' ' ¥t ¥f ¥v ¥0.  */
  while (is_nvspace (c));

  buffer->cur--;
  return 1;
}

/* See if the characters of a number token are valid in a name (no
   '.', '+' or '-').  */
static int
name_p (pfile, string)
     cpp_reader *pfile;
     const cpp_string *string;
{
  unsigned int i;

  for (i = 0; i < string->len; i++)
    if (!is_idchar (string->text[i]))
      return 0;

  return 1;  
}

/* Parse an identifier, skipping embedded backslash-newlines.  This is
   a critical inner loop.  The common case is an identifier which has
   not been split by backslash-newline, does not contain a dollar
   sign, and has already been scanned (roughly 10:1 ratio of
   seen:unseen identifiers in normal code; the distribution is
   Poisson-like).  Second most common case is a new identifier, not
   split and no dollar sign.  The other possibilities are rare and
   have been relegated to parse_identifier_slow.  */
static cpp_hashnode *
parse_identifier (pfile)
     cpp_reader *pfile;
{
  cpp_hashnode *result;
  const U_CHAR *cur;

  /* Fast-path loop.  Skim over a normal identifier.
     N.B. ISIDNUM does not include $.  */
  cur = pfile->buffer->cur;
  while (ISIDNUM (*cur))
    cur++;

  /* Check for slow-path cases.  */
  if (*cur == '?' || *cur == '¥¥' || *cur == '$')
    result = parse_identifier_slow (pfile, cur);
  else
    {
      const U_CHAR *base = pfile->buffer->cur - 1;
      result = (cpp_hashnode *)
	ht_lookup (pfile->hash_table, base, cur - base, HT_ALLOC);
      pfile->buffer->cur = cur;
    }

  /* Rarely, identifiers require diagnostics when lexed.
     XXX Has to be forced out of the fast path.  */
  if (__builtin_expect ((result->flags & NODE_DIAGNOSTIC)
			&& !pfile->state.skipping, 0))
    {
      /* It is allowed to poison the same identifier twice.  */
      if ((result->flags & NODE_POISONED) && !pfile->state.poisoned_ok)
	cpp_error (pfile, "attempt to use poisoned ¥"%s¥"",
		   NODE_NAME (result));

      /* Constraint 6.10.3.5: __VA_ARGS__ should only appear in the
	 replacement list of a variadic macro.  */
      if (result == pfile->spec_nodes.n__VA_ARGS__
	  && !pfile->state.va_args_ok)
	cpp_pedwarn (pfile,
	"__VA_ARGS__ can only appear in the expansion of a C99 variadic macro");
    }

  return result;
}

/* Slow path.  This handles identifiers which have been split, and
   identifiers which contain dollar signs.  The part of the identifier
   from PFILE->buffer->cur-1 to CUR has already been scanned.  */
static cpp_hashnode *
parse_identifier_slow (pfile, cur)
     cpp_reader *pfile;
     const U_CHAR *cur;
{
  cpp_buffer *buffer = pfile->buffer;
  const U_CHAR *base = buffer->cur - 1;
  struct obstack *stack = &pfile->hash_table->stack;
  unsigned int c, saw_dollar = 0, len;

  /* Copy the part of the token which is known to be okay.  */
  obstack_grow (stack, base, cur - base);

  /* Now process the part which isn't.  We are looking at one of
     '$', '¥¥', or '?' on entry to this loop.  */
  c = *cur++;
  buffer->cur = cur;
  do
    {
      while (is_idchar (c))
        {
          obstack_1grow (stack, c);

          if (c == '$')
            saw_dollar++;

          c = *buffer->cur++;
        }

      /* Potential escaped newline?  */
      buffer->backup_to = buffer->cur - 1;
      if (c != '?' && c != '¥¥')
        break;
      c = skip_escaped_newlines (pfile);
    }
  while (is_idchar (c));

  /* Step back over the unwanted char.  */
  BACKUP ();

  /* $ is not an identifier character in the standard, but is commonly
     accepted as an extension.  Don't warn about it in skipped
     conditional blocks.  */
  if (saw_dollar && CPP_PEDANTIC (pfile) && ! pfile->state.skipping)
    cpp_pedwarn (pfile, "'$' character(s) in identifier");

  /* Identifiers are null-terminated.  */
  len = obstack_object_size (stack);
  obstack_1grow (stack, '¥0');

  return (cpp_hashnode *)
    ht_lookup (pfile->hash_table, obstack_finish (stack), len, HT_ALLOCED);
}

/* Parse a number, beginning with character C, skipping embedded
   backslash-newlines.  LEADING_PERIOD is non-zero if there was a "."
   before C.  Place the result in NUMBER.  */
static void
parse_number (pfile, number, c, leading_period)
     cpp_reader *pfile;
     cpp_string *number;
     cppchar_t c;
     int leading_period;
{
  cpp_buffer *buffer = pfile->buffer;
  unsigned char *dest, *limit;

  dest = BUFF_FRONT (pfile->u_buff);
  limit = BUFF_LIMIT (pfile->u_buff);

  /* Place a leading pe