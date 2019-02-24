/* Output routines for graphical representation.
   Copyright (C) 1998, 1999, 2000, 2001 Free Software Foundation, Inc.
   Contributed by Ulrich Drepper <drepper@cygnus.com>, 1998.

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

/* !kawai! */
#include "config.h"
#include "system.h"
/* end of !kawai! */

#include "rtl.h"
#include "flags.h"
#include "output.h"
#include "function.h"
#include "hard-reg-set.h"
#include "basic-block.h"
#include "toplev.h"
#include "graph.h"

static const char *const graph_ext[] =
{
  /* no_graph */ "",
  /* vcg */      ".vcg",
};

static void start_fct PARAMS ((FILE *));
static void start_bb PARAMS ((FILE *, int));
static void node_data PARAMS ((FILE *, rtx));
static void draw_edge PARAMS ((FILE *, int, int, int, int));
static void end_fct PARAMS ((FILE *));
static void end_bb PARAMS ((FILE *));

/* Output text for new basic block.  */
static void
start_fct (fp)
     FILE *fp;
{

  switch (graph_dump_format)
    {
    case vcg:
      fprintf (fp, "¥
graph: { title: ¥"%s¥"¥nfolding: 1¥nhidden: 2¥nnode: { title: ¥"%s.0¥" }¥n",
	       current_function_name, current_function_name);
      break;
    case no_graph:
      break;
    }
}

static void
start_bb (fp, bb)
     FILE *fp;
     int bb;
{
  switch (graph_dump_format)
    {
    case vcg:
      fprintf (fp, "¥
graph: {¥ntitle: ¥"%s.BB%d¥"¥nfolding: 1¥ncolor: lightblue¥n¥
label: ¥"basic block %d",
	       current_function_name, bb, bb);
      break;
    case no_graph:
      break;
    }

#if 0
  /* FIXME Should this be printed?  It makes the graph significantly larger.  */

  /* Print the live-at-start register list.  */
  fputc ('¥n', fp);
  EXECUTE_IF_SET_IN_REG_SET (basic_block_live_at_start[bb], 0, i,
			     {
			       fprintf (fp, " %d", i);
			       if (i < FIRST_PSEUDO_REGISTER)
				 fprintf (fp, " [%s]",
					  reg_names[i]);
			     });
#endif

  switch (graph_dump_format)
    {
    case vcg:
      fputs ("¥"¥n¥n", fp);
      break;
    case no_graph:
      break;
    }
}

static void
node_data (fp, tmp_rtx)
     FILE *fp;
     rtx tmp_rtx;
{

  if (PREV_INSN (tmp_rtx) == 0)
    {
      /* This is the first instruction.  Add an edge from the starting
	 block.  */
      switch (graph_dump_format)
	{
	case vcg:
	  fprintf (fp, "¥
edge: { sourcename: ¥"%s.0¥" targetname: ¥"%s.%d¥" }¥n",
		   current_function_name,
		   current_function_name, XINT (tmp_rtx, 0));
	  break;
	case no_graph:
	  break;
	}
    }

  switch (graph_dump_format)
    {
    case vcg:
      fprintf (fp, "node: {¥n  title: ¥"%s.%d¥"¥n  color: %s¥n  ¥
label: ¥"%s %d¥n",
	       current_function_name, XINT (tmp_rtx, 0),
	       GET_CODE (tmp_rtx) == NOTE ? "lightgrey"
	       : GET_CODE (tmp_rtx) == INSN ? "green"
	       : GET_CODE (tmp_rtx) == JUMP_INSN ? "darkgreen"
	       : GET_CODE (tmp_rtx) == CALL_INSN ? "darkgreen"
	       : GET_CODE (tmp_rtx) == CODE_LABEL ?  "¥
darkgrey¥n  shape: ellipse" : "white",
	       GET_RTX_NAME (GET_CODE (tmp_rtx)), XINT (tmp_rtx, 0));
      break;
    case no_graph:
      break;
    }

  /* Print the RTL.  */
  if (GET_CODE (tmp_rtx) == NOTE)
    {
      const char *name = "";
      if (NOTE_LINE_NUMBER (tmp_rtx) < 0)
	name =  GET_NOTE_INSN_NAME (NOTE_LINE_NUMBER (tmp_rtx));
      fprintf (fp, " %s", name);
    }
  else if (INSN_P (tmp_rtx))
    print_r