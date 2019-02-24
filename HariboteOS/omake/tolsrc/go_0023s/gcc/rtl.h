/* Register Transfer Language (RTL) definitions for GNU C-Compiler
   Copyright (C) 1987, 1991, 1992, 1993, 1994, 1995, 1996, 1997, 1998,
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

#ifndef GCC_RTL_H
#define GCC_RTL_H

struct function;

#include "machmode.h"

#undef FFS  /* Some systems predefine this symbol; don't let it interfere.  */
#undef FLOAT /* Likewise.  */
#undef ABS /* Likewise.  */
#undef PC /* Likewise.  */

/* Value used by some passes to "recognize" noop moves as valid
 instructions.  */
#define NOOP_MOVE_INSN_CODE	INT_MAX

/* Register Transfer Language EXPRESSIONS CODES */

#define RTX_CODE	enum rtx_code
enum rtx_code  {

#define DEF_RTL_EXPR(ENUM, NAME, FORMAT, CLASS)   ENUM ,
#include "rtl.def"		/* rtl expressions are documented here */
#undef DEF_RTL_EXPR

  LAST_AND_UNUSED_RTX_CODE};	/* A convenient way to get a value for
				   NUM_RTX_CODE.
				   Assumes default enum value assignment.  */

#define NUM_RTX_CODE ((int) LAST_AND_UNUSED_RTX_CODE)
				/* The cast here, saves many elsewhere.  */

extern const unsigned char rtx_length[NUM_RTX_CODE];
#define GET_RTX_LENGTH(CODE)		(rtx_length[(int) (CODE)])

extern const char * const rtx_name[NUM_RTX_CODE];
#define GET_RTX_NAME(CODE)		(rtx_name[(int) (CODE)])

extern const char * const rtx_format[NUM_RTX_CODE];
#define GET_RTX_FORMAT(CODE)		(rtx_format[(int) (CODE)])

extern const char rtx_class[NUM_RTX_CODE];
#define GET_RTX_CLASS(CODE)		(rtx_class[(int) (CODE)])

/* The flags and bitfields of an ADDR_DIFF_VEC.  BASE is the base label
   relative to which the offsets are calculated, as explained in rtl.def.  */
typedef struct
{
  /* Set at the start of shorten_branches - ONLY WHEN OPTIMIZING - : */
  unsigned min_align: 8;
  /* Flags: */
  unsigned base_after_vec: 1; /* BASE is after the ADDR_DIFF_VEC.  */
  unsigned min_after_vec: 1;  /* minimum address target label is
				 after the ADDR_DIFF_VEC.  */
  unsigned max_after_vec: 1;  /* maximum address target label is
				 after the ADDR_DIFF_VEC.  */
  unsigned min_after_base: 1; /* minimum address target label is
				 after BASE.  */
  unsigned max_after_base: 1; /* maximum address target label is
				 after BASE.  */
  /* Set by the actual branch shortening process - ONLY WHEN OPTIMIZING - : */
  unsigned offset_unsigned: 1; /* offsets have to be treated as unsigned.  */
  unsigned : 2;
  unsigned scale : 8;
} addr_diff_vec_flags;

/* Structure used to describe the attributes of a MEM.  These are hashed
   so MEMs that the same attributes share a data structure.  This means
   they cannot be modified in place.  If any element is nonzero, it means
   the value of the corresponding attribute is unknown.  */
typedef struct
{
  HOST_WIDE_INT alias;		/* Memory alias set.  */
  tree expr;			/* expr corresponding to MEM.  */
  rtx offset;			/* Offset from start of DECL, as CONST_INT.  */
  rtx size;			/* Size in bytes, as a CONST_INT.  */
  unsigned int align;		/* Alignment of MEM in bits.  */
} mem_attrs;

/* Common union for an element of an rtx.  */

typedef union rtunion_def
{
  HOST_WIDE_INT rtwint;
  int rtint;
  unsigned int rtuint;
  const char *rtstr;
  rtx rtx;
  rtvec rtvec;
  enum machine_mode rttype;
  addr_diff_vec_flags rt_addr_diff_vec_flags;
  struct cselib_val_struct *rt_cselib;
  struct bitmap_head_def *rtbit;
  tree rttree;
  struct basic_block_def *bb;
  mem_attrs *rtmem;
} rtunion;

/* RTL expression ("rtx").  */

struct rtx_def
{
  /* The kind of expression this is.  */
  ENUM_BITFIELD(rtx_code) code: 16;

  /* The kind of value the expression has.  */
  ENUM_BITFIELD(machine_mode) mode : 8;

  /* 1 in an INSN if it can alter flow of control
     within this function.
     MEM_KEEP_ALIAS_SET_P in a MEM.
     LINK_COST_ZERO in an INSN_LIST.
     SET_IS_RETURN_P in a SET.  */
  unsigned int jump : 1;
  /* 1 in an INSN if it can call another function.
     LINK_COST_FREE in an INSN_LIST.  */
  unsigned int call : 1;
  /* 1 in a REG if value of this expression will never change during
     the current function, even though it is not manifestly constant.
     1 in a MEM if contents of memory are constant.  This does not
     necessarily mean that the value of this expression is constant.
     1 in a SUBREG if it is from a promoted variable that is unsigned.
     1 in a SYMBOL_REF if it addresses something in the per-function
     constants pool.
     1 in a CALL_INSN if it is a const call.
     1 in a JUMP_INSN if it is a branch that should be annulled.  Valid from
     reorg until end of compilation; cleared before used.  */
  unsigned int unchanging : 1;
  /* 1 in a MEM expression if contents of memory are volatile.
     1 in an INSN, CALL_INSN, JUMP_INSN, CODE_LABEL or BARRIER
     if it is deleted.
     1 in a REG expression if corresponds to a variable declared by the user.
     0 for an internally generated temporary.
     In a SYMBOL_REF, this flag is used for machine-specific purposes.
     In a LABEL_REF or in a REG_LABEL note, this is LABEL_REF_NONLOCAL_P.  */
  unsigned int volatil : 1;
  /* 1 in a MEM referring to a field of an aggregate.
     0 if the MEM was a variable or the result of a * operator in C;
     1 if it was the result of a . or -> operator (on a struct) in C.
     1 in a REG if the register is used only in exit code a loop.
     1 in a SUBREG expression if was generated from a variable with a
     promoted mode.
     1 in a CODE_LABEL if the label is used for nonlocal gotos
     and must not be deleted even if its count is zero.
     1 in a LABEL_REF if this is a reference to a label outside the
     current loop.
     1 in an INSN, JUMP_INSN, or CALL_INSN if this insn must be scheduled
     together with the preceding insn.  Valid only within sched.
     1 in an INSN, JUMP_INSN, or CALL_INSN if insn is in a delay slot and
     from the target of a branch.  Valid from reorg until end of compilation;
     cleared before used.
     1 in an INSN if this insn is dead code.  Valid only during
     dead-code elimination phase; cleared before use.  */
  unsigned int in_struct : 1;
  /* 1 if this rtx is used.  This is used for copying shared structure.
     See `unshare_all_rtl'.
     In a REG, this is not needed for that purpose, and used instead
     in `leaf_renumber_regs_insn'.
     In a SYMBOL_REF, means that emit_library_call
     has used it as the function.  */
  unsigned int used : 1;
  /* Nonzero if this rtx came from procedure integration.
     In a REG, nonzero means this reg refers to the return value
     of the current function.
     1 in a SYMBOL_REF if the symbol is weak.  */
  unsigned integrated : 1;
  /* 1 in an INSN or a SET if this rtx is related to the call frame,
     either changing how we compute the frame address or saving and
     restoring registers in the prologue and epilogue.
     1 in a MEM if the MEM refers to a scalar, rather than a member of
     an aggregate.
     1 in a REG if the register is a pointer.
     1 in a SYMBOL_REF if it addresses something in the per-function
     constant string pool.  */
  unsigned frame_related : 1;

  /* The first element of the operands of this rtx.
     The number of operands and their types are controlled
     by the `code' field, according to rtl.def.  */
  rtunion fld[1];
};

#define NULL_RTX (rtx) 0

/* Define macros to access the `code' field of the rtx.  */

#define GET_CODE(RTX)	    ((enum rtx_code) (RTX)->code)
#define PUT_CODE(RTX, CODE) ((RTX)->code = (ENUM_BITFIELD(rtx_code)) (CODE))

#define GET_MODE(RTX)	    ((enum machine_mode) (RTX)->mode)
#define PUT_MODE(RTX, MODE) ((RTX)->mode = (ENUM_BITFIELD(machine_mode)) (MODE))

#define RTX_INTEGRATED_P(RTX) ((RTX)->integrated)
#define RTX_UNCHANGING_P(RTX) ((RTX)->unchanging)
#define RTX_FRAME_RELATED_P(RTX) ((RTX)->frame_related)

/* RTL vector.  These appear inside RTX's when there is a need
   for a variable number of things.  The principle use is inside
   PARALLEL expressions.  */

struct rtvec_def {
  int num_elem;		/* number of elements */
  rtx elem[1];
};

#define NULL_RTVEC (rtvec) 0

#define GET_NUM_ELEM(RTVEC)		((RTVEC)->num_elem)
#define PUT_NUM_ELEM(RTVEC, NUM)	((RTVEC)->num_elem = (NUM))

/* Predicate yielding nonzero iff X is an rtl for a register.  */
#define REG_P(X) (GET_CODE (X) == REG)

/* Predicate yielding nonzero iff X is a label insn.  */
#define LABEL_P(X) (GET_CODE (X) == CODE_LABEL)

/* Predicate yielding nonzero iff X is a jump insn.  */
#define JUMP_P(X) (GET_CODE (X) == JUMP_INSN)

/* Predicate yielding nonzero iff X is a note insn.  */
#define NOTE_P(X) (GET_CODE (X) == NOTE)

/* Predicate yielding nonzero iff X is a barrier insn.  */
#define BARRIER_P(X) (GET_CODE (X) == BARRIER)

/* Predicate yielding nonzero iff X is a data for a jump table.  */
#define JUMP_TABLE_DATA_P(INSN) ¥
  (JUMP_P (INSN) && (GET_CODE (PATTERN (INSN)) == ADDR_VEC || ¥
		     GET_CODE (PATTERN (INSN)) == ADDR_DIFF_VEC))

/* 1 if X is a constant value that is an integer.  */

#define CONSTANT_P(X)   ¥
  (GET_CODE (X) == LABEL_REF || GET_CODE (X) == SYMBOL_REF		¥
   || GET_CODE (X) == CONST_INT || GET_CODE (X) == CONST_DOUBLE		¥
   || GET_CODE (X) == CONST || GET_CODE (X) == HIGH			¥
   || GET_CODE (X) == CONST_VECTOR	                                ¥
   || GET_CODE (X) == CONSTANT_P_RTX)

/* General accessor macros for accessing the fields of an rtx.  */

#if defined ENABLE_RTL_CHECKING && (GCC_VERSION >= 2007)
/* The bit with a star outside the statement expr and an & inside is
   so that N can be evaluated only once.  */
#define RTL_CHECK1(RTX, N, C1) __extension__				¥
(*({ rtx _rtx = (RTX); int _n = (N);					¥
     enum rtx_code _code = GET_CODE (_rtx);				¥
     if (_n < 0 || _n >= GET_RTX_LENGTH (_code))			¥
       rtl_check_failed_bounds (_rtx, _n, __FILE__, __LINE__,		¥
				__FUNCTION__);				¥
     if (GET_RTX_FORMAT(_code)[_n] != C1)				¥
       rtl_check_failed_type1 (_rtx, _n, C1, __FILE__, __LINE__,	¥
			       __FUNCTION__);				¥
     &_rtx->fld[_n]; }))

#define RTL_CHECK2(RTX, N, C1, C2) __extension__			¥
(*({ rtx _rtx = (RTX); int _n = (N);					¥
     enum rtx_code _code = GET_CODE (_rtx);				¥
     if (_n < 0 || _n >= GET_RTX_LENGTH (_code))			¥
       rtl_check_failed_bounds (_rtx, _n, __FILE__, __LINE__,		¥
				__FUNCTION__);				¥
     if (GET_RTX_FORMAT(_code)[_n] != C1				¥
	 && GET_RTX_FORMAT(_code)[_n] != C2)				¥
       rtl_check_failed_type2 (_rtx, _n, C1, C2, __FILE__, __LINE__,	¥
			       __FUNCTION__);				¥
     &_rtx->fld[_n]; }))

#define RTL_CHECKC1(RTX, N, C) __extension__				¥
(*({ rtx _rtx = (RTX); int _n = (N);					¥
     if (GET_CODE (_rtx) != (C))					¥
       rtl_check_failed_code1 (_rtx, (C), __FILE__, __LINE__,		¥
			       __FUNCTION__);				¥
     &_rtx->fld[_n]; }))

#define RTL_CHECKC2(RTX, N, C1, C2) __extension__			¥
(*({ rtx _rtx = (RTX); int _n = (N);					¥
     enum rtx_code _code = GET_CODE (_rtx);				¥
     if (_code != (C1) && _code != (C2))				¥
       rtl_check_failed_code2 (_rtx, (C1), (C2), __FILE__, __LINE__,	¥
			       __FUNCTION__); ¥
     &_rtx->fld[_n]; }))

#define RTVEC_ELT(RTVEC, I) __extension__				¥
(*({ rtvec _rtvec = (RTVEC); int _i = (I);				¥
     if (_i < 0 || _i >= GET_NUM_ELEM (_rtvec))				¥
       rtvec_check_failed_bounds (_rtvec, _i, __FILE__, __LINE__,	¥
				  __FUNCTION__);			¥
     &_rtvec->elem[_i]; }))

extern void rtl_check_failed_bounds PARAMS ((rtx, int,
					   const char *, int, const char *))
    ATTRIBUTE_NORETURN;
extern void rtl_check_failed_type1 PARAMS ((rtx, int, int,
					  const char *, int, const char *))
    ATTRIBUTE_NORETURN;
extern void rtl_check_failed_type2 PARAMS ((rtx, int, int, int,
					  const char *, int, const char *))
    ATTRIBUTE_NORETURN;
extern void rtl_check_failed_code1 PARAMS ((rtx, enum rtx_code,
					  const char *, int, const char *))
    ATTRIBUTE_NORETURN;
extern void rtl_check_failed_code2 PARAMS ((rtx, enum rtx_code, enum rtx_code,
					  const char *, int, const char *))
    ATTRIBUTE_NORETURN;
extern void rtvec_check_failed_bounds PARAMS ((rtvec, int,
					     const char *, int, const char *))
    ATTRIBUTE_NORETURN;

#else   /* not ENABLE_RTL_CHECKING */

#define RTL_CHECK1(RTX, N, C1)      ((RTX)->fld[N])
#define RTL_CHECK2(RTX, N, C1, C2)  ((RTX)->fld[N])
#define RTL_CHECKC1(RTX, N, C)	    ((RTX)->fld[N])
#define RTL_CHECKC2(RTX, N, C1, C2) ((RTX)->fld[N])
#define RTVEC_ELT(RTVEC, I)	    ((RTVEC)->elem[I])

#endif

#define XWINT(RTX, N)	(RTL_CHECK1 (RTX, N, 'w').rtwint)
#define XINT(RTX, N)	(RTL_CHECK2 (RTX, N, 'i', 'n').rtint)
#define XSTR(RTX, N)	(RTL_CHECK2 (RTX, N, 's', 'S').rtstr)
#define XEXP(RTX, N)	(RTL_CHECK2 (RTX, N, 'e', 'u').rtx)
#define XVEC(RTX, N)	(RTL_CHECK2 (RTX, N, 'E', 'V').rtvec)
#define XMODE(RTX, N)	(RTL_CHECK1 (RTX, N, 'M').rttype)
#define XBITMAP(RTX, N) (RTL_CHECK1 (RTX, N, 'b').rtbit)
#define XTREE(RTX, N)   (RTL_CHECK1 (RTX, N, 't').rttree)
#define XBBDEF(RTX, N)	(RTL_CHECK1 (RTX, N, 'B').bb)
#define XTMPL(RTX, N)	(RTL_CHECK1 (RTX, N, 'T').rtstr)

#define XVECEXP(RTX, N, M)	RTVEC_ELT (XVEC (RTX, N), M)
#define XVECLEN(RTX, N)		GET_NUM_ELEM (XVEC (RTX, N))

/* These are like XWINT, etc. except that they expect a '0' field instead
   of the normal type code.  */

#define X0WINT(RTX, N)	   (RTL_CHECK1 (RTX, N, '0').rtwint)
#define X0INT(RTX, N)	   (RTL_CHECK1 (RTX, N, '0').rtint)
#define X0UINT(RTX, N)	   (RTL_CHECK1 (RTX, N, '0').rtuint)
#define X0STR(RTX, N)	   (RTL_CHECK1 (RTX, N, '0').rtstr)
#define X0EXP(RTX, N)	   (RTL_CHECK1 (RTX, N, '0').rtx)
#define X0VEC(RTX, N)	   (RTL_CHECK1 (RTX, N, '0').rtvec)
#define X0MODE(RTX, N)	   (RTL_CHECK1 (RTX, N, '0').rttype)
#define X0BITMAP(RTX, N)   (RTL_CHECK1 (RTX, N, '0').rtbit)
#define X0TREE(RTX, N)	   (RTL_CHECK1 (RTX, N, '0').rttree)
#define X0BBDEF(RTX, N)	   (RTL_CHECK1 (RTX, N, '0').bb)
#define X0ADVFLAGS(RTX, N) (RTL_CHECK1 (RTX, N, '0').rt_addr_diff_vec_flags)
#define X0CSELIB(RTX, N)   (RTL_CHECK1 (RTX, N, '0').rt_cselib)
#define X0MEMATTR(RTX, N)  (RTL_CHECK1 (RTX, N, '0').rtmem)

#define XCWINT(RTX, N, C)     (RTL_CHECKC1 (RTX, N, C).rtwint)
#define XCINT(RTX, N, C)      (RTL_CHECKC1 (RTX, N, C).rtint)
#define XCUINT(RTX, N, C)     (RTL_CHECKC1 (RTX, N, C).rtuint)
#define XCSTR(RTX, N, C)      (RTL_CHECKC1 (RTX, N, C).rtstr)
#define XCEXP(RTX, N, C)      (RTL_CHECKC1 (RTX, N, C).rtx)
#define XCVEC(RTX, N, C)      (RTL_CHECKC1 (RTX, N, C).rtvec)
#define XCMODE(RTX, N, C)     (RTL_CHECKC1 (RTX, N, C).rttype)
#define XCBITMAP(RTX, N, C)   (RTL_CHECKC1 (RTX, N, C).rtbit)
#define XCTREE(RTX, N, C)     (RTL_CHECKC1 (RTX, N, C).rttree)
#define XCBBDEF(RTX, N, C)    (RTL_CHECKC1 (RTX, N, C).bb)
#define XCADVFLAGS(RTX, N, C) (RTL_CHECKC1 (RTX, N, C).rt_addr_diff_vec_flags)
#define XCCSELIB(RTX, N, C)   (RTL_CHECKC1 (RTX, N, C).rt_cselib)

#define XCVECEXP(RTX, N, M, C)	RTVEC_ELT (XCVEC (RTX, N, C), M)
#define XCVECLEN(RTX, N, C)	GET_NUM_ELEM (XCVEC (RTX, N, C))

#define XC2EXP(RTX, N, C1, C2)      (RTL_CHECKC2 (RTX, N, C1, C2).rtx)

/* ACCESS MACROS for particular fields of insns.  */

/* Determines whether X is an insn.  */
#define INSN_P(X)       (GET_RTX_CLASS (GET_CODE(X)) == 'i')

/* Holds a unique number for each insn.
   These are not necessarily sequentially increasing.  */
#define INSN_UID(INSN)  XINT (INSN, 0)

/* Chain insns together in sequence.  */
#define PREV_INSN(INSN)	XEXP (INSN, 1)
#define NE