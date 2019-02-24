/* Allocate registers within a basic block, for GNU compiler.
   Copyright (C) 1987, 1988, 1991, 1993, 1994, 1995, 1996, 1997, 1998,
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

/* Allocation of hard register numbers to pseudo registers is done in
   two passes.  In this pass we consider only regs that are born and
   die once within one basic block.  We do this one basic block at a
   time.  Then the next pass allocates the registers that remain.
   Two passes are used because this pass uses methods that work only
   on linear code, but that do a better job than the general methods
   used in global_alloc, and more quickly too.

   The assignments made are recorded in the vector reg_renumber
   whose space is allocated here.  The rtl code itself is not altered.

   We assign each instruction in the basic block a number
   which is its order from the beginning of the block.
   Then we can represent the lifetime of a pseudo register with
   a pair of numbers, and check for conflicts easily.
   We can record the availability of hard registers with a
   HARD_REG_SET for each instruction.  The HARD_REG_SET
   contains 0 or 1 for each hard reg.

   To avoid register shuffling, we tie registers together when one
   dies by being copied into another, or dies in an instruction that
   does arithmetic to produce another.  The tied registers are
   allocated as one.  Registers with different reg class preferences
   can never be tied unless the class preferred by one is a subclass
   of the one preferred by the other.

   Tying is represented with "quantity numbers".
   A non-tied register is given a new quantity number.
   Tied registers have the same quantity number.

   We have provision to exempt registers, even when they are contained
   within the block, that can be tied to others that are not contained in it.
   This is so that global_alloc could process them both and tie them then.
   But this is currently disabled since tying in global_alloc is not
   yet implemented.  */

/* Pseudos allocated here can be reallocated by global.c if the hard register
   is used as a spill register.  Currently we don't allocate such pseudos
   here if their preferred class is likely to be used by spills.  */

#include "config.h"
#include "system.h"
#include "rtl.h"
#include "tm_p.h"
#include "flags.h"
#include "hard-reg-set.h"
#include "basic-block.h"
#include "regs.h"
#include "function.h"
#include "insn-config.h"
#include "insn-attr.h"
#include "recog.h"
#include "output.h"
#include "toplev.h"
#include "except.h"
#include "integrate.h"

/* Next quantity number available for allocation.  */

static int next_qty;

/* Information we maintain about each quantity.  */
struct qty
{
  /* The number of refs to quantity Q.  */

  int n_refs;

  /* The frequency of uses of quantity Q.  */

  int freq;

  /* Insn number (counting from head of basic block)
     where quantity Q was born.  -1 if birth has not been recorded.  */

  int birth;

  /* Insn number (counting from head of basic block)
     where given quantity died.  Due to the way tying is done,
     and the fact that we consider in this pass only regs that die but once,
     a quantity can die only once.  Each quantity's life span
     is a set of consecutive insns.  -1 if death has not been recorded.  */

  int death;

  /* Number of words needed to hold the data in given quantity.
     This depends on its machine mode.  It is used for these purposes:
     1. It is used in computing the relative importances of qtys,
	which determines the order in which we look for regs for them.
     2. It is used in rules that prevent tying several registers of
	different sizes in a way that is geometrically impossible
	(see combine_regs).  */

  int size;

  /* Number of times a reg tied to given qty lives across a CALL_INSN.  */

  int n_calls_crossed;

  /* The register number of one pseudo register whose reg_qty value is Q.
     This register should be the head of the chain
     maintained in reg_next_in_qty.  */

  int first_reg;

  /* Reg class contained in (smaller than) the preferred classes of all
     the pseudo regs that are tied in given quantity.
     This is the preferred class for allocating that quantity.  */

  enum reg_class min_class;

  /* Register class within which we allocate given qty if we can't get
     its preferred class.  */

  enum reg_class alternate_class;

  /* This holds the mode of the registers that are tied to given qty,
     or VOIDmode if registers with differing modes are tied together.  */

  enum machine_mode mode;

  /* the hard reg number chosen for given quantity,
     or -1 if none was found.  */

  short phys_reg;

  /* Nonzero if this quantity has been used in a SUBREG in some
     way that is illegal.  */

  char changes_mode;

};

static struct qty *qty;

/* These fields are kept separately to speedup their clearing.  */

/* We maintain two hard register sets that indicate suggested hard registers
   for each quantity.  The first, phys_copy_sugg, contains hard registers
   that are tied to the quantity by a simple copy.  The second contains all
   hard registers that are tied to the quantity via an arithmetic operation.

   The former register set is given priority for allocation.  This tends to
   eliminate copy insns.  */

/* Element Q is a set of hard registers that are suggested for quantity Q by
   copy insns.  */

static HARD_REG_SET *qty_phys_copy_sugg;

/* Element Q is a set of hard registers that are suggested for quantity Q by
   arithmetic insns.  */

static HARD_REG_SET *qty_phys_sugg;

/* Element Q is the number of suggested registers in qty_phys_copy_sugg.  */

static short *qty_phys_num_copy_sugg;

/* Element Q is the number of suggested registers in qty_phys_sugg.  */

static short *qty_phys_num_sugg;

/* If (REG N) has been assigned a quantity number, is a register number
   of another register assigned the same quantity number, or -1 for the
   end of the chain.  qty->first_reg point to the head of this chain.  */

static int *reg_next_in_qty;

/* reg_qty[N] (where N is a pseudo reg number) is the qty number of that reg
   if it is >= 0,
   of -1 if this register cannot be allocated by local-alloc,
   or -2 if not known yet.

   Note that if we see a use or death of pseudo register N with
   reg_qty[N] == -2, register N must be local to the current block.  If
   it were used in more than one block, we would have reg_qty[N] == -1.
   This relies on the fact that if reg_basic_block[N] is >= 0, register N
   will not appear in any other block.  We save a considerable number of
   tests by exploiting this.

   If N is < FIRST_PSEUDO_REGISTER, reg_qty[N] is undefined and should not
   be referenced.  */

static int *reg_qty;

/* The offset (in words) of register N within its quantity.
   This can be nonzero if register N is SImode, and has been tied
   to a subreg of a DImode register.  */

static char *reg_offset;

/* Vector of substitutions of register numbers,
   used to map pseudo regs into hardware regs.
   This is set up as a result of register allocation.
   Element N is the hard reg assigned to pseudo reg N,
   or is -1 if no hard reg was assigned.
   If N is a hard reg number, element N is N.  */

short *reg_renumber;

/* Set of hard registers live at the current point in the scan
   of the instructions in a basic block.  */

static HARD_REG_SET regs_live;

/* Each set of hard registers indicates registers live at a particular
   point in the basic block.  For N even, regs_live_at[N] says which
   hard registers are needed *after* insn N/2 (i.e., they may not
   conflict with the outputs of insn N/2 or the inputs of insn N/2 + 1.

   If an object is to conflict with the inputs of insn J but not the
   outputs of insn J + 1, we say it is born at index J*2 - 1.  Similarly,
   if it is to conflict with the outputs of insn J but not the inputs of
   insn J + 1, it is said to die at index J*2 + 1.  */

static HARD_REG_SET *regs_live_at;

/* Communicate local vars `insn_number' and `insn'
   from `block_alloc' to `reg_is_set', `wipe_dead_reg', and `alloc_qty'.  */
static int this_insn_number;
static rtx this_insn;

struct equivalence
{
  /* Set when an attempt should be made to replace a register
     with the associated src_p entry.  */

  char replace;

  /* Set when a REG_EQUIV note is found or created.  Use to
     keep track of what memory accesses might be created later,
     e.g. by reload.  */

  rtx replacement;

  rtx *src_p;

  /* Loop depth is used to recognize equivalences which appear
     to be present within the same loop (or in an inner loop).  */

  int loop_depth;

  /* The list of each instruction which initializes this register.  */

  rtx init_insns;
};

/* reg_equiv[N] (where N is a pseudo reg number) is the equivalence
   structure for that register.  */

static struct equivalence *reg_equiv;

/* Nonzero if we recorded an equivalence for a LABEL_REF.  */
static int recorded_label_ref;

static void alloc_qty		PARAMS ((int, enum machine_mode, int, int));
static void validate_equiv_mem_from_store PARAMS ((rtx, rtx, void *));
static int validate_equiv_mem	PARAMS ((rtx, rtx, rtx));
static int equiv_init_varies_p  PARAMS ((rtx));
static int equiv_init_movable_p PARAMS ((rtx, int));
static int contains_replace_regs PARAMS ((rtx));
static int memref_referenced_p	PARAMS ((rtx, rtx));
static int memref_used_between_p PARAMS ((rtx, rtx, rtx));
static void update_equiv_regs	PARAMS ((void));
static void no_equiv		PARAMS ((rtx, rtx, void *));
static void block_alloc		PARAMS ((int));
static int qty_sugg_compare    	PARAMS ((int, int));
static int qty_sugg_compare_1	PARAMS ((const PTR, const PTR));
static int qty_compare    	PARAMS ((int, int));
static int qty_compare_1	PARAMS ((const PTR, const PTR));
static int combine_regs		PARAMS ((rtx, rtx, int, int, rtx, int));
static int reg_meets_class_p	PARAMS ((int, enum reg_class));
static void update_qty_class	PARAMS ((int, int));
static void reg_is_set		PARAMS ((rtx, rtx, void *));
static void reg_is_born		PARAMS ((rtx, int));
static void wipe_dead_reg	PARAMS ((rtx, int));
static int find_free_reg	PARAMS ((enum reg_class, enum machine_mode,
				       int, int, int, int, int));
static void mark_life		PARAMS ((int, enum machine_mode, int));
static void post_mark_life	PARAMS ((int, enum machine_mode, int, int, int));
static int no_conflict_p	PARAMS ((rtx, rtx, rtx));
static int requires_inout	PARAMS ((const char *));

/* Allocate a new quantity (new within current basic block)
   for register number REGNO which is born at index BIRTH
   within the block.  MODE and SIZE are info on reg REGNO.  */

static void
alloc_qty (regno, mode, size, birth)
     int regno;
     enum machine_mode mode;
     int size, birth;
{
  int qtyno = next_qty++;

  reg_qty[regno] = qtyno;
  reg_offset[regno] = 0;
  reg_next_in_qty[regno] = -1;

  qty[qtyno].first_reg = regno;
  qty[qtyno].size = size;
  qty[qtyno].mode = mode;
  qty[qtyno].birth = birth;
  qty[qtyno].n_calls_crossed = REG_N_CALLS_CROSSED (regno);
  qty[qtyno].min_class = reg_preferred_class (regno);
  qty[qtyno].alternate_class = reg_alternate_class (regno);
  qty[qtyno].n_refs = REG_N_REFS (regno);
  qty[qtyno].freq = REG_FREQ (regno);
  qty[qtyno].changes_mode = REG_CHANGES_MODE (regno);
}

/* Main entry point of this file.  */

int
local_alloc ()
{
  int b, i;
  int max_qty;

  /* We need to keep track of whether or not we recorded a LABEL_REF so
     that we know if the jump optimizer needs to be rerun.  */
  recorded_label_ref = 0;

  /* Leaf functions and non-leaf functions have different needs.
     If defined, let the machine say what kind of ordering we
     should use.  */
#ifdef ORDER_REGS_FOR_LOCAL_ALLOC
  ORDER_REGS_FOR_LOCAL_ALLOC;
#endif

  /* Promote REG_EQUAL notes to REG_EQUIV notes and adjust status of affected
     registers.  */
  update_equiv_regs ();

  /* This sets the maximum number of quantities we can have.  Quantity
     numbers start at zero and we can have one for each pseudo.  */
  max_qty = (max_regno - FIRST_PSEUDO_REGISTER);

  /* Allocate vectors of temporary data.
     See the declarations of these variables, above,
     for what they mean.  */

  qty = (struct qty *) xmalloc (max_qty * sizeof (struct qty));
  qty_phys_copy_sugg
    = (HARD_REG_SET *) xmalloc (max_qty * sizeof (HARD_REG_SET));
  qty_phys_num_copy_sugg = (short *) xmalloc (max_qty * sizeof (short));
  qty_phys_sugg = (HARD_REG_SET *) xmalloc (max_qty * sizeof (HARD_REG_SET));
  qty_phys_num_sugg = (short *) xmalloc (max_qty * sizeof (short));

  reg_qty = (int *) xmalloc (max_regno * sizeof (int));
  reg_offset = (char *) xmalloc (max_regno * sizeof (char));
  reg_next_in_qty = (int *) xmalloc (max_regno * sizeof (int));

  /* Determine which pseudo-registers can be allocated by local-alloc.
     In general, these are the registers used only in a single block and
     which only die once.

     We need not be concerned with which block actually uses the register
     since we will never see it outside that block.  */

  for (i = FIRST_PSEUDO_REGISTER; i < max_regno; i++)
    {
      if (REG_BASIC_BLOCK (i) >= 0 && REG_N_DEATHS (i) == 1)
	reg_qty[i] = -2;
      else
	reg_qty[i] = -1;
    }

  /* Force loop below to initialize entire quantity array.  */
  next_qty = max_qty;

  /* Allocate each block's local registers, block by block.  */

  for (b = 0; b < n_basic_blocks; b++)
    {
      /* NEXT_QTY indicates which elements of the `qty_...'
	 vectors might need to be initialized because they were used
	 for the previous block; it is set to the entire array before
	 block 0.  Initialize those, with explicit loop if there are few,
	 else with bzero and bcopy.  Do not initialize vectors that are
	 explicit set by `alloc_qty'.  */

      if (next_qty < 6)
	{
	  for (i = 0; i < next_qty; i++)
	    {
	      CLEAR_HARD_REG_SET (qty_phys_copy_sugg[i]);
	      qty_phys_num_copy_sugg[i] = 0;
	      CLEAR_HARD_REG_SET (qty_phys_sugg[i]);
	      qty_phys_num_sugg[i] = 0;
	    }
	}
      else
	{
#define CLEAR(vector)  ¥
	  memset ((char *) (vector), 0, (sizeof (*(vector))) * next_qty);

	  CLEAR (qty_phys_copy_sugg);
	  CLEAR (qty_phys_num_copy_sugg);
	  CLEAR (qty_phys_sugg);
	  CLEAR (qty_phys_num_sugg);
	}

      next_qty = 0;

      block_alloc (b);
    }

  free (qty);
  free (qty_phys_copy_sugg);
  free (qty_phys_num_copy_sugg);
  free (qty_phys_sugg);
  free (qty_phys_num_sugg);

  free (reg_qty);
  free (reg_offset);
  free (reg_next_in_qty);

  return recorded_label_ref;
}

/* Used for communication between the following two functions: contains
   a MEM that we wish to ensure remains unchanged.  */
static rtx equiv_mem;

/* Set nonzero if EQUIV_MEM is modified.  */
static int equiv_mem_modified;

/* If EQUIV_MEM is modified by modifying DEST, indicate that it is modified.
   Called via note_stores.  */

static void
validate_equiv_mem_from_store (dest, set, data)
     rtx dest;
     rtx set ATTRIBUTE_UNUSED;
     void *data ATTRIBUTE_UNUSED;
{
  if ((GET_CODE (dest) == REG
       && reg_overlap_mentioned_p (dest, equiv_mem))
      || (GET_CODE (dest) == MEM
	  && true_dependence (dest, VOIDmode, equiv_mem, rtx_varies_p)))
    equiv_mem_modified = 1;
}

/* 