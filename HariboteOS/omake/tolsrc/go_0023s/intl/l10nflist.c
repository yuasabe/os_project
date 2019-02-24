/* Copyright (C) 1995-1999, 2000, 2001 Free Software Foundation, Inc.
   Contributed by Ulrich Drepper <drepper@gnu.ai.mit.edu>, 1995.

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

/* Tell glibc's <string.h> to provide a prototype for stpcpy().
   This must come before <config.h> because <config.h> may include
   <features.h>, and once <features.h> has been included, it's too late.  */
#ifndef _GNU_SOURCE
# define _GNU_SOURCE	1
#endif

/* !kawai! */
#ifdef HAVE_CONFIG_H
# include "../gcc/config.h"
#endif
/* end of !kawai! */

#include "../include/string.h"

#if defined _LIBC || defined HAVE_ARGZ_H
/* !kawai! */
/* # include <argz.h> */
/* end of !kawai! */
#endif
#include "../include/ctype.h"
#include "../include/stdlib.h"

#include "loadinfo.h"

/* On some strange systems still no definition of NULL is found.  Sigh!  */
#ifndef NULL
# if defined __STDC__ && __STDC__
#  define NULL ((void *) 0)
# else
#  define NULL 0
# endif
#endif

/* @@ end of prolog @@ */

#ifdef _LIBC
/* Rename the non ANSI C functions.  This is required by the standard
   because some ANSI C functions will require linking with this object
   file and the name space must not be polluted.  */
# ifndef stpcpy
#  define stpcpy(dest, src) __stpcpy(dest, src)
# endif
#else
# ifndef HAVE_STPCPY
static char *stpcpy PARAMS ((char *dest, const char *src));
# endif
#endif

/* Define function which are usually not available.  */

#if !defined _LIBC && !defined HAVE___ARGZ_COUNT
/* Returns the number of strings in ARGZ.  */
static size_t argz_count__ PARAMS ((const char *argz, size_t len));

static size_t
argz_count__ (argz, len)
     const char *argz;
     size_t len;
{
  size_t count = 0;
  while (len > 0)
    {
      size_t part_len = strlen (argz);
      argz += part_len + 1;
      len -= part_len + 1;
      count++;
    }
  return count;
}
# undef __argz_count
# define __argz_count(argz, len) argz_count__ (argz, len)
#endif	/* !_LIBC && !HAVE___ARGZ_COUNT */

#if !defined _LIBC && !defined HAVE___ARGZ_STRINGIFY
/* Make '¥0' separated arg vector ARGZ printable by converting all the '¥0's
   except the last into the character SEP.  */
static void argz_stringify__ PARAMS ((char *argz, size_t len, int sep));

static void
argz_stringify__ (argz, len, sep)
     char *argz;
     size_t len;
     int sep;
{
  while (len > 0)
    {
      size_t part_len = strlen (argz);
      argz += part_len;
      len -= part_len + 1;
      if (len > 0)
	*argz++ = sep;
    }
}
# undef __argz_stringify
# define __argz_stringify(argz, len, sep) argz_stringify__ (argz, len, sep)
#endif	/* !_LIBC && !HAVE___ARGZ_STRINGIFY */

#if !defined _LIBC && !defined HAVE___ARGZ_NEXT
static char *argz_next__ PARAMS ((char *argz, size_t argz_len,
				  const char *entry));

static char *
argz_next__ (argz, argz_len, entry)
     char *argz;
     size_t argz_len;
     const char *entry;
{
  if (entry)
    {
      if (entry < argz + argz_len)
        entry = strchr (entry, '¥0') + 1;

      return entry >= argz + argz_len ? NULL : (char *) entry;
    }
  else
    if (argz_len > 0)
      return argz;
    else
      return 0;
}
# undef __argz_next
# define __argz_next(argz, len, entry) argz_next__ (argz, len, entry)
#endif	/* !_LIBC && !HAVE___ARGZ_NEXT */


/* Return number of bits set in 