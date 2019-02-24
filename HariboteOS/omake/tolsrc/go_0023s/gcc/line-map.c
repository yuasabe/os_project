/* Map logical line numbers to (source file, line number) pairs.
   Copyright (C) 2001
   Free Software Foundation, Inc.

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
#include "line-map.h"
#include "intl.h"

static void trace_include
  PARAMS ((const struct line_maps *, const struct line_map *));

/* Initialize a line map set.  */

void
init_line_maps (set)
     struct line_maps *set;
{
  set->maps = 0;
  set->allocated = 0;
  set->used = 0;
  set->last_listed = -1;
  set->trace_includes = false;
  set->depth = 0;
}

/* Free a line map set.  */

void
free_line_maps (set)
     struct line_maps *set;
{
  if (set->maps)
    {
      struct line_map *map;

      /* Depending upon whether we are handling preprocessed input or
	 not, this can be a user error or an ICE.  */
      for (map = CURRENT_LINE_MAP (set); ! MAIN_FILE_P (map);
	   map = INCLUDED_FROM (set, map))
	fprintf (stderr, "line-map.c: file ¥"%s¥" entered but not left¥n",
		 map->to_file);

      free (set->maps);
    }
}

/* Add a mapping of logical source line to physical source file and
   line number.  Ther text pointed to by TO_FILE must have a lifetime
   at least as long as the final call to lookup_line ().

   FROM_LINE should be monotonic increasing across calls to this
   function.  */

const struct line_map *
add_line_map (set, reason, sysp, from_line, to_file, to_line)
     struct line_maps *set;
     enum lc_reason reason;
     unsigned int sysp;
     unsigned int from_line;
     const char *to_file;
     unsigned int to_line;
{
  struct line_map *map;

  if (set->used && from_line < set->maps[set->used - 1].from_line)
    abort ();

  if (set->used == set->allocated)
    {
      set->allocated = 2 * set->allocated + 256;
      set->maps = (struct line_map *)
	xrealloc (set->maps, set->allocated * sizeof (struct line_map));
    }

  map = &set->maps[set->used++];

  /* If we don't keep our line maps consistent, we can easily
     segfault.  Don't rely on the client to do it for us.  */
  if (set->depth == 0)
    reason = LC_ENTER;
  else if (reason == LC_LEAVE)
    {
      struct line_map *from;
      bool error;

      if (MAIN_FILE_P (map - 1))
	{
	  error = true;
	  reason = LC_RENAME;
	  from = map - 1;
	}
      else
	{
	  from = INCLUDED_FROM (set, map - 1);
	  error = to_file && strcmp (from->to_file, to_file);
	}

      /* Depending upon whether we are handling preprocessed input or
	 not, this can be a user error or an ICE.  */
      if (error)
	fprintf (stderr, "line-map.c: file ¥"%s¥" left but not entered¥n",
		 to_file);

      /* A TO_FILE of NULL is special - we use the natural values.  */
      if (error || to_file == NULL)
	{
	  to_file = from->to_file;
	  to_line = LAST_SOURCE_LINE (from) + 1;
	  sysp = from->sysp;
	}
    }

  map->reason = reason;
  map->sysp = sysp;
  map->from_line = from_line;
  map->to_file = to_file;
  map->to_line = to_line;

  if (reason == LC_ENTER)
    {
      set->depth++;
      map->included_from = set->used - 2;
      if (set->trace_includes)
	trace_include (set, map);
    }
  else if (reason == LC_RENAME)
    map->included_fr