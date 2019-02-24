/* obstack.c - subroutines used implicitly by object stack macros
   Copyright (C) 1988,89,90,91,92,93,94,96,97 Free Software Foundation, Inc.


   NOTE: This source is derived from an old version taken from the GNU C
   Library (glibc).

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
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
   USA.  */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/* !kawai! */
#include "../include/obstack.h"
/* end of !kawai! */

/* NOTE BEFORE MODIFYING THIS FILE: This version number must be
   incremented whenever callers compiled using an old obstack.h can no
   longer properly call the functions in this obstack.c.  */
#define OBSTACK_INTERFACE_VERSION 1

/* Comment out all this code if we are using the GNU C Library, and are not
   actually compiling the library itself, and the installed library
   supports the same library interface we do.  This code is part of the GNU
   C Library, but also included in many other GNU distributions.  Compiling
   and linking in this code is a waste when using the GNU C library
   (especially if it is a shared library).  Rather than having every GNU
   program understand `configure --with-gnu-libc' and omit the object
   files, it is simpler to just do this in the source for each such file.  */

#include "../include/stdio.h"		/* Random thing to get __GNU_LIBRARY__.  */
#if !defined (_LIBC) && defined (__GNU_LIBRARY__) && __GNU_LIBRARY__ > 1
#include <gnu-versions.h>
#if _GNU_OBSTACK_INTERFACE_VERSION == OBSTACK_INTERFACE_VERSION
#define ELIDE_CODE
#endif
#endif


#ifndef ELIDE_CODE


#if defined (__STDC__) && __STDC__
#define POINTER void *
#else
#define POINTER char *
#endif

/* Determine default alignment.  */
struct fooalign {char x; double d;};
#define DEFAULT_ALIGNMENT  ¥
  ((PTR_INT_TYPE) ((char *) &((struct fooalign *) 0)->d - (char *) 0))
/* If malloc were really smart, it would round addresses to DEFAULT_ALIGNMENT.
   But in fact it might be less smart and round addresses to as much as
   DEFAULT_ROUNDING.  So we prepare for it to do that.  */
union fooround {long x; double d;};
#define DEFAULT_ROUNDING (sizeof (union fooround))

/* When we copy a long block of data, this is the unit to do it with.
   On some machines, copying successive ints does not work;
   in such a case, redefine COPYING_UNIT to `long' (if that works)
   or `char' as a last resort.  */
#ifndef COPYING_UNIT
#define COPYING_UNIT int
#endif


/* The functions allocating more room by calling `obstack_chunk_alloc'
   jump to the handler pointed to by `obstack_alloc_failed_handler'.
   This variable by default points to the internal function
   `print_and_abort'.  */
#if defined (__STDC__) && __STDC__
static void print_and_abort (void);
void (*obstack_alloc_failed_handler) (void) = print_and_abort;
#else
static void print_and_abort ();
void (*obstack_alloc_failed_handler) () = print_and_abort;
#endif

/* Exit value used when `print_and_abort' is used.  */
#if defined __GNU_LIBRARY__ || defined HAVE_STDLIB_H
#include "../include/stdlib.h"
#endif
#ifndef EXIT_FAILURE
#define EXIT_FAILURE 1
#endif
int obstack_exit_failure = EXIT_FAILURE;

/* The non-GNU-C macros copy the obstack into this global variable
   to avoid multiple evaluation.  */

struct obstack *_obstack;

/* Define a macro that either calls functions with the traditional malloc/free
   calling interface, or calls functions with th