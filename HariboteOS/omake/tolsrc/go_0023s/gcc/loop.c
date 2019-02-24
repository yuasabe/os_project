/* Perform various loop optimizations, including strength reduction.
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

/* This is the loop optimization pass of the compiler.
   It finds invariant computations within loops and moves them
   to the beginning of the loop.  Then it identifies basic and
   general induction variables.  Strength reduction is applied to the general
   induction variables, and induction variable elimination is applied to
   the basic induction variables.

   It also finds cases where
   a register is set within the loop by zero-extending a narrower value
   and changes these to zero the entire register once before the loop
   and merely copy the low part within the loop.

   Most of the complexity is in heuristics to decide when it is worth
   while to do these things.  */

/* !kawai! */
#include "config.h"
#include "system.h"
#include "rtl.h"
#include "tm_p.h"
#include "../include/obstack.h"
#include "function.h"
#include "expr.h"
#include "hard-reg-set.h"
#include "basic-block.h"
#include "insn-config.h"
#include "regs.h"
#include "recog.h"
#include "flags.h"
#include "real.h"
#include "loop.h"
#include "cselib.h"
#include "except.h"
#include "toplev.h"
#include "predict.h"
#include "insn-flags.h"
#include "optabs.h"
/* end of !kawai! */

/* Not really meaningful values, but at least something.  */
#ifndef SIMULTANEOUS_PREFETCHES
#define SIMULTANEOUS_PREFETCHES 3
#endif
#ifndef PREFETCH_BLOCK
#define PREFETCH_BLOCK 32
#endif
#ifndef HAVE_prefetch
#define HAVE_prefetch 0
#define CODE_FOR_prefetch 0
#define gen_prefetch(a,b,c) (abort(), NULL_RTX)
#endif

/* Give up the prefetch optimizations once we exceed a given threshhold.
   It is unlikely that we would be able to optimize something in a loop
   with so many detected prefetches.  */
#define MAX_PREFETCHES 100
/* The number of prefetch blocks that are beneficial to fetch at once before
   a loop with a known (and low) iteration count.  */
#define PREFETCH_BLOCKS_BEFORE_LOOP_MAX  6
/* For very tiny loops it is not worthwhile to prefetch even before the loop,
   since it is likely that the data are already in the cache.  */
#define PREFETCH_BLOCKS_BEFORE_LOOP_MIN  2
/* The minimal number of prefetch blocks that a loop must consume to make
   the emitting of prefetch instruction in the body of loop worthwhile.  */
#define PREFETCH_BLOCKS_IN_LOOP_MIN  6

/* Parameterize some prefetch heuristics so they can be turned on and off
   easily for performance testing on new architecures.  These can be
   defined in target-dependent files.  */

/* Prefetch is worthwhile only when loads/stores are dense.  */
#ifndef PREFETCH_ONLY_DENSE_MEM
#define PREFETCH_ONLY_DENSE_MEM 1
#endif

/* Define what we mean by "dense" loads and stores; This value divided by 256
   is the minimum percentage of memory references that worth prefetching.  */
#ifndef PREFETCH_DENSE_MEM
#define PREFETCH_DENSE_MEM 220
#endif

/* Do not prefetch for a loop whose iteration count is known to be low.  */
#ifndef PREFETCH_NO_LOW_LOOPCNT
#define PREFETCH_NO_LOW_LOOPCNT 1
#endif

/* Define what we mean by a "low" iteration count.  */
#ifndef PREFETCH_LOW_LOOPCNT
#define PREFETCH_LOW_LOOPCNT 32
#endif

/* Do not prefetch for a loop that contains a function call; such a loop is
   probably not an internal loop.  */
#ifndef PREFETCH_NO_CALL
#define PREFETCH_NO_CALL 1
#endif

/* Do not prefetch accesses with an extreme stride.  */
#ifndef PREFETCH_NO_EXTREME_STRIDE
#define PREFETCH_NO_EXTREME_STRIDE 1
#endif

/* Define what we mean by an "extreme" stride.  */
#ifndef PREFETCH_EXTREME_STRIDE
#define PREFETCH_EXTREME_STRIDE 4096
#endif

/* Do not handle reversed order prefetches (negative stride).  */
#ifndef PREFETCH_NO_REVERSE_ORDER
#define PREFETCH_NO_REVERSE_ORDER 1
#endif

/* Prefetch even if the GIV is not always executed.  */
#ifndef PREFETCH_NOT_ALWAYS
#define PREFETCH_NOT_ALWAYS 0
#endif

/* If the loop requires more prefetches than the target can process in
   parallel then don't prefetch anything in that loop.  */
#ifndef PREFETCH_LIMIT_TO_SIMULTANEOUS
#define PREFETCH_LIMIT_TO_SIMULTANEOUS 1
#endif

#define LOOP_REG_LIFETIME(LOOP, REGNO) ¥
((REGNO_LAST_LUID (REGNO) - REGNO_FIRST_LUID (REGNO)))

#define LOOP_REG_GLOBAL_P(LOOP, REGNO) ¥
((REGNO_LAST_LUID (REGNO) > INSN_LUID ((LOOP)->end) ¥
 || REGNO_FIRST_LUID (REGNO) < INSN_LUID ((LOOP)->start)))

#define LOOP_REGNO_NREGS(REGNO, SET_DEST) ¥
((REGNO) < FIRST_PSEUDO_REGISTER ¥
 ? HARD_REGNO_NREGS ((REGNO), GET_MODE (SET_DEST)) : 1)


/* Vector mapping INSN_UIDs to luids.
   The luids are like uids but increase monotonically always.
   We use them to see whether a jump comes from outside a given loop.  */

int *uid_luid;

/* Indexed by INSN_UID, contains the ordinal giving the (innermost) loop
   number the insn is contained in.  */

struct loop **uid_loop;

/* 1 + largest uid of any insn.  */

int max_uid_for_loop;

/* 1 + luid of last insn.  */

static int max_luid;

/* Number of loops detected in current function.  Used as index to the
   next few tables.  */

static int max_loop_num;

/* Bound on pseudo register number before loop optimization.
   A pseudo has valid regscan info if its number is < max_reg_before_loop.  */
unsigned int max_reg_before_loop;

/* The value to pass to the next call of reg_scan_update.  */
static int loop_max_reg;

#define obstack_chunk_alloc xmalloc
#define obstack_chunk_free free

/* During the analysis of a loop, a chain of `struct movable's
   is made to record all the movable insns found.
   Then the entire chain can be scanned to decide which to move.  */

struct movable
{
  rtx insn;			/* A movable insn */
  rtx set_src;			/* The expression this reg is set from.  */
  rtx set_dest;			/* The destination of this SET.  */
  rtx dependencies;		/* When INSN is libcall, this is an EXPR_LIST
				   of any registers used within the LIBCALL.  */
  int consec;			/* Number of consecutive following insns
				   that must be moved with this one.  */
  unsigned int regno;		/* The register it sets */
  short lifetime;		/* lifetime of that register;
				   may be adjusted when matching movables
				   that load the same value are found.  */
  short savings;		/* Number of insns we can move for this reg,
				   including other movables that force this
				   or match this one.  */
  unsigned int cond : 1;	/* 1 if only conditionally movable */
  unsigned int force : 1;	/* 1 means MUST move this insn */
  unsigned int global : 1;	/* 1 means reg is live outside this loop */
		/* If PARTIAL is 1, GLOBAL means something different:
		   that the reg is live outside the range from where it is set
		   to the following label.  */
  unsigned int done : 1;	/* 1 inhibits further processing of this */

  unsigned int partial : 1;	/* 1 means this reg is used for zero-extending.
				   In particular, moving it does not make it
				   invariant.  */
  unsigned int move_insn : 1;	/* 1 means that we call emit_move_insn to
				   load SRC, rather than copying INSN.  */
  unsigned int move_insn_first:1;/* Same as above, if this is necessary for the
				    first insn of a consecutive sets group.  */
  unsigned int is_equiv : 1;	/* 1 means a REG_EQUIV is present on INSN.  */
  enum machine_mode savemode;   /* Nonzero means it is a mode for a low part
				   that we should avoid changing when clearing
				   the rest of the reg.  */
  struct movable *match;	/* First entry for same value */
  struct movable *forces;	/* An insn that must be moved if this is */
  struct movable *next;
};


FILE *loop_dump_stream;

/* Forward declarations.  */

static void invalidate_loops_containing_label PARAMS ((rtx));
static void find_and_verify_loops PARAMS ((rtx, struct loops *));
static void mark_loop_jump PARAMS ((rtx, struct loop *));
static void prescan_loop PARAMS ((struct loop *));
static int reg_in_basic_block_p PARAMS ((rtx, rtx));
static int consec_sets_invariant_p PARAMS ((const struct loop *,
					    rtx, int, rtx));
static int labels_in_range_p PARAMS ((rtx, int));
static void count_one_set PARAMS ((struct loop_regs *, rtx, rtx, rtx *));
static void note_addr_stored PARAMS ((rtx, rtx, void *));
static void note_set_pseudo_multiple_uses PARAMS ((rtx, rtx, void *));
static int loop_reg_used_before_p PARAMS ((const struct loop *, rtx, rtx));
static void scan_loop PARAMS ((struct loop*, int));
#if 0
static void replace_call_address PARAMS ((rtx, rtx, rtx));
#endif
static rtx skip_consec_insns PARAMS ((rtx, int));
static int libcall_benefit PARAMS ((rtx));
static void ignore_some_movables PARAMS ((struct loop_movables *));
static void force_movables PARAMS ((struct loop_movables *));
static void combine_movables PARAMS ((struct loop_movables *,
				      struct loop_regs *));
static int num_unmoved_movables PARAMS ((const struct loop *));
static int regs_match_p PARAMS ((rtx, rtx, struct loop_movables *));
static int rtx_equal_for_loop_p PARAMS ((rtx, rtx, struct loop_movables *,
					 struct loop_regs *));
static void add_label_notes PARAMS ((rtx, rtx));
static void move_movables PARAMS ((struct loop *loop, struct loop_movables *,
				   int, int));
static void loop_movables_add PARAMS((struct loop_movables *,
				      struct movable *));
static void loop_movables_free PARAMS((struct loop_movables *));
static int count_nonfixed_reads PARAMS ((const struct loop *, rtx));
static void loop_bivs_find PARAMS((struct loop *));
static void loop_bivs_init_find PARAMS((struct loop *));
static void loop_bivs_check PARAMS((struct loop *));
static void loop_givs_find PARAMS((struct loop *));
static void loop_givs_check PARAMS((struct loop *));
static int loop_biv_eliminable_p PARAMS((struct loop *, struct iv_class *,
					 int, int));
static int loop_giv_reduce_benefit PARAMS((struct loop *, struct iv_class *,
					   struct induction *, rtx));
static void loop_givs_dead_check PARAMS((struct loop *, struct iv_class *));
static void loop_givs_reduce PARAMS((struct loop *, struct iv_class *));
static void loop_givs_rescan PARAMS((struct loop *, struct iv_class *,
				     rtx *));
static void loop_ivs_free PARAMS((struct loop *));
static void strength_reduce PARAMS ((struct loop *, int));
static void find_single_use_in_loop PARAMS ((struct loop_regs *, rtx, rtx));
static int valid_initial_value_p PARAMS ((rtx, rtx, int, rtx));
static void find_mem_givs PARAMS ((const struct loop *, rtx, rtx, int, int));
static void record_biv PARAMS ((struct loop *, struct induction *,
				rtx, rtx, rtx, rtx, rtx *,
				int, int));
static void check_final_value PARAMS ((const struct loop *,
				       struct induction *));
static void loop_ivs_dump PARAMS((const struct loop *, FILE *, int));
static void loop_iv_class_dump PARAMS((const struct iv_class *, FILE *, int));
static void loop_biv_dump PARAMS((const struct induction *, FILE *, int));
static void loop_giv_dump PARAMS((const struct induction *, FILE *, int));
static void record_giv PARAMS ((const struct loop *, struct induction *,
				rtx, rtx, rtx, rtx, rtx, rtx, int,
				enum g_types, int, int, rtx *));
static void update_giv_derive PARAMS ((const struct loop *, rtx));
static void check_ext_dependent_givs PARAMS ((struct iv_class *,
					      struct loop_info *));
static int basic_induction_var PARAMS ((const struct loop *, rtx,
					enum machine_mode, rtx, rtx,
					rtx *, rtx *, rtx **));
static rtx simplify_giv_expr PARAMS ((const struct loop *, rtx, rtx *, int *));
static int general_induction_var PARAMS ((const struct loop *loop, rtx, rtx *,
					  rtx *, rtx *, rtx *, int, int *,
					  enum machine_mode));
static int consec_sets_giv PARAMS ((const struct loop *, int, rtx,
				    rtx, rtx, rtx *, rtx *, rtx *, rtx *));
static int check_dbra_loop PARAMS ((struct loop *, int));
static rtx express_from_1 PARAMS ((rtx, rtx, rtx));
static rtx combine_givs_p PARAMS ((struct induction *, struct induction *));
static int cmp_combine_givs_stats PARAMS ((const PTR, const PTR));
static void combine_givs PARAMS ((struct loop_regs *, struct iv_class *));
static int product_cheap_p PARAMS ((rtx, rtx));
static int maybe_eliminate_biv PARAMS ((const struct loop *, struct iv_class *,
					int, int, int));
static int maybe_eliminate_biv_1 PARAMS ((const struct loop *, rtx, rtx,
					  struct iv_class *, int,
					  basic_block, rtx));
static int last_use_this_basic_block PARAMS ((rtx, rtx));
static void record_initial PARAMS ((rtx, rtx, void *));
static void update_reg_last_use PARAMS ((rtx, rtx));
static rtx next_insn_in_loop PARAMS ((const struct loop *, rtx));
static void loop_regs_scan PARAMS ((const struct loop *, int));
static int count_insns_in_loop PARAMS ((const struct loop *));
static void load_mems PARAMS ((const struct loop *));
static int insert_loop_mem PARAMS ((rtx *, void *));
static int replace_loop_mem PARAMS ((rtx *, void *));
static void replace_loop_mems PARAMS ((rtx, rtx, rtx));
static int replace_loop_reg PARAMS ((rtx *, void *));
static void replace_loop_regs PARAMS ((rtx insn, rtx, rtx));
static void note_reg_stored PARAMS ((rtx, rtx, void *));
static void try_copy_prop PARAMS ((const struct loop *, rtx, unsigned int));
static void try_swap_copy_prop PARAMS ((const struct loop *, rtx,
					 unsigned int));
static int replace_label PARAMS ((rtx *, void *));
static rtx check_insn_for_givs PARAMS((struct loop *, rtx, int, int));
static rtx check_insn_for_bivs PARAMS((struct loop *, rtx, int, int));
static rtx gen_add_mult PARAMS ((rtx, rtx, rtx, rtx));
static void loop_regs_update PARAMS ((const struct loop *, rtx));
static int iv_add_mult_cost PARAMS ((rtx, rtx, rtx, rtx));

static rtx loop_insn_emit_after PARAMS((const struct loop *, basic_block,
					rtx, rtx));
static rtx loop_call_insn_emit_before PARAMS((const struct loop *,
					      basic_block, rtx, rtx));
static rtx loop_call_insn_hoist PARAMS((const struct loop *, rtx));
static rtx loop_insn_sink_or_swim PARAMS((const struct loop *, rtx));

static void loop_dump_aux PARAMS ((const struct loop *, FILE *, int));
static void loop_delete_insns PARAMS ((rtx, rtx));
static HOST_WIDE_INT remove_constant_addition PARAMS ((rtx *));
void debug_ivs PARAMS ((const struct loop *));
void debug_iv_class PARAMS ((const struct iv_class *));
void debug_biv PARAMS ((const struct induction *));
void debug_giv PARAMS ((const struct induction *));
void debug_loop PARAMS ((const struct loop *));
void debug_loops PARAMS ((const struct loops *));

typedef struct rtx_pair
{
  rtx r1;
  rtx r2;
} rtx_pair;

typedef struct loop_replace_args
{
  rtx match;
  rtx replacement;
  rtx insn;
} loop_replace_args;

/* Nonzero iff INSN is between START and END, inclusive.  */
#define INSN_IN_RANGE_P(INSN, START, END)	¥
  (INSN_UID (INSN) < max_uid_for_loop		¥
   && INSN_LUID (INSN) >= INSN_LUID (START)	¥
   && INSN_LUID (INSN) <= INSN_LUID (END))

/* Indirect_jump_in_function is computed once per function.  */
static int indirect_jump_in_function;
static int indirect_jump_in_function_p PARAMS ((rtx));

static int compute_luids PARAMS ((rtx, rtx, int));

static int biv_elimination_giv_has_0_offset PARAMS ((struct induction *,
						     struct induction *,
						     rtx));

/* Benefit penalty, if a giv is not replaceable, i.e. must emit an insn to
   copy the value of the stre