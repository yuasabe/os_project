/* Handle aliases for locale names.
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

#define LOCALE_ALIAS_PATH	""

/* !kawai! */
#ifdef HAVE_CONFIG_H
# include "../gcc/config.h"
#endif
/* end of !kawai! */

#include "../include/ctype.h"
#include "../include/stdio.h"
/* #include <sys/types.h> */

# define alloca __builtin_alloca
# define HAVE_ALLOCA 1

#include "../include/stdlib.h"

#include "../include/string.h"

#include "gettextP.h"

/* @@ end of prolog @@ */

#ifdef _LIBC
/* Rename the non ANSI C functions.  This is required by the standard
   because some ANSI C functions will require linking with this object
   file and the name space must not be polluted.  */
# define strcasecmp __strcasecmp

# ifndef mempcpy
#  define mempcpy __mempcpy
# endif
# define HAVE_MEMPCPY	1

/* We need locking here since we can be called from different places.  */
# include <bits/libc-lock.h>

__libc_lock_define_initialized (static, lock);
#endif

#ifndef internal_function
# define internal_function
#endif

/* For those losing systems which don't have `alloca' we have to add
   some additional code emulating it.  */
#ifdef HAVE_ALLOCA
# define freea(p) /* nothing */
#else
# define alloca(n) malloc (n)
# define freea(p) free (p)
#endif

#if defined _LIBC_REENTRANT || defined HAVE_FGETS_UNLOCKED
# undef fgets
# define fgets(buf, len, s) fgets_unlocked (buf, len, s)
#endif
#if defined _LIBC_REENTRANT || defined HAVE_FEOF_UNLOCKED
# undef feof
# define feof(s) feof_unlocked (s)
#endif


struct alias_map
{
  const char *alias;
  const char *value;
};


static char *string_space;
static size_t string_space_act;
static size_t string_space_max;
static struct alias_map *map;
static size_t nmap;
static size_t maxmap;


/* Prototypes for local functions.  */
static size_t read_alias_file PARAMS ((const char *fname, int fname_len))
     internal_function;
static int extend_alias_table PARAMS ((void));
static int alias_compare PARAMS ((const struct alias_map *map1,
				  const struct alias_map *map2));


const char *
_nl_expand_alias (name)
    const char *name;
{
  static const char *locale_alias_path = LOCALE_ALIAS_PATH;
  struct alias_map *retval;
  const char *result = NULL;
  size_t added;

#ifdef _LIBC
  __libc_lock_lock (lock);
#endif

  do
    {
      struct alias_map item;

      item.alias = name;

      if (nmap > 0)
	retval = (struct alias_map *) bsearch (&item, map, nmap,
					       sizeof (struct alias_map),
					       (int (*) PARAMS ((const void *,
								 const void *))
						) alias_compare);
      else
	retval = NULL;

      /* We really found an alias.  Return the value.  */
      if (retval != NULL)
	{
	  result = retval->value;
	  break;
	}

      /* Perhaps we can find another alias file.  */
      added = 0;
      while (added == 0 && locale_alias_path[0] != '¥0')
	{
	  const char *start;

	  while (locale_alias_path[0] == PATH_SEPARATOR)
	    ++locale_alias_path;
	  start = loca