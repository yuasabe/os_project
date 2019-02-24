/* Lexical analyzer for C and Objective C.
   Copyright (C) 1987, 1988, 1989, 1992, 1994, 1995, 1996, 1997
   1998, 1999, 2000 Free Software Foundation, Inc.

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

/* !kawai! */
#include "config.h"
#include "system.h"

#include "rtl.h"
#include "tree.h"
#include "expr.h"
#include "input.h"
#include "output.h"
#include "c-lex.h"
#include "c-tree.h"
#include "flags.h"
#include "timevar.h"
#include "cpplib.h"
#include "c-pragma.h"
#include "toplev.h"
#include "intl.h"
#include "tm_p.h"
#include "../include/splay-tree.h"
#include "debug.h"
/* end of !kawai! */

/* MULTIBYTE_CHARS support only works for native compilers.
   ??? Ideally what we want is to model widechar support after
   the current floating point support.  */
#ifdef CROSS_COMPILE
#undef MULTIBYTE_CHARS
#endif

#ifdef MULTIBYTE_CHARS
#include "mbchar.h"
#include <locale.h>
#endif /* MULTIBYTE_CHARS */
#ifndef GET_ENVIRONMENT
#define GET_ENVIRONMENT(ENV_VALUE,ENV_NAME) ((ENV_VALUE) = getenv (ENV_NAME))
#endif

/* The current line map.  */
static const struct line_map *map;

/* The line used to refresh the lineno global variable after each token.  */
static unsigned int src_lineno;

/* We may keep statistics about how long which files took to compile.  */
static int header_time, body_time;
static splay_tree file_info_tree;

/* Cause the `yydebug' variable to be defined.  */
#define YYDEBUG 1

/* File used for outputting assembler code.  */
extern FILE *asm_out_file;

#undef WCHAR_TYPE_SIZE
#define WCHAR_TYPE_SIZE TYPE_PRECISION (wchar_type_node)

/* Number of bytes in a wide character.  */
#define WCHAR_BYTES (WCHAR_TYPE_SIZE / BITS_PER_UNIT)

int indent_level;        /* Number of { minus number of }.  */
int pending_lang_change; /* If we need to switch languages - C++ only */
int c_header_level;	 /* depth in C headers - C++ only */

/* Nonzero tells yylex to ignore ¥ in string constants.  */
static int ignore_escape_flag;

static void parse_float		PARAMS ((PTR));
static tree lex_number		PARAMS ((const char *, unsigned int));
static tree lex_string		PARAMS ((const unsigned char *, unsigned int,
					 int));
static tree lex_charconst	PARAMS ((const cpp_token *));
static void update_header_times	PARAMS ((const char *));
static int dump_one_header	PARAMS ((splay_tree_node, void *));
static void cb_line_change     PARAMS ((cpp_reader *, const cpp_token *, int));
static void cb_ident		PARAMS ((cpp_reader *, unsigned int,
					 const cpp_string *));
static void cb_file_change    PARAMS ((cpp_reader *, const struct line_map *));
static void cb_def_pragma	PARAMS ((cpp_reader *, unsigned int));
static void cb_define		PARAMS ((cpp_reader *, unsigned int,
					 cpp_hashnode *));
static void cb_undef		PARAMS ((cpp_reader *, unsigned int,
					 cpp_hashnode *));

const char *
init_c_lex (filename)
     const char *filename;
{
  struct cpp_callbacks *cb;
  struct c_fileinfo *toplevel;

  /* Set up filename timing.  Must happen before cpp_read_main_file.  */
  file_info_tree = splay_tree_new ((splay_tree_compare_fn)strcmp,
				   0,
				   (splay_tree_delete_value_fn)free);
  toplevel = get_fileinfo ("<top level>");
  if (flag_detailed_statistics)
    {
      header_time = 0;
      body_time = get_run_time ();
      toplevel->time = body_time;
    }
  
#ifdef MULTIBYTE_CHARS
  /* Change to the native locale for multibyte conversions.  */
  setlocale (LC_CTYPE, "");
  GET_ENVIRONMENT (literal_codeset, "LANG");
#endif

  cb = cpp_get_callbacks (parse_in);

  cb->line_change = cb_line_change;
  cb->ident = cb_ident;
  cb->file_change = cb_file_change;
  cb->def_pragma = cb_def_pragma;

  /* Set the debug callbacks if we can use them.  */
  if (debug_info_level == DINFO_LEVEL_VERBOSE
      && (write_symbols == DWARF_DEBUG || write_symbols == DWARF2_DEBUG
          || write_symbols == VMS_AND_DWARF2_DEBUG))
    {
      cb->define = cb_define;
      cb->undef = cb_undef;
    }

  /* Start it at 0.  */
  lineno = 0;

  if (filename == NULL || !strcmp (filename, "-"))
    filename = "";

  return cpp_read_main_file (parse_in, filename, ident_hash);
}

/* A thin wrapper around the real parser that initializes the 
   integrated preprocessor after debug output has been initialized.
   Also, make sure the start_source_file debug hook gets called for
   the primary source file.  */

int
yyparse()
{
  (*debug_hooks->start_source_file) (lineno, input_filename);
  cpp_finish_options (parse_in);

  return yyparse_1();
}

struct c_fileinfo *
get_fileinfo (name)
     const char *name;
{
  splay_tree_node n;
  struct c_fileinfo *fi;

  n = splay_tree_lookup (file_info_tree, (splay_tree_key) name);
  if (n)
    return (struct c_fileinfo *) n->value;

  fi = (struct c_fileinfo *) xmalloc (sizeof (struct c_fileinfo));
  fi->time = 0;
  fi->interface_only = 0;
  fi->interface_unknown = 1;
  splay_tree_insert (file_info_tree, (splay_tree_key) name,
		     (splay_tree_value) fi);
  return fi;
}

static void
update_header_times (name)
     const char *name;
{
  /* Changing files again.  This means currently collected time
     is charged against header time, and body time starts back at 0.  */
  if (flag_detailed_statistics)
    {
      int this_time = get_run_time ();
      struct c_fileinfo *file = get_fileinfo (name);
      header_time += this_time - body_time;
      file->time += this_time - body_time;
      body_time = this_time;
    }
}

static int
dump_one_header (n, dummy)
     splay_tree_node n;
     void *dummy ATTRIBUTE_UNUSED;
{
  print_time ((const char *) n->key,
	      ((struct c_fileinfo *) n->value)->time);
  return 0;
}

void
dump_time_statistics ()
{
  struct c_fileinfo *file = get_fileinfo (input_filename);
  int this_time = get_run_time ();
  file->time += this_time - body_time;

  fprintf (stderr, "¥n******¥n");
  print_time ("header files (total)", header_time);
  print_time ("main file (total)", this_time - body_time);
  fprintf (stderr, "ratio = %g : 1¥n",
	   (double)header_time / (double)(this_time - body_time));
  fprintf (stderr, "¥n******¥n");

  splay_tree_foreach (file_info_tree, dump_one_header, 0);
}

/* Not yet handled: #pragma, #define, #undef.
   No need to deal with linemarkers under normal conditions.  */

static void
cb_ident (pfile, line, str)
     cpp_reader *pfile ATTRIBUTE_UNUSED;
     unsigned int line ATTRIBUTE_UNUSED;
     const cpp_string *str ATTRIBUTE_UNUSED;
{
#ifdef ASM_OUTPUT_IDENT
  if (! flag_no_ident)
    {
      /* Convert escapes in the string.  */
      tree value = lex_string (str->text, str->len, 0);
      ASM_OUTPUT_IDENT (asm_out_file, TREE_STRING_POINTER (value));
    }
#endif
}

/* Called at the start of every non-empty line.  TOKEN is the first
   lexed token on the line.  Used for diagnostic line numbers.  */
static void
cb_line_change (pfile, token, parsing_args)
     cpp_reader *pfile ATTRIBUTE_UNUSED;
     const cpp_token *token;
     int parsing_args ATTRIBUTE_UNUSED;
{
  src_lineno = SOURCE_LINE (map, token->line);
}

static void
cb_file_change (pfile, new_map)
     cpp_reader *pfile ATTRIBUTE_UNUSED;
     const struct line_map *new_map;
{
  unsigned int to_line = SOURCE_LINE (new_map, new_map->to_line);

  if (new_map->reason == LC_ENTER)
    {
      /* Don't stack the main buffer on the input stack;
	 we already did in compile_file.  */
      if (map == NULL)
	main_input_filename = new_map->to_file;
      else
	{
          int included_at = SOURCE_LINE (new_map - 1, new_map->from_line - 1);

	  lineno = included_at;
	  push_srcloc (new_map->to_file, 1);
	  input_file_stack->indent_level = indent_level;
	  (*debug_hooks->start_source_file) (included_at, new_map->to_file);
#ifndef NO_IMPLICIT_EXTERN_C
	  if (c_header_level)
	    ++c_header_level;
	  else if (new_map->sysp == 2)
	    {
	      c_header_level = 1;
	      ++pending_lang_change;
	    }
#endif
	}
    }
  else if (new_map->reason == LC_LEAVE)
    {
#ifndef NO_IMPLICIT_EXTERN_C
      if (c_header_level && --c_header_level == 0)
	{
	  if (new_map->sysp == 2)
	    warning ("badly nested C headers from preprocessor");
	  --pending_lang_change;
	}
#endif
#if 0
      if (indent_level != input_file_stack->indent_level)
	{
	  warning_with_file_and_line
	    (input_filename, lineno,
	     "this file contains more '%c's than '%c's",
	     indent_level > input_file_stack->indent_level ? '{' : '}',
	     indent_level > input_file_stack->indent_level ? '}' : '{');
	}
#endif
      pop_srcloc ();
      
      (*debug_hooks->end_source_file) (to_line);
    }

  update_header_times (new_map->to_file);
  in_system_header = new_map->sysp != 0;
  input_filename = new_map->to_file;
  lineno = to_line;
  map = new_map;

  /* Hook for C++.  */
  extract_interface_info ();
}

static void
cb_def_pragma (pfile, line)
     cpp_reader *pfile;
     unsigned int line;
{
  /* Issue a warning message if we have been asked to do so.  Ignore
     unknown pragmas in system headers unless an explicit
     -Wunknown-pragmas has been given.  */
  if (warn_unknown_pragmas > in_system_header)
    {
      const unsigned char *space, *name;
      const cpp_token *s;

      space = name = (const unsigned char *) "";
      s = cpp_get_token (pfile);
      if (s->type != CPP_EOF)
	{
	  space = cpp_token_as_text (pfile, s);
	  s = cpp_get_token (pfile);
	  if (s->type == CPP_NAME)
	    name = cpp_token_as_text (pfile, s);
	}

      lineno = SOURCE_LINE (map, line);
      warning ("ignoring #pragma %s %s", space, name);
    }
}

/* #define callback for DWARF and DWARF2 debug info.  */
static void
cb_define (pfile, line, node)
     cpp_reader *pfile;
     unsigned int line;
     cpp_hashnode *node;
{
  (*debug_hooks->define) (SOURCE_LINE (map, line),
			  (const char *) cpp_macro_definition (pfile, node));
}

/* #undef callback for DWARF and DWARF2 debug info.  */
static void
cb_undef (pfile, line, node)
     cpp_reader *pfile ATTRIBUTE_UNUSED;
     unsigned int line;
     cpp_hashnode *node;
{
  (*debug_hooks->undef) (SOURCE_LINE (map, line),
			 (const char *) NODE_NAME (node));
}

#if 0 /* not yet */
/* Returns nonzero if C is a universal-character-name.  Give an error if it
   is not one which may appear in an identifier, as per [extendid].

   Note that extended character support in identifiers has not yet been
   implemented.  It is my personal opinion that this is not a desirable
   feature.  Portable code cannot count on support for more than the basic
   identifier character set.  */

static inline int
is_extended_char (c)
     int c;
{
#ifdef TARGET_EBCDIC
  return 0;
#else
  /* ASCII.  */
  if (c < 0x7f)
    return 0;

  /* None of the valid chars are outside the Basic Multilingual Plane (the
     low 16 bits).  */
  if (c > 0xffff)
    {
      error ("universal-character-name '¥¥U%08x' not valid in identifier", c);
      return 1;
    }
  
  /* Latin */
  if ((c >= 0x00c0 && c <= 0x00d6)
      || (c >= 0x00d8 && c <= 0x00f6)
      || (c >= 0x00f8 && c <= 0x01f5)
      || (c >= 0x01fa && c <= 0x0217)
      || (c >= 0x0250 && c <= 0x02a8)
      || (c >= 0x1e00 && c <= 0x1e9a)
      || (c >= 0x1ea0 && c <= 0x1ef9))
    return 1;

  /* Greek */
  if ((c == 0x0384)
      || (c >= 0x0388 && c <= 0x038a)
      || (c == 0x038c)
      || (c >= 0x038e && c <= 0x03a1)
      || (c >= 0x03a3 && c <= 0x03ce)
      || (c >= 0x03d0 && c <= 0x03d6)
      || (c == 0x03da)
      || (c == 0x03dc)
      || (c == 0x03de)
      || (c == 0x03e0)
      || (c >= 0x03e2 && c <= 0x03f3)
      || (c >= 0x1f00 && c <= 0x1f15)
      || (c >= 0x1f18 && c <= 0x1f1d)
      || (c >= 0x1f20 && c <= 0x1f45)
      || (c >= 0x1f48 && c <= 0x1f4d)
      || (c >= 0x1f50 && c <= 0x1f57)
      || (c == 0x1f59)
      || (c == 0x1f5b)
      || (c == 0x1f5d)
      || (c >= 0x1f5f && c <= 0x1f7d)
      || (c >= 0x1f80 && c <= 0x1fb4)
      || (c >= 0x1fb6 && c <= 0x1fbc)
      || (c >= 0x1fc2 && c <= 0x1fc4)
      || (c >= 0x1fc6 && c <= 0x1fcc)
      || (c >= 0x1fd0 && c <= 0x1fd3)
      || (c >= 0x1fd6 && c <= 0x1fdb)
      || (c >= 0x1fe0 && c <= 0x1fec)
      || (c >= 0x1ff2 && c <= 0x1ff4)
      || (c >= 0x1ff6 && c <= 0x1ffc))
    return 1;

  /* Cyrillic */
  if ((c >= 0x0401 && c <= 0x040d)
      || (c >= 0x040f && c <= 0x044f)
      || (c >= 0x0451 && c <= 0x045c)
      || (c >= 0x045e && c <= 0x0481)
      || (c >= 0x0490 && c <= 0x04c4)
      || (c >= 0x04c7 && c <= 0x04c8)
      || (c >= 0x04cb && c <= 0x04cc)
      || (c >= 0x04d0 && c <= 0x04eb)
      || (c >= 0x04ee && c <= 0x04f5)
      || (c >= 0x04f8 && c <= 0x04f9))
    return 1;

  /* Armenian */
  if ((c >= 0x0531 && c <= 0x0556)
      || (c >= 0x0561 && c <= 0x0587))
    return 1;

  /* Hebrew */
  if ((c >= 0x05d0 && c <= 0x05ea)
      || (c >= 0x05f0 && c <= 0x05f4))
    return 1;

  /* Arabic */
  if ((c >= 0x0621 && c <= 0x063a)
      || (c >= 0x0640 && c <= 0x0652)
      || (c >= 0x0670 && c <= 0x06b7)
      || (c >= 0x06ba && c <= 0x06be)
      || (c >= 0x06c0 && c <= 0x06ce)
      || (c >= 0x06e5 && c <= 0x06e7))
    return 1;

  /* Devanagari */
  if ((c >= 0x0905 && c <= 0x0939)
      || (c >= 0x0958 && c <= 0x0962))
    return 1;

  /* Bengali */
  if ((c >= 0x0985 && c <= 0x098c)
      || (c >= 0x098f && c <= 0x0990)
      || (c >= 0x0993 && c <= 0x09a8)
      || (c >= 0x09aa && c <= 0x09b0)
      || (c == 0x09b2)
      || (c >= 0x09b6 && c <= 0x09b9)
      || (c >= 0x09dc && c <= 0x09dd)
      || (c >= 0x09df && c <= 0x09e1)
      || (c >= 0x09f0 && c <= 0x09f1))
    return 1;

  /* Gurmukhi */
  if ((c >= 0x0a05 && c <= 0x0a0a)
      || (c >= 0x0a0f && c <= 0x0a10)
      || (c >= 0x0a13 && c <= 0x0a28)
      || (c >= 0x0a2a && c <= 0x0a30)
      || (c >= 0x0a32 && c <= 0x0a33)
      || (c >= 0x0a35 && c <= 0x0a36)
      || (c >= 0x0a38 && c <= 0x0a39)
      || (c >= 0x0a59 && c <= 0x0a5c)
      || (c == 0x0a5e))
    return 1;

  /* Gujarati */
  if ((c >= 0x0a85 && c <= 0x0a8b)
      || (c == 0x0a8d)
      || (c >= 0x0a8f && c <= 0x0a91)
      || (c >= 0x0a93 && c <= 0x0aa8)
      || (c >= 0x0aaa && c <= 0x0ab0)
      || (c >= 0x0ab2 && c <= 0x0ab3)
      || (c >= 0x0ab5 && c <= 0x0ab9)
      || (c == 0x0ae0))
    return 1;

  /* Oriya */
  if ((c >= 0x0b05 && c <= 0x0b0c)
      || (c >= 0x0b0f && c <= 0x0b10)
      || (c >= 0x0b13 && c <= 0x0b28)
      || (c >= 0x0b2a && c <= 0x0b30)
      || (c >= 0x0b32 && c <= 0x0b33)
      || (c >= 0x0b36 && c <= 0x0b39)
      || (c >= 0x0b5c && c <= 0x0b5d)
      || (c >= 0x0b5f && c <= 0x0b61))
    return 1;

  /* Tamil */
  if ((c >= 0x0b85 && c <= 0x0b8a)
      || (c >= 0x0b8e && c <= 0x0b90)
      || (c >= 0x0b92 && c <= 0x0b95)
      || (c >= 0x0b99 && c <= 0x0b9a)
      || (c == 0x0b9c)
      || (c >= 0x0b9e && c <= 0x0b9f)
      || (c >= 0x0ba3 && c <= 0x0ba4)
      || (c >= 0x0ba8 && c <= 0x0baa)
      || (c >= 0x0bae && c <= 0x0bb5)
      || (c >= 0x0bb7 && c <= 0x0bb9))
    return 1;

  /* Telugu */
  if ((c >= 0x0c05 && c <= 0x0c0c)
      || (c >= 0x0c0e && c <= 0x0c10)
      || (c >= 0x0c12 && c <= 0x0c28)
      || (c >= 0x0c2a && c <= 0x0c33)
      || (c >= 0x0c35 && c <= 0x0c39)
      || (c >= 0x0c60 && c <= 0x0c61))
    return 1;

  /* Kannada */
  if ((c >= 0x0c85 && c <= 0x0c8c)
      || (c >= 0x0c8e && c <= 0x0c90)
      || (c >= 0x0c92 && c <= 0x0c