/* Implements exception handling.
   Copyright (C) 1989, 1992, 1993, 1994, 1995, 1996, 1997, 1998,
   1999, 2000, 2001, 2002 Free Software Foundation, Inc.
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


/* An exception is an event that can be signaled from within a
   function. This event can then be "caught" or "trapped" by the
   callers of this function. This potentially allows program flow to
   be transferred to any arbitrary code associated with a function call
   several levels up the stack.

   The intended use for this mechanism is for signaling "exceptional
   events" in an out-of-band fashion, hence its name. The C++ language
   (and many other OO-styled or functional languages) practically
   requires such a mechanism, as otherwise it becomes very difficult
   or even impossible to signal failure conditions in complex
   situations.  The traditional C++ example is when an error occurs in
   the process of constructing an object; without such a mechanism, it
   is impossible to signal that the error occurs without adding global
   state variables and error checks around every object construction.

   The act of causing this event to occur is referred to as "throwing
   an exception". (Alternate terms include "raising an exception" or
   "signaling an exception".) The term "throw" is used because control
   is returned to the callers of the function that is signaling the
   exception, and thus there is the concept of "throwing" the
   exception up the call stack.

   [ Add updated documentation on how to use this.  ]  */

/* !kawai! */
#include "config.h"
#include "system.h"
#include "rtl.h"
#include "tree.h"
#include "flags.h"
#include "function.h"
#include "expr.h"
#include "libfuncs.h"
#include "insn-config.h"
#include "except.h"
#include "integrate.h"
#include "hard-reg-set.h"
#include "basic-block.h"
#include "output.h"
#include "dwarf2asm.h"
#include "dwarf2out.h"
#include "dwarf2.h"
#include "toplev.h"
#include "../include/hashtab.h"
#include "intl.h"
#include "ggc.h"
#include "tm_p.h"
#include "target.h"
/* end of !kawai! */ 

/* Provide defaults for stuff that may not be defined when using
   sjlj exceptions.  */
#ifndef EH_RETURN_STACKADJ_RTX
#define EH_RETURN_STACKADJ_RTX 0
#endif
#ifndef EH_RETURN_HANDLER_RTX
#define EH_RETURN_HANDLER_RTX 0
#endif
#ifndef EH_RETURN_DATA_REGNO
#define EH_RETURN_DATA_REGNO(N) INVALID_REGNUM
#endif


/* Nonzero means enable synchronous exceptions for non-call instructions.  */
int flag_non_call_exceptions;

/* Protect cleanup actions with must-not-throw regions, with a call
   to the given failure handler.  */
tree (*lang_protect_cleanup_actions) PARAMS ((void));

/* Return true if type A catches type B.  */
int (*lang_eh_type_covers) PARAMS ((tree a, tree b));

/* Map a type to a runtime object to match type.  */
tree (*lang_eh_runtime_type) PARAMS ((tree));

/* A hash table of label to region number.  */

struct ehl_map_entry
{
  rtx label;
  struct eh_region *region;
};

static htab_t exception_handler_label_map;

static int call_site_base;
static unsigned int sjlj_funcdef_number;
static htab_t type_to_runtime_map;

/* Describe the SjLj_Function_Context structure.  */
static tree sjlj_fc_type_node;
static int sjlj_fc_call_site_ofs;
static int sjlj_fc_data_ofs;
static int sjlj_fc_personality_ofs;
static int sjlj_fc_lsda_ofs;
static int sjlj_fc_jbuf_ofs;

/* Describes one exception region.  */
struct eh_region
{
  /* The immediately surrounding region.  */
  struct eh_region *outer;

  /* The list of immediately contained regions.  */
  struct eh_region *inner;
  struct eh_region *next_peer;

  /* An identifier for this region.  */
  int region_number;

  /* When a region is deleted, its parents inherit the REG_EH_REGION
     numbers already assigned.  */
  bitmap aka;

  /* Each region does exactly one thing.  */
  enum eh_region_type
  {
    ERT_UNKNOWN = 0,
    ERT_CLEANUP,
    ERT_TRY,
    ERT_CATCH,
    ERT_ALLOWED_EXCEPTIONS,
    ERT_MUST_NOT_THROW,
    ERT_THROW,
    ERT_FIXUP
  } type;

  /* Holds the action to perform based on the preceding type.  */
  union {
    /* A list of catch blocks, a surrounding try block,
       and the label for continuing after a catch.  */
    struct {
      struct eh_region *catch;
      struct eh_region *last_catch;
      struct eh_region *prev_try;
      rtx continue_label;
    } try;

    /* The list through the catch handlers, the list of type objects
       matched, and the list of associated filters.  */
    struct {
      struct eh_region *next_catch;
      struct eh_region *prev_catch;
      tree type_list;
      tree filter_list;
    } catch;

    /* A tree_list of allowed types.  */
    struct {
      tree type_list;
      int filter;
    } allowed;

    /* The type given by a call to "throw foo();", or discovered
       for a throw.  */
    struct {
      tree type;
    } throw;

    /* Retain the cleanup expression even after expansion so that
       we can match up fixup regions.  */
    struct {
      tree exp;
    } cleanup;

    /* The real region (by expression and by pointer) that fixup code
       should live in.  */
    struct {
      tree cleanup_exp;
      struct eh_region *real_region;
    } fixup;
  } u;

  /* Entry point for this region's handler before landing pads are built.  */
  rtx label;

  /* Entry point for this region's handler from the runtime eh library.  */
  rtx landing_pad;

  /* Entry point for this region's handler from an inner region.  */
  rtx post_landing_pad;

  /* The RESX insn for handing off control to the next outermost handler,
     if appropriate.  */
  rtx resume;
};

/* Used to save exception status for each function.  */
struct eh_status
{
  /* The tree of all regions for this function.  */
  struct eh_region *region_tree;

  /* The same information as an indexable array.  */
  struct eh_region **region_array;

  /* The most recently open region.  */
  struct eh_region *cur_region;

  /* This is the region for which we are processing catch blocks.  */
  struct eh_region *try_region;

  /* A stack (TREE_LIST) of lists of handlers.  The TREE_VALUE of each
     node is itself a TREE_CHAINed list of handlers for regions that
     are not yet closed. The TREE_VALUE of each entry contains the
     handler for the corresponding entry on the ehstack.  */
  tree protect_list;

  rtx filter;
  rtx exc_ptr;

  int built_landing_pads;
  int last_region_number;

  varray_type ttype_data;
  varray_type ehspec_data;
  varray_type action_record_data;

  struct call_site_record
  {
    rtx landing_pad;
    int action;
  } *call_site_data;
  int call_site_data_used;
  int call_site_data_size;

  rtx ehr_stackadj;
  rtx ehr_handler;
  rtx ehr_label;

  rtx sjlj_fc;
  rtx sjlj_exit_after;
};


static void mark_eh_region			PARAMS ((struct eh_region *));
static int mark_ehl_map_entry			PARAMS ((PTR *, PTR));
static void mark_ehl_map			PARAMS ((void *));

static void free_region				PARAMS ((struct eh_region *));

static int t2r_eq				PARAMS ((const PTR,
							 const PTR));
static hashval_t t2r_hash			PARAMS ((const PTR));
static int t2r_mark_1				PARAMS ((PTR *, PTR));
static void t2r_mark				PARAMS ((PTR));
static void add_type_for_runtime		PARAMS ((tree));
static tree lookup_type_for_runtime		PARAMS ((tree));

static struct eh_region *expand_eh_region_end	PARAMS ((void));

static rtx get_exception_filter			PARAMS ((struct function *));

static void collect_eh_region_array		PARAMS ((void));
static void resolve_fixup_regions		PARAMS ((void));
static void remove_fixup_regions		PARAMS ((void));
static void remove_unreachable_regions		PARAMS ((rtx));
static void convert_from_eh_region_ranges_1	PARAMS ((rtx *, int *, int));

static struct eh_region *duplicate_eh_region_1	PARAMS ((struct eh_region *,
						     struct inline_remap *));
static void duplicate_eh_region_2		PARAMS ((struct eh_region *,
							 struct eh_region **));
static int ttypes_filter_eq			PARAMS ((const PTR,
							 const PTR));
static hashval_t ttypes_filter_hash		PARAMS ((const PTR));
static int ehspec_filter_eq			PARAMS ((const PTR,
							 const PTR));
static hashval_t ehspec_filter_hash		PARAMS ((const PTR));
static int add_ttypes_entry			PARAMS ((htab_t, tree));
static int add_ehspec_entry			PARAMS ((htab_t, htab_t,
							 tree));
static void assign_filter_values		PARAMS ((void));
static void build_post_landing_pads		PARAMS ((void));
static void connect_post_landing_pads		PARAMS ((void));
static void dw2_build_landing_pads		PARAMS ((void));

struct sjlj_lp_info;
static bool sjlj_find_directly_reachable_regions
     PARAMS ((struct sjlj_lp_info *));
static void sjlj_assign_call_site_values
     PARAMS ((rtx, struct sjlj_lp_info *));
static void sjlj_mark_call_sites
     PARAMS ((struct sjlj_lp_info *));
static void sjlj_emit_function_enter		PARAMS ((rtx));
static void sjlj_emit_function_exit		PARAMS ((void));
static void sjlj_emit_dispatch_table
     PARAMS ((rtx, struct sjlj_lp_info *));
static void sjlj_build_landing_pads		PARAMS ((void));

static hashval_t ehl_hash			PARAMS ((const PTR));
static int ehl_eq				PARAMS ((const PTR,
							 const PTR));
static void ehl_free				PARAMS ((PTR));
static void add_ehl_entry			PARAMS ((rtx,
							 struct eh_region *));
static void remove_exception_handler_label	PARAMS ((rtx));
static void remove_eh_handler			PARAMS ((struct eh_region *));
static int for_each_eh_label_1			PARAMS ((PTR *, PTR));

struct reachable_info;

/* The return value of reachable_next_level.  */
enum reachable_code
{
  /* The given exception is not processed by the given region.  */
  RNL_NOT_CAUGHT,
  /* The given exception may need processing by the given region.  */
  RNL_MAYBE_CAUGHT,
  /* The given exception is completely processed by the given region.  */
  RNL_CAUGHT,
  /* The given exception is completely processed by the runtime.  */
  RNL_BLOCKED
};

static int check_handled			PARAMS ((tree, tree));
static void add_reachable_handler
     PARAMS ((struct reachable_info *, struct eh_region *,
	      struct eh_region *));
static enum reachable_code reachable_next_level
     PARAMS ((struct eh_region *, tree, struct reachable_info *));

static int action_record_eq			PARAMS ((const PTR,
							 const PTR));
static hashval_t action_record_hash		PARAMS ((const PTR));
static int add_action_record			PARAMS ((htab_t, int, int));
static int collect_one_action_chain		PARAMS ((htab_t,
							 struct eh_region *));
static int add_call_site			PARAMS ((rtx, int));

static void push_uleb128			PARAMS ((varray_type *,
							 unsigned int));
static void push_sleb128			PARAMS ((varray_type *, int));
#ifndef HAVE_AS_LEB128
static int dw2_size_of_call_site_table		PARAMS ((void));
static int sjlj_size_of_call_site_table		PARAMS ((void));
#endif
static void dw2_output_call_site_table		PARAMS ((void));
static void sjlj_output_call_site_table		PARAMS ((void));


/* Routine to see if exception handling is turned on.
   DO_WARN is non-zero if we want to inform the user that exception
   handling is turned off.

   This is used to ensure that -fexceptions has been specified if the
   compiler tries to use any exception-specific functions.  */

int
doing_eh (do_warn)
     int do_warn;
{
  if (! flag_exceptions)
    {
      static int warned = 0;
      if (! warned && do_warn)
	{
	  error ("exception handling disabled, use -fexceptions to enable");
	  warned = 1;
	}
      return 0;
    }
  return 1;
}


void
init_eh ()
{
  ggc_add_root (&exception_handler_label_map, 1, 1, mark_ehl_map);

  if (! flag_exceptions)
    return;

  type_to_runtime_map = htab_create (31, t2r_hash, t2r_eq, NULL);
  ggc_add_root (&type_to_runtime_map, 1, sizeof (htab_t), t2r_mark);

  /* Create the SjLj_Function_Context structure.  This should match
     the definition in unwind-sjlj.c.  */
  if (USING_SJLJ_EXCEPTIONS)
    {
      tree f_jbuf, f_per, f_lsda, f_prev, f_cs, f_data, tmp;

      sjlj_fc_type_node = make_lang_type (RECORD_TYPE);
      ggc_add_tree_root (&sjlj_fc_type_node, 1);

      f_prev = build_decl (FIELD_DECL, get_identifier ("__prev"),
			   build_pointer_type (sjlj_fc_type_node));
      DECL_FIELD_CONTEXT (f_prev) = sjlj_fc_type_node;

      f_cs = build_decl (FIELD_DECL, get_identifier ("__call_site"),
			 integer_type_node);
      DECL_FIELD_CONTEXT (f_cs) = sjlj_fc_type_node;

      tmp = build_index_type (build_int_2 (4 - 1, 0));
      tmp = build_array_type (type_for_mode (word_mode, 1), tmp);
      f_data = build_decl (FIELD_DECL, get_identifier ("__data"), tmp);
      DECL_FIELD_CONTEXT (f_data) = sjlj_fc_type_node;

      f_per = build_decl (FIELD_DECL, get_identifier ("__personality"),
			  ptr_type_node);
      DECL_FIELD_CONTEXT (f_per) = sjlj_fc_type_node;

      f_lsda = build_decl (FIELD_DECL, get_identifier ("__lsda"),
			   ptr_type_node);
      DECL_FIELD_CONTEXT (f_lsda) = sjlj_fc_type_node;

#ifdef DONT_USE_BUILTIN_SETJMP
#ifdef JMP_BUF_SIZE
      tmp = build_int_2 (JMP_BUF_SIZE - 1, 0);
#else
      /* Should be large enough for most systems, if it is not,
	 JMP_BUF_SIZE should be defined with the proper value.  It will
	 also tend to be larger than necessary for most systems, a more
	 optimal port will define JMP_BUF_SIZE.  */
      tmp = build_int_2 (FIRST_PSEUDO_REGISTER + 2 - 1, 0);
#endif
#else
      /* This is 2 for builtin_setjmp, plus whatever the target requires
	 via STACK_SAVEAREA_MODE (SAVE_NONLOCAL).  */
      tmp = build_int_2 ((GET_MODE_SIZE (STACK_SAVEAREA_MODE (SAVE_NONLOCAL))
			  / GET_MODE_SIZE (Pmode)) + 2 - 1, 0);
#endif
      tmp = build_index_type (tmp);
      tmp = build_array_type (ptr_type_node, tmp);
      f_jbuf = build_decl (FIELD_DECL, get_identifier ("__jbuf"), tmp);
#ifdef DONT_USE_BUILTIN_SETJMP
      /* We don't know what the alignment requirements of the
	 runtime's jmp_buf has.  Overestimate.  */
      DECL_ALIGN (f_jbuf) = BIGGEST_ALIGNMENT;
      DECL_USER_ALIGN (f_jbuf) = 1;
#endif
      DECL_FIELD_CONTEXT (f_jbuf) = sjlj_fc_type_node;

      TYPE_FIELDS (sjlj_fc_type_node) = f_prev;
      TREE_CHAIN (f_prev) = f_cs;
      TREE_CHAIN (f_cs) = f_data;
      TREE_CHAIN (f_data) = f_per;
      TREE_CHAIN (f_per) = f_lsda;
      TREE_CHAIN (f_lsda) = f_jbuf;

      layout_type (sjlj_fc_type_node);

      /* Cache the interesting field offsets so that we have
	 easy access from rtl.  */
      sjlj_fc_call_site_ofs
	= (tree_low_cst (DECL_FIELD_OFFSET (f_cs), 1)
	   + tree_low_cst (DECL_FIELD_BIT_OFFSET (f_cs), 1) / BITS_PER_UNIT);
      sjlj_fc_data_ofs
	= (tree_low_cst (DECL_FIELD_OFFSET (f_data), 1)
	   + tree_low_cst (DECL_FIELD_BIT_OFFSET (f_data), 1) / BITS_PER_UNIT);
      sjlj_fc_personality_ofs
	= (tree_low_cst (DECL_FIELD_OFFSET (f_per), 1)
	   + tree_low_cst (DECL_FIELD_BIT_OFFSET (f_per), 1) / BITS_PER_UNIT);
      sjlj_fc_lsda_ofs
	= (tree_low_cst (DECL_FIELD_OFFSET (f_lsda), 1)
	   + tree_low_cst (DECL_FIELD_BIT_OFFSET (f_lsda), 1) / BITS_PER_UNIT);
      sjlj_fc_jbuf_ofs
	= (tree_low_cst (DECL_FIELD_OFFSET (f_jbuf), 1)
	   + tree_low_cst (DECL_FIELD_BIT_OFFSET (f_jbuf), 1) / BITS_PER_UNIT);
    }
}

void
init_eh_for_function ()
{
  cfun->eh = (struct eh_status *) xcalloc (1, sizeof (struct eh_status));
}

/* Mark EH for GC.  */

static void
mark_eh_region (region)
     struct eh_region *region;
{
  if (! region)
    return;

  switch (region->type)
    {
    case ERT_UNKNOWN:
      /* This can happen if a nested function is inside the body of a region
	 and we do a GC as part of processing it.  */
      break;
    case ERT_CLEANUP:
      ggc_mark_tree (region->u.cleanup.exp);
      break;
    case ERT_TRY:
      ggc_mark_rtx (region->u.try.continue_label);
      break;
    case ERT_CATCH:
      ggc_mark_tree (region->u.catch.type_list);
      ggc_mark_tree (region->u.catch.filter_list);
      break;
    case ERT_ALLOWED_EXCEPTIONS:
      ggc_mark_tree (region->u.allowed.type_list);
      break;
    case ERT_MUST_NOT_THROW:
      break;
    case ERT_THROW:
      ggc_mark_tree (region->u.throw.type);
      break;
    case ERT_FIXUP:
      ggc_mark_tree (region->u.fixup.cleanup_exp);
      break;
    default:
      abort ();
    }

  ggc_mark_rtx (region->label);
  ggc_mark_rtx (region->resume);
  ggc_mark_rtx (region->landing_pad);
  ggc_mark_rtx (region->post_landing_pad);
}

static int
mark_ehl_map_entry (pentry, data)
     PTR *pentry;
     PTR data ATTRIBUTE_UNUSED;
{
  struct ehl_map_entry *entry = *(struct ehl_map_entry **) pentry;
  ggc_mark_rtx (entry->label);
  return 1;
}

static void
mark_ehl_map (pp)
    void *pp;
{
  htab_t map = *(htab_t *) pp;
  if (map)
    htab_traverse (map, mark_ehl_map_entry, NULL);
}

void
mark_eh_status (eh)
     struct eh_status *eh;
{
  int i;

  if (eh == 0)
    return;

  /* If we've called collect_eh_region_array, use it.  Otherwise walk
     the tree non-recursively.  */
  if (eh->region_array)
    {
      for (i = eh->last_region_number; i > 0; --i)
	{
	  struct eh_region *r = eh->region_array[i];
	  if (r && r->region_number == i)
	    mark_eh_region (r);
	}
    }
  else if (eh->region_tree)
    {
      struct eh_region *r = eh->region_tree;
      while (1)
	{
	  mark_eh_region (r);
	  if (r->inner)
	    r = r->inner;
	  else if (r->next_peer)
	    r = r->next_peer;
	  else
	    {
	      do {
		r = r->outer;
		if (r == NULL)
		  goto tree_done;
	      } while (r->next_peer == NULL);
	      r = r->next_peer;
	    }
	}
    tree_done:;
    }

  ggc_mark_tree (eh->protect_list);
  ggc_mark_rtx (eh->filter);
  ggc_mark_rtx (eh->exc_ptr);
  ggc_mark_tree_varray (eh->ttype_data);

  if (eh->call_site_data)
    {
      for (i = eh->call_site_data_used - 1; i >= 0; --i)
	ggc_mark_rtx (eh->call_site_data[i].landing_pad);
    }

  ggc_mark_rtx (eh->ehr_stackadj);
  ggc_mark_rtx (eh->ehr_handler);
  ggc_mark_rtx (eh->ehr_label);

  ggc_mark_rtx (eh->sjlj_fc);
  ggc_mark_rtx (eh->sjlj_exit_after);
}

static inline void
free_region (r)
     struct eh_region *r;
{
  /* Note that the aka bitmap is freed by regset_release_memory.  But if
     we ever replace with a non-obstack implementation, this would be
     the place to do it.  */
  free (r);
}

void
free_eh_status (f)
     struct function *f;
{
  struct eh_status *eh = f->eh;

  if (eh->region_array)
    {
      int i;
      for (i = eh->last_region_number; i > 0; --i)
	{
	  struct eh_region *r = eh->region_array[i];
	  /* Mind we don't free a region struct more than once.  */
	  if (r && r->region_number == i)
	    free_region (r);
	}
      free (eh->region_array);
    }
  else if (eh->region_tree)
    {
      struct eh_region *next, *r = eh->region_tree;
      while (1)
	{
	  if (r->inner)
	    r = r->inner;
	  else if (r->next_peer)
	    {
	      next = r->next_peer;
	      free_region (r);
	      r = next;
	    }
	  else
	    {
	      do {
	        next = r->outer;
	        free_region (r);
	        r = next;
		if (r == NULL)
		  goto tree_done;
	      } while (r->next_peer == NULL);
	      next = r->next_peer;
	      free_region (r);
	      r = next;
	    }
	}
    tree_done:;
    }

  VARRAY_FREE (eh->ttype_data);
  VARRAY_FREE (eh->ehspec_data);
  VARRAY_FREE (eh->action_record_data);
  if (eh->call_site_