/* Function declarations for libiberty.

   Copyright 2001, 2002 Free Software Foundation, Inc.
   
   Note - certain prototypes declared in this header file are for
   functions whoes implementation copyright does not belong to the
   FSF.  Those prototypes are present in this file for reference
   purposes only and their presence in this file should not construed
   as an indication of ownership by the FSF of the implementation of
   those functions in any way or form whatsoever.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
   
   Written by Cygnus Support, 1994.

   The libiberty library provides a number of functions which are
   missing on some operating systems.  We do not declare those here,
   to avoid conflicts with the system header files on operating
   systems that do support those functions.  In this file we only
   declare those functions which are specific to libiberty.  */

#ifndef LIBIBERTY_H
#define LIBIBERTY_H

#ifdef __cplusplus
extern "C" {
#endif

#include "ansidecl.h"

#ifdef ANSI_PROTOTYPES
/* Get a definition for size_t.  */
#include "../include/stddef.h"
/* Get a definition for va_list.  */
#include "../include/stdarg.h"
#endif

/* Build an argument vector from a string.  Allocates memory using
   malloc.  Use freeargv to free the vector.  */

extern char **buildargv PARAMS ((const char *)) ATTRIBUTE_MALLOC;

/* Free a vector returned by buildargv.  */

extern void freeargv PARAMS ((char **));

/* Duplicate an argument vector. Allocates memory using malloc.  Use
   freeargv to free the vector.  */

extern char **dupargv PARAMS ((char **)) ATTRIBUTE_MALLOC;


/* Return the last component of a path name.  Note that we can't use a
   prototype here because the parameter is declared inconsistently
   across different systems, sometimes as "char *" and sometimes as
   "const char *" */

/* HAVE_DECL_* is a three-state macro: undefined, 0 or 1.  If it is
   undefined, we haven't run the autoconf check so provide the
   declaration without arguments.  If it is 0, we checked and failed
   to find the declaration so provide a fully prototyped one.  If it
   is 1, we found it so don't provide any declaration at all.  */
#if defined (__GNU_LIBRARY__ ) || defined (__linux__) || defined (__FreeBSD__) || defined (__OpenBSD__) || defined (__CYGWIN__) || defined (__CYGWIN32__) || (defined (HAVE_DECL_BASENAME) && !HAVE_DECL_BASENAME)
extern char *basename PARAMS ((const char *));
#else
# if !defined (HAVE_DECL_BASENAME)
extern char *basename ();
# endif
#endif

/* A well-defined basename () that is always compiled in.  */

extern const char *lbasename PARAMS ((const char *));

/* Concatenate an arbitrary number of strings.  You must pass NULL as
   the last argument of this function, to terminate the list of
   strings.  Allocates memory using xmalloc.  */

extern char *concat PARAMS ((const char *, ...)) ATTRIBUTE_MALLOC;

/* Concatenate an arbitrary number of strings.  You must pass NULL as
   the last argument of this function, to terminate the list of
   strings.  Allocates memory using xmalloc.  The first argument is
   not one of the strings to be concatenated, but if not NULL is a
   pointer to be freed after the new string is created, similar to the
   way xrealloc works.  */

extern char *reconcat PARAMS ((char *, const char *, ...)) ATTRIBUTE_MALLOC;

/* Determ