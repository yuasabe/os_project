/* Extended regular expression matching and search library,
   version 0.12.
   (Implements POSIX draft P1003.2/D11.2, except for some of the
   internationalization features.)
   Copyright (C) 1993-1999, 2000, 2001 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

/* This file has been modified for usage in libiberty.  It includes "xregex.h"
   instead of <regex.h>.  The "xregex.h" header file renames all external
   routines with an "x" prefix so they do not collide with the native regex
   routines or with other components regex routines. */
/* AIX requires this to be the first thing in the file. */
#if defined _AIX && !defined REGEX_MALLOC
  #pragma alloca
#endif

#undef	_GNU_SOURCE
#define _GNU_SOURCE

/* !kawai! */
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
/* end of !kawai! */

#ifndef PARAMS
# if defined __GNUC__ || (defined __STDC__ && __STDC__)
#  define PARAMS(args) args
# else
#  define PARAMS(args) ()
# endif  /* GCC.  */
#endif  /* Not PARAMS.  */

#ifndef INSIDE_RECURSION

# if defined STDC_HEADERS && !defined emacs
#  include "../include/stddef.h"
# else
/* We need this for `regex.h', and perhaps for the Emacs include files.  */
# endif

# define WIDE_CHAR_SUPPORT (HAVE_WCTYPE_H && HAVE_WCHAR_H && HAVE_BTOWC)

/* For platform which support the ISO C amendement 1 functionality we
   support user defined character classes.  */
# if defined _LIBC || WIDE_CHAR_SUPPORT
/* Solaris 2.5 has a bug: <wchar.h> must be included before <wctype.h>.  */
#  include <wchar.h>
#  include <wctype.h>
# endif

# ifdef _LIBC
/* We have to keep the namespace clean.  */
#  define regfree(preg) __regfree (preg)
#  define regexec(pr, st, nm, pm, ef) __regexec (pr, st, nm, pm, ef)
#  define regcomp(preg, pattern, cflags) __regcomp (preg, pattern, cflags)
#  define regerror(errcode, preg, errbuf, errbuf_size) ¥
	__regerror(errcode, preg, errbuf, errbuf_size)
#  define re_set_registers(bu, re, nu, st, en) ¥
	__re_set_registers (bu, re, nu, st, en)
#  define re_match_2(bufp, string1, size1, string2, size2, pos, regs, stop) ¥
	__re_match_2 (bufp, string1, size1, string2, size2, pos, regs, stop)
#  define re_match(bufp, string, size, pos, regs) ¥
	__re_match (bufp, string, size, pos, regs)
#  define re_search(bufp, string, size, startpos, range, regs) ¥
	__re_search (bufp, string, size, startpos, range, regs)
#  define re_compile_pattern(pattern, length, bufp) ¥
	__re_compile_pattern (pattern, length, bufp)
#  define re_set_syntax(syntax) __re_set_syntax (syntax)
#  define re_search_2(bufp, st1, s1, st2, s2, startpos, range, regs, stop) ¥
	__re_search_2 (bufp, st1, s1, st2, s2, startpos, range, regs, stop)
#  define re_compile_fastmap(bufp) __re_compile_fastmap (bufp)

#  define btowc __btowc

/* We are also using some library internals.  */
#  include <locale/localeinfo.h>
#  include <locale/elem-hash.h>
#  include <langinfo.h>
#  include <locale/coll-lookup.h>
# endif

/* This is for other GNU distributions with internationalized messages.  */
# if (HAVE_LIBINTL_H && ENABLE_NLS) || defined _LIBC
#  include <libintl.h>
#  ifdef _LIBC
#   undef gettext
#   define gettext(msgid) __dcgettext ("libc", msgid, LC_MESSAGES)
#  endif
# else
#  define gettext(msgid) (msgid)
# endif

# ifndef gettext_noop
/* This define is so xgettext can find the internationalizable
   strings.  */
#  define gettext_noop(String) String
# endif

/* The `emacs' switch turns on certain matching commands
   that make sense only in Emacs. */
# ifdef emacs

#  include "lisp.h"
#  include "buffer.h"
#  include "syntax.h"

# else  /* not emacs */

/* If we are not linking with Emacs proper,
   we can't use the relocating allocator
   even if config.h says that we can.  */
#  undef REL_ALLOC

#  if defined STDC_HEADERS || defined _LIBC
#   include "../include/stdlib.h"
#  endif

/* When used in Emacs's lib-src, we need to get bzero and bcopy somehow.
   If nothing else has been done, use the method below.  */
#  ifdef INHIBIT_STRING_HEADER
#   if !(defined HAVE_BZERO && defined HAVE_BCOPY)
#    if !defined bzero && !defined bcopy
#     undef INHIBIT_STRING_HEADER
#    endif
#   endif
#  endif

/* This is the normal way of making sure we have a bcopy and a bzero.
   This is used in most programs--a few other programs avoid this
   by defining INHIBIT_STRING_HEADER.  */
#  ifndef INHIBIT_STRING_HEADER
#   if defined HAVE_STRING_H || defined STDC_HEADERS || defined _LIBC
#    include "../include/string.h"
#    ifndef bzero
#     ifndef _LIBC
#      define bzero(s, n)	(memset (s, '¥0', n), (s))
#     else
#      define bzero(s, n)	__bzero (s, n)
#     endif
#    endif
#   else
#    ifndef memcmp
#     define memcmp(s1, s2, n)	bcmp (s1, s2, n)
#    endif
#    ifndef memcpy
#     define memcpy(d, s, n)	(bcopy (s, d, n), (d))
#    endif
#   endif
#  endif

/* Define the syntax stuff for ¥<, ¥>, etc.  */

/* This must be nonzero for the wordchar and notwordchar pattern
   commands in re_match_2.  */
#  ifndef Sword
#   define Sword 1
#  endif

#  ifdef SWITCH_ENUM_BUG
#   define SWITCH_ENUM_CAST(x) ((int)(x))
#  else
#   define SWITCH_ENUM_CAST(x) (x)
#  endif

# endif /* not emacs */

# if defined _LIBC || HAVE_LIMITS_H
#  include "../include/limits.h"
# endif

# ifndef MB_LEN_MAX
#  define MB_LEN_MAX 1
# endif

/* !kawai! */
/* Get the interface, including the syntax bits.  */
# include "../include/xregex.h"  /* change for libiberty */
/* end of !kawai! */

/* isalpha etc. are used for the character classes.  */
# include "../include/ctype.h"

/* Jim Meyering writes:

   "... Some ctype macros are valid only for character codes that
   isascii says are ASCII (SGI's IRIX-4.0.5 is one such system --when
   using /bin/cc or gcc but without giving an ansi option).  So, all
   ctype uses should be through macros like ISPRINT...  If
   STDC_HEADERS is defined, then autoconf has verified that the ctype
   macros don't need to be guarded with references to isascii. ...
   Defining isascii to 1 should let any compiler worth its salt
   eliminate the && through constant folding."
   Solaris defines some of these symbols so we must undefine them first.  */

# undef ISASCII
# if defined STDC_HEADERS || (!defined isascii && !defined HAVE_ISASCII)
#  define ISASCII(c) 1
# else
#  define ISASCII(c) isascii(c)
# endif

# ifdef isblank
#  define ISBLANK(c) (ISASCII (c) && isblank (c))
# else
#  define ISBLANK(c) ((c) == ' ' || (c) == '¥t')
# endif
# ifdef isgraph
#  define ISGRAPH(c) (ISASCII (c) && isgraph (c))
# else
#  define ISGRAPH(c) (ISASCII (c) && isprint (c) && !isspace (c))
# endif

# undef ISPRINT
# define ISPRINT(c) (ISASCII (c) && isprint (c))
# define ISDIGIT(c) (ISASCII (c) && isdigit (c))
# define ISALNUM(c) (ISASCII (c) && isalnum (c))
# define ISALPHA(c) (ISASCII (c) && isalpha (c))
# define ISCNTRL(c) (ISASCII (c) && iscntrl (c))
# define ISLOWER(c) (ISASCII (c) && islower (c))
# define ISPUNCT(c) (ISASCII (c) && ispunct (c))
# define ISSPACE(c) (ISASCII (c) && isspace (c))
# define ISUPPER(c) (ISASCII (c) && isupper (c))
# define ISXDIGIT(c) (ISASCII (c) && isxdigit (c))

# ifdef _tolower
#  define TOLOWER(c) _tolower(c)
# else
#  define TOLOWER(c) tolower(c)
# endif

# ifndef NULL
#  define NULL (void *)0
# endif

/* We remove any previous definition of `SIGN_EXTEND_CHAR',
   since ours (we hope) works properly with all combinations of
   machines, compilers, `char' and `unsigned char' argument types.
   (Per Bothner suggested the basic approach.)  */
# undef SIGN_EXTEND_CHAR
# if __STDC__
#  define SIGN_EXTEND_CHAR(c) ((signed char) (c))
# else  /* not __STDC__ */
/* As in Harbison and Steele.  */
#  define SIGN_EXTEND_CHAR(c) ((((unsigned char) (c)) ^ 128) - 128)
# endif

# ifndef emacs
/* How many characters in the character set.  */
#  define CHAR_SET_SIZE 256

#  ifdef SYNTAX_TABLE

extern char *re_syntax_table;

#  else /* not SYNTAX_TABLE */

static char re_syntax_table[CHAR_SET_SIZE];

static void init_syntax_once PARAMS ((void));

static void
init_syntax_once ()
{
   register int c;
   static int done = 0;

   if (done)
     return;
   bzero (re_syntax_table, sizeof re_syntax_table);

   for (c = 0; c < CHAR_SET_SIZE; ++c)
     if (ISALNUM (c))
	re_syntax_table[c] = Sword;

   re_syntax_table['_'] = Sword;

   done = 1;
}

#  endif /* not SYNTAX_TABLE */

#  define SYNTAX(c) re_syntax_table[(unsigned char) (c)]

# endif /* emacs */

/* Integer type for pointers.  */
# if !defined _LIBC && !defined HAVE_UINTPTR_T
typedef unsigned long int uintptr_t;
# endif

/* Should we use malloc or alloca?  If REGEX_MALLOC is not defined, we
   use `alloca' instead of `malloc'.  This is because using malloc in
   re_search* or re_match* could cause memory leaks when C-g is used in
   Emacs; also, malloc is slower and causes storage fragmentation.  On
   the other hand, malloc is more portable, and easier to debug.

   Because we sometimes use alloca, some routines have to be macros,
   not functions -- `alloca'-allocated space disappears at the end of the
   function it is called in.  */

# ifdef REGEX_MALLOC

#  define REGEX_ALLOCATE malloc
#  define REGEX_REALLOCATE(source, osize, nsize) realloc (source, nsize)
#  define REGEX_FREE free

# else /* not REGEX_MALLOC  */

/* Emacs already defines alloca, sometimes.  */
#  ifndef alloca

/* Make alloca work the best possible way.  */
#    define alloca __builtin_alloca

#  endif /* not alloca */

#  define REGEX_ALLOCATE alloca

/* Assumes a `char *destination' variable.  */
#  define REGEX_REALLOCATE(source, osize, nsize)			¥
  (destination = (char *) alloca (nsize),				¥
   memcpy (destination, source, osize))

/* No need to do anything to free, after alloca.  */
#  define REGEX_FREE(arg) ((void)0) /* Do nothing!  But inhibit gcc warning.  */

# endif /* not REGEX_MALLOC */

/* Define how to allocate the failure stack.  */

# if defined REL_ALLOC && defined REGEX_MALLOC

#  define REGEX_ALLOCATE_STACK(size)				¥
  r_alloc (&failure_stack_ptr, (size))
#  define REGEX_REALLOCATE_STACK(source, osize, nsize)		¥
  r_re_alloc (&failure_stack_ptr, (nsize))
#  define REGEX_FREE_STACK(ptr)					¥
  r_alloc_free (&failure_stack_ptr)

# else /* not using relocating allocator */

#  ifdef REGEX_MALLOC

#   define REGEX_ALLOCATE_STACK malloc
#   define REGEX_REALLOCATE_STACK(source, osize, nsize) realloc (source, nsize)
#   define REGEX_FREE_STACK free

#  else /* not REGEX_MALLOC */

#   define REGEX_ALLOCATE_STACK alloca

#   define REGEX_REALLOCATE_STACK(source, osize, nsize)			¥
   REGEX_REALLOCATE (source, osize, nsize)
/* No need to explicitly free anything.  */
#   define REGEX_FREE_STACK(arg)

#  endif /* not REGEX_MALLOC */
# endif /* not using relocating allocator */


/* True if `size1' is non-NULL and PTR is pointing anywhere inside
   `string1' or just past its end.  This works if PTR is NULL, which is
   a good thing.  */
# define FIRST_STRING_P(ptr) 					¥
  (size1 && string1 <= (ptr) && (ptr) <= string1 + size1)

/* (Re)Allocate N items of type T using malloc, or fail.  */
# define TALLOC(n, t) ((t *) malloc ((n) * sizeof (t)))
# define RETALLOC(addr, n, t) ((addr) = (t *) realloc (addr, (n) * sizeof (t)))
# d