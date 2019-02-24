/* Instruction scheduling pass.
   Copyright (C) 1992, 1993, 1994, 1995, 1996, 1997, 1998,
   1999, 2000, 2002 Free Software Foundation, Inc.
   Contributed by Michael Tiemann (tiemann@cygnus.com) Enhanced by,
   and currently maintained by, Jim Wilson (wilson@cygnus.com)

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

#include "config.h"
#include "system.h"
#include "toplev.h"
#include "rtl.h"
#include "tm_p.h"
#include "regs.h"
#include "hard-reg-set.h"
#include "basic-block.h"
#include "insn-attr.h"
#include "sched-int.h"

#ifdef INSN_SCHEDULING
/* target_units bitmask has 1 for each unit in the cpu.  It should be
   possible to compute this variable from the machine description.
   But currently it is computed by examining the insn list.  Since
   this is only needed for visualization, it seems an acceptable
   solution.  (For understanding the mapping of bits to units, see
   definition of function_units[] in "insn-attrtab.c".)  */

static int target_units = 0;

static char *safe_concat PARAMS ((char *, char *, const char *));
static int get_visual_tbl_length PARAMS ((void));
static void print_exp PARAMS ((char *, rtx, int));
static void print_value PARAMS ((char *, rtx, int));
static void print_pattern PARAMS ((char *, rtx, int));
static void print_insn PARAMS ((char *, rtx, int));

/* Print names of units on which insn can/should execute, for debugging.  */

void
insn_print_units (insn)
     rtx insn;
{
  int i;
  int unit = insn_unit (insn);

  if (unit == -1)
    fprintf (sched_dump, "none");
  else if (unit >= 0)
    fprintf (sched_dump, "%s", function_units[unit].name);
  else
    {
      fprintf (sched_dump, "[");
      for (i = 0, unit = ‾unit; unit; i++, unit >>= 1)
	if (unit & 1)
	  {
	    fprintf (sched_dump, "%s", function_units[i].name);
	    if (unit != 1)
	      fprintf (sched_dump, " ");
	  }
      fprintf (sched_dump, "]");
    }
}

/* MAX_VISUAL_LINES is the maximum number of lines in visualization table
   of a basic block.  If more lines are needed, table is splitted to two.
   n_visual_lines is the number of lines printed so far for a block.
   visual_tbl contains the block visualization info.
   vis_no_unit holds insns in a cycle that are not mapped to any unit.  */
#define MAX_VISUAL_LINES 100
#define INSN_LEN 30
int n_visual_lines;
static unsigned visual_tbl_line_length;
char *visual_tbl;
int n_vis_no_unit;
#define MAX_VISUAL_NO_UNIT 20
rtx vis_no_unit[MAX_VISUAL_NO_UNIT];

/* Finds units that are in use in this function.  Required only
   for visualization.  */

void
init_target_units ()
{
  rtx insn;
  int unit;

  for (insn = get_last_insn (); insn; insn = PREV_INSN (insn))
    {
      if (! INSN_P (insn))
	continue;

      unit = insn_unit (insn);

      if (unit < 0)
	target_units |= ‾unit;
      else
	target_units |= (1 << unit);
    }
}

/* Return the length of the visualization table.  */

static int
get_visual_tbl_length ()
{
  int unit, i;
  int n, n1;
  char *s;

  /* Compute length of one field in line.  */
  s = (char *) alloca (INSN_LEN + 6);
  sprintf (s, "  %33s", "uname");
  n1 = strlen (s);

  /* Compute length of one line.  */
  n = strlen (";; ");
  n += n1;
  for (unit = 0; unit < FUNCTION_UNITS_SIZE; unit++)
    if (function_units[unit].bitmask & target_units)
      for (i = 0; i < function_units[unit].multiplicity