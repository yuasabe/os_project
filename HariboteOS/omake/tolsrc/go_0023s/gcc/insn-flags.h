/* Generated automatically by the program `genflags'
   from the machine description file `md'.  */

#ifndef GCC_INSN_FLAGS_H
#define GCC_INSN_FLAGS_H

#define HAVE_cmpdi_ccno_1_rex64 (TARGET_64BIT && ix86_match_ccmode (insn, CCNOmode))
#define HAVE_cmpdi_1_insn_rex64 (TARGET_64BIT && ix86_match_ccmode (insn, CCmode))
#define HAVE_cmpqi_ext_3_insn (!TARGET_64BIT && ix86_match_ccmode (insn, CCmode))
#define HAVE_cmpqi_ext_3_insn_rex64 (TARGET_64BIT && ix86_match_ccmode (insn, CCmode))
#define HAVE_x86_fnstsw_1 (TARGET_80387)
#define HAVE_x86_sahf_1 (!TARGET_64BIT)
#define HAVE_popsi1 (!TARGET_64BIT)
#define HAVE_movsi_insv_1 (!TARGET_64BIT)
#define HAVE_pushdi2_rex64 (TARGET_64BIT)
#define HAVE_popdi1 (TARGET_64BIT)
#define HAVE_swapxf 1
#define HAVE_swaptf 1
#define HAVE_zero_extendhisi2_and (TARGET_ZERO_EXTEND_WITH_AND && !optimize_size)
#define HAVE_zero_extendsidi2_32 (!TARGET_64BIT)
#define HAVE_zero_extendsidi2_rex64 (TARGET_64BIT)
#define HAVE_zero_extendhidi2 (TARGET_64BIT)
#define HAVE_zero_extendqidi2 (TARGET_64BIT)
#define HAVE_extendsidi2_rex64 (TARGET_64BIT)
#define HAVE_extendhidi2 (TARGET_64BIT)
#define HAVE_extendqidi2 (TARGET_64BIT)
#define HAVE_extendhisi2 1
#define HAVE_extendqihi2 1
#define HAVE_extendqisi2 1
#define HAVE_truncdfsf2_3 (TARGET_80387)
#define HAVE_truncdfsf2_sse_only (!TARGET_80387 && TARGET_SSE2)
#define HAVE_fix_truncdi_nomemory (TARGET_80387 && FLOAT_MODE_P (GET_MODE (operands[1])) ¥
   && (!SSE_FLOAT_MODE_P (GET_MODE (operands[1])) || !TARGET_64BIT))
#define HAVE_fix_truncdi_memory (TARGET_80387 && FLOAT_MODE_P (GET_MODE (operands[1])) ¥
   && (!SSE_FLOAT_MODE_P (GET_MODE (operands[1])) || !TARGET_64BIT))
#define HAVE_fix_truncsfdi_sse (TARGET_64BIT && TARGET_SSE)
#define HAVE_fix_truncdfdi_sse (TARGET_64BIT && TARGET_SSE2)
#define HAVE_fix_truncsi_nomemory (TARGET_80387 && FLOAT_MODE_P (GET_MODE (operands[1])) ¥
   && !SSE_FLOAT_MODE_P (GET_MODE (operands[1])))
#define HAVE_fix_truncsi_memory (TARGET_80387 && FLOAT_MODE_P (GET_MODE (operands[1])) ¥
   && !SSE_FLOAT_MODE_P (GET_MODE (operands[1])))
#define HAVE_fix_truncsfsi_sse (TARGET_SSE)
#define HAVE_fix_truncdfsi_sse (TARGET_SSE2)
#define HAVE_fix_trunchi_nomemory (TARGET_80387 && FLOAT_MODE_P (GET_MODE (operands[1])) ¥
   && !SSE_FLOAT_MODE_P (GET_MODE (operands[1])))
#define HAVE_fix_trunchi_memory (TARGET_80387 && FLOAT_MODE_P (GET_MODE (operands[1])) ¥
   && !SSE_FLOAT_MODE_P (GET_MODE (operands[1])))
#define HAVE_x86_fnstcw_1 (TARGET_80387)
#define HAVE_x86_fldcw_1 (TARGET_80387)
#define HAVE_floathisf2 (TARGET_80387 && !TARGET_SSE)
#define HAVE_floathidf2 (TARGET_80387 && !TARGET_SSE2)
#define HAVE_floathixf2 (!TARGET_64BIT && TARGET_80387)
#define HAVE_floathitf2 (TARGET_80387)
#define HAVE_floatsixf2 (!TARGET_64BIT && TARGET_80387)
#define HAVE_floatsitf2 (TARGET_80387)
#define HAVE_floatdixf2 (!TARGET_64BIT && TARGET_80387)
#define HAVE_floatditf2 (TARGET_80387)
#define HAVE_addqi3_cc (ix86_binary_operator_ok (PLUS, QImode, operands))
#define HAVE_addsi_1_zext (TARGET_64BIT && ix86_binary_operator_ok (PLUS, SImode, operands))
#define HAVE_addqi_ext_1 (!TARGET_64BIT)
#define HAVE_subdi3_carry_rex64 (TARGET_64BIT && ix86_binary_operator_ok (MINUS, DImode, operands))
#define HAVE_subsi3_carry (ix86_binary_operator_ok (MINUS, SImode, operands))
#define HAVE_subsi3_carry_zext (TARGET_64BIT && ix86_binary_operator_ok (MINUS, SImode, operands))
#define HAVE_divqi3 (TARGET_QIMODE_MATH)
#define HAVE_udivqi3 (TARGET_QIMODE_MATH)
#define HAVE_divmodhi4 (TARGET_HIMODE_MATH)
#define HAVE_udivmoddi4 (TARGET_64BIT)
#define HAVE_udivmodsi4 1
#define HAVE_testsi_1 (ix86_match_ccmode (insn, CCNOmode))
#define HAVE_andqi_ext_0 ((unsigned HOST_WIDE_INT)INTVAL (operands[2]) <= 0xff)
#define HAVE_negsf2_memory (ix86_unary_operator_ok (NEG, SFmode, operands))
#define HAVE_negsf2_ifs (TARGET_SSE ¥
   && (reload_in_progress || reload_completed ¥
       || (register_operand (operands[0], VOIDmode) ¥
	   && register_operand (operands[1], VOIDmode))))
#define HAVE_negdf2_memory (ix86_unary_operator_ok (NEG, DFmode, operands))
#define HAVE_negdf2_ifs (!TARGET_64BIT && TARGET_SSE2 ¥
   && (reload_in_progress || reload_completed ¥
       || (register_operand (operands[0], VOIDmode) ¥
	   && register_operand (operands[1], VOIDmode))))
#define HAVE_abssf2_memory (ix86_unary_operator_ok (ABS, SFmode, operands))
#define HAVE_abssf2_ifs (TARGET_SSE ¥
   && (reload_in_progress || reload_completed ¥
       || (register_operand (operands[0], VOIDmode) ¥
	   && register_operand (operands[1], VOIDmode))))
#define HAVE_absdf2_memory (ix86_unary_operator_ok (ABS, DFmode, operands))
#define HAVE_absdf2_ifs (!TARGET_64BIT && TARGET_SSE2 ¥
   && (reload_in_progress || reload_completed ¥
       || (register_operand (operands[0], VOIDmode) ¥
	   && register_operand (operands[1], VOIDmode))))
#define HAVE_ashldi3_1 (!TARGET_64BIT && TARGET_CMOVE)
#define HAVE_x86_shld_1 1
#define HAVE_ashrdi3_63_rex64 (TARGET_64BIT && INTVAL (operands[2]) == 63 && (TARGET_USE_CLTD || optimize_size) ¥
   && ix86_binary_operator_ok (ASHIFTRT, DImode, operands))
#define HAVE_ashrdi3_1 (!TARGET_64BIT && TARGET_CMOVE)
#define HAVE_x86_shrd_1 1
#define HAVE_ashrsi3_31 (INTVAL (operands[2]) == 31 && (TARGET_USE_CLTD || optimize_size) ¥
   && ix86_binary_operator_ok (ASHIFTRT, SImode, operands))
#define HAVE_lshrdi3_1 (!TARGET_64BIT && TARGET_CMOVE)
#define HAVE_setcc_2 1
#define HAVE_jump 1
#define HAVE_doloop_end_internal (!TARGET_64BIT && TARGET_USE_LOOP)
#define HAVE_blockage 1
#define HAVE_return_internal (reload_completed)
#define HAVE_return_pop_internal (reload_completed)
#define HAVE_return_indirect_internal (reload_completed)
#define HAVE_nop 1
#define HAVE_prologue_set_got (!TARGET_64BIT)
#define HAVE_prologue_get_pc (!TARGET_64BIT)
#define HAVE_eh_return_si (!TARGET_64BIT)
#define HAVE_eh_return_di (TARGET_64BIT)
#define HAVE_leave (!TARGET_64BIT)
#define HAVE_leave_rex64 (TARGET_64BIT)
#define HAVE_ffssi_1 1
#define HAVE_sqrtsf2_1 (! TARGET_NO_FANCY_MATH_387 && TARGET_80387 ¥
   && (TARGET_SSE_MATH && TARGET_MIX_SSE_I387))
#define HAVE_sqrtsf2_1_sse_only (TARGET_SSE_MATH && (!TARGET_80387 || !TARGET_MIX_SSE_I387))
#define HAVE_sqrtsf2_i387 (! TARGET_NO_FANCY_MATH_387 && TARGET_80387 ¥
   && !TARGET_SSE_MATH)
#define HAVE_sqrtdf2_1 (! TARGET_NO_FANCY_MATH_387 && TARGET_80387 ¥
   && (TARGET_SSE2 && TARGET_SSE_MATH && TARGET_MIX_SSE_I387))
#define HAVE_sqrtdf2_1_sse_only (TARGET_SSE2 && TARGET_SSE_MATH && (!TARGET_80387 || !TARGET_MIX_SSE_I387))
#define HAVE_sqrtdf2_i387 (! TARGET_NO_FANCY_MATH_387 && TARGET_80387 ¥
   && (!TARGET_SSE2 || !TARGET_SSE_MATH))
#define HAVE_sqrtxf2 (!TARGET_64BIT && TARGET_80387 && !TARGET_NO_FANCY_MATH_387  ¥
   && (TARGET_IEEE_FP || flag_unsafe_math_optimizations) )
#define HAVE_sqrttf2 (! TARGET_NO_FANCY_MATH_387 && TARGET_80387  ¥
   && (TARGET_IEEE_FP || flag_unsafe_math_optimizations) )
#define HAVE_sindf2 (! TARGET_NO_FANCY_MATH_387 && TARGET_80387  ¥
   && flag_unsafe_math_optimizations)
#define HAVE_sinsf2 (! TARGET_NO_FANCY_MATH_387 && TARGET_80387  ¥
   && flag_unsafe_math_optimizations)
#define HAVE_sinxf2 (!TARGET_64BIT && TARGET_80387 && !TARGET_NO_FANCY_MATH_387 ¥
   && flag_unsafe_math_optimizations)
#define HAVE_sintf2 (! TARGET_NO_FANCY_MATH_387 && TARGET_80387  ¥
   && flag_unsafe_math_optimizations)
#define HAVE_cosdf2 (! TARGET_NO_FANCY_MATH_387 && TARGET_80387  ¥
   && flag_unsafe_math_optimizations)
#define HAVE_cossf2 (! TARGET_NO_FANCY_MATH_387 && TARGET_80387  ¥
   && flag_unsafe_math_optimizations)
#define HAVE_cosxf2 (! TARGET_NO_FANCY_MATH_387 && TARGET_80387 ¥
   && flag_unsafe_math_optimizations)
#define HAVE_costf2 (! TARGET_NO_FANCY_MATH_387 && TARGET_80387  ¥
   && flag_unsafe_math_optimizations)
#define HAVE_cld 1
#define HAVE_strmovdi_rex_1 (TARGET_64BIT && (TARGET_SINGLE_STRINGOP || optimize_size))
#define HAVE_strmovsi_1 (!TARGET_64BIT && (TARGET_SINGLE_STRINGOP || optimize_size))
#define HAVE_strmovsi_rex_1 (TARGET_64BIT && (TARGET_SINGLE_STRINGOP || optimize_size))
#define HAVE_strmovhi_1 (!TARGET_64BIT && (TARGET_SINGLE_STRINGOP || optimize_size))
#define HAVE_strmovhi_rex_1 (TARGET_64BIT && (TARGET_SINGLE_STRINGOP || optimize_size))
#define HAVE_strmovqi_1 (!TARGET_64BIT && (TARGET_SINGLE_STRINGOP || optimize_size))
#define HAVE_strmovqi_rex_1 (TARGET_64BIT && (TARGET_SINGLE_STRINGOP || optimize_size))
#define HAVE_rep_movdi_rex64 (TARGET_64BIT)
#define HAVE_rep_movsi (!TARGET_64BIT)
#define HAVE_rep_movsi_rex64 (TARGET_64BIT)
#define HAVE_rep_movqi (!TARGET_64BIT)
#define HAVE_rep_movqi_rex64 (TARGET_64BIT)
#define HAVE_strsetdi_rex_1 (TARGET_64BIT && (TARGET_SINGLE_STRINGOP || optimize_size))
#define HAVE_strsetsi_1 (!TARGET_64BIT && (TARGET_SINGLE_STRINGOP || optimize_size))
#define HAVE_strsetsi_rex_1 (TARGET_64BIT && (TARGET_SINGLE_STRINGOP || optimize_size))
#define HAVE_strsethi_1 (!TARGET_64BIT && (TARGET_SINGLE_STRINGOP || optimize_size))
#define HAVE_strsethi_rex_1 (TARGET_64BIT && (TARGET_SINGLE_STRINGOP || optimize_size))
#define HAVE_strsetqi_1 (!TARGET_64BIT && (TARGET_SINGLE_STRINGOP || optimize_size))
#define HAVE_strsetqi_rex_1 (TARGET_64BIT && (TARGET_SINGLE_STRINGOP || optimize_size))
#define HAVE_rep_stosdi_rex64 (TARGET_64BIT)
#define HAVE_rep_stossi (!TARGET_64BIT)
#define HAVE_rep_stossi_rex64 (TARGET_64BIT)
#define HAVE_rep_stosqi (!TARGET_64BIT)
#define HAVE_rep_stosqi_rex64 (TARGET_64BIT)
#define HAVE_cmpstrqi_nz_1 (!TARGET_64BIT)
#define HAVE_cmpstrqi_nz_rex_1 (TARGET_64BIT)
#define HAVE_cmpstrqi_1 (!TARGET_64BIT)
#define HAVE_cmpstrqi_rex_1 (TARGET_64BIT)
#define HAVE_strlenqi_1 (!TARGET_64BIT)
#define HAVE_strlenqi_rex_1 (TARGET_64BIT)
#define HAVE_x86_movdicc_0_m1_rex64 (TARGET_64BIT)
#define HAVE_x86_movsicc_0_m1 1
#define HAVE_pro_epilogue_adjust_stack_rex64 (TARGET_64BIT)
#define HAVE_sse_movsfcc (TARGET_SSE ¥
   && (GET_CODE (operands[2]) != MEM || GET_CODE (operands[3]) != MEM) ¥
   && (!TARGET_IEEE_FP ¥
       || (GET_CODE (operands[1]) != EQ && GET_CODE (operands[1]) != NE)))
#define HAVE_sse_movsfcc_eq (TARGET_SSE ¥
   && (GET_CODE (operands[2]) != MEM || GET_CODE (operands[3]) != MEM))
#define HAVE_sse_movdfcc (TARGET_SSE2 ¥
   && (GET_CODE (operands[2]) != MEM || GET_CODE (operands[3]) != MEM) ¥
   && (!TARGET_IEEE_FP ¥
       || (GET_CODE (operands[1]) != EQ && GET_CODE (operands[1]) != NE)))
#define HAVE_sse_movdfcc_eq (TARGET_SSE ¥
   && (GET_CODE (operands[2]) != MEM || GET_CODE (operands[3]) != MEM))
#define HAVE_allocate_stack_worker_1 (!TARGET_64BIT && TARGET_STACK_PROBE)
#define HAVE_allocate_stack_worker_rex64 (TARGET_64BIT && TARGET_STACK_PROBE)
#define HAVE_trap 1
#define HAVE_movv4sf_internal (TARGET_SSE)
#define HAVE_movv4si_internal (TARGET_SSE)
#define HAVE_movv8qi_internal (TARGET_MMX)
#define HAVE_movv4hi_internal (TARGET_MMX)
#define HAVE_movv2si_internal (TARGET_MMX)
#define HAVE_movv2sf_internal (TARGET_3DNOW)
#define HAVE_movti_internal (TARGET_SSE && !TARGET_64BIT)
#define HAVE_sse_movaps (TARGET_SSE)
#define HAVE_sse_movups (TARGET_SSE)
#define HAVE_sse_movmskps (TARGET_SSE)
#define HAVE_mmx_pmovmskb (TARGET_SSE || TARGET_3DNOW_A)
#define HAVE_mmx_maskmovq ((TARGET_SSE || TARGET_3DNOW_A) && !TARGET_64BIT)
#define HAVE_mmx_maskmovq_rex ((TARGET_SSE || TARGET_3DNOW_A) && TARGET_64BIT)
#define HAVE_sse_movntv4sf (TARGET_SSE)
#define HAVE_sse_movntdi (TARGET_SSE || TARGET_3DNOW_A)
#define HAVE_sse_movhlps (TARGET_SSE)
#define HAVE_sse_movlhps (TARGET_SSE)
#define HAVE_sse_movhps (TARGET_SSE ¥
   && (GET_CODE (operands[1]) == MEM || GET_CODE (operands[2]) == MEM))
#define HAVE_sse_movlps (TARGET_SSE ¥
   && (GET_CODE (operands[1]) == MEM || GET_CODE (operands[2]) == MEM))
#define HAVE_sse_loadss (TARGET_SSE)
#define HAVE_sse_movss (TARGET_SSE)
#define HAVE_sse_storess (TARGET_SSE)
#define HAVE_sse_shufps (TARGET_SSE)
#define HAVE_addv4sf3 (TARGET_SSE)
#define HAVE_vmaddv4sf3 (TARGET_SSE)
#define HAVE_subv4sf3 (TARGET_SSE)
#define HAVE_vmsubv4sf3 (TARGET_SSE)
#define HAVE_mulv4sf3 (TARGET_SSE)
#define HAVE_vmmulv4sf3 (TARGET_SSE)
#define HAVE_divv4sf3 (TARGET_SSE)
#