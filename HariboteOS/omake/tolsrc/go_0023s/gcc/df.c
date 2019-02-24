/* Dataflow support routines.
   Copyright (C) 1999, 2000, 2001, 2002 Free Software Foundation, Inc.
   Contributed by Michael P. Hayes (m.hayes@elec.canterbury.ac.nz,
                                    mhayes@redhat.com)

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
02111-1307, USA.


OVERVIEW:

This file provides some dataflow routines for computing reaching defs,
upward exposed uses, live variables, def-use chains, and use-def
chains.  The global dataflow is performed using simple iterative
methods with a worklist and could be sped up by ordering the blocks
with a depth first search order.

A `struct ref' data structure (ref) is allocated for every register
reference (def or use) and this records the insn and bb the ref is
found within.  The refs are linked together in chains of uses and defs
for each insn and for each register.  Each ref also has a chain field
that links all the use refs for a def or all the def refs for a use.
This is used to create use-def or def-use chains.


USAGE:

Here's an example of using the dataflow routines.

      struct df *df;

      df = df_init ();

      df_analyse (df, 0, DF_ALL);

      df_dump (df, DF_ALL, stderr);

      df_finish (df);


df_init simply creates a poor man's object (df) that needs to be
passed to all the dataflow routines.  df_finish destroys this
object and frees up any allocated memory.

df_analyse performs the following:

1. Records defs and uses by scanning the insns in each basic block
   or by scanning the insns queued by df_insn_modify.
2. Links defs and uses into insn-def and insn-use chains.
3. Links defs and uses into reg-def and reg-use chains.
4. Assigns LUIDs to each insn (for modified blocks).
5. Calculates local reaching definitions.
6. Calculates global reaching definitions.
7. Creates use-def chains.
8. Calculates local reaching uses (upwards exposed uses).
9. Calculates global reaching uses.
10. Creates def-use chains.
11. Calculates local live registers.
12. Calculates global live registers.
13. Calculates register lifetimes and determines local registers.


PHILOSOPHY:

Note that the dataflow information is not updated for every newly
deleted or created insn.  If the dataflow information requires
updating then all the changed, new, or deleted insns needs to be
marked with df_insn_modify (or df_insns_modify) either directly or
indirectly (say through calling df_insn_delete).  df_insn_modify
marks all the modified insns to get processed the next time df_analyse
 is called.

Beware that tinkering with insns may invalidate the dataflow information.
The philosophy behind these routines is that once the dataflow
information has been gathered, the user should store what they require
before they tinker with any insn.  Once a reg is replaced, for example,
then the reg-def/reg-use chains will point to the wrong place.  Once a
whole lot of changes have been made, df_analyse can be called again
to update the dataflow information.  Currently, this is not very smart
with regard to propagating changes to the dataflow so it should not
be called very often.


DATA STRUCTURES:

The basic object is a REF (reference) and this may either be a DEF
(definition) or a USE of a register.

These are linked into a variety of lists; namely reg-def, reg-use,
  insn-def, insn-use, def-use, and use-def lists.  For example,
the reg-def lists contain all the refs that define a given register
while the insn-use lists contain all the refs used by an insn.

Note that the reg-def and reg-use chains are generally short (except for the
hard registers) and thus it is much faster to search these chains
rather than searching the def or use bitmaps.

If the insns are in SSA form then the reg-def and use-def lists
should only contain the single defining ref.

TODO:

1) Incremental dataflow analysis.

Note that if a loop invariant insn is hoisted (or sunk), we do not
need to change the def-use or use-def chains.  All we have to do is to
change the bb field for all the associated defs and uses and to
renumber the LUIDs for the original and new basic blocks of the insn.

When shadowing loop mems we create new uses and defs for new pseudos
so we do not affect the existing dataflow information.

My current strategy is to queue up all modified, created, or deleted
insns so when df_analyse is called we can easily determine all the new
or deleted refs.  Currently the global dataflow information is
recomputed from scratch but this could be propagated more efficiently.

2) Improved global data flow computation using depth first search.

3) Reduced memory requirements.

We could operate a pool of ref structures.  When a ref is deleted it
gets returned to the pool (say by linking on to a chain of free refs).
This will require a pair of bitmaps for defs and uses so that we can
tell which ones have been changed.  Alternatively, we could
periodically squeeze the def and use tables and associated bitmaps and
renumber the def and use ids.

4) Ordering of reg-def and reg-use lists.

Should the first entry in the def list be the first def (within a BB)?
Similarly, should the first entry in the use list be the last use
(within a BB)?

5) Working with a sub-CFG.

Often the whole CFG does not need to be analysed, for example,
when optimising a loop, only certain registers are of interest.
Perhaps there should be a bitmap argument to df_analyse to specify
 which registers should be analysed?   */

#define HANDLE_SUBREG 

/* !kawai! */
#include "config.h"
#include "system.h"
#include "rtl.h"
#include "tm_p.h"
#include "insn-config.h"
#include "recog.h"
#include "function.h"
#include "regs.h"
#include "../include/obstack.h"
#include "hard-reg-set.h"
#include "basic-block.h"
#include "sbitmap.h"
#include "bitmap.h"
#include "df.h"
#include "../include/fibheap.h"
/* end of !kawai! */

#define FOR_ALL_BBS(BB, CODE)					¥
do {								¥
  int node_;							¥
  for (node_ = 0; node_ < n_basic_blocks; node_++)		¥
    {(BB) = BASIC_BLOCK (node_); CODE;};} while (0)

#define FOR_EACH_BB_IN_BITMAP(BITMAP, MIN, BB, CODE)		¥
do {								¥
  unsigned int node_;						¥
  EXECUTE_IF_SET_IN_BITMAP (BITMAP, MIN, node_, 		¥
    {(BB) = BASIC_BLOCK (node_); CODE;});} while (0)

#define FOR_EACH_BB_IN_BITMAP_REV(BITMAP, MIN, BB, CODE)	¥
do {								¥
  unsigned int node_;						¥
  EXECUTE_IF_SET_IN_BITMAP_REV (BITMAP, node_, 		¥
    {(BB) = BASIC_BLOCK (node_); CODE;});} while (0)

#define FOR_EACH_BB_IN_SBITMAP(BITMAP, MIN, BB, CODE)           ¥
do {                                                            ¥
  unsigned int node_;                                           ¥
  EXECUTE_IF_SET_IN_SBITMAP (BITMAP, MIN, node_,                ¥
    {(BB) = BASIC_BLOCK (node_); CODE;});} while (0)

#define obstack_chunk_alloc xmalloc
#define obstack_chunk_free free

static struct obstack df_ref_obstack;
static struct df *ddf;

static void df_reg_table_realloc PARAMS((struct df *, int));
#if 0
static void df_def_table_realloc PARAMS((struct df *, int));
#endif
static void df_insn_table_realloc PARAMS((struct df *, int));
static void df_bitmaps_alloc PARAMS((struct df *, int));
static void df_bitmaps_free PARAMS((struct df *, int));
static void df_free PARAMS((struct df *));
static void df_alloc PARAMS((struct df *, int));

static rtx df_reg_clobber_gen PARAMS((unsigned int));
static rtx df_reg_use_gen PARAMS((unsigned int));

static inline struct df_link *df_link_create PARAMS((struct ref *,
						     struct df_link *));
static struct df_link *df_ref_unlink PARAMS((struct df_link **, struct ref *));
static void df_def_unlink PARAMS((struct df *, struct ref *));
static void df_use_unlink PARAMS((struct df *, struct ref *));
static void df_insn_refs_unlink PARAMS ((struct df *, basic_block, rtx));
#if 0
static void df_bb_refs_unlink PARAMS ((struct df *, basic_block));
static void df_refs_unlink PARAMS ((struct df *, bitmap));
#endif

static struct ref *df_ref_create PARAMS((struct df *,
					 rtx, rtx *, rtx,
					 enum df_ref_type, enum df_ref_flags));
static void df_ref_record_1 PARAMS((struct df *, rtx, rtx *,
				    rtx, enum df_ref_type,
				    enum df_ref_flags));
static void df_ref_record PARAMS((struct df *, rtx, rtx *,
				  rtx, enum df_ref_type,
				  enum df_ref_flags));
static void df_def_record_1 PARAMS((struct df *, rtx, basic_block, rtx));
static void df_defs_record PARAMS((struct df *, rtx, basic_block, rtx));
static void df_uses_record PARAMS((struct df *, rtx *,
				   enum df_ref_type, basic_block, rtx,
				   enum df_ref_flags));
static void df_insn_refs_record PARAMS((struct df *, basic_block, rtx));
static void df_bb_refs_record PARAMS((struct df *, basic_block));
static void df_refs_record PARAMS((struct df *, bitmap));

static void df_bb_reg_def_chain_create PARAMS((struct df *, basic_block));
static void df_reg_def_chain_create PARAMS((struct df *, bitmap));
static void df_bb_reg_use_chain_create PARAMS((struct df *, basic_block));
static void df_reg_use_chain_create PARAMS((struct df *, bitmap));
static void df_bb_du_chain_create PARAMS((struct df *, basic_block, bitmap));
static void df_du_chain_create PARAMS((struct df *, bitmap));
static void df_bb_ud_chain_create PARAMS((struct df *, basic_block));
static void df_ud_chain_create PARAMS((struct df *, bitmap));
static void df_bb_rd_local_compute PARAMS((struct df *, basic_block));
static void df_rd_local_compute PARAMS((struct df *, bitmap));
static void df_bb_ru_local_compute PARAMS((struct df *, basic_block));
static void df_ru_local_compute PARAMS((struct df *, bitmap));
static void df_bb_lr_local_compute PARAMS((struct df *, basic_block));
static void df_lr_local_compute PARAMS((struct df *, bitmap));
static void df_bb_reg_info_compute PARAMS((struct df *, basic_block, bitmap));
static void df_reg_info_compute PARAMS((struct df *, bitmap));

static int df_bb_luids_set PARAMS((struct df *df, basic_block));
static int df_luids_set PARAMS((struct df *df, bitmap));

static int df_modified_p PARAMS ((struct df *, bitmap));
static int df_refs_queue PARAMS ((struct df *));
static int df_refs_process PARAMS ((struct df *));
static int df_bb_refs_update PARAMS ((struct df *, basic_block));
static int df_refs_update PARAMS ((struct df *));
static void df_analyse_1 PARAMS((struct df *, bitmap, int, int));

static void df_insns_modify PARAMS((struct df *, basic_block,
				    rtx, rtx));
static int df_rtx_mem_replace PARAMS ((rtx *, void *));
static int df_rtx_reg_replace PARAMS ((rtx *, void *));
void df_refs_reg_replace PARAMS ((struct df *, bitmap,
					 struct df_link *, rtx, rtx));

static int df_def_dominates_all_uses_p PARAMS((struct df *, struct ref *def));
static int df_def_dominates_uses_p PARAMS((struct df *,
					   struct ref *def, bitmap));
static struct ref *df_bb_regno_last_use_find PARAMS((struct df *, basic_block,
						     unsigned int));
static struct ref *df_bb_regno_first_def_find PARAMS((struct df *, basic_block,
						      unsigned int));
static struct ref *df_bb_insn_regno_last_use_find PARAMS((struct df *,
							  basic_block,
							  rtx, unsigned int));
static struct ref *df_bb_insn_regno_first_def_find PARAMS((struct df *,
							   basic_block,
							   rtx, unsigned int));

static void df_chain_dump PARAMS((struct df_link *, FILE *file));
static void df_chain_dump_regno PARAMS((struct df_link *, FILE *file));
static void df_regno_debug PARAMS ((struct df *, unsigned int, FILE *));
static void df_ref_debug PARAMS ((struct df *, struct ref *, FILE *));
static void df_rd_transfer_function PARAMS ((int, int *, bitmap, bitmap, 
					     bitmap, bitmap, void *));
static void df_ru_transfer_function PARAMS ((int, int *, bitmap, bitmap, 
					     bitmap, bitmap, void *));
static void df_lr_transfer_function PARAMS ((int, int *, bitmap, bitmap, 
					     bitmap, bitmap, void *));
static void hybrid_search_bitmap PARAMS ((basic_block, bitmap *, bitmap *, 
					  bitmap *, bitmap *, enum df_flow_dir, 
					  enum df_confluence_op, 
					  transfer_function_bitmap, 
					  sbitmap, sbitmap, void *));
static void hybrid_search_sbitmap PARAMS ((basic_block, sbitmap *, sbitmap *,
					   sbitmap *, sbitmap *, enum df_flow_dir,
					   enum df_confluence_op,
					   transfer_function_sbitmap,
					   sbitmap, sbitmap, void *));
static inline bool read_modify_subreg_p PARAMS ((rtx));


/* Local memory allocation/deallocation routines.  */


/* Increase the insn info table by SIZE more elements.  */
static void
df_insn_table_realloc (df, size)
     struct df *df;
     int size;
{
  /* Make table 25 percent larger by default.  */
  if (! size)
    size = df->insn_size / 4;

  size += df->insn_size;

  df->insns = (struct insn_info *)
    xrealloc (df->insns, size * sizeof (struct insn_info));

  memset (df->insns + df->insn_size, 0,
	  (size - df->insn_size) * sizeof (struct insn_info));

  df->insn_size = size;

  if (! df->insns_modified)
    {
      df->insns_modified = BITMAP_XMALLOC ();
      bitmap_zero (df->insns_modified);
    }
}


/* Increase the reg info table by SIZE more elements.  */
static void
df_reg_table_realloc (df, size)
     struct df *df;
     int size;
{
  /* Make table 25 percent larger by default.  */
  if (! size)
    size = df->reg_size / 4;

  size += df->reg_size;

  df->regs = (struct reg_info *)
    xrealloc (df->regs, size * sizeof (struct reg_info));

  /* Zero the new entries.  */
  memset (df->regs + df->reg_size, 0,
	  (size - df->reg_size) * sizeof (struct reg_info));

  df->reg_size = size;
}


#if 0
/* Not currently used.  */
static void
df_def_table_realloc (df, size)
     struct df *df;
     int size;
{
  int i;
  struct ref *refs;

  /* Make table 25 percent larger by default.  */
  if (! size)
    size = df->def_size / 4;

  df->def_size += size;
  df->defs = xrealloc (df->defs,
		       df->def_size * sizeof (*df->defs));

  /* Allocate a new block of memory and link into list of blocks
     that will need to be freed later.  */

  refs = xmalloc (size * sizeof (*refs));

  /* Link all the new refs together, overloading the chain field.  */
  for (i = 0; i < size - 1; i++)
      refs[i].chain = (struct df_link *)(refs + i + 1);
  refs[size - 1].chain = 0;
}
#endif



/* Allocate bitmaps for each basic block.  */
static void
df_bitmaps_alloc (df, flags)
     struct df *df;
     int flags;
{
  unsigned int i;
  int dflags = 0;

  /* Free the bitmaps if they need resizing.  */
  if ((flags & DF_LR) && df->n_regs < (unsigned int)max_reg_num ())
    dflags |= DF_LR | DF_RU;
  if ((flags & DF_RU) && df->n_uses < df->use_id)
    dflags |= DF_RU;
  if ((flags & DF_RD) && df->n_defs < df->def_id)
    dflags |= DF_RD;

  if (dflags)
    df_bitmaps_free (df, dflags);

  df->n_defs = df->def_id;
  df->n_uses = df->use_id;

  for (i = 0; i < df->n_bbs; i++)
    {
      basic_block bb = BASIC_BLOCK (i);
      struct bb_info *bb_info = DF_BB_INFO (df, bb);

      if (flags & DF_RD && ! bb_info->rd_in)
	{
	  /* Allocate bitmaps for reaching definitions.  */
	  bb_info->rd_kill = BITMAP_XMALLOC ();
	  bitmap_zero (bb_info->rd_kill);
	  bb_info->rd_gen = BITMAP_XMALLOC ();
	  bitmap_zero (bb_info->rd_gen);
	  bb_info->rd_in = BITMAP_XMALLOC ();
	  bb_info->rd_out = BITMAP_XMALLOC ();
	  bb_info->rd_valid = 0;
	}

      if (flags & DF_RU && ! bb_info->ru_in)
	{
	  /* Allocate bitmaps for upward exposed uses.  */
	  bb_info->r