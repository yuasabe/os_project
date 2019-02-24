/* Load needed message catalogs.
   Copyright (C) 1995-1999, 2000, 2001 Free Software Foundation, Inc.

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

/* Tell glibc's <string.h> to provide a prototype for mempcpy().
   This must come before <config.h> because <config.h> may include
   <features.h>, and once <features.h> has been included, it's too late.  */
#ifndef _GNU_SOURCE
# define _GNU_SOURCE    1
#endif

/* !kawai! */
#ifdef HAVE_CONFIG_H
# include "../gcc/config.h"
#endif
/* end of !kawai! */

#include "../include/ctype.h"
#include "../include/errno.h"

# define alloca __builtin_alloca
# define HAVE_ALLOCA 1

#include "../include/stdlib.h"
#include "../include/string.h"


#if (defined HAVE_MMAP_FILE && !defined DISALLOW_MMAP) ¥
    || (defined _LIBC && defined _POSIX_MAPPED_FILES)
/* !kawai! */
/* # include <sys/mman.h> */
/* end of !kawai! */
# undef HAVE_MMAP
# define HAVE_MMAP	1
#else
# undef HAVE_MMAP
#endif

#include "gettext.h"
#include "gettextP.h"

#ifdef _LIBC
# include "../locale/localeinfo.h"
#endif

/* GCC LOCAL: These macros are used below.  */
/* The extra casts work around common compiler bugs.  */
#define INTTYPE_SIGNED(t) (! ((t) 0 < (t) -1))
/* The outer cast is needed to work around a bug in Cray C 5.0.3.0.
   It is necessary at least when t == time_t.  */
#define INTTYPE_MINIMUM(t) ((t) (INTTYPE_SIGNED (t) ¥
                             ? ‾ (t) 0 << (sizeof(t) * CHAR_BIT - 1) : (t) 0))
#define INTTYPE_MAXIMUM(t) ((t) (‾ (t) 0 - INTTYPE_MINIMUM (t)))

/* @@ end of prolog @@ */

#ifdef _LIBC
/* Rename the non ISO C functions.  This is required by the standard
   because some ISO C functions will require linking with this object
   file and the name space must not be polluted.  */
# define open   __open
# define close  __close
# define read   __read
# define mmap   __mmap
# define munmap __munmap
#endif

/* Names for the libintl functions are a problem.  They must not clash
   with existing names and they should follow ANSI C.  But this source
   code is also used in GNU C Library where the names have a __
   prefix.  So we have to make a difference here.  */
#ifdef _LIBC
# define PLURAL_PARSE __gettextparse
#else
# define PLURAL_PARSE gettextparse__
#endif

/* For those losing systems which don't have `alloca' we have to add
   some additional code emulating it.  */
#ifdef HAVE_ALLOCA
# define freea(p) /* nothing */
#else
# define alloca(n) malloc (n)
# define freea(p) free (p)
#endif

/* For systems that distinguish between text and binary I/O.
   O_BINARY is usually declared in <fcntl.h>. */
#if !defined O_BINARY && defined _O_BINARY
  /* For MSC-compatible compilers.  */
# define O_BINARY _O_BINARY
# define O_TEXT _O_TEXT
#endif
#ifdef __BEOS__
  /* BeOS 5 has O_BINARY and O_TEXT, but they have no effect.  */
# undef O_BINARY
# undef O_TEXT
#endif
/* On reasonable systems, binary I/O is the default.  */
#ifndef O_BINARY
# define O_BINARY 0
#endif

/* We need a sign, whether a new catalog was loaded, which can be associated
   with all translations.  This is important if the translations are
   cached by one of GCC's features.  */
int _nl_msg_cat_cntr;

#if (defined __GNUC__ && !defined __APPLE_CC__) ¥
    || (defined __STDC_VERSION__ && __STDC_VERSION__ >= 199901L)

/* These structs are the constant expr