/* CPP Library.
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
#include "cpplib.h"
#include "cpphash.h"
#include "prefix.h"
#include "intl.h"
#include "version.h"
#include "mkdeps.h"
#include "cppdefault.h"
#include "except.h"	/* for USING_SJLJ_EXCEPTIONS */

/* Predefined symbols, built-in macros, and the default include path.  */

#ifndef GET_ENV_PATH_LIST
#define GET_ENV_PATH_LIST(VAR,NAME)	do { (VAR) = getenv (NAME); } while (0)
#endif

/* Windows does not natively support inodes, and neither does MSDOS.
   Cygwin's emulation can generate non-unique inodes, so don't use it.
   VMS has non-numeric inodes.  */
#ifdef VMS
# define INO_T_EQ(A, B) (!memcmp (&(A), &(B), sizeof (A)))
# define INO_T_COPY(DEST, SRC) memcpy(&(DEST), &(SRC), sizeof (SRC))
#else
# if (defined _WIN32 && ! defined (_UWIN)) || defined __MSDOS__
#  define INO_T_EQ(A, B) 0
# else
#  define INO_T_EQ(A, B) ((A) == (B))
# endif
# define INO_T_COPY(DEST, SRC) (DEST) = (SRC)
#endif

/* Internal structures and prototypes.  */

/* A `struct pending_option' remembers one -D, -A, -U, -include, or
   -imacros switch.  */
typedef void (* cl_directive_handler) PARAMS ((cpp_reader *, const char *));
struct pending_option
{
  struct pending_option *next;
  const char *arg;
  cl_directive_handler handler;
};

/* The `pending' structure accumulates all the options that are not
   actually processed until we hit cpp_read_main_file.  It consists of
   several lists, one for each type of option.  We keep both head and
   tail pointers for quick insertion.  */
struct cpp_pending
{
  struct pending_option *directive_head, *directive_tail;

  struct search_path *quote_head, *quote_tail;
  struct search_path *brack_head, *brack_tail;
  struct search_path *systm_head, *systm_tail;
  struct search_path *after_head, *after_tail;

  struct pending_option *imacros_head, *imacros_tail;
  struct pending_option *include_head, *include_tail;
};

#ifdef __STDC__
#define APPEND(pend, list, elt) ¥
  do {  if (!(pend)->list##_head) (pend)->list##_head = (elt); ¥
	else (pend)->list##_tail->next = (elt); ¥
	(pend)->list##_tail = (elt); ¥
  } while (0)
#else
#define APPEND(pend, list, elt) ¥
  do {  if (!(pend)->list/**/_head) (pend)->list/**/_head = (elt); ¥
	else (pend)->list/**/_tail->next = (elt); ¥
	(pend)->list/**/_tail = (elt); ¥
  } while (0)
#endif

static void print_help                  PARAMS ((void));
static void path_include		PARAMS ((cpp_reader *,
						 char *, int));
static void init_library		PARAMS ((void));
static void init_builtins		PARAMS ((cpp_reader *));
static void mark_named_operators	PARAMS ((cpp_reader *));
static void append_include_chain	PARAMS ((cpp_reader *,
						 char *, int, int));
static struct search_path * remove_dup_dir	PARAMS ((cpp_reader *,
						 struct search_path *));
static struct search_path * remove_dup_dirs PARAMS ((cpp_reader *,
						 struct search_path *));
static void merge_include_chains	PARAMS ((cpp_reader *));
static bool push_include		PARAMS ((cpp_reader *,
						 struct pending_option *));
static void free_chain			PARAMS ((struct pending_option *));
static void set_lang			PARAMS ((cpp_reader *, enum c_lang));
static void init_dependency_output	PARAMS ((cpp_reader *));
static void init_standard_includes	PARAMS ((cpp_reader *));
static void read_original_filename	PARAMS ((cpp_reader *));
static void new_pending_directive	PARAMS ((struct cpp_pending *,
						 const char *,
						 cl_directive_handler));
static void output_deps			PARAMS ((cpp_reader *));
static int parse_option			PARAMS ((const char *));

/* Fourth argument to append_include_chain: chain to use.
   Note it's never asked to append to the quote chain.  */
enum { BRACKET = 0, SYSTEM, AFTER };

/* If we have designated initializers (GCC >2.7) these tables can be
   initialized, constant data.  Otherwise, they have to be filled in at
   runtime.  */
#if HAVE_DESIGNATED_INITIALIZERS

#define init_trigraph_map()  /* Nothing.  */
#define TRIGRAPH_MAP ¥
__extension__ const U_CHAR _cpp_trigraph_map[UCHAR_MAX + 1] = {

#define END };
#define s(p, v) [p] = v,

#else

#define TRIGRAPH_MAP U_CHAR _cpp_trigraph_map[UCHAR_MAX + 1] = { 0 }; ¥
 static void init_trigraph_map PARAMS ((void)) { ¥
 unsigned char *x = _cpp_trigraph_map;

#define END }
#define s(p, v) x[p] = v;

#endif

TRIGRAPH_MAP
  s('=', '#')	s(')', ']')	s('!', '|')
  s('(', '[')	s('¥'', '^')	s('>', '}')
  s('/', '¥¥')	s('<', '{')	s('-', '‾')
END

#undef s
#undef END
#undef TRIGRAPH_MAP

/* Given a colon-separated list of file names PATH,
   add all the names to the search path for include files.  */
static void
path_include (pfile, list, path)
     cpp_reader *pfile;
     char *list;
     int path;
{
  char *p, *q, *name;

  p = list;

  do
    {
      /* Find the end of this name.  */
      q = p;
      while (*q != 0 && *q != PATH_SEPARATOR) q++;
      if (q == p)
	{
	  /* An empty name in the path stands for the current directory.  */
	  name = (char *) xmalloc (2);
	  name[0] = '.';
	  name[1] = 0;
	}
      else
	{
	  /* Otherwise use the directory that is named.  */
	  name = (char *) xmalloc (q - p + 1);
	  memcpy (name, p, q - p);
	  name[q - p] = 0;
	}

      append_include_chain (pfile, name, path, 0);

      /* Advance past this name.  */
      if (*q == 0)
	break;
      p = q + 1;
    }
  while (1);
}

/* Append DIR to include path PATH.  DIR must be allocated on the
   heap; this routine takes responsibility for freeing it.  CXX_AWARE
   is non-zero if the header contains extern "C" guards for C++,
   otherwise it is zero.  */
static void
append_include_chain (pfile, dir, path, cxx_aware)
     cpp_reader *pfile;
     char *dir;
     int path;
     int cxx_aware ATTRIBUTE_UNUSED;
{
  struct cpp_pending *pend = CPP_OPTION (pfile, pending);
  struct search_path *new;
/*  struct stat st; */
  unsigned int len;

  if (*dir == '¥0')
    {
      free (dir);
      dir = xstrdup (".");
    }
  _cpp_simplify_pathname (dir);

#if 0
  if (stat (dir, &st))
    {
      /* Dirs that don't exist are silently ignored.  */
      if (errno != ENOENT)
	cpp_notice_from_errno (pfile, dir);
      else if (CPP_OPTION (pfile, verbose))
	fprintf (stderr, _("ignoring nonexistent directory ¥"%s¥"¥n"), dir);
      free (dir);
      return;
    }
#endif

#if 0
  if (!S_ISDIR (st.st_mode))
    {
      cpp_notice (pfile, "%s: Not a directory", dir);
      free (dir);
      return;
    }
#endif

  len = strlen (dir);
  if (len > pfile->max_include_len)
    pfile->max_include_len = len;

  new = (struct search_path *) xmalloc (sizeof (struct search_path));
  new->name = dir;
  new->len = len;
#if 0
  INO_T_COPY (new->ino, st.st_ino);
  new->dev  = st.st_dev;
#endif
  /* Both systm and after include file lists should be treated as system
     include files since these two lists are really just a concatenation
     of one "system" list.  */
  if (path == SYSTEM || path == AFTER)
#ifdef NO_IMPLICIT_EXTERN_C
    new->sysp = 1;
#else
    new->sysp = cxx_aware ? 1 : 2;
#endif
  else
    new->sysp = 0;
  new->name_map = NULL;
  new->next = NULL;

  switch (path)
    {
    case BRACKET:	APPEND (pend, brack, new); break;
    case SYSTEM:	APPEND (pend, systm, new); break;
    case AFTER:		APPEND (pend, after, new); break;
    }
}

/* Handle a duplicated include path.  PREV is the link in the chain
   before the duplicate.  The duplicate is removed from the chain and
   freed.  Returns PREV.  */
static struct search_path *
remove_dup_dir (pfile, prev)
     cpp_reader *pfile;
     struct search_path *prev;
{
  struct search_path *cur = prev->next;

  if (CPP_OPTION (pfile, verbose))
    fprintf (stderr, _("ignoring duplicate directory ¥"%s¥"¥n"), cur->name);

  prev->next = cur->next;
  free ((PTR) cur->name);
  free (cur);

  return prev;
}

/* Remove duplicate directories from a chain.  Returns the tail of the
   chain, or NULL if the chain is empty.  This algorithm is quadratic
   in the number of -I switches, which is acceptable since there
   aren't usually that many of them.  */
static struct search_path *
remove_dup_dirs (pfile, head)
     cpp_reader *pfile;
     struct search_path *head;
{
  struct search_path *prev = NULL, *cur, *other;

  for (cur = head; cur; cur = cur->next)
    {
      for (other = head; other != cur; other = other->next)
        if (INO_T_EQ (cur->ino, other->ino) && cur->dev == other->dev)
	  {
	    if (cur->sysp && !other->sysp)
	      {
		cpp_warning (pfile,
			     "changing search order for system directory ¥"%s¥"",
			     cur->name);
		if (strcmp (cur->name, other->name))
		  cpp_warning (pfile, 
			       "  as it is the same as non-system directory ¥"%s¥"",
			       other->name);
		else
		  cpp_warning (pfile, 
			       "  as it has already been specified as a non-system directory");
	      }
	    cur = remove_dup_dir (pfile, prev);
	    break;
	  }
      prev = cur;
    }

  return prev;
}

/* Merge the four include chains together in the order quote, bracket,
   system, after.  Remove duplicate dirs (as determined by
   INO_T_EQ()).  The system_include and after_include chains are never
   referred to again after this function; all access is through the
   bracket_include path.  */
static void
merge_include_chains (pfile)
     cpp_reader *pfile;
{
  struct search_path *quote, *brack, *systm, *qtail;

  struct cpp_pending *pend = CPP_OPTION (pfile, pending);

  quote = pend->quote_head;
  brack = pend->brack_head;
  systm = pend->systm_head;
  qtail = pend->quote_tail;

  /* Paste together bracket, system, and after include chains.  */
  if (systm)
    pend->systm_tail->next = pend->after_head;
  else
    systm = pend->after_head;

  if (brack)
    pend->brack_tail->next = systm;
  else
    brack = systm;

  /* This is a bit tricky.  First we drop dupes from the quote-include
     list.  Then we drop dupes from the bracket-include list.
     Finally, if qtail and brack are the same directory, we cut out
     brack and move brack up to point to qtail.

     We can't just merge the lists and then uniquify them because
     then we may lose directories from the <> search path that should
     be there; consider -Ifoo -Ibar -I- -Ifoo -Iquux. It is however
     safe to treat -Ibar -Ifoo -I- -Ifoo -Iquux as if written
     -Ibar -I- -Ifoo -Iquux.  */

  remove_dup_dirs (pfile, brack);
  qtail = remove_dup_dirs (pfile, quote);

  if (quote)
    {
      qtail->next = brack;

      /* If brack == qtail, remove brack as it's simpler.  */
      if (brack && INO_T_EQ (qtail->ino, brack->ino)
	  && qtail->dev == brack->dev)
	brack = remove_dup_dir (pfile, qtail);
    }
  else
    quote = brack;

  CPP_OPTION (pfile, quote_include) = quote;
  CPP_OPTION (pfile, bracket_include) = brack;
}

/* A set of booleans indicating what CPP features each source language
   requires.  */
struct lang_flags
{
  char c99;
  char objc;
  char cplusplus;
  char extended_numbers;
  char trigraphs;
  char dollars_in_ident;
  char cplusplus_comments;
  char digraphs;
};

/* ??? Enable $ in identifiers in assembly? */
static const struct lang_flags lang_defaults[] =
{ /*              c99 objc c++ xnum trig dollar c++comm digr  */
  /* GNUC89 */  { 0,  0,   0,  1,   0,   1,     1,      1     },
  /* GNUC99 */  { 1,  0,   0,  1,   0,   1,     1,      1     },
  /* STDC89 */  { 0,  0,   0,  0,   1,   0,     0,      0     },
  /* STDC94 */  { 0,  0,   0,  0,   1,   0,     0,      1     },
  /* STDC99 */  { 1,  0,   0,  1,   1,   0,     1,      1     },
  /* GNUCXX */  { 0,  0,   1,  1,   0,   1,     1,      1     },
  /* CXX98  */  { 0,  0,   1,  1,   1,   0,     1,      1     },
  /* OBJC   */  { 0,  1,   0,  1,   0,   1,     1,      1     },
  /* OBJCXX */  { 0,  1,   1,  1,   0,   1,     1,      1     },
  /* ASM    */  { 0,  0,   0,  1,   0,   0,     1,      0     }
};

/* Sets internal flags correctly for a given language.  */
static void
set_lang (pfile, lang)
     cpp_reader *pfile;
     enum c_lang lang;
{
  const struct lang_flags *l = &lang_defaults[(int) lang];
  
  CPP_OPTION (pfile, lang) = lang;

  CPP_OPTION (pfile, c99)		 = l->c99;
  CPP_OPTION (pfile, objc)		 = l->objc;
  CPP_OPTION (pfile, cplusplus)		 = l->cplusplus;
  CPP_OPTION (pfile, extended_numbers)	 = l->extended_numbers;
  CPP_OPTION (pfile, trigraphs)		 = l->trigraphs;
  CPP_OPTION (pfile, dollars_in_ident)	 = l->dollars_in_ident;
  CPP_OPTION (pfile, cplusplus_comments) = l->cplusplus_comments;
  CPP_OPTION (pfile, digraphs)		 = l->digraphs;
}

#ifdef HOST_EBCDIC
static int opt_comp PARAMS ((const void *, const void *));

/* Run-time sorting of options array.  */
static int
opt_comp (p1, p2)
     const void *p1, *p2;
{
  return strcmp (((struct cl_option *) p1)->opt_text,
		 ((struct cl_option *) p2)->opt_text);
}
#endif

/* init initializes library global state.  It might not need to
   do anything depending on the platform and compiler.  */
static void
init_library ()
{
  static int initialized = 0;

  if (! initialized)
    {
      initialized = 1;

#ifdef HOST_EBCDIC
      /* For non-ASCII hosts, the cl_options array needs to be sorted at
	 runtime.  */
      qsort (cl_options, N_OPTS, sizeof (struct cl_option), opt_comp);
#endif

      /* Set up the trigraph map.  This doesn't need to do anything if
	 we were compiled with a compiler that supports C99 designated
	 initializers.  */
      init_trigraph_map ();
    }
}

/* Initialize a cpp_reader structure.  */
cpp_reader *
cpp_create_reader (lang)
     enum c_lang lang;
{
  cpp_reader *pfile;

  /* Initialise this instance of the library if it hasn't been already.  */
  init_library ();

  pfile = (cpp_reader *) xcalloc (1, sizeof (cpp_reader));

  set_lang (pfile, lang);
  CPP_OPTION (pfile, warn_import) = 1;
  CPP_OPTION (pfile, discard_comments) = 1;
  CPP_OPTION (pfile, show_column) = 1;
  CPP_OPTION (pfile, tabstop) = 8;
  CPP_OPTION (pfile, operator_names) = 1;
#if DEFAULT_SIGNED_CHAR
  CPP_OPTION (pfile, signed_char) = 1;
#else
  CPP_OPTION (pfile, signed_char) = 0;
#endif

  CPP_OPTION (pfile, pending) =
    (struct cpp_pending *) xcalloc (1, sizeof (struct cpp_pending));

  /* It's simplest to just create this struct whether or not it will
     be needed.  */
  pfile->deps = deps_init ();

  /* Initialise the line map.  Start at logical line 1, so we can use
     a line number of zero for special states.  */
  init_line_maps (&pfile->line_maps);
  pfile->line = 1;

  /* Initialize lexer state.  */
  pfile->state.save_comments = ! CPP_OPTION (pfile, discard_comments);

  /* Set up static tokens.  */
  pfile->date.type = CPP_EOF;
  pfile->avoid_paste.type = CPP_PADDING;
  pfile->avoid_paste.val.source = NULL;
  pfile->eof.type = CPP_EOF;
  pfile->eof.flags = 0;

  /* Create a token buffer for the lexer.  */
  _cpp_init_tokenrun (&pfile->base_run, 250);
  pfile->cur_run = &pfile->base_run;
  pfile->cur_token = pfile->base_run.base;

  /* Initialise the base context.  */
  pfile->context = &pfile->base_context;
  pfile->base_co