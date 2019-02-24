/* Functions to support general ended bitmaps.
   Copyright (C) 1997, 1998, 1999, 2000, 2001 Free Software Foundation, Inc.

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 2, or (at your option) any later
version.

GCC is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with GCC; see the file COPYING.  If not, write to the Free
Software Foundation, 59 Temple Place - Suite 330, Boston, MA
02111-1307, USA.  */

#define GENERATOR_FILE	1

/* !kawai! */
#include "config.h"
#include "system.h"
#include "rtl.h"
#include "flags.h"
#include "../include/obstack.h"
#include "bitmap.h"
/* end of !kawai! */

/* Obstack to allocate bitmap elements from.  */
static struct obstack bitmap_obstack;
static int bitmap_obstack_init = FALSE;

#ifndef INLINE
#ifndef __GNUC__
#define INLINE
#else
#define INLINE __inline__
#endif
#endif

/* Global data */
bitmap_element bitmap_zero_bits;	/* An element of all zero bits.  */
static bitmap_element *bitmap_free;	/* Freelist of bitmap elements.  */

static void bitmap_element_free		PARAMS ((bitmap, bitmap_element *));
static bitmap_element *bitmap_element_allocate PARAMS ((void));
static int bitmap_element_zerop		PARAMS ((bitmap_element *));
static void bitmap_element_link		PARAMS ((bitmap, bitmap_element *));
static bitmap_element *bitmap_find_bit	PARAMS ((bitmap, unsigned int));

/* Free a bitmap element.  Since these are allocated off the
   bitmap_obstack, "free" actually means "put onto the freelist".  */

static INLINE void
bitmap_element_free (head, elt)
     bitmap head;
     bitmap_element *elt;
{
  bitmap_element *next = elt->next;
  bitmap_element *prev = elt->prev;

  if (prev)
    prev->next = next;

  if (next)
    next->prev = prev;

  if (head->first == elt)
    head->first = next;

  /* Since the first thing we try is to insert before current,
     make current the next entry in preference to the previous.  */
  if (head->current == elt)
    {
      head->current = next != 0 ? next : prev;
      if (head->current)
	head->indx = head->current->indx;
    }

  elt->next = bitmap_free;
  bitmap_free = elt;
}

/* Allocate a bitmap element.  The bits are cleared, but nothing else is.  */

static INLINE bitmap_element *
bitmap_element_allocate ()
{
  bitmap_element *element;

  if (bitmap_free != 0)
    {
      element = bitmap_free;
      bitmap_free = element->next;
    }
  else
    {
      /* We can't use gcc_obstack_init to initialize the obstack since
	 print-rtl.c now calls bitmap functions, and bitmap is linked
	 into the gen* functions.  */
      if (!bitmap_obstack_init)
	{
	  bitmap_obstack_init = TRUE;

	  /* Let particular systems override the size of a chunk.  */
#ifndef OBSTACK_CHUNK_SIZE
#define OBSTACK_CHUNK_SIZE 0
#endif
	  /* Let them override the alloc and free routines too.  */
#ifndef OBSTACK_CHUNK_ALLOC
#define OBSTACK_CHUNK_ALLOC xmalloc
#endif
#ifndef OBSTACK_CHUNK_FREE
#define OBSTACK_CHUNK_FREE free
#endif

#if !defined(__GNUC__) || (__GNUC__ < 2)
#define __alignof__(type) 0
#endif

	  obstack_specify_allocation (&bitmap_obstack, OBSTACK_CHUNK_SIZE,
				      __alignof__ (bitmap_element),
				      (void *(*) PARAMS ((long))) OBSTACK_CHUNK_ALLOC,
				      (void (*) PARAMS ((void *))) OBSTACK_CHUNK_FREE);
	}

      element = (bitmap_element *) obstack_alloc (&bitmap_obstack,
						  sizeof (bitmap_element));
    }

  memset (element->bits, 0, sizeof (element->bits));

  return element;
}

/* Release any memory allocated by bitmaps.  */

void
bitmap_release_memory ()
{
  bitmap_free = 0;
  if (bitmap_obstack_init)
    {
      bitmap_o