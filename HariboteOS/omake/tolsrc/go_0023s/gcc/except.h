/* Exception Handling interface routines.
   Copyright (C) 1996, 1997, 1998, 1999, 2000, 2001, 2002
   Free Software Foundation, Inc.
   Contributed by Mike Stump <mrs@cygnus.com>.

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


#ifndef GCC_VARRAY_H
struct varray_head_tag;
#define varray_type struct varray_head_tag *
#endif

struct function;

struct inline_remap;

/* Per-function EH data.  Used only in except.c, but GC and others
   manipulate pointers to the opaque type.  */
struct eh_status;

/* Internal structure describing a region.  */
struct eh_region;

/* Test: is exception handling turned on?  */
extern int doing_eh			        PARAMS ((int));

/* Start an exception handling region.  All instructions emitted after
   this point are considered to be part of the region until an
   expand_eh_region_end variant is invoked.  */
extern void expand_eh_region_start		PARAMS ((void));

/* End an exception handling region for a cleanup.  HANDLER is an
   expression to expand for the cleanup.  */
extern void expand_eh_region_end_cleanup	PARAMS ((tree));

/* End an exception handling region for a try block, and prepares
   for subsequent calls to expand_start_catch.  */
extern void expand_start_all_catch		PARAMS ((void));

/* Begin a catch clause.  TYPE is an object to be matched by the
   runtime, or a list of such objects, or null if this is a catch-all
   clause.  */
extern void expand_start_catch			PARAMS ((tree));

/* End a catch clause.  Control will resume after the try/catch block.  */
extern void expand_end_catch			PARAMS ((void));

/* End a sequence of catch handlers for a try block.  */
extern void expand_end_all_catch		PARAMS ((void));

/* End an exception region for an exception type filter.  ALLOWED is a
   TREE_LIST of TREE_VALUE objects to be matched by the runtime.
   FAILURE is a function to invoke if a mismatch occurs.  */
extern void expand_eh_region_end_allowed	PARAMS ((tree, tree));

/* End an exception region for a must-not-throw filter.  FAILURE is a
   function to invoke if an uncaught exception propagates this far.  */
extern void expand_eh_region_end_must_not_throw	PARAMS ((tree));

/* End an exception region for a throw.  No handling goes on here,
   but it's the easiest way for the front-end to indicate what type
   is being thrown.  */
extern void expand_eh_region_end_throw		PARAMS ((tree));

/* End a fixup region.  Within this region the cleanups for the immediately
   enclosing region are _not_ run.  This is used for goto cleanup to avoid
   destroying an object twice.  */
extern void expand_eh_region_end_fixup		PARAMS ((tree));

/* Begin a region that will contain entries created with
   add_partial_entry.  */
extern void begin_protect_partials              PARAMS ((void));

/* Create a new exception region and add the handler for the region
   onto a list. These regions will be ended (and their handlers emitted)
   when end_protect_partials is invoked.  */
extern void add_partial_entry			PARAMS ((tree));

/* End all of the pending exception regions that have handlers added with
   add_partial_entry.  */
extern void end_protect_partials		PARAMS ((void));

/* Invokes CALLBACK for every exception handler label.  Only used by old
   loop hackery; should not be used by new code.  */
extern void for_each_eh_label			PARAMS ((void (*) (rtx)));

/* Determine if the given INSN c