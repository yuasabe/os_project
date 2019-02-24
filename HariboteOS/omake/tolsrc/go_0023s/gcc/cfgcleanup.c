/* Control flow optimization code for GNU compiler.
   Copyright (C) 1987, 1988, 1992, 1993, 1994, 1995, 1996, 1997, 1998,
   1999, 2000, 2001, 2002 Free Software Foundation, Inc.

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

/* This file contains optimizer of the control flow.  The main entrypoint is
   cleanup_cfg.  Following optimizations are performed:

   - Unreachable blocks removal
   - Edge forwarding (edge to the forwarder block is forwarded to it's
     successor.  Simplification of the branch instruction is performed by
     underlying infrastructure so branch can be converted to simplejump or
     eliminated).
   - Cross jumping (tail merging)
   - Conditional jump-around-simplejump simplification
   - Basic block merging.  */

#include "config.h"
#include "system.h"
#include "rtl.h"
#include "hard-reg-set.h"
#include "basic-block.h"
#include "timevar.h"
#include "output.h"
#include "insn-config.h"
#include "flags.h"
#include "recog.h"
#include "toplev.h"
#include "cselib.h"
#include "tm_p.h"
#include "target.h"

/* !kawai! */
#include "../include/obstack.h"
/* end of !kawai! */

/* cleanup_cfg maintains following flags for each basic block.  */

enum bb_flags
{
    /* Set if life info needs to be recomputed for given BB.  */
    BB_UPDATE_LIFE = 1,
    /* Set if BB is the forwarder block to avoid too many
       forwarder_block_p calls.  */
    BB_FORWARDER_BLOCK = 2
};

#define BB_FLAGS(BB) (enum bb_flags) (BB)->aux
#define BB_SET_FLAG(BB, FLAG) ¥
  (BB)->aux = (void *) (long) ((enum bb_flags) (BB)->aux | (FLAG))
#define BB_CLEAR_FLAG(BB, FLAG) ¥
  (BB)->aux = (void *) (long) ((enum bb_flags) (BB)->aux & ‾(FLAG))

#define FORWARDER_BLOCK_P(BB) (BB_FLAGS (BB) & BB_FORWARDER_BLOCK)

static bool try_crossjump_to_edge	PARAMS ((int, edge, edge));
static bool try_crossjump_bb		PARAMS ((int, basic_block));
static bool outgoing_edges_match	PARAMS ((int,
						 basic_block, basic_block));
static int flow_find_cross_jump		PARAMS ((int, basic_block, basic_block,
						 rtx *, rtx *));
static bool insns_match_p		PARAMS ((int, rtx, rtx));

static bool delete_unreachable_blocks	PARAMS ((void));
static bool label_is_jump_target_p	PARAMS ((rtx, rtx));
static bool tail_recursion_label_p	PARAMS ((rtx));
static void merge_blocks_move_predecessor_nojumps PARAMS ((basic_block,
							  basic_block));
static void merge_blocks_move_successor_nojumps PARAMS ((basic_block,
							basic_block));
static bool merge_blocks		PARAMS ((edge,basic_block,basic_block,
						 int));
static bool try_optimize_cfg		PARAMS ((int));
static bool try_simplify_condjump	PARAMS ((basic_block));
static bool try_forward_edges		PARAMS ((int, basic_block));
static edge thread_jump			PARAMS ((int, edge, basic_block));
static bool mark_effect			PARAMS ((rtx, bitmap));
static void notice_new_block		PARAMS ((basic_block));
static void update_forwarder_flag	PARAMS ((basic_block));

/* Set flags for newly created block.  */

static void
notice_new_block (bb)
     basic_block bb;
{
  if (!bb)
    return;

  BB_SET_FLAG (bb, BB_UPDATE_LIFE);
  if (forwarder_block_p (bb))
    BB_SET_FLAG (bb, BB_FORWARDER_BLOCK);
}

/* Recompute forwarder flag after block has been modified.  */

static void
update_forwarder_flag (bb)
     basic_block bb;
{
  if (forwarder_block_p (bb))
    BB_SET_FLAG (bb, BB_FORWARDER_BLOCK);
  else
    