/* Medium-level subroutines: convert bit-field store and extract
   and shifts, multiplies and divides to rtl instructions.
   Copyright (C) 1987, 1988, 1989, 1992, 1993, 1994, 1995, 1996, 1997, 1998,
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


#include "config.h"
#include "system.h"
#include "toplev.h"
#include "rtl.h"
#include "tree.h"
#include "tm_p.h"
#include "flags.h"
#include "insn-config.h"
#include "expr.h"
#include "optabs.h"
#include "real.h"
#include "recog.h"

static void store_fixed_bit_field	PARAMS ((rtx, unsigned HOST_WIDE_INT,
						 unsigned HOST_WIDE_INT,
						 unsigned HOST_WIDE_INT, rtx));
static void store_split_bit_field	PARAMS ((rtx, unsigned HOST_WIDE_INT,
						 unsigned HOST_WIDE_INT, rtx));
static rtx extract_fixed_bit_field	PARAMS ((enum machine_mode, rtx,
						 unsigned HOST_WIDE_INT,
						 unsigned HOST_WIDE_INT,
						 unsigned HOST_WIDE_INT,
						 rtx, int));
static rtx mask_rtx			PARAMS ((enum machine_mode, int,
						 int, int));
static rtx lshift_value			PARAMS ((enum machine_mode, rtx,
						 int, int));
static rtx extract_split_bit_field	PARAMS ((rtx, unsigned HOST_WIDE_INT,
						 unsigned HOST_WIDE_INT, int));
static void do_cmp_and_jump		PARAMS ((rtx, rtx, enum rtx_code,
						 enum machine_mode, rtx));

/* Non-zero means divides or modulus operations are relatively cheap for
   powers of two, so don't use branches; emit the operation instead.
   Usually, this will mean that the MD file will emit non-branch
   sequences.  */

static int sdiv_pow2_cheap, smod_pow2_cheap;

#ifndef SLOW_UNALIGNED_ACCESS
#define SLOW_UNALIGNED_ACCESS(MODE, ALIGN) STRICT_ALIGNMENT
#endif

/* For compilers that support multiple targets with different word sizes,
   MAX_BITS_PER_WORD contains the biggest value of BITS_PER_WORD.  An example
   is the H8/300(H) compiler.  */

#ifndef MAX_BITS_PER_WORD
#define MAX_BITS_PER_WORD BITS_PER_WORD
#endif

/* Reduce conditional compilation elsewhere.  */
#ifndef HAVE_insv
#define HAVE_insv	0
#define CODE_FOR_insv	CODE_FOR_nothing
#define gen_insv(a,b,c,d) NULL_RTX
#endif
#ifndef HAVE_extv
#define HAVE_extv	0
#define CODE_FOR_extv	CODE_FOR_nothing
#define gen_extv(a,b,c,d) NULL_RTX
#endif
#ifndef HAVE_extzv
#define HAVE_extzv	0
#define CODE_FOR_extzv	CODE_FOR_nothing
#define gen_extzv(a,b,c,d) NULL_RTX
#endif

/* Cost of various pieces of RTL.  Note that some of these are indexed by
   shift count and some by mode.  */
static int add_cost, negate_cost, zero_cost;
static int shift_cost[MAX_BITS_PER_WORD];
static int shiftadd_cost[MAX_BITS_PER_WORD];
static int shiftsub_cost[MAX_BITS_PER_WORD];
static int mul_cost[NUM_MACHINE_MODES];
static int div_cost[NUM_MACHINE_MODES];
static int mul_widen_cost[NUM_MACHINE_MODES];
static int mul_highpart_cost[NUM_MACHINE_MODES];

void
init_expmed ()
{
  /* This is "some random pseudo register" for purposes of calling recog
     to see what insns exist.  */
  rtx reg = gen_rtx_REG (word_mode, 10000);
  rtx shift_insn, shiftadd_insn, shiftsub_insn;
  int dummy;
  int m;
  enum machine_mode mode, wider_mode;

  start_sequence ();

  reg = gen_rtx_REG (word_mode, 10000);

  zero_cost = rtx_cost (const0_rtx, 0);
  add_cost = rtx_cost (gen_rtx_PLUS (word_mode, reg, reg), SET);

  shift_insn = emit_insn (gen_rtx_SET (VOIDmode, reg,
				       gen_rtx_ASHIFT (word_mode, reg,
						       const0_rtx)));

  shiftadd_insn
    = emit_insn (gen_rtx_SET (VOIDmode, reg,
			      gen_rtx_PLUS (word_mode,
					    gen_rtx_MULT (word_mode,
							  reg, const0_rtx),
					    reg)));

  shiftsub_insn
    = emit_insn (gen_rtx_SET (VOIDmode, reg,
			      gen_rtx_MINUS (word_mode,
					     gen_rtx_MULT (word_mode,
							   reg, const0_rtx),
					     reg)));

  init_recog ();

  shift_cost[0] = 0;
  shiftadd_cost[0] = shiftsub_cost[0] = add_cost;

  for (m = 1; m < MAX_BITS_PER_WORD; m++)
    {
      shift_cost[m] = shiftadd_cost[m] = shiftsub_cost[m] = 32000;

      XEXP (SET_SRC (PATTERN (shift_insn)), 1) = GEN_INT (m);
      if (recog (PATTERN (shift_insn), shift_insn, &dummy) >= 0)
	shift_cost[m] = rtx_cost (SET_SRC (PATTERN (shift_insn)), SET);

      XEXP (XEXP (SET_SRC (PATTERN (shiftadd_insn)), 0), 1)
	= GEN_INT ((HOST_WIDE_INT) 1 << m);
      if (recog (PATTERN (shiftadd_insn), shiftadd_insn, &dummy) >= 0)
	shiftadd_cost[m] = rtx_cost (SET_SRC (PATTERN (shiftadd_insn)), SET);

      XEXP (XEXP (SET_SRC (PATTERN (shiftsub_insn)), 0), 1)
	= GEN_INT ((HOST_WIDE_INT) 1 << m);
      if (recog (PATTERN (shiftsub_insn), shiftsub_insn, &dummy) >= 0)
	shiftsub_cost[m] = rtx_cost (SET_SRC (PATTERN (shiftsub_insn)), SET);
    }

  negate_cost = rtx_cost (gen_rtx_NEG (word_mode, reg), SET);

  sdiv_pow2_cheap
    = (rtx_cost (gen_rtx_DIV (word_mode, reg, GEN_INT (32)), SET)
       <= 2 * add_cost);
  smod_pow2_cheap
    = (rtx_cost (gen_rtx_MOD (word_mode, reg, GEN_INT (32)), SET)
       <= 2 * add_cost);

  for (mode = GET_CLASS_NARROWEST_MODE (MODE_INT);
       mode != VOIDmode;
       mode = GET_MODE_WIDER_MODE (mode))
    {
      reg = gen_rtx_REG (mode, 10000);
      div_cost[(int) mode] = rtx_cost (gen_rtx_UDIV (mode, reg, reg), SET);
      mul_cost[(int) mode] = rtx_cost (gen_rtx_MULT (mode, reg, reg), SET);
      wider_mode = GET_MODE_WIDER_MODE (mode);
      if (wider_mode != VOIDmode)
	{
	  mul_widen_cost[(int) wider_mode]
	    = rtx_cost (gen_rtx_MULT (wider_mode,
				      gen_rtx_ZERO_EXTEND (wider_mode, reg),
				      gen_rtx_ZERO_EXTEND (wider_mode, reg)),
			SET);
	  mul_highpart_cost[(int) mode]
	    = rtx_cost (gen_rtx_TRUNCATE
			(mode,
			 gen_rtx_LSHIFTRT (wider_mode,
					   gen_rtx_MULT (wider_mode,
							 gen_rtx_ZERO_EXTEND
							 (wider_mode, reg),
							 gen_rtx_ZERO_EXTEND
							 (wider_mode, reg)),
					   GEN_INT (GET_MODE_BITSIZE (mode)))),
			SET);
	}
    }

  end_sequence ();
}

/* Return an rtx representing minus the value of X.
   MODE is the intended mode of the result,
   useful if X is a CONST_INT.  */

rtx
negate_rtx (mode, x)
     enum machine_mode mode;
     rtx x;
{
  rtx result = simplify_unary_operation (NEG, mode, x, mode);

  if (result == 0)
    result = expand_unop (mode, neg_optab, x, NULL_RTX, 0);

  return result;
}

/* Report on the availability of insv/extv/extzv and the desired mode
   of each of their operands.  Returns MAX_MACHINE_MODE if HAVE_foo
   is false; else the mode of the specified operand.  If OPNO is -1,
   all the caller cares about is whether the insn is available.  */
enum machine_mode
mode_for_extraction (pattern, opno)
     enum extraction_pattern pattern;
     int opno;
{
  const struct insn_data *data;

  switch (pattern)
    {
    case EP_insv:
      if (HAVE_insv)
	{
	  data = &insn_data[CODE_FOR_insv];
	  break;
	}
      return MAX_MACHINE_MODE;

    case EP_extv:
      if (HAVE_extv)
	{
	  data = &insn_data[CODE_FOR_extv];
	  break;
	}
      return MAX_MACHINE_MODE;

    case EP_extzv:
      if (HAVE_extzv)
	{
	  data = &insn_data[CODE_FOR_extzv];
	  break;
	}
      return MAX_MACHINE_MODE;

    default:
      abort ();
    }

  if (opno == -1)
    return VOIDmode;

  /* Everyone who uses this function used to follow it with
     if (result == VOIDmode) result = word_mode; */
  if (data->operand[opno].mode == VOIDmode)
    return word_mode;
  return data->operand[opno].mode;
}


/* Generate code to store value from rtx VALUE
   into a bit-field within structure STR_RTX
   containing BITSIZE bits starting at bit BITNUM.
   FIELDMODE is the machine-mode of the FIELD_DECL node for this field.
   ALIGN is the alignment that STR_RTX is known to have.
   TOTAL_SIZE is the size of the structure in bytes, or -1 if varying.  */

/* ??? Note that there are two different ideas here for how
   to determine the size to count bits within, for a register.
   One is BITS_PER_WORD, and the other is the size of operand 3
   of the insv pattern.

   If operand 3 of the insv pattern is VOIDmode, then we will use BITS_PER_WORD
   else, we use the mode of operand 3.  */

rtx
store_bit_field (str_rtx, bitsize, bitnum, fieldmode, value, total_size)
     rtx str_rtx;
     unsigned HOST_WIDE_INT bitsize;
     unsigned HOST_WIDE_INT bitnum;
     enum machine_mode fieldmode;
     rtx value;
     HOST_WIDE_INT total_size;
{
  unsigned int unit
    = (GET_CODE (str_rtx) == MEM) ? BITS_PER_UNIT : BITS_PER_WORD;
  unsigned HOST_WIDE_INT offset = bitnum / unit;
  unsigned HOST_WIDE_INT bitpos = bitnum % unit;
  rtx op0 = str_rtx;
  int byte_offset;

  enum machine_mode op_mode = mode_for_extraction (EP_insv, 3);

  /* Discount the part of the structure before the desired byte.
     We need to know how many bytes are safe to reference after it.  */
  if (total_size >= 0)
    total_size -= (bitpos / BIGGEST_ALIGNMENT
		   * (BIGGEST_ALIGNMENT / BITS_PER_UNIT));

  while (GET_CODE (op0) == SUBREG)
    {
      /* The following line once was done only if WORDS_BIG_ENDIAN,
	 but I think that is a mistake.  WORDS_BIG_ENDIAN is
	 meaningful at a much higher level; when structures are copied
	 between memory and regs, the higher-numbered regs
	 always get higher addresses.  */
      offset += (SUBREG_BYTE (op0) / UNITS_PER_WORD);
      /* We used to adjust BITPOS here, but now we do the whole adjustment
	 right after the loop.  */
      op0 = SUBREG_REG (op0);
    }

  value = protect_from_queue (value, 0);

  if (flag_force_mem)
    {
      int old_generating_concat_p = generating_concat_p;
      generating_concat_p = 0;
      value = force_not_mem (value);
      generating_concat_p = old_generating_concat_p;
    }

  /* If the target is a register, overwriting the entire object, or storing
     a full-word or multi-word field can be done with just a SUBREG.

     If the target is memory, storing any naturally aligned field can be
     done with a simple store.  For targets that support fast unaligned
     memory, any naturally sized, unit aligned field can be done directly.  */

  byte_offset = (bitnum % BITS_PER_WORD) / BITS_PER_UNIT
                + (offset * UNITS_PER_WORD);

  if (bitpos == 0
      && bitsize == GET_MODE_BITSIZE (fieldmode)
      && (GET_CODE (op0) != MEM
	  ? ((GET_MODE_SIZE (fieldmode) >= UNITS_PER_WORD
	     || GET_MODE_SIZE (GET_MODE (op0)) == GET_MODE_SIZE (fieldmode))
            && byte_offset % GET_MODE_SIZE (fieldmode) == 0)
	  : (! SLOW_UNALIGNED_ACCESS (fieldmode, MEM_ALIGN (op0))
	     || (offset * BITS_PER_UNIT % bitsize == 0
		 && MEM_ALIGN (op0) % GET_MODE_BITSIZE (fieldmode) == 0))))
    {
      if (GET_MODE (op0) != fieldmode)
	{
	  if (GET_CODE (op0) == SUBREG)
	    {
	      if (GET_MODE (SUBREG_REG (op0)) == fieldmode
		  || GET_MODE_CLASS (fieldmode) == MODE_INT
		  || GET_MODE_CLASS (fieldmode) == MODE_PARTIAL_INT)
		op0 = SUBREG_REG (op0);
	      else
		/* Else we've got some float mode source being extracted into
		   a different float mode destination -- this combination of
		   subregs results in Severe Tire Damage.  */
		abort ();
	    }
	  if (GET_CODE (op0) == REG)
	    op0 = gen_rtx_SUBREG (fieldmode, op0, byte_offset);
	  else
	    op0 = adjust_address (op0, fieldmode, offset);
	}
      emit_move_insn (op0, value);
      return value;
    }

  /* Make sure we are playing with integral modes.  Pun with subregs
     if we aren't.  