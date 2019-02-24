/* Create and destroy argument vectors (argv's)
   Copyright (C) 1992, 2001 Free Software Foundation, Inc.
   Written by Fred Fish @ Cygnus Support

This file is part of the libiberty library.
Libiberty is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public
License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

Libiberty is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public
License along with libiberty; see the file COPYING.LIB.  If
not, write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */


/*  Create and destroy argument vectors.  An argument vector is simply an
    array of string pointers, terminated by a NULL pointer. */

/* !kawai! */
#include "../include/ansidecl.h"
#include "../include/libiberty.h"
/* end of !kawai! */

#define ISBLANK(ch) ((ch) == ' ' || (ch) == '¥t')

/*  Routines imported from standard C runtime libraries. */

#ifdef __STDC__

#include "../include/stddef.h"
#include "../include/string.h"
#include "../include/stdlib.h"

#else	/* !__STDC__ */

#if !defined _WIN32 || defined __GNUC__
extern char *memcpy ();		/* Copy memory region */
extern int strlen ();		/* Count length of string */
extern char *malloc ();		/* Standard memory allocater */
extern char *realloc ();	/* Standard memory reallocator */
extern void free ();		/* Free malloc'd memory */
extern char *strdup ();		/* Duplicate a string */
#endif

#endif	/* __STDC__ */


#ifndef NULL
#define NULL 0
#endif

#ifndef EOS
#define EOS '¥0'
#endif

#define INITIAL_MAXARGC 8	/* Number of args + NULL in initial argv */


/*

@deftypefn Extension char** dupargv (char **@var{vector})

Duplicate an argument vector.  Simply scans through @var{vector},
duplicating each argument until the terminating @code{NULL} is found.
Returns a pointer to the argument vector if successful.  Returns
@code{NULL} if there is insufficient memory to complete building the
argument vector.

@end deftypefn

*/

char **
dupargv (argv)
     char **argv;
{
  int argc;
  char **copy;
  
  if (argv == NULL)
    return NULL;
  
  /* the vector */
  for (argc = 0; argv[argc] != NULL; argc++);
  copy = (char **) malloc ((argc + 1) * sizeof (char *));
  if (copy == NULL)
    return NULL;
  
  /* the strings */
  for (argc = 0; argv[argc] != NULL; argc++)
    {
      int len = strlen (argv[argc]);
      copy[argc] = malloc (sizeof (char *) * (len + 1));
      if (copy[argc] == NULL)
	{
	  freeargv (copy);
	  return NULL;
	}
      strcpy (copy[argc], argv[argc]);
    }
  copy[argc] = NULL;
  return copy;
}

/*

@deftypefn Extension void freeargv (char **@var{vector})

Free an argument vector that was built using @code{buildargv}.  Simply
scans through @var{vector}, freeing the memory for each argument until
the terminating @code{NULL} is found, and then frees @var{vector}
itself.

@end deftypefn

*/

void freeargv (vector)
char **vector;
{
  register char **scan;

  if (vector != NULL)
    {
      for (scan = vector; *scan != NULL; scan++)
	{
	  free (*scan);
	}
      free (vector);
    }
}

/*

@deftypefn Extension char** buildargv (char *@var{sp})

Given a pointer to a string, parse the string extracting fields
separated by whitespace and optionally enclosed within either single
or double quotes (which are stripped off), and build a vector of
pointers to copies of the string for each field.  The input string
remains unchanged.  The last element of the vector is followed by a
@code{NULL} element.

All of the memory for the pointer array and copies of the string
is obtained from @code{malloc}.  All of the memory can be returne