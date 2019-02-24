/* An expandable hash tables datatype.  
   Copyright (C) 1999, 2000, 2001, 2002 Free Software Foundation, Inc.
   Contributed by Vladimir Makarov (vmakarov@cygnus.com).

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

/* This package implements basic hash table functionality.  It is possible
   to search for an entry, create an entry and destroy an entry.

   Elements in the table are generic pointers.

   The size of the table is not fixed; if the occupancy of the table
   grows too high the hash table will be expanded.

   The abstract data implementation is based on generalized Algorithm D
   from Knuth's book "The art of computer programming".  Hash table is
   expanded by creation of new hash table and transferring elements from
   the old table to the new table. */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_STDLIB_H
#include "../include/stdlib.h"
#endif

#ifdef HAVE_STRING_H
#include "../include/string.h"
#endif

#include "../include/stdio.h"

/* !kawai! */
#include "../include/libiberty.h"
#include "../include/hashtab.h"
/* end of !kawai! */

/* This macro defines reserved value for empty table entry. */

#define EMPTY_ENTRY    ((PTR) 0)

/* This macro defines reserved value for table entry which contained
   a deleted element. */

#define DELETED_ENTRY  ((PTR) 1)

static unsigned long higher_prime_number PARAMS ((unsigned long));
static hashval_t hash_pointer PARAMS ((const void *));
static int eq_pointer PARAMS ((const void *, const void *));
static int htab_expand PARAMS ((htab_t));
static PTR *find_empty_slot_for_expand  PARAMS ((htab_t, hashval_t));

/* At some point, we could make these be NULL, and modify the
   hash-table routines to handle NULL specially; that would avoid
   function-call overhead for the common case of hashing pointers.  */
htab_hash htab_hash_pointer = hash_pointer;
htab_eq htab_eq_pointer = eq_pointer;

/* The following function returns a nearest prime number which is
   greater than N, and near a power of two. */

static unsigned long
higher_prime_number (n)
     unsigned long n;
{
  /* These are primes that are near, but slightly smaller than, a
     power of two.  */
  static const unsigned long primes[] = {
    (unsigned long) 7,
    (unsigned long) 13,
    (unsigned long) 31,
    (unsigned long) 61,
    (unsigned long) 127,
    (unsigned long) 251,
    (unsigned long) 509,
    (unsigned long) 1021,
    (unsigned long) 2039,
    (unsigned long) 4093,
    (unsigned long) 8191,
    (unsigned long) 16381,
    (unsigned long) 32749,
    (unsigned long) 65521,
    (unsigned long) 131071,
    (unsigned long) 262139,
    (unsigned long) 524287,
    (unsigned long) 1048573,
    (unsigned long) 2097143,
    (unsigned long) 4194301,
    (unsigned long) 8388593,
    (unsigned long) 16777213,
    (unsigned long) 33554393,
    (unsigned long) 67108859,
    (unsigned long) 134217689,
    (unsigned long) 268435399,
    (unsigned long) 536870909,
    (unsigned long) 1073741789,
    (unsigned long) 2147483647,
					/* 4294967291L */
    ((unsigned long) 2147483647) + ((unsigned long) 2147483644),
  };

  const unsigned long *low = &primes[0];
  const unsigned long *high = &primes[sizeof(primes) / sizeof(primes[0])];

  while (low != high)
    {
      con