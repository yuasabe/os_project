/* Implementation of the bindtextdomain(3) function
   Copyright (C) 1995-1998, 2000, 2001 Free Software Foundation, Inc.

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

/* !kawai! */
#ifdef HAVE_CONFIG_H
# include "../gcc/config.h"
#endif
/* end of !kawai! */

#include "../include/stddef.h"
#include "../include/stdlib.h"
#include "../include/string.h"

#ifdef _LIBC
# include <libintl.h>
#else
# include "libgnuintl.h"
#endif
#include "gettextP.h"

#ifdef _LIBC
/* We have to handle multi-threaded applications.  */
# include <bits/libc-lock.h>
#else
/* Provide dummy implementation if this is outside glibc.  */
# define __libc_rwlock_define(CLASS, NAME)
# define __libc_rwlock_wrlock(NAME)
# define __libc_rwlock_unlock(NAME)
#endif

/* The internal variables in the standalone libintl.a must have different
   names than the internal variables in GNU libc, otherwise programs
   using libintl.a cannot be linked statically.  */
#if !defined _LIBC
# define _nl_default_dirname _nl_default_dirname__
# define _nl_domain_bindings _nl_domain_bindings__
#endif

/* Some compilers, like SunOS4 cc, don't have offsetof in <stddef.h>.  */
#ifndef offsetof
# define offsetof(type,ident) ((size_t)&(((type*)0)->ident))
#endif

/* @@ end of prolog @@ */

/* Contains the default location of the message catalogs.  */
extern const char _nl_default_dirname[];

/* List with bindings of specific domains.  */
extern struct binding *_nl_domain_bindings;

/* Lock variable to protect the global data in the gettext implementation.  */
__libc_rwlock_define (extern, _nl_state_lock)


/* Names for the libintl functions are a problem.  They must not clash
   with existing names and they should follow ANSI C.  But this source
   code is also used in GNU C Library where the names have a __
   prefix.  So we have to make a difference here.  */
#ifdef _LIBC
# define BINDTEXTDOMAIN __bindtextdomain
# define BIND_TEXTDOMAIN_CODESET __bind_textdomain_codeset
# ifndef strdup
#  define strdup(str) __strdup (str)
# endif
#else
# define BINDTEXTDOMAIN bindtextdomain__
# define BIND_TEXTDOMAIN_CODESET bind_textdomain_codeset__
#endif

/* Prototypes for local functions.  */
static void set_binding_values PARAMS ((const char *domainname,
					const char **dirnamep,
					const char **codesetp));
     
/* Specifies the directory name *DIRNAMEP and the output codeset *CODESETP
   to be used for the DOMAINNAME message catalog.
   If *DIRNAMEP or *CODESETP is NULL, the corresponding attribute is not
   modified, only the current value is returned.
   If DIRNAMEP or CODESETP is NULL, the corresponding attribute is neither
   modified nor returned.  */
static void
set_binding_values (domainname, dirnamep, codesetp)
     const char *domainname;
     const char **dirnamep;
     const char **codesetp;
{
  struct binding *binding;
  int modified;

  /* Some sanity checks.  */
  if (domainname == NULL || domainname[0] == '¥0')
    {
      if (dirnamep)
	*dirnamep = NULL;
      if (codesetp)
	*codesetp = NULL;
      return;
    }

  __libc_rwlock_wrlock (_nl_state_lock);

  modified = 0;

  for (binding = _nl_domain_bindings; binding != NULL; binding = binding->next)
    {
      int compare = strcmp (domainname, binding->domainname);
      if (compare == 0)
	/* We found it!  */
	break;
      if (c