/* Expands front end tree to back end RTL for GNU C-Compiler
   Copyright (C) 1987, 1988, 1989, 1991, 1992, 1993, 1994, 1995, 1996, 1997,
   1998, 1999, 2000, 2001, 2002 Free Software Foundation, Inc.

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

/* This file handles the generation of rtl code from tree structure
   at the level of the function as a whole.
   It creates the rtl expressions for parameters and auto variables
   and has full responsibility for allocating stack slots.

   `expand_function_start' is called at the beginning of a function,
   before the function body is parsed, and `expand_function_end' is
   called after parsing the body.

   Call `assign_stack_local' to allocate a stack slot for a local variable.
   This is usually done during the RTL generation for the function body,
   but it can also be done in the reload pass when a pseudo-register does
   not get a hard register.

   Call `put_var_into_stack' when you learn, belatedly, that a variable
   previously given a pseudo-register must in fact go in the stack.
   This function changes the DECL_RTL to be a stack slot instead of a reg
   then scans all the RTL instructions so far generated to correct them.  */

/* !kawai! */
#include "config.h"
#include "system.h"
#include "rtl.h"
#include "tree.h"
#include "flags.h"
#include "except.h"
#include "function.h"
#include "expr.h"
#include "libfuncs.h"
#include "regs.h"
#include "hard-reg-set.h"
#include "insn-config.h"
#include "recog.h"
#include "output.h"
#include "basic-block.h"
#include "../include/obstack.h"
#include "toplev.h"
#include "hash.h"
#include "ggc.h"
#include "tm_p.h"
#include "integrate.h"
/* end of !kawai! */

#ifndef TRAMPOLINE_ALIGNMENT
#define TRAMPOLINE_ALIGNMENT FUNCTION_BOUNDARY
#endif

#ifndef LOCAL_ALIGNMENT
#define LOCAL_ALIGNMENT(TYPE, ALIGNMENT) ALIGNMENT
#endif

/* Some systems use __main in a way incompatible with its use in gcc, in these
   cases use the macros NAME__MAIN to give a quoted symbol and SYMBOL__MAIN to
   give the same symbol without quotes for an alternative entry point.  You
   must define both, or neither.  */
#ifndef NAME__MAIN
#define NAME__MAIN "__main"
#define SYMBOL__MAIN __main
#endif

/* Round a value to the lowest integer less than it that is a multiple of
   the required alignment.  Avoid using division in case the value is
   negative.  Assume the alignment is a power of two.  */
#define FLOOR_ROUND(VALUE,ALIGN) ((VALUE) & ‾((ALIGN) - 1))

/* Similar, but round to the next highest integer that meets the
   alignment.  */
#define CEIL_ROUND(VALUE,ALIGN)	(((VALUE) + (ALIGN) - 1) & ‾((ALIGN)- 1))

/* NEED_SEPARATE_AP means that we cannot derive ap from the value of fp
   during rtl generation.  If they are different register numbers, this is
   always true.  It may also be true if
   FIRST_PARM_OFFSET - STARTING_FRAME_OFFSET is not a constant during rtl
   generation.  See fix_lexical_addr for details.  */

#if ARG_POINTER_REGNUM != FRAME_POINTER_REGNUM
#define NEED_SEPARATE_AP
#endif

/* Nonzero if function being compiled doesn't contain any calls
   (ignoring the prologue and epilogue).  This is set prior to
   local register allocation and is valid for the remaining
   compiler passes.  */
int current_function_is_leaf;

/* Nonzero if function being compiled doesn't contain any instructions
   that can throw an exception.  This is set prior to final.  */

int current_function_nothrow;

/* Nonzero if function being compiled doesn't modify the stack pointer
   (ignoring the prologue and epilogue).  This is only valid after
   life_analysis has run.  */
int current_function_sp_is_unchanging;

/* Nonzero if the function being compiled is a leaf function which only
   uses leaf registers.  This is valid after reload (specifically after
   sched2) and is useful only if the port defines LEAF_REGISTERS.  */
int current_function_uses_only_leaf_regs;

/* Nonzero once virtual register instantiation has been done.
   assign_stack_local uses frame_pointer_rtx when this is nonzero.
   calls.c:emit_library_call_value_1 uses it to set up
   post-instantiation libcalls.  */
int virtuals_instantiated;

/* Assign unique numbers to labels generated for profiling.  */
static int profile_label_no;

/* These variables hold pointers to functions to create and destroy
   target specific, per-function data structures.  */
void (*init_machine_status) PARAMS ((struct function *));
void (*free_machine_status) PARAMS ((struct function *));
/* This variable holds a pointer to a function to register any
   data items in the target specific, per-function data structure
   that will need garbage collection.  */
void (*mark_machine_status) PARAMS ((struct function *));

/* Likewise, but for language-specific data.  */
void (*init_lang_status) PARAMS ((struct function *));
void (*save_lang_status) PARAMS ((struct function *));
void (*restore_lang_status) PARAMS ((struct function *));
void (*mark_lang_status) PARAMS ((struct function *));
void (*free_lang_status) PARAMS ((struct function *));

/* The FUNCTION_DECL for an inline function currently being expanded.  */
tree inline_function_decl;

/* The currently compiled function.  */
struct function *cfun = 0;

/* These arrays record the INSN_UIDs of the prologue and epilogue insns.  */
static varray_type prologue;
static varray_type epilogue;

/* Array of INSN_UIDs to hold the INSN_UIDs for each sibcall epilogue
   in this function.  */
static varray_type sibcall_epilogue;

/* In order to evaluate some expressions, such as function calls returning
   structures in memory, we need to temporarily allocate stack locations.
   We record each allocated temporary in the following structure.

   Associated with each temporary slot is a nesting level.  When we pop up
   one level, all temporaries associated with the previous level are freed.
   Normally, all temporaries are freed after the execution of the statement
   in which they were created.  However, if we are inside a ({...}) grouping,
   the result may be in a temporary and hence must be preserved.  If the
   result could be in a temporary, we preserve it if we can determine which
   one it is in.  If we cannot determine which temporary may contain the
   result, all temporaries are preserved.  A temporary is preserved by
   pretending it was allocated at the previous nesting level.

   Automatic variables are also assigned temporary slots, at the nesting
   level where they are defined.  They are marked a "kept" so that
   free_temp_slots will not free them.  */

struct temp_slot
{
  /* Points to next temporary slot.  */
  struct temp_slot *next;
  /* The rtx to used to reference the slot.  */
  rtx slot;
  /* The rtx used to represent the address if not the address of the
     slot above.  May be an EXPR_LIST if multiple addresses exist.  */
  rtx address;
  /* The alignment (in bits) of the slot.  */
  unsigned int align;
  /* The size, in units, of the slot.  */
  HOST_WIDE_INT size;
  /* The type of the object in the slot, or zero if it doesn't correspond
     to a type.  We use this to determine whether a slot can be reused.
     It can be reused if objects of the type of the new slot will always
     conflict with objects of the type of the old slot.  */
  tree type;
  /* The value of `sequence_rtl_expr' when this temporary is allocated.  */
  tree rtl_expr;
  /* Non-zero if this temporary is currently in use.  */
  char in_use;
  /* Non-zero if this temporary has its address taken.  */
  char addr_taken;
  /* Nesting level at which this slot is being used.  */
  int level;
  /* Non-zero if this should survive a call to free_temp_slots.  */
  int keep;
  /* The offset of the slot from the frame_pointer, including extra space
     for alignment.  This info is for combine_temp_slots.  */
  HOST_WIDE_INT base_offset;
  /* The size of the slot, including extra space for alignment.  This
     info is for combine_temp_slots.  */
  HOST_WIDE_INT full_size;
};

/* This structure is used to record MEMs or pseudos used to replace VAR, any
   SUBREGs of VAR, and any MEMs containing VAR as an address.  We need to
   maintain this list in case two operands of an insn were required to match;
   in that case we must ensure we use the same replacement.  */

struct fixup_replacement
{
  rtx old;
  rtx new;
  struct fixup_replacement *next;
};

struct insns_for_mem_entry
{
  /* The KEY in HE will be a MEM.  */
  struct hash_entry he;
  /* These are the INSNS which reference the MEM.  */
  rtx insns;
};

/* Forward declarations.  */

static rtx assign_stack_local_1 PARAMS ((enum machine_mode, HOST_WIDE_INT,
					 int, struct function *));
static struct temp_slot *find_temp_slot_from_address  PARAMS ((rtx));
static void put_reg_into_stack	PARAMS ((struct function *, rtx, tree,
					 enum machine_mode, enum machine_mode,
					 int, unsigned int, int,
					 struct hash_table *));
static void schedule_fixup_var_refs PARAMS ((struct function *, rtx, tree,
					     enum machine_mode,
					     struct hash_table *));
static void fixup_var_refs	PARAMS ((rtx, enum machine_mode, int, rtx,
					 struct hash_table *));
static struct fixup_replacement
  *find_fixup_replacement	PARAMS ((struct fixup_replacement **, rtx));
static void fixup_var_refs_insns PARAMS ((rtx, rtx, enum machine_mode,
					  int, int, rtx));
static void fixup_var_refs_insns_with_hash
				PARAMS ((struct hash_table *, rtx,
					 enum machine_mode, int, rtx));
static void fixup_var_refs_insn PARAMS ((rtx, rtx, enum machine_mode,
					 int, int, rtx));
static void fixup_var_refs_1	PARAMS ((rtx, enum machine_mode, rtx *, rtx,
					 struct fixup_replacement **, rtx));
static rtx fixup_memory_subreg	PARAMS ((rtx, rtx, enum machine_mode, int));
static rtx walk_fixup_memory_subreg  PARAMS ((rtx, rtx, enum machine_mode, 
					      int));
static rtx fixup_stack_1	PARAMS ((rtx, rtx));
static void optimize_bit_field	PARAMS ((rtx, rtx, rtx *));
static void instantiate_decls	PARAMS ((tree, int));
static void instantiate_decls_1	PARAMS ((tree, int));
static void instantiate_decl	PARAMS ((rtx, HOST_WIDE_INT, int));
static rtx instantiate_new_reg	PARAMS ((rtx, HOST_WIDE_INT *));
static int instantiate_virtual_regs_1 PARAMS ((rtx *, rtx, int));
static void delete_handlers	PARAMS ((void));
static void pad_to_arg_alignment PARAMS ((struct args_size *, int,
					  struct args_size *));
#ifndef ARGS_GROW_DOWNWARD
static void pad_below		PARAMS ((struct args_size *, enum machine_mode,
					 tree));
#endif
static rtx round_trampoline_addr PARAMS ((rtx));
static rtx adjust_trampoline_addr PARAMS ((rtx));
static tree *identify_blocks_1	PARAMS ((rtx, tree *, tree *, tree *));
static void reorder_blocks_0	PARAMS ((tree));
static void reorder_blocks_1	PARAMS ((rtx, tree, varray_type *));
static void reorder_fix_fragments PARAMS ((tree));
static tree blocks_nreverse	PARAMS ((tree));
static int all_blocks		PARAMS ((tree, tree *));
static tree *get_block_vector   PARAMS ((tree, int *));
extern tree debug_find_var_in_block_tree PARAMS ((tree, tree));
/* We always define `record_insns' even if its not used so that we
   can always export `prologue_epilogue_contains'.  */
static void record_insns	PARAMS ((rtx, varray_type *)) ATTRIBUTE_UNUSED;
static int contains		PARAMS ((rtx, varray_type));
#ifdef HAVE_return
static void emit_return_into_block PARAMS ((basic_block, rtx));
#endif
static void put_addressof_into_stack PARAMS ((rtx, struct hash_table *));
static bool purge_addressof_1 PARAMS ((rtx *, rtx, int, int,
					  struct hash_table *));
static void purge_single_hard_subreg_set PARAMS ((rtx));
#if defined(HAVE_epilogue) && defined(INCOMING_RETURN_ADDR_RTX)
static rtx keep_stack_depressed PARAMS ((rtx));
#endif
static int is_addressof		PARAMS ((rtx *, void *));
static struct hash_entry *insns_for_mem_newfunc PARAMS ((struct hash_entry *,
							 struct hash_table *,
							 hash_table_key));
static unsigned long insns_for_mem_hash PARAMS ((hash_table_key));
static bool insns_for_mem_comp PARAMS ((hash_table_key, hash_table_key));
static int insns_for_mem_walk   PARAMS ((rtx *, void *));
static void compute_insns_for_mem PARAMS ((rtx, rtx, struct hash_table *));
static void mark_function_status PARAMS ((struct function *));
static void maybe_mark_struct_function PARAMS ((void *));
static void prepare_function_start PARAMS ((void));
static void do_clobber_return_reg PARAMS ((rtx, void *));
static void do_use_return_reg PARAMS ((rtx, void *));

/* Pointer to chain of `struct function' for containing functions.  */
static struct function *outer_function_chain;

/* Given a function decl for a containing function,
   return the `struct function' for it.  */

struct function *
find_function_data (decl)
     tree decl;
{
  struct function *p;

  for (p = outer_function_chain; p; p = p->outer)
    if (p->decl == decl)
      return p;

  abort ();
}

/* Save the current context for compilation of a nested function.
   This is called from language-specific code.  The caller should use
   the save_lang_status callback to save any language-specific state,
   since this function knows only about language-independent
   variables.  */

void
push_function_context_to (context)
     tree context;
{
  struct function *p;

  if (context)
    {
      if (context == current_function_decl)
	cfun->contains_functions = 1;
      else
	{
	  struct function *containing = find_function_data (context);
	  containing->contains_functions = 1;
	}
    }

  if (cfun == 0)
    init_dummy_function_start ();
  p = cfun;

  p->outer = outer_function_chain;
  outer_function_chain = p;
  p->fixup_var_refs_queue = 0;

  if (save_lang_status)
    (*save_lang_status) (p);

  cfun = 0;
}

void
push_function_context ()
{
  push_function_context_to (current_function_decl);
}

/* Restore the last saved context, at the end of a nested function.
   This function is called from language-specific code.  */

void
pop_function_context_from (context)
     tree context ATTRIBUTE_UNUSED;
{
  struct function *p = outer_function_chain;
  struct var_refs_queue *queue;

  cfun = p;
  outer_function_chain = p->outer;

  current_function_decl = p->decl;
  reg_renumber = 0;

  restore_emit_status (p);

  if (restore_lang_status)
    (*restore_lang_status) (p);

  /* Finish doing put_var_into_stack for any of our variables which became
     addressable during the nested function.  If only one entry has to be
     fixed up, just do that one.  Otherwise, first make a list of MEMs that
     are not to be unshared.  */
  if (p->fixup_var_refs_queue == 0)
    ;
  else if (p->fixup_var_refs_queue->next == 0)
    fixup_var_refs (p->fixup_var_refs_queue->modified,
		    p->fixup_var_refs_queue->promoted_mode,
		    p->fixup_var_refs_queue->unsignedp,
		    p->fixup_var_refs_queue->modified, 0);
  else
    {
      rtx list = 0;

      for (queue = p->fixup_var_refs_queue; queue; queue = queue->next)
	list = gen_rtx_EXPR_LIST (VOIDmode, queue->modified, list);

      for (queue = p->fixup_var_refs_queue; queue; queue = queue->next)
	fixup_var_refs (queue->modified, queue->promoted_mode,
			queue->unsignedp, list, 0);

    }

  p->fixup_var_refs_queue = 0;

  /* Reset variables that have known state during rtx generation.  */
  rtx_equal_function_value_matters = 1;
  virtuals_instantiated = 0;
  generating_concat_p = 1;
}

void
pop_function_context ()
{
  pop_function_context_from (current_function_decl);
}

/* Clear out all parts of the state in F that can safely be discarded
   after the function has been parsed, but not compiled, to let
   garbage collection reclaim the memory.  */

void
free_after_parsing (f)
     struct function *f;
{
  /* f->expr->forced_labels is used by code generation.  */
  /* f->emit->regno_reg_rtx is used by code generation.  */
  /* f->varasm is used by code generation.  */
  /* f->eh->eh_return_stub_label is used by code generation.  */

  if (free_lang_status)
    (*free_lang_status) (f);
  free_stmt_status (f);
}

/* Clear out all parts of the state in F that can safely be discarded
   after the function has been compiled, to let garbage collection
   reclaim the memory.  */

void
free_after_compilation (f)
     struct function *f;
{
  free_eh_status (f);
  free_expr_status (f);
  free_emit_status (f);
  free_varasm_status (f);

  if (free_machine_status)
    (*free_machine_status) (f);

  if (f->x_parm_reg_stack_loc)
    free (f->x_parm_reg_stack_loc);

  f->x_temp_slots = NULL;
  f->arg_offset_rtx = NULL;
  f->return_rtx = NULL;
  f->internal_arg_pointer = NULL;
  f->x_nonlocal_labels = NULL;
  f->x_nonlocal_goto_handler_slots = NULL;
  f->x_nonlocal_goto_handler_labels = NULL;
  f->x_nonlocal_goto_stack_level = NULL;
  f->x_cleanup_label = NULL;
  f->x_return_label = NULL;
  f->x_save_expr_regs = NULL;
  f->x_stack_slot_list = NULL;
  f->x_rtl_expr_chain = NULL;
  f->x_tail_recursion_label = NULL;
  f->x_tail_recursion_reentry = NULL;
  f->x_arg_pointer_save_area = NULL;
  f->x_clobber_return_insn = NULL;
  f->x_context_display = NULL;
  f->x_trampoline_list = NULL;
  f->x_parm_birth_insn = NULL;
  f->x_last_parm_insn = NULL;
  f->x_parm_reg_stack_loc = NULL;
  f->fixup_var_refs_queue = NULL;
  f->original_arg_vector = NULL;
  f->original_decl_initial = NULL;
  f->inl_last_parm_insn = NULL;
  f->epilogue_delay_list = NULL;
}

/* Allocate fixed slots in the stack frame of the current function.  */

/* Return size needed for stack frame based on slots so far allocated in
   function F.
   This size counts from zero.  It is not rounded to PREFERRED_STACK_BOUNDARY;
   the caller may have to do that.  */

HOST_WIDE_INT
get_func_frame_size (f)
     struct function *f;
{
#ifdef FRAME_GROWS_DOWNWARD
  return -f->x_frame_offset;
#else
  return f->x_frame_offset;
#endif
}

/* Return size needed for stack frame based on slots so far allocated.
   This size counts from zero.  It is not rounded to PREFERRED_STACK_BOUNDARY;
   the caller may have to do that.  */
HOST_WIDE_INT
get_frame_size ()
{
  return get_func_frame_size (cfun);
}

/* Allocate a stack slot of SIZE bytes and return a MEM rtx for it
   with machine mode MODE.

   ALIGN controls the amount of alignment for the address of the slot:
   0 means according to MODE,
   -1 means use BIGGEST_ALIGNMENT and round size to multiple of that,
   positive specifies alignment boundary in bits.

   We do not round to stack_boundary here.

   FUNCTION specifies the function to allocate in.  */

static rtx
assign_stack_local_1 (mode, size, align, function)
     enum machine_mode mode;
     HOST_WIDE_INT size;
     int align;
     struct function *function;
{
  rtx x, addr;
  int bigend_correction = 0;
  int alignment;
  int frame_off, frame_alignment, frame_phase;

  if (align == 0)
    {
      tree type;

      if (mode == BLKmode)
	alignment = BIGGEST_ALIGNMENT;
      else
	alignment = GET_MODE_ALIGNMENT (mode);

      /* Allow the target to (possibly) increase the alignment of this
	 stack slot.  */
      type = type_for_mode (mode, 0);
      if (type)
	alignment = LOCAL_ALIGNMENT (type, alignment);

      alignment /= BITS_PER_UNIT;
    }
  else if (align == -1)
    {
      alignment = BIGGEST_ALIGNMENT / BITS_PER_UNIT;
      size = CEIL_ROUND (size, alignment);
    }
  else
    alignment = align / BITS_PER_UNIT;

#ifdef FRAME_GROWS_D