/* An abstract string datatype.
   Copyright (C) 1998, 1999, 2000, 2002 Free Software Foundation, Inc.
   Contributed by Mark Mitchell (mark@markmitchell.com).

This file is part of GNU CC.
   
GNU CC is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

In addition to the permissions in the GNU General Public License, the
Free Software Foundation gives you unlimited permission to link the
compiled version of this file into combinations with other programs,
and to distribute those combinations without any restriction coming
from the use of this file.  (The General Public License restrictions
do apply in other respects; for example, they cover modification of
the file, and distribution when not linked into a combined
executable.)

GNU CC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU CC; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "../include/stdio.h"

#ifdef HAVE_STRING_H
#include "../include/string.h"
#endif

#ifdef HAVE_STDLIB_H
#include "../include/stdlib.h"
#endif

/* !kawai! */
#include "../include/libiberty.h"
#include "../include/dyn-string.h"
/* end of !kawai! */

/* If this file is being compiled for inclusion in the C++ runtime
   library, as part of the demangler implementation, we don't want to
   abort if an allocation fails.  Instead, percolate an error code up
   through the call chain.  */

#if defined(IN_LIBGCC2) || defined(IN_GLIBCPP_V3)
#define RETURN_ON_ALLOCATION_FAILURE
#endif

/* Performs in-place initialization of a dyn_string struct.  This
   function can be used with a dyn_string struct on the stack or
   embedded in another object.  The contents of of the string itself
   are still dynamically allocated.  The string initially is capable
   of holding at least SPACE characeters, including the terminating
   NUL.  If SPACE is 0, it will silently be increated to 1.  

   If RETURN_ON_ALLOCATION_FAILURE is defined and memory allocation
   fails, returns 0.  Otherwise returns 1.  */

int
dyn_string_init (ds_struct_ptr, space)
     struct dyn_string *ds_struct_ptr;
     int space;
{
  /* We need at least one byte in which to store the terminating NUL.  */
  if (space == 0)
    space = 1;

#ifdef RETURN_ON_ALLOCATION_FAILURE
  ds_struct_ptr->s = (char *) malloc (space);
  if (ds_struct_ptr->s == NULL)
    return 0;
#else
  ds_struct_ptr->s = (char *) xmalloc (space);
#endif
  ds_struct_ptr->allocated = space;
  ds_struct_ptr->length = 0;
  ds_struct_ptr->s[0] = '¥0';

  return 1;
}

/* Create a new dynamic string capable of holding at least SPACE
   characters, including the terminating NUL.  If SPACE is 0, it will
   be silently increased to 1.  If RETURN_ON_ALLOCATION_FAILURE is
   defined and memory allocation fails, returns NULL.  Otherwise
   returns the newly allocated string.  */

dyn_string_t 
dyn_string_new (space)
     int space;
{
  dyn_string_t result;
#ifdef RETURN_ON_ALLOCATION_FAILURE
  result = (dyn_string_t) malloc (sizeof (struct dyn_string));
  if (result == NULL)
    return NULL;
  if (!dyn_string_init (result, space))
    {
      free (result);
      return NULL;
    }
#else
  result = (dyn_string_t) xmalloc (sizeof (struct dyn_string));
  dyn_string_init (result, space);
#endif
  return result;
}

/* Free the memory used by DS.  */

void 
dyn_string_delete (ds)
     dyn_string_t ds;
{
  free (ds->s);
  free (ds);
}

/* Returns the contents of DS in a buffer allocated with malloc.  It
   is the caller's 