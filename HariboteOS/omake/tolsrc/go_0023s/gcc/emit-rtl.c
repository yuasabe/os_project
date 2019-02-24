/* Emit RTL for the GNU C-Compiler expander.
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


/* Middle-to-low level generation of rtx code and insns.

   This file contains the functions `gen_rtx', `gen_reg_rtx'
   and `gen_label_rtx' that are the usual ways of creating rtl
   expressions for most purposes.

   It also has the functions for creating insns and linking
   them in the doubly-linked chain.

   The patterns of the insns are created by machine-dependent
   routines in insn-emit.c, which is generated automatically from
   the machine description.  These routines use `gen_rtx' to make
   the individual rtx's of the pattern; what is machine dependent
   is the kind of rtx's they make and what arguments they use.  */

/* !kawai! */
#include "config.h"
#include "system.h"
#include "toplev.h"
#include "rtl.h"
#include "tree.h"
#include "tm_p.h"
#include "flags.h"
#include "function.h"
#include "expr.h"
#include "regs.h"
#include "hard-reg-set.h"
#include "../include/hashtab.h"
#include "insn-config.h"
#include "recog.h"
#include "real.h"
#include "../include/obstack.h"
#include "bitmap.h"
#include "basic-block.h"
#include "ggc.h"
#include "debug.h"
#include "langhooks.h"
/* end of !kawai! */

/* Commonly used modes.  */

enum machine_mode byte_mode;	/* Mode whose width is BITS_PER_UNIT.  */
enum machine_mode word_mode;	/* Mode whose width is BITS_PER_WORD.  */
enum machine_mode double_mode;	/* Mode whose width is DOUBLE_TYPE_SIZE.  */
enum machine_mode ptr_mode;	/* Mode whose width is POINTER_SIZE.  */


/* This is *not* reset after each function.  It gives each CODE_LABEL
   in the entire compilation a unique label number.  */

static int label_num = 1;

/* Highest label number in current function.
   Zero means use the value of label_num instead.
   This is nonzero only when belatedly compiling an inline function.  */

static int last_label_num;

/* Value label_num had when set_new_first_and_last_label_number was called.
   If label_num has not changed since then, last_label_num is valid.  */

static int base_label_num;

/* Nonzero means do not generate NOTEs for source line numbers.  */

static int no_line_numbers;

/* Commonly used rtx's, so that we only need space for one copy.
   These are initialized once for the entire compilation.
   All of these except perhaps the floating-point CONST_DOUBLEs
   are unique; no other rtx-object will be equal to any of these.  */

rtx global_rtl[GR_MAX];

/* We record floating-point CONST_DOUBLEs in each floating-point mode for
   the values of 0, 1, and 2.  For the integer entries and VOIDmode, we
   record a copy of const[012]_rtx.  */

rtx const_tiny_rtx[3][(int) MAX_MACHINE_MODE];

rtx const_true_rtx;

REAL_VALUE_TYPE dconst0;
REAL_VALUE_TYPE dconst1;
REAL_VALUE_TYPE dconst2;
REAL_VALUE_TYPE dconstm1;

/* All references to the following fixed hard registers go through
   these unique rtl objects.  On machines where the frame-pointer and
   arg-pointer are the same register, they use the same unique object.

   After register allocation, other rtl objects which used to be pseudo-regs
   may be clobbered to refer to the frame-pointer register.
   But references that were originally to the frame-pointer can be
   distinguished from the others because they contain frame_pointer_rtx.

   When to use frame_pointer_rtx and hard_frame_pointer_rtx is a little
   tricky: until register elimination has taken place hard_frame_pointer_rtx
   should be used if it is being set, and frame_pointer_rtx otherwise.  After
   register elimination hard_frame_pointer_rtx should always be used.
   On machines where the two registers are same (most) then these are the
   same.

   In an inline procedure, the stack and frame pointer rtxs may not be
   used for anything else.  */
rtx struct_value_rtx;		/* (REG:Pmode STRUCT_VALUE_REGNUM) */
rtx struct_value_incoming_rtx;	/* (REG:Pmode STRUCT_VALUE_INCOMING_REGNUM) */
rtx static_chain_rtx;		/* (REG:Pmode STATIC_CHAIN_REGNUM) */
rtx static_chain_incoming_rtx;	/* (REG:Pmode STATIC_CHAIN_INCOMING_REGNUM) */
rtx pic_offset_table_rtx;	/* (REG:Pmode PIC_OFFSET_TABLE_REGNUM) */

/* This is used to implement __builtin_return_address for some machines.
   See for instance the MIPS port.  */
rtx return_address_pointer_rtx;	/* (REG:Pmode RETURN_ADDRESS_POINTER_REGNUM) */

/* We make one copy of (const_int C) where C is in
   [- MAX_SAVED_CONST_INT, MAX_SAVED_CONST_INT]
   to save space during the compilation and simplify comparisons of
   integers.  */

rtx const_int_rtx[MAX_SAVED_CONST_INT * 2 + 1];

/* A hash table storing CONST_INTs whose absolute value is greater
   than MAX_SAVED_CONST_INT.  */

static htab_t const_int_htab;

/* A hash table storing memory attribute structures.  */
static htab_t mem_attrs_htab;

/* start_sequence and gen_sequence can make a lot of rtx expressions which are
   shortly thrown away.  We use two mechanisms to prevent this waste:

   For sizes up to 5 elements, we keep a SEQUENCE and its associated
   rtvec for use by gen_sequence.  One entry for each size is
   sufficient because most cases are calls to gen_sequence followed by
   immediately emitting the SEQUENCE.  Reuse is safe since emitting a
   sequence is destructive on the insn in it anyway and hence can't be
   redone.

   We do not bother to save this cached data over nested function calls.
   Instead, we just reinitialize them.  */

#define SEQUENCE_RESULT_SIZE 5

static rtx sequence_result[SEQUENCE_RESULT_SIZE];

/* During RTL generation, we also keep a list of free INSN rtl codes.  */
static rtx free_insn;

#define first_insn (cfun->emit->x_first_insn)
#define last_insn (cfun->emit->x_last_insn)
#define cur_insn_uid (cfun->emit->x_cur_insn_uid)
#define last_linenum (cfun->emit->x_last_linenum)
#define last_filename (cfun->emit->x_last_filename)
#define first_label_num (cfun->emit->x_first_label_num)

static rtx make_jump_insn_raw		PARAMS ((rtx));
static rtx make_call_insn_raw		PARAMS ((rtx));
static rtx find_line_note		PARAMS ((rtx));
static void mark_sequence_stack         PARAMS ((struct sequence_stack *));
static rtx change_address_1		PARAMS ((rtx, enum machine_mode, rtx,
						 int));
static void unshare_all_rtl_1		PARAMS ((rtx));
static void unshare_all_decls		PARAMS ((tree));
static void reset_used_decls		PARAMS ((tree));
static void mark_label_nuses		PARAMS ((rtx));
static hashval_t const_int_htab_hash    PARAMS ((const void *));
static int const_int_htab_eq            PARAMS ((const void *,
						 const void *));
static hashval_t mem_attrs_htab_hash    PARAMS ((const void *));
static int mem_attrs_htab_eq            PARAMS ((const void *,
						 const void *));
static void mem_attrs_mark		PARAMS ((const void *));
static mem_attrs *get_mem_attrs		PARAMS ((HOST_WIDE_INT, tree, rtx,
						 rtx, unsigned int,
						 enum machine_mode));
static tree component_ref_for_mem_expr	PARAMS ((tree));
static rtx gen_const_vector_0		PARAMS ((enum machine_mode));

/* Probability of the conditional branch currently proceeded by try_split.
   Set to -1 otherwise.  */
int split_branch_probability = -1;

/* Returns a hash code for X (which is a really a CONST_INT).  */

static hashval_t
const_int_htab_hash (x)
     const void *x;
{
  return (hashval_t) INTVAL ((const struct rtx_def *) x);
}

/* Returns non-zero if the value represented by X (which is really a
   CONST_INT) is the same as that given by Y (which is really a
   HOST_WIDE_INT *).  */

static int
const_int_htab_eq (x, y)
     const void *x;
     const void *y;
{
  return (INTVAL ((const struct rtx_def *) x) == *((const HOST_WIDE_INT *) y));
}

/* Returns a hash code for X (which is a really a mem_attrs *).  */

static hashval_t
mem_attrs_htab_hash (x)
     const void *x;
{
  mem_attrs *p = (mem_attrs *) x;

  return (p->alias ^ (p->align * 1000)
	  ^ ((p->offset ? INTVAL (p->offset) : 0) * 50000)
	  ^ ((p->size ? INTVAL (p->size) : 0) * 2500000)
	  ^ (size_t) p->expr);
}

/* Returns non-zero if the value represented by X (which is really a
   mem_attrs *) is the same as that given by Y (which is also really a
   mem_attrs *).  */

static int
mem_attrs_htab_eq (x, y)
     const void *x;
     const void *y;
{
  mem_attrs *p = (mem_attrs *) x;
  mem_attrs *q = (mem_attrs *) y;

  return (p->alias == q->alias && p->expr == q->expr && p->offset == q->offset
	  && p->size == q->size && p->align == q->align);
}

/* This routine is called when we determine that we need a mem_attrs entry.
   It marks the associated decl and RTL as being used, if present.  */

static void
mem_attrs_mark (x)
     const void *x;
{
  mem_attrs *p = (mem_attrs *) x;

  if (p->expr)
    ggc_mark_tree (p->expr);

  if (p->offset)
    ggc_mark_rtx (p->offset);

  if (p->size)
    ggc_mark_rtx (p->size);
}

/* Allocate a new mem_attrs structure and insert it into the hash table if
   one identical to it is not already in the table.  We are doing this for
   MEM of mode MODE.  */

static mem_attrs *
get_mem_attrs (alias, expr, offset, size, align, mode)
     HOST_WIDE_INT alias;
     tree expr;
     rtx offset;
     rtx size;
     unsigned int align;
     enum machine_mode mode;
{
  mem_attrs attrs;
  void **slot;

  /* If everything is the default, we can just return zero.  */
  if (alias == 0 && expr == 0 && offset == 0
      && (size == 0
	  || (mode != BLKmode && GET_MODE_SIZE (mode) == INTVAL (size)))
      && (align == BITS_PER_UNIT
	  || (STRICT_ALIGNMENT
	      && mode != BLKmode && align == GET_MODE_ALIGNMENT (mode))))
    return 0;

  attrs.alias = alias;
  attrs.expr = expr;
  attrs.offset = offset;
  attrs.size = size;
  attrs.align = align;

  slot = htab_find_slot (mem_attrs_htab, &attrs, INSERT);
  if (*slot == 0)
    {
      *slot = ggc_alloc (sizeof (mem_attrs));
      memcpy (*slot, &attrs, sizeof (mem_attrs));
    }

  return *slot;
}

/* Generate a new REG rtx.  Make sure ORIGINAL_REGNO is set properly, and
   don't attempt to share with the various global pieces of rtl (such as
   frame_pointer_rtx).  */

rtx
gen_raw_REG (mode, regno)
     enum machine_mode mode;
     int regno;
{
  rtx x = gen_rtx_raw_REG (mode, regno);
  ORIGINAL_REGNO (x) = regno;
  return x;
}

/* There are some RTL codes that require special attention; the generation
   functions do the raw handling.  If you add to this list, modify
   special_rtx in gengenrtl.c as well.  */

rtx
gen_rtx_CONST_INT (mode, arg)
     enum machine_mode mode ATTRIBUTE_UNUSED;
     HOST_WIDE_INT arg;
{
  void **slot;

  if (arg >= - MAX_SAVED_CONST_INT && arg <= MAX_SAVED_CONST_INT)
    return const_int_rtx[arg + MAX_SAVED_CONST_INT];

#if STORE_FLAG_VALUE != 1 && STORE_FLAG_VALUE != -1
  if (const_true_rtx && arg == STORE_FLAG_VALUE)
    return const_true_rtx;
#endif

  /* Look up the CONST_INT in the hash table.  */
  slot = htab_find_slot_with_hash (const_int_htab, &arg,
				   (hashval_t) arg, INSERT);
  if (*slot == 0)
    *slot = gen_rtx_raw_CONST_INT (VOIDmode, arg);

  return (rtx) *slot;
}

rtx
gen_int_mode (c, mode)
     HOST_WIDE_INT c;
     enum machine_mode mode;
{
  return GEN_INT (trunc_int_for_mode (c, mode));
}

/* CONST_DOUBLEs needs special handling because their length is known
   only at run-time.  */

rtx
gen_rtx_CONST_DOUBLE (mode, arg0, arg1)
     enum machine_mode mode;
     HOST_WIDE_INT arg0, arg1;
{
  rtx r = rtx_alloc (CONST_DOUBLE);
  int i;

  PUT_MODE (r, mode);
  X0EXP (r, 0) = NULL_RTX;
  XWINT (r, 1) = arg0;
  XWINT (r, 2) = arg1;

  for (i = GET_RTX_LENGTH (CONST_DOUBLE) - 1; i > 2; --i)
    XWINT (r, i) = 0;

  return r;
}

rtx
gen_rtx_REG (mode, regno)
     enum machine_mode mode;
     int regno;
{
  /* In case the MD file explicitly references the frame pointer, have
     all such references point to the same frame pointer.  This is
     used during frame pointer elimination to distinguish the explicit
     references to these registers from pseudos that happened to be
     assigned to them.

     If we have eliminated the frame pointer or arg pointer, we will
     be using it as a normal register, for example as a spill
     register.  In such cases, we might be accessing it in a mode that
     is not Pmode and therefore cannot use the pre-allocated rtx.

     Also don't do this when we are making new REGs in reload, since
     we don't want to get confused with the real pointers.  */

  if (mode == Pmode && !reload_in_progress)
    {
      if (regno == FRAME_POINTER_REGNUM)
	return frame_pointer_rtx;
#if FRAME_POINTER_REGNUM != HARD_FRAME_POINTER_REGNUM
      if (regno == HARD_FRAME_POINTER_REGNUM)
	return hard_frame_pointer_rtx;
#endif
#if FRAME_POINTER_REGNUM != ARG_POINTER_REGNUM && HARD_FRAME_POINTER_REGNUM != ARG_POINTER_REGNUM
      if (regno == ARG_POINTER_REGNUM)
	return arg_pointer_rtx;
#endif
#ifdef RETURN_ADDRESS_POINTER_REGNUM
      if (regno == RETURN_ADDRESS_POINTER_REGNUM)
	return return_address_pointer_rtx;
#endif
      if (regno == PIC_OFFSET_TABLE_REGNUM
	  && fixed_regs[PIC_OFFSET_TABLE_REGNUM])
        return pic_offset_table_rtx;
      if (regno == STACK_POINTER_REGNUM)
	return stack_pointer_rtx;
    }

  return gen_raw_REG (mode, regno);
}

rtx
gen_rtx_MEM (mode, addr)
     enum machine_mode mode;
     rtx addr;
{
  rtx rt = gen_rtx_raw_MEM (mode, addr);

  /* This field is not cleared by the mere allocation of the rtx, so
     we clear it here.  */
  MEM_ATTRS (rt) = 0;

  return rt;
}

rtx
gen_rtx_SUBREG (mode, reg, offset)
     enum machine_mode mode;
     rtx reg;
     int offset;
{
  /* This is the most common failure type.
     Catch it early so we can see who does it.  */
  if ((offset % GET_MODE_SIZE (mode)) != 0)
    abort ();

  /* This check isn't usable right now because combine will
     throw arbitrary crap like a CALL into a SUBREG in
     gen_lowpart_for_combine so we must just eat it.  */
#if 0
  /* Check for this too.  */
  if (offset >= GET_MODE_SIZE (GET_MODE (reg)))
    abort ();
#endif
  return gen_rtx_fmt_ei (SUBREG, mode, reg, offset);
}

/* Generate a SUBREG representing the least-significant part of REG if MODE
   is smaller than mode of REG, otherwise paradoxical SUBREG.  */

rtx
gen_lowpart_SUBREG (mode, reg)
     enum machine_mode mode;
     rtx reg;
{
  enum machine_mode inmode;

  inmode = GET_MODE (reg);
  if (inmode == VOIDmode)
    inmode = mode;
  return gen_rtx_SUBREG (mode, reg,
			 subreg_lowpart_offset (mode, inmode));
}

/* rtx gen_rtx (code, mode, [element1, ..., elementn])
**
**	    This routine generates an RTX of the size specified by
**	<code>, which is an RTX code.   The RTX structure is initialized
**	from the arguments <element1> through <elementn>, which are
**	interpreted according to the specific RTX type's format.   The
**	special machine mode associated with the rtx (if any) is specified
**	in <mode>.
**
**	    gen_rtx can be invoked in a way which resembles the lisp-like
**	rtx it will generate.   For example, the following rtx structure:
**
**	      (plus:QI (mem:QI (reg:SI 1))
**		       (mem:QI (plusw:SI (reg:SI 2) (reg:SI 3))))
**
**		...would be generated by the following C code:
**
**		gen_rtx (PLUS, QImode,
**		    gen_rtx (MEM, QImode,
**			gen_rtx (REG, SI