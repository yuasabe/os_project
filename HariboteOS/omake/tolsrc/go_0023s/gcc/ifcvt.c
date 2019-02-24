/* If-conversion support.
   Copyright (C) 2000, 2001 Free Software Foundation, Inc.

   This file is part of GCC.

   GCC is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   GCC is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License
   along with GCC; see the file COPYING.  If not, write to the Free
   Software Foundation, 59 Temple Place - Suite 330, Boston, MA
   02111-1307, USA.  */

#include "config.h"
#include "system.h"

#include "rtl.h"
#include "regs.h"
#include "function.h"
#include "flags.h"
#include "insn-config.h"
#include "recog.h"
#include "except.h"
#include "hard-reg-set.h"
#include "basic-block.h"
#include "expr.h"
#include "real.h"
#include "output.h"
#include "toplev.h"
#include "tm_p.h"


#ifndef HAVE_conditional_execution
#define HAVE_conditional_execution 0
#endif
#ifndef HAVE_conditional_move
#define HAVE_conditional_move 0
#endif
#ifndef HAVE_incscc
#define HAVE_incscc 0
#endif
#ifndef HAVE_decscc
#define HAVE_decscc 0
#endif
#ifndef HAVE_trap
#define HAVE_trap 0
#endif
#ifndef HAVE_conditional_trap
#define HAVE_conditional_trap 0
#endif

#ifndef MAX_CONDITIONAL_EXECUTE
#define MAX_CONDITIONAL_EXECUTE   (BRANCH_COST + 1)
#endif

#define NULL_EDGE	((struct edge_def *)NULL)
#define NULL_BLOCK	((struct basic_block_def *)NULL)

/* # of IF-THEN or IF-THEN-ELSE blocks we looked at  */
static int num_possible_if_blocks;

/* # of IF-THEN or IF-THEN-ELSE blocks were converted to conditional
   execution.  */
static int num_updated_if_blocks;

/* # of basic blocks that were removed.  */
static int num_removed_blocks;

/* True if life data ok at present.  */
static bool life_data_ok;

/* The post-dominator relation on the original block numbers.  */
static sbitmap *post_dominators;

/* Forward references.  */
static int count_bb_insns		PARAMS ((basic_block));
static rtx first_active_insn		PARAMS ((basic_block));
static int last_active_insn_p		PARAMS ((basic_block, rtx));
static int seq_contains_jump		PARAMS ((rtx));

static int cond_exec_process_insns	PARAMS ((rtx, rtx, rtx, rtx, int));
static rtx cond_exec_get_condition	PARAMS ((rtx));
static int cond_exec_process_if_block	PARAMS ((basic_block, basic_block,
						 basic_block, basic_block));

static rtx noce_get_condition		PARAMS ((rtx, rtx *));
static int noce_operand_ok		PARAMS ((rtx));
static int noce_process_if_block	PARAMS ((basic_block, basic_block,
						 basic_block, basic_block));

static int process_if_block		PARAMS ((basic_block, basic_block,
						 basic_block, basic_block));
static void merge_if_block		PARAMS ((basic_block, basic_block,
						 basic_block, basic_block));

static int find_if_header		PARAMS ((basic_block));
static int find_if_block		PARAMS ((basic_block, edge, edge));
static int find_if_case_1		PARAMS ((basic_block, edge, edge));
static int find_if_case_2		PARAMS ((basic_block, edge, edge));
static int find_cond_trap		PARAMS ((basic_block, edge, edge));
static rtx block_has_only_trap		PARAMS ((basic_block));
static int find_memory			PARAMS ((rtx *, void *));
static int dead_or_predicable		PARAMS ((basic_block, basic_block,
						 basic_block, basic_block, int));
static void noce_emit_move_insn		PARAMS ((rtx, rtx));

/* Abuse the basic_block AUX field to store the original block index,
   as well as a flag indicating that the block should be rescaned for
   life analysis.  */

#define SET_ORIG_INDEX(BB,I)	((BB)->aux = (void *)((size_t)(I) << 1))
#define ORIG_INDEX(BB)		((size_t)(BB)->aux >> 1)
#define SET_UPDATE_LIFE(BB)	((BB)->aux = (void *)((size_t)(BB)->aux | 1))
#define UPDATE_LIFE(BB)		((size_t)(B