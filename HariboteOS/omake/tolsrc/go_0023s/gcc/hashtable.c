/* Hash tables.
   Copyright (C) 2000, 2001 Free Software Foundation, Inc.

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
Foundation, 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

 In other words, you are welcome to use, share and improve this program.
 You are forbidden to forbid anyone else to use, share and improve
 what you give them.   Help stamp out software-hoarding!  */

#include "config.h"
#include "system.h"
#include "hashtable.h"

/* The code below is a specialization of Vladimir Makarov's expandable
   hash tables (see libiberty/hashtab.c).  The abstraction penalty was
   too high to continue using the generic form.  This code knows
   intrinsically how to calculate a hash value, and how to compare an
   existing entry with a potential new one.  Also, the ability to
   delete members from the table has been removed.  */

static unsigned int calc_hash PARAMS ((const unsigned char *, unsigned int));
static void ht_expand PARAMS ((hash_table *));

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

/* Initialise an obstack.  */
void
gcc_obstack_init (obstack)
     struct obstack *obstack;
{
  _obstack_begin (obstack, OBSTACK_CHUNK_SIZE, 0,
		  (void *(*) PARAMS ((long))) OBSTACK_CHUNK_ALLOC,
		  (void (*) PARAMS ((void *))) OBSTACK_CHUNK_FREE);
}

/* Calculate the hash of the string STR of length LEN.  */

static unsigned int
calc_hash (str, len)
     const unsigned char *str;
     unsigned int len;
{
  unsigned int n = len;
  unsigned int r = 0;
#define HASHSTEP(r, c) ((r) * 67 + ((c) - 113));

  while (n--)
    r = HASHSTEP (r, *str++);

  return r + len;
#undef HASHSTEP
}

/* Initialize an identifier hashtable.  */

hash_table *
ht_create (order)
     unsigned int order;
{
  unsigned int nslots = 1 << order;
  hash_table *table;

  table = (hash_table *) xmalloc (sizeof (hash_table));
  memset (table, 0, sizeof (hash_table));

  /* Strings need no alignment.  */
  gcc_obstack_init (&table->stack);
  obstack_alignment_mask (&table->stack) = 0;

  table->entries = (hashnode *) xcalloc (nslots, sizeof (hashnode));
  table->nslots = nslots;
  return table;
}

/* Frees all memory associated with a hash table.  */

void
ht_destroy (table)
     hash_table *table;
{
  obstack_free (&table->stack, NULL);
  free (table->entries);
  free (table);
}

/* Returns the hash entry for the a STR of length LEN.  If that string
   already exists in the table, returns the existing entry, and, if
   INSERT is CPP_ALLOCED, frees the last obstack object.  If the
   identifier hasn't been seen before, and INSERT is CPP_NO_INSERT,
   returns NULL.  Otherwise insert and returns a new entry.  A new
   string is alloced if INSERT is CPP_ALLOC, otherwise INSERT is
   CPP_ALLOCED and the item is assumed to be at the top of the
   obstack.  */
hashnode
ht_lookup (table, str, len, insert)
     hash_table *table;
     const unsigned char *str;
     unsigned int len;
     enum ht_lookup_option insert;
{
  unsigned int hash = calc_hash (str, len);
  unsigned int hash2;
  unsigned int index;
  size_t sizemask;
  hashnode node;

  sizemask = table->nslots - 1;
  index = hash & sizemask;

  /* hash2 must be odd, 