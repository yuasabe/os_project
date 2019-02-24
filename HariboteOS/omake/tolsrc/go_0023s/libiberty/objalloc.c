/* objalloc.c -- routines to allocate memory for objects
   Copyright 1997 Free Software Foundation, Inc.
   Written by Ian Lance Taylor, Cygnus Solutions.

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
Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */

/* !kawai! */
#include "../include/ansidecl.h"
#include "config.h"

#include "../include/objalloc.h"
/* end of !kawai! */

/* Get a definition for NULL.  */
#include "../include/stdio.h"


#ifdef ANSI_PROTOTYPES
/* Get a definition for size_t.  */
#include "../include/stddef.h"
#endif

#include "../include/stdlib.h"


/* These routines allocate space for an object.  Freeing allocated
   space may or may not free all more recently allocated space.

   We handle large and small allocation requests differently.  If we
   don't have enough space in the current block, and the allocation
   request is for more than 512 bytes, we simply pass it through to
   malloc.  */

/* The objalloc structure is defined in objalloc.h.  */

/* This structure appears at the start of each chunk.  */

struct objalloc_chunk
{
  /* Next chunk.  */
  struct objalloc_chunk *next;
  /* If this chunk contains large objects, this is the value of
     current_ptr when this chunk was allocated.  If this chunk
     contains small objects, this is NULL.  */
  char *current_ptr;
};

/* The aligned size of objalloc_chunk.  */

#define CHUNK_HEADER_SIZE					¥
  ((sizeof (struct objalloc_chunk) + OBJALLOC_ALIGN - 1)	¥
   &‾ (OBJALLOC_ALIGN - 1))

/* We ask for this much memory each time we create a chunk which is to
   hold small objects.  */

#define CHUNK_SIZE (4096 - 32)

/* A request for this amount or more is just passed through to malloc.  */

#define BIG_REQUEST (512)

/* Create an objalloc structure.  */

struct objalloc *
objalloc_create ()
{
  struct objalloc *ret;
  struct objalloc_chunk *chunk;

  ret = (struct objalloc *) malloc (sizeof *ret);
  if (ret == NULL)
    return NULL;

  ret->chunks = (PTR) malloc (CHUNK_SIZE);
  if (ret->chunks == NULL)
    {
      free (ret);
      return NULL;
    }

  chunk = (struct objalloc_chunk *) ret->chunks;
  chunk->next = NULL;
  chunk->current_ptr = NULL;

  ret->current_ptr = (char *) chunk + CHUNK_HEADER_SIZE;
  ret->current_space = CHUNK_SIZE - CHUNK_HEADER_SIZE;

  return ret;
}

/* Allocate space from an objalloc structure.  */

PTR
_objalloc_alloc (o, len)
     struct objalloc *o;
     unsigned long len;
{
  /* We avoid confusion from zero sized objects by always allocating
     at least 1 byte.  */
  if (len == 0)
    len = 1;

  len = (len + OBJALLOC_ALIGN - 1) &‾ (OBJALLOC_ALIGN - 1);

  if (len <= o->current_space)
    {
      o->current_ptr += len;
      o->current_space -= len;
      return (PTR) (o->current_ptr - len);
    }

  if (len >= BIG_REQUEST)
    {
      char *ret;
      struct objalloc_chunk *chunk;

      ret = (char *) malloc (CHUNK_HEADER_SIZE + len);
      if (ret == NULL)
	return NULL;

      chunk = (struct objalloc_chunk *) ret;
      chunk->next = (struct objalloc_chunk *) o->chunks;
      chunk->current_ptr = o->current_ptr;

      o->chunks = (PTR) chunk;

      return (PTR) (ret + CHUNK_HEADER_SIZE);
    }
  else
    {
      struct objalloc_chunk *chunk;

      chunk = (struct objalloc_chunk *) malloc (CHUNK_SIZE);
      if (chunk == NULL)
	return NULL;
      chunk->next = (struct objalloc_chunk *) o->chunks;
      chunk->c