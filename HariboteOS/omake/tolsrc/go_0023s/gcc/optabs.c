/* Expand the basic unary and binary arithmetic operations, for GNU compiler.
   Copyright (C) 1987, 1988, 1992, 1993, 1994, 1995, 1996, 1997, 1998,
   1999, 2000, 2001 Free Software Foundation, Inc.

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

/* Include insn-config.h before expr.h so that HAVE_conditional_move
   is properly defined.  */
#include "insn-config.h"
#include "rtl.h"
#include "tree.h"
#include "tm_p.h"
#include "flags.h"
#include "function.h"
#include "except.h"
#include "expr.h"
#include "optabs.h"
#include "libfuncs.h"
#include "recog.h"
#include "reload.h"
#include "ggc.h"
#include "real.h"

/* Each optab contains info on how this target machine
   can perform a particular operation
   for all sizes and kinds of operands.

   The operation to be performed is often specified
   by passing one of these optabs as an argument.

   See expr.h for documentation of these optabs.  */

optab optab_table[OTI_MAX];

rtx libfunc_table[LTI_MAX];

/* Tables of patterns for extending one integer mode to another.  */
enum insn_code extendtab[MAX_MACHINE_MODE][MAX_MACHINE_MODE][2];

/* Tables of patterns for converting between fixed and floating point.  */
enum insn_code fixtab[NUM_MACHINE_MODES][NUM_MACHINE_MODES][2];
enum insn_code fixtrunctab[NUM_MACHINE_MODES][NUM_MACHINE_MODES][2];
enum insn_code floattab[NUM_MACHINE_MODES][NUM_MACHINE_MODES][2];

/* Contains the optab used for each rtx code.  */
optab code_to_optab[NUM_RTX_CODE + 1];

/* Indexed by the rtx-code for a conditional (eg. EQ, LT,...)
   gives the gen_function to make a branch to test that condition.  */

rtxfun bcc_gen_fctn[NUM_RTX_CODE];

/* Indexed by the rtx-code for a conditional (eg. EQ, LT,...)
   gives the insn code to make a store-condition insn
   to test that condition.  */

enum insn_code setcc_gen_code[NUM_RTX_CODE];

#ifdef HAVE_conditional_move
/* Indexed by the machine mode, gives the insn code to make a conditional
   move insn.  This is not indexed by the rtx-code like bcc_gen_fctn and
   setcc_gen_code to cut down on the number of named patterns.  Consider a day
   when a lot more rtx codes are conditional (eg: for the ARM).  */

enum insn_code movcc_gen_code[NUM_MACHINE_MODES];
#endif

static int add_equal_note	PARAMS ((rtx, rtx, enum rtx_code, rtx, rtx));
static rtx widen_operand	PARAMS ((rtx, enum machine_mode,
				       enum machine_mode, int, int));
static int expand_cmplxdiv_straight PARAMS ((rtx, rtx, rtx, rtx,
					   rtx, rtx, enum machine_mode,
					   int, enum optab_methods,
					   enum mode_class, optab));
static int expand_cmplxdiv_wide PARAMS ((rtx, rtx, rtx, rtx,
				       rtx, rtx, enum machine_mode,
				       int, enum optab_methods,
				       enum mode_class, optab));
static void prepare_cmp_insn PARAMS ((rtx *, rtx *, enum rtx_code *, rtx,
				      enum machine_mode *, int *,
				      enum can_compare_purpose));
static enum insn_code can_fix_p	PARAMS ((enum machine_mode, enum machine_mode,
				       int, int *));
static enum insn_code can_float_p PARAMS ((enum machine_mode,
					   enum machine_mode,
					   int));
static rtx ftruncify	PARAMS ((rtx));
static optab new_optab	PARAMS ((void));
static inline optab init_optab	PARAMS ((enum rtx_code));
static inline optab init_optabv	PARAMS ((enum rtx_code));
static void init_libfuncs PARAMS ((optab, int, int, const char *, int));
static void init_integral_libfuncs PARAMS ((optab, const char *, int));
static void init_floating_libfuncs PARAMS ((optab, const char *, int));
#ifdef HAVE_conditional_trap
static void init_traps PARAMS ((void));
#endif
static void emit_cmp_and_jump_insn_1 PARAMS ((rtx, rtx, enum machine_mode,
					    enum rtx_code, int, rtx));
static void prepare_float_lib_cmp PARAMS ((rtx *, rtx *, enum rtx_code *,
					 enum machine_mode *, int *));

/* Add a REG_EQUAL note to the last insn in SEQ.  TARGET is being set to
   the result of operation CODE applied to OP0 (and OP1 if it is a binary
   operation).

   If the last insn does not set TARGET, don't do anything, but return 1.

   If a previous insn sets TARGET and TARGET is one of OP0 or OP1,
   don't add the REG_EQUAL note but return 0.  Our caller can then try
   again, ensuring that TARGET is not one of the operands.  */

static int
add_equal_note (seq, target, code, op0, op1)
     rtx seq;
     rtx target;
     enum rtx_code code;
     rtx op0, op1;
{
  rtx set;
  int i;
  rtx note;

  if ((GET_RTX_CLASS (code) != '1' && GET_RTX_CLASS (code) != '2'
       && GET_RTX_CLASS (code) != 'c' && GET_RTX_CLASS (code) != '<')
      || GET_CODE (seq) != SEQUENCE
      || (set = single_set (XVECEXP (seq, 0, XVECLEN (seq, 0) - 1))) == 0
      || GET_CODE (target) == ZERO_EXTRACT
      || (! rtx_equal_p (SET_DEST (set), target)
	  /* For a STRICT_LOW_PART, the REG_NOTE applies to what is inside the
	     SUBREG.  */
	  && (GET_CODE (SET_DEST (set)) != STRICT_LOW_PART
	      || ! rtx_equal_p (SUBREG_REG (XEXP (SET_DEST (set), 0)),
				target))))
    return 1;

  /* If TARGET is in OP0 or OP1, check if anything in SEQ sets TARGET
     besides the last insn.  */
  if (reg_overlap_mentioned_p (target, op0)
      || (op1 && reg_overlap_mentioned_p (target, op1)))
    for (i = XVECLEN (seq, 0) - 2; i >= 0; i--)
      if (reg_set_p (target, XVECEXP (seq, 0, i)))
	return 0;

  if (GET_RTX_CLASS (code) == '1')
    note = gen_rtx_fmt_e (code, GET_MODE (target), copy_rtx (op0));
  else
    note = gen_rtx_fmt_ee (code, GET_MODE (target), copy_rtx (op0), copy_rtx (op1));

  set_unique_reg_note (XVECEXP (seq, 0, XVECLEN (seq, 0) - 1), REG_EQUAL, note);

  return 1;
}

/* Widen OP to MODE and return the rtx for the widened operand.  UNSIGNEDP
   says whether OP is signed or unsigned.  NO_EXTEND is nonzero if we need
   not actually do a sign-extend or zero-extend, but can leave the 
   higher-order bits of the result rtx undefined, for example, in the case
   of logical operations, but not right shifts.  */

static rtx
widen_operand (op, mode, oldmode, unsignedp, no_extend)
     rtx op;
     enum machine_mode mode, oldmode;
     int unsignedp;
     int no_extend;
{
  rtx result;

  /* If we don't have to extend and this is a constant, return it.  */
  if (no_extend && GET_MODE (op) == VOIDmode)
    return op;

  /* If we must extend do so.  If OP is a SUBREG for a promoted object, also
     extend since it will be more efficient to do so unless the signedness of
     a promoted object differs from our extension.  */
  if (! no_extend
      || (GET_CODE (op) == SUBREG && SUBREG_PROMOTED_VAR_P (op)
	  && SUBREG_PROMOTED_UNSIGNED_P (op) == unsignedp))
    return convert_modes (mode, oldmode, op, unsignedp);

  /* If MODE is no wider than a single word, we return a paradoxical
     SUBREG.  */
  if (GET_MODE_SIZE (mode) <= UNITS_PER_WORD)
    return gen_rtx_SUBREG (mode, force_reg (GET_MODE (op), op), 0);

  /* Otherwise, get an object of MODE, clobber it, and set the low-order
     part to OP.  */

  result = gen_reg_rtx (mode);
  emit_insn (gen_rtx_CLOBBER (VOIDmode, result));
  emit_move_insn (gen_lowpart (GET_MODE (op), result), op);
  return result;
}

/* Generate code to perform a straightforward complex divide.  */

static int
expand_cmplxdiv_straight (real0, real1, imag0, imag1, realr, imagr, submode,
			  unsignedp, methods, class, binoptab)
  rtx real0, real1, imag0, imag1, realr, imagr;
  enum machine_mode submode;
  int unsignedp;
  enum optab_methods methods;
  enum mode_class class;
  optab binoptab;
{
  rtx divisor;
  rtx real_t, imag_t;
  rtx temp1, temp2;
  rtx res;
  optab this_add_optab = add_optab;
  optab this_sub_optab = sub_optab;
  optab this_neg_optab = neg_optab;
  optab this_mul_optab = smul_optab;
	      
  if (binoptab == sdivv_optab)
    {
      this_add_optab = addv_optab;
      this_sub_optab = subv_optab;
      this_neg_optab = negv_optab;
      this_mul_optab = smulv_optab;
    }

  /* Don't fetch these from memory more than once.  */
  real0 = force_reg (submode, real0);
  real1 = force_reg (submode, real1);

  if (imag0 != 0)
    imag0 = force_reg (submode, imag0);

  imag1 = force_reg (submode, imag1);

  /* Divisor: c*c + d*d.  */
  temp1 = expand_binop (submode, this_mul_optab, real1, real1,
			NULL_RTX, unsignedp, methods);

  temp2 = expand_binop (submode, this_mul_optab, imag1, imag1,
			NULL_RTX, unsignedp, methods);

  if (temp1 == 0 || temp2 == 0)
    return 0;

  divisor = expand_binop (submode, this_add_optab, temp1, temp2,
			  NULL_RTX, unsignedp, methods);
  if (divisor == 0)
    return 0;

  if (imag0 == 0)
    {
      /* Mathematically, ((a)(c-id))/divisor.  */
      /* Computationally, (a+i0) / (c+id) = (ac/(cc+dd)) + i(-ad/(cc+dd)).  */

      /* Calculate the dividend.  */
      real_t = expand_binop (submode, this_mul_optab, real0, real1,
			     NULL_RTX, unsignedp, methods);
		  
      imag_t = expand_binop (submode, this_mul_optab, real0, imag1,
			     NULL_RTX, unsignedp, methods);

      if (real_t == 0 || imag_t == 0)
	return 0;

      imag_t = expand_unop (submode, this_neg_optab, imag_t,
			    NULL_RTX, unsignedp);
    }
  else
    {
      /* Mathematically, ((a+ib)(c-id))/divider.  */
      /* Calculate the dividend.  */
      temp1 = expand_binop (submode, this_mul_optab, real0, real1,
			    NULL_RTX, unsignedp, methods);

      temp2 = expand_binop (submode, this_mul_optab, imag0, imag1,
			    NULL_RTX, unsignedp, methods);

      if (temp1 == 0 || temp2 == 0)
	return 0;

      real_t = expand_binop (submode, this_add_optab, temp1, temp2,
			     NULL_RTX, unsignedp, methods);
		  
      temp1 = expand_binop (submode, this_mul_optab, imag0, real1,
			    NULL_RTX, unsignedp, methods);

      temp2 = expand_binop (submode, this_mul_optab, real0, imag1,
			    NULL_RTX, unsignedp, methods);

      if (temp1 == 0 || temp2 == 0)
	return 0;

      imag_t = expand_binop (submode, this_sub_optab, temp1, temp2,
			     NULL_RTX, unsignedp, methods);

      if (real_t == 0 || imag_t == 0)
	return 0;
    }

  if (class == MODE_COMPLEX_FLOAT)
    res = expand_binop (submode, binoptab, real_t, divisor,
			realr, unsignedp, methods);
  else
    res = expand_divmod (0, TRUNC_DIV_EXPR, submode,
			 real_t, divisor, realr, unsignedp);

  if (res == 0)
    return 0;

  if (res != realr)
    emit_move_insn (realr, res);

  if (class == MODE_COMPLEX_FLOAT)
    res = expand_binop (submode, binoptab, imag_t, divisor,
			imagr, unsignedp, methods);
  else
    res = expand_divmod (0, TRUNC_DIV_EXPR, submode,
			 imag_t, divisor, imagr, unsignedp);

  if (res == 0)
    return 0;

  if (res != imagr)
    emit_move_insn (imagr, res);

  return 1;
}

/* Generate code to perform a wide-input-range-acceptable complex divide.  */

static int
expand_cmplxdiv_wide (real0, real1, imag0, imag1, realr, imagr, submode,
		      unsignedp, methods, class, binoptab)
  rtx real0, real1, imag0, imag1, realr, imagr;
  enum machine_mode submode;
  int unsignedp;
  enum optab_methods methods;
  enum mode_class class;
  optab binoptab;
{
  rtx ratio, divisor;
  rtx real_t, imag_t;
  rtx temp1, temp2, lab1, lab2;
  enum machine_mode mode;
  rtx res;
  optab this_add_optab = add_optab;
  optab this_sub_optab = sub_optab;
  optab this_neg_optab = neg_optab;
  optab this_mul_optab = 