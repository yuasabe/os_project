/* Definitions of target machine for GNU compiler for IA-32.
   Copyright (C) 1988, 1992, 1994, 1995, 1996, 1997, 1998, 1999, 2000,
   2001, 2002 Free Software Foundation, Inc.

This file is part of GNU CC.

GNU CC is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

GNU CC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU CC; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */

/* The purpose of this file is to define the characteristics of the i386,
   independent of assembler syntax or operating system.

   Three other files build on this one to describe a specific assembler syntax:
   bsd386.h, att386.h, and sun386.h.

   The actual tm.h file for a particular system should include
   this file, and then the file for the appropriate assembler syntax.

   Many macros that specify assembler syntax are omitted entirely from
   this file because they really belong in the files for particular
   assemblers.  These include RP, IP, LPREFIX, PUT_OP_SIZE, USE_STAR,
   ADDR_BEG, ADDR_END, PRINT_IREG, PRINT_SCALE, PRINT_B_I_S, and many
   that start with ASM_ or end in ASM_OP.  */

/* Stubs for half-pic support if not OSF/1 reference platform.  */

#ifndef HALF_PIC_P
#define HALF_PIC_P() 0
#define HALF_PIC_NUMBER_PTRS 0
#define HALF_PIC_NUMBER_REFS 0
#define HALF_PIC_ENCODE(DECL)
#define HALF_PIC_DECLARE(NAME)
#define HALF_PIC_INIT()	error ("half-pic init called on systems that don't support it")
#define HALF_PIC_ADDRESS_P(X) 0
#define HALF_PIC_PTR(X) (X)
#define HALF_PIC_FINISH(STREAM)
#endif

/* Define the specific costs for a given cpu */

struct processor_costs {
  const int add;		/* cost of an add instruction */
  const int lea;		/* cost of a lea instruction */
  const int shift_var;		/* variable shift costs */
  const int shift_const;	/* constant shift costs */
  const int mult_init;		/* cost of starting a multiply */
  const int mult_bit;		/* cost of multiply per each bit set */
  const int divide;		/* cost of a divide/mod */
  int movsx;			/* The cost of movsx operation.  */
  int movzx;			/* The cost of movzx operation.  */
  const int large_insn;		/* insns larger than this cost more */
  const int move_ratio;		/* The threshold of number of scalar
				   memory-to-memory move insns.  */
  const int movzbl_load;	/* cost of loading using movzbl */
  const int int_load[3];	/* cost of loading integer registers
				   in QImode, HImode and SImode relative
				   to reg-reg move (2).  */
  const int int_store[3];	/* cost of storing integer register
				   in QImode, HImode and SImode */
  const int fp_move;		/* cost of reg,reg fld/fst */
  const int fp_load[3];		/* cost of loading FP register
				   in SFmode, DFmode and XFmode */
  const int fp_store[3];	/* cost of storing FP register
				   in SFmode, DFmode and XFmode */
  const int mmx_move;		/* cost of moving MMX register.  */
  const int mmx_load[2];	/* cost of loading MMX register
				   in SImode and DImode */
  const int mmx_store[2];	/* cost of storing MMX register
				   in SImode and DImode */
  const int sse_move;		/* cost of moving SSE register.  */
  const int sse_load[3];	/* cost of loading SSE register
				   in SImode, DImode and TImode*/
  const int sse_store[3];	/* cost of storing SSE register
				   in SImode, DImode and TImode*/
  const int mmxsse_to_integer;	/* cost of moving mmxsse register to
				   integer and vice versa.  */
  const int prefetch_block;	/* bytes moved to cache for prefetch.  */
  const int simultaneous_prefetches; /* number of parallel prefetch
				   operations.  */
};

extern const struct processor_costs *ix86_cost;

/* Run-time compilation parameters selecting different hardware subsets.  */

extern int target_flags;

/* Macros used in the machine description to test the flags.  */

/* configure can arrange to make this 2, to force a 486.  */

#ifndef TARGET_CPU_DEFAULT
#define TARGET_CPU_DEFAULT 0
#endif

/* Masks for the -m switches */
#define MASK_80387		0x00000001	/* Hardware floating point */
#define MASK_RTD		0x00000002	/* Use ret that pops args */
#define MASK_ALIGN_DOUBLE	0x00000004	/* align doubles to 2 word boundary */
#define MASK_SVR3_SHLIB		0x00000008	/* Uninit locals into bss */
#define MASK_IEEE_FP		0x00000010	/* IEEE fp comparisons */
#define MASK_FLOAT_RETURNS	0x00000020	/* Return float in st(0) */
#define MASK_NO_FANCY_MATH_387	0x00000040	/* Disable sin, cos, sqrt */
#define MASK_OMIT_LEAF_FRAME_POINTER 0x080      /* omit leaf frame pointers */
#define MASK_STACK_PROBE	0x00000100	/* Enable stack probing */
#define MASK_NO_ALIGN_STROPS	0x00000200	/* Enable aligning of string ops.  */
#define MASK_INLINE_ALL_STROPS	0x00000400	/* Inline stringops in all cases */
#define MASK_NO_PUSH_ARGS	0x00000800	/* Use push instructions */
#define MASK_ACCUMULATE_OUTGOING_ARGS 0x00001000/* Accumulate outgoing args */
#define MASK_ACCUMULATE_OUTGOING_ARGS_SET 0x00002000
#define MASK_MMX		0x00004000	/* Support MMX regs/builtins */
#define MASK_MMX_SET		0x00008000
#define MASK_SSE		0x00010000	/* Support SSE regs/builtins */
#define MASK_SSE_SET		0x00020000
#define MASK_SSE2		0x00040000	/* Support SSE2 regs/builtins */
#define MASK_SSE2_SET		0x00080000
#define MASK_3DNOW		0x00100000	/* Support 3Dnow builtins */
#define MASK_3DNOW_SET		0x00200000
#define MASK_3DNOW_A		0x00400000	/* Support Athlon 3Dnow builtins */
#define MASK_3DNOW_A_SET	0x00800000
#define MASK_128BIT_LONG_DOUBLE 0x01000000	/* long double size is 128bit */
#define MASK_64BIT		0x02000000	/* Produce 64bit code */
/* ... overlap with subtarget options starts by 0x04000000.  */
#define MASK_NO_RED_ZONE	0x04000000	/* Do not use red zone */

/* Use the floating point instructions */
#define TARGET_80387 (target_flags & MASK_80387)

/* Compile using ret insn that pops args.
   This will not work unless you use prototypes at least
   for all functions that can take varying numbers of args.  */  
#define TARGET_RTD (target_flags & MASK_RTD)

/* Align doubles to a two word boundary.  This breaks compatibility with
   the published ABI's for structures containing doubles, but produces
   faster code on the pentium.  */
#define TARGET_ALIGN_DOUBLE (target_flags & MASK_ALIGN_DOUBLE)

/* Use push instructions to save outgoing args.  */
#define TARGET_PUSH_ARGS (!(target_flags & MASK_NO_PUSH_ARGS))

/* Accumulate stack adjustments to prologue/epilogue.  */
#define TARGET_ACCUMULATE_OUTGOING_ARGS ¥
 (target_flags & MASK_ACCUMULATE_OUTGOING_ARGS)

/* Put uninitialized locals into bss, not data.
   Meaningful only on svr3.  */
#define TARGET_SVR3_SHLIB (target_flags & MASK_SVR3_SHLIB)

/* Use IEEE floating point comparisons.  These handle correctly the cases
   where the result of a comparison is unordered.  Normally SIGFPE is
   generated in such cases, in which case this isn't needed.  */
#define TARGET_IEEE_FP (target_flags & MASK_IEEE_FP)

/* Functions that return a floating point value may return that value
   in the 387 FPU or in 386 integer registers.  If set, this flag causes
   the 387 to be used, which is compatible with most calling conventions.  */
#define TARGET_FLOAT_RETURNS_IN_80387 (target_flags & MASK_FLOAT_RETURNS)

/* Long double is 128bit instead of 96bit, even when only 80bits are used.
   This mode wastes cache, but avoid misaligned data accesses and simplifies
   address calculations.  */
#define TARGET_128BIT_LONG_DOUBLE (target_flags & MASK_128BIT_LONG_DOUBLE)

/* Disable generation of FP sin, cos and sqrt operations for 387.
   This is because FreeBSD lacks these in the math-emulator-code */
#define TARGET_NO_FANCY_MATH_387 (target_flags & MASK_NO_FANCY_MATH_387)

/* Don't create frame pointers for leaf functions */
#define TARGET_OMIT_LEAF_FRAME_POINTER ¥
  (target_flags & MASK_OMIT_LEAF_FRAME_POINTER)

/* Debug GO_IF_LEGITIMATE_ADDRESS */
#define TARGET_DEBUG_ADDR (ix86_debug_addr_string != 0)

/* Debug FUNCTION_ARG macros */
#define TARGET_DEBUG_ARG (ix86_debug_arg_string != 0)

/* 64bit Sledgehammer mode */
#ifdef TARGET_BI_ARCH
#define TARGET_64BIT (target_flags & MASK_64BIT)
#else
#ifdef TARGET_64BIT_DEFAULT
#define TARGET_64BIT 1
#else
#define TARGET_64BIT 0
#endif
#endif

#define TARGET_386 (ix86_cpu == PROCESSOR_I386)
#define TARGET_486 (ix86_cpu == PROCESSOR_I486)
#define TARGET_PENTIUM (ix86_cpu == PROCESSOR_PENTIUM)
#define TARGET_PENTIUMPRO (ix86_cpu == PROCESSOR_PENTIUMPRO)
#define TARGET_K6 (ix86_cpu == PROCESSOR_K6)
#define TARGET_ATHLON (ix86_cpu == PROCESSOR_ATHLON)
#define TARGET_PENTIUM4 (ix86_cpu == PROCESSOR_PENTIUM4)

#define CPUMASK (1 << ix86_cpu)
extern const int x86_use_leave, x86_push_memory, x86_zero_extend_with_and;
extern const int x86_use_bit_test, x86_cmove, x86_deep_branch;
extern const int x86_branch_hints, x86_unroll_strlen;
extern const int x86_double_with_add, x86_partial_reg_stall, x86_movx;
extern const int x86_use_loop, x86_use_fiop, x86_use_mov0;
extern const int x86_use_cltd, x86_read_modify_write;
extern const int x86_read_modify, x86_split_long_moves;
extern const int x86_promote_QImode, x86_single_stringop;
extern const int x86_himode_math, x86_qimode_math, x86_promote_qi_regs;
extern const int x86_promote_hi_regs, x86_integer_DFmode_moves;
extern const int x86_add_esp_4, x86_add_esp_8, x86_sub_esp_4, x86_sub_esp_8;
extern const int x86_partial_reg_dependency, x86_memory_mismatch_stall;
extern const int x86_accumulate_outgoing_args, x86_prologue_using_move;
extern const int x86_epilogue_using_move, x86_decompose_lea;
extern const int x86_arch_always_fancy_math_387;
extern int x86_prefetch_sse;

#define TARGET_USE_LEAVE (x86_use_leave & CPUMASK)
#define TARGET_PUSH_MEMORY (x86_push_memory & CPUMASK)
#define TARGET_ZERO_EXTEND_WITH_AND (x86_zero_extend_with_and & CPUMASK)
#define TARGET_USE_BIT_TEST (x86_use_bit_test & CPUMASK)
#define TARGET_UNROLL_STRLEN (x86_unroll_strlen & CPUMASK)
/* For sane SSE instruction set generation we need fcomi instruction.  It is
   safe to enable all CMOVE instructions.  */
#define TARGET_CMOVE ((x86_cmove & (1 << ix86_arch)) || TARGET_SSE)
#define TARGET_DEEP_BRANCH_PREDICTION (x86_deep_branch & CPUMASK)
#define TARGET_BRANCH_PREDICTION_HINTS (x86_branch_hints & CPUMASK)
#define TARGET_DOUBLE_WITH_ADD (x86_double_with_add & CPUMASK)
#define TARGET_USE_SAHF ((x86_use_sahf & CPUMASK) && !TARGET_64BIT)
#define TARGET_MOVX (x86_movx & CPUMASK)
#define TARGET_PARTIAL_REG_STALL (x86_partial_reg_stall & CPUMASK)
#define TARGET_USE_LOOP (x86_use_loop & CPUMASK)
#define TARGET_USE_FIOP (x86_use_fiop & CPUMASK)
#define TARGET_USE_MOV0 (x86_use_mov0 & CPUMASK)
#define TARGET_USE_CLTD (x86_use_cltd & CPUMASK)
#define TARGET_SPLIT_LONG_MOVES (x86_split_long_moves & CPUMASK)
#define TARGET_READ_MODIFY_WRITE (x86_read_modify_write & CPUMASK)
#define TARGET_READ_MODIFY (x86_read_modify & CPUMASK)
#define TARGET_PROMOTE_QImode (x86_promote_QImode & CPUMASK)
#define TARGET_SINGLE_STRINGOP (x86_single_stringop & CPUMASK)
#define TARGET_QIMODE_MATH (x86_qimode_math & CPUMASK)
#define TARGET_HIMODE_MATH (x86_himode_math & CPUMASK)
#define TARGET_PROMOTE_QI_REGS (x86_promote_qi_regs & CPUMASK)
#define TARGET_PROMOTE_HI_REGS (x86_promote_hi_regs & CPUMASK)
#define TARGET_ADD_ESP_4 (x86_add_esp_4 & CPUMASK)
#define TARGET_ADD_ESP_8 (x86_add_esp_8 & CPUMASK)
#define TARGET_SUB_ESP_4 (x86_sub_esp_4 & CPUMASK)
#define TARGET_SUB_ESP_8 (x86_sub_esp_8 & CPUMASK)
#define TARGET_INTEGER_DFMODE_MOVES (x86_integer_DFmode_moves & CPUMASK)
#define TARGET_PARTIAL_REG_DEPENDENCY (x86_partial_reg_dependency & CPUMASK)
#define TARGET_MEMORY_MISMATCH_STALL (x86_memory_mismatch_stall & CPUMASK)
#define TARGET_PROLOGUE_USING_MOVE (x86_prologue_using_move & CPUMASK)
#define TARGET_EPILOGUE_USING_MOVE (x86_epilogue_using_move & CPUMASK)
#define TARGET_DECOMPOSE_LEA (x86_decompose_lea & CPUMASK)
#define TARGET_PREFETCH_SSE (x86_prefetch_sse)

#define TARGET_STACK_PROBE (target_flags & MASK_STACK_PROBE)

#define TARGET_ALIGN_STRINGOPS (!(target_flags & MASK_NO_ALIGN_STROPS))
#define TARGET_INLINE_ALL_STRINGOPS (target_flags & MASK_INLINE_ALL_STROPS)

#define ASSEMBLER_DIALECT (ix86_asm_dialect)

#define TARGET_SSE ((target_flags & (MASK_SSE | MASK_SSE2)) != 0)
#define TARGET_SSE2 ((target_flags & MASK_SSE2) != 0)
#define TARGET_SSE_MATH ((ix86_fpmath & FPMATH_SSE) != 0)
#define TARGET_MIX_SSE_I387 ((ix86_fpmath & FPMATH_SSE) ¥
			     && (ix86_fpmath & FPMATH_387))
#define TARGET_MMX ((target_flags & MASK_MMX) != 0)
#define TARGET_3DNOW ((target_flags & MASK_3DNOW) != 0)
#define TARGET_3DNOW_A ((target_flags & MASK_3DNOW_A) != 0)

#define TARGET_RED_ZONE (!(target_flags & MASK_NO_RED_ZONE))

/* WARNING: Do not mark empty strings for translation, as calling
            gettext on an empty string does NOT return an empty
            string. */


#define TARGET_SWITCHES							      ¥
{ { "80387",			 MASK_80387, N_("Use hardware fp") },	      ¥
  { "no-80387",			-MASK_80387, N_("Do not use hardware fp") },  ¥
  { "hard-float",		 MASK_80387, N_("Use hardware fp") },	      ¥
  { "soft-float",		-MASK_80387, N_("Do not use hardware fp") },  ¥
  { "no-soft-float",		 MASK_80387, N_("Use hardware fp") },	      ¥
  { "386",			 0, "" /*Deprecated.*/},		      ¥
  { "486",			 0, "" /*Deprecated.*/},		      ¥
  { "pentium",			 0, "" /*Deprecated.*/},		      ¥
  { "pentiumpro",		 0, "" /*Deprecated.*/},		      ¥
  { "intel-syntax",		 0, "" /*Deprecated.*/},	 	      ¥
  { "no-intel-syntax",		 0, "" /*Deprecated.*/},	 	      ¥
  { "rtd",			 MASK_RTD,				      ¥
    N_("Alternate calling convention") },				      ¥
  { "no-rtd",			-MASK_RTD,				      ¥
    N_("Use normal calling convention") },				      ¥
  { "align-double",		 MASK_ALIGN_DOUBLE,			      ¥
    N_("Align some doubles on dword boundary") },			      ¥
  { "no-align-double",		-MASK_ALIGN_DOUBLE,			      ¥
    N_("Align doubles on word boundary") },				      ¥
  { "svr3-shlib",		 MASK_SVR3_SHLIB,			      ¥
    N_("Uninitialized locals in .bss")  },				      ¥
  { "no-svr3-shlib",		-MASK_SVR3_SHLIB,			      ¥
    N_("Uninitialized locals in .data") },				      ¥
  { "ieee-fp",			 MASK_IEEE_FP,				      ¥
    N_("Use IEEE math for fp comparisons") },				      ¥
  { "no-ieee-fp",		-MASK_IEEE_FP,				      ¥
    N_("Do not use IEEE math for fp comparisons") },			      ¥
  { "fp-ret-in-387",		 MASK_FLOAT_RETURNS,			      ¥
    N_("Return values of functions in FPU registers") },		      ¥
  { "no-fp-ret-in-387",		-MASK_FLOAT_RETURNS ,			      ¥
    N_("Do not return values of functions in FPU registers")},		      ¥
  { "no-fancy-math-387",	 MASK_NO_FANCY_MATH_387,		      ¥
    N_("Do not generate sin, cos, sqrt for FPU") },			      ¥
  { "fancy-math-387",		-MASK_NO_FANCY_MATH_387,		      ¥
     N_("Generate sin, cos, sqrt for FPU")},				      ¥
  { "omit-leaf-frame-pointer",	 MASK_OMIT_LEAF_FRAME_POINTER,		      ¥
    N_("Omit the frame pointer in leaf functions") },			      ¥
  { "no-omit-leaf-frame-pointer",-MASK_OMIT_LEAF_FRAME_POINTER, "" },	      ¥
  { "stack-arg-probe",		 MASK_STACK_PROBE,			      ¥
    N_("Enable stack probing") },					      ¥
  { "no-stack-arg-probe",	-MASK_STACK_PROBE, "" },		      ¥
  { "windows",			0, 0 /* undocumented */ },		      ¥
  { "dll",			0,  0 /* undocumented */ },		      ¥
  { "align-stringops",		-MASK_NO_ALIGN_STROPS,			      ¥
    N_("Align destination of the string operations") },			      ¥
  { "no-align-stringops",	 MASK_NO_ALIGN_STROPS,			      ¥
    N_("Do not align destination of the string operations") },		      ¥
  { "inline-all-stringops",	 MASK_INLINE_ALL_STROPS,		      ¥
    N_("Inline all known string operations") },				      ¥
  { "no-inline-all-stringops",	-MASK_INLINE_ALL_STROPS,		      ¥
    N_("Do not inline all known string operations") },			      ¥
  { "push-args",		-MASK_NO_PUSH_ARGS,			      ¥
    N_("Use push instructions to save outgoing arguments") },		      ¥
  { "no-push-args",		MASK_NO_PUSH_ARGS,			      ¥
    N_("Do not use push instructions to save outgoing arguments") },	      ¥
  { "accumulate-outgoing-args",	(MASK_ACCUMULATE_OUTGOING_ARGS		      ¥
				 | MASK_ACCUMULATE_OUTGOING_ARGS_SET),	      ¥
    N_("Use push instructions to save outgoing arguments") },		      ¥
  { "no-accumulate-outgoing-args",MASK_ACCUMULATE_OUTGOING_ARGS_SET,	      ¥
    N_("Do not use push instructions to save outgoing arguments") },	      ¥
  { "mmx",			 MASK_MMX | MASK_MMX_SET,		      ¥
    N_("Support MMX built-in functions") },				      ¥
  { "no-mmx",			 -MASK_MMX,				      ¥
    N_("Do not support MMX built-in functions") },			      ¥
  { "no-mmx",			 MASK_MMX_SET, "" },			      ¥
  { "3dnow",                     MASK_3DNOW | MASK_3DNOW_SET,		      ¥
    N_("Support 3DNow! built-in functions") },				      ¥
  { "no-3dnow",                  -MASK_3DNOW, "" },			      ¥
  { "no-3dnow",                  MASK_3DNOW_SET,			      ¥
    N_("Do not support 3DNow! built-in functions") },			      ¥
  { "sse",			 MASK_SSE | MASK_SSE_SET,		      ¥
    N_("Support MMX and SSE built-in functions and code generation") },	      ¥
  { "no-sse",			 -MASK_SSE, "" },	 		      ¥
  { "no-sse",			 MASK_SSE_SET,				      ¥
    N_("Do not support MMX and SSE built-in functions and code generation") },¥
  { "sse2",			 MASK_SSE2 | MASK_SSE2_SET,		      ¥
    N_("Support MMX, SSE and SSE2 built-in functions and code generation") }, ¥
  { "no-sse2",			 -MASK_SSE2, "" },			      ¥
  { "no-sse2",			 MASK_SSE2_SET,				      ¥
    N_("Do not support MMX, SSE and SSE2 built-in functions and code generation") },    ¥
  { "128bit-long-double",	 MASK_128BIT_LONG_DOUBLE,		      ¥
    N_("sizeof(long double) is 16") },					      ¥
  { "96bit-long-double",	-MASK_128BIT_LONG_DOUBLE,		      ¥
    N_("sizeof(long double) is 12") },					      ¥
  { "64",			MASK_64BIT,				      ¥
    N_("Generate 64bit x86-64 code") },					      ¥
  { "32",			-MASK_64BIT,				      ¥
    N_("Generate 32bit i386 code") },					      ¥
  { "red-zone",			-MASK_NO_RED_ZONE,			      ¥
    N_("Use red-zone in the x86-64 code") },				      ¥
  { "no-red-zone",		MASK_NO_RED_ZONE,			      ¥
    N_("Do not use red-zone in the x86-64 code") },			      ¥
  SUBTARGET_SWITCHES							      ¥
  { "", TARGET_DEFAULT, 0 }}

#ifdef TARGET_64BIT_DEFAULT
#define TARGET_DEFAULT (MASK_64BIT | TARGET_SUBTARGET_DEFAULT)
#else
#define TARGET_DEFAULT TARGET_SUBTARGET_DEFAULT
#endif

/* Which processor to schedule for. The cpu attribute defines a list that
   mirrors this list, so changes to i386.md must be made at the same time.  */

enum processor_type
{
  PROCESSOR_I386,			/* 80386 */
  PROCESSOR_I486,			/* 80486DX, 80486SX, 80486DX[24] */
  PROCESSOR_PENTIUM,
  PROCESSOR_PENTIUMPRO,
  PROCESSOR_K6,
  PROCESSOR_ATHLON,
  PROCESSOR_PENTIUM4,
  PROCESSOR_max
};
enum fpmath_unit
{
  FPMATH_387 = 1,
  FPMATH_SSE = 2
};

extern enum processor_type ix86_cpu;
extern enum fpmath_unit ix86_fpmath;

extern int ix86_arch;

/* This macro is similar to `TARGET_SWITCHES' but defines names of
   command options that have values.  Its definition is an
   initializer with a subgrouping for each command option.

   Each subgrouping contains a string constant, that defines the
   fixed part of the option name, and the address of a variable.  The
   variable, type `char *', is set to the variable part of the given
   option if the fixed part matches.  The actual option name is made
   by appending `-m' to the specified name.  */
#define TARGET_OPTIONS						¥
{ { "cpu=",		&ix86_cpu_string,			¥
    N_("Schedule code for given CPU")},				¥
  { "fpmath=",		&ix86_fpmath_string,			¥
    N_("Generate floating point mathematics using given instruction set")},¥
  { "arch=",		&ix86_arch_string,			¥
    N_("Generate c