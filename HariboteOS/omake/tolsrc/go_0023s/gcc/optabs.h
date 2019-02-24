/* Definitions for code generation pass of GNU compiler.
   Copyright (C) 2001 Free Software Foundation, Inc.

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

GCC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GCC; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */

#ifndef GCC_OPTABS_H
#define GCC_OPTABS_H

#include "insn-codes.h"

/* Optabs are tables saying how to generate insn bodies
   for various machine modes and numbers of operands.
   Each optab applies to one operation.
   For example, add_optab applies to addition.

   The insn_code slot is the enum insn_code that says how to
   generate an insn for this operation on a particular machine mode.
   It is CODE_FOR_nothing if there is no such insn on the target machine.

   The `lib_call' slot is the name of the library function that
   can be used to perform the operation.

   A few optabs, such as move_optab and cmp_optab, are used
   by special code.  */

typedef struct optab
{
  enum rtx_code code;
  struct {
    enum insn_code insn_code;
    rtx libfunc;
  } handlers [NUM_MACHINE_MODES];
} * optab;

/* Given an enum insn_code, access the function to construct
   the body of that kind of insn.  */
#define GEN_FCN(CODE) (*insn_data[(int) (CODE)].genfun)

/* Enumeration of valid indexes into optab_table.  */
enum optab_index
{
  OTI_add,
  OTI_addv,
  OTI_sub,
  OTI_subv,

  /* Signed and fp multiply */
  OTI_smul,
  OTI_smulv,
  /* Signed multiply, return high word */
  OTI_smul_highpart,
  OTI_umul_highpart,
  /* Signed multiply with result one machine mode wider than args */
  OTI_smul_widen,
  OTI_umul_widen,

  /* Signed divide */
  OTI_sdiv,
  OTI_sdivv,
  /* Signed divide-and-remainder in one */
  OTI_sdivmod,
  OTI_udiv,
  OTI_udivmod,
  /* Signed remainder */
  OTI_smod,
  OTI_umod,
  /* Convert float to integer in float fmt */
  OTI_ftrunc,

  /* Logical and */
  OTI_and,
  /* Logical or */
  OTI_ior,
  /* Logical xor */
  OTI_xor,

  /* Arithmetic shift left */
  OTI_ashl,
  /* Logical shift right */
  OTI_lshr,  
  /* Arithmetic shift right */
  OTI_ashr,
  /* Rotate left */
  OTI_rotl,
  /* Rotate right */
  OTI_rotr,
  /* Signed and floating-point minimum value */
  OTI_smin,
  /* Signed and floating-point maximum value */
  OTI_smax,
  /* Unsigned minimum value */
  OTI_umin,
  /* Unsigned maximum value */
  OTI_umax,

  /* Move instruction.  */
  OTI_mov,
  /* Move, preserving high part of register.  */
  OTI_movstrict,

  /* Unary operations */
  /* Negation */
  OTI_neg,
  OTI_negv,
  /* Abs value */
  OTI_abs,
  OTI_absv,
  /* Bitwise not */
  OTI_one_cmpl,
  /* Find first bit set */
  OTI_ffs,
  /* Square root */
  OTI_sqrt,
  /* Sine */
  OTI_sin,
  /* Cosine */
  OTI_cos,

  /* Compare insn; two operands.  */
  OTI_cmp,
  /* Used only for libcalls for unsigned comparisons.  */
  OTI_ucmp,
  /* tst insn; compare one operand against 0 */
  OTI_tst,

  /* String length */
  OTI_strlen,

  /* Combined compare & jump/store flags/move operations.  */
  OTI_cbranch,
  OTI_cmov,
  OTI_cstore,
    
  /* Push instruction.  */
  OTI_push,

  OTI_MAX
};

extern optab optab_table[OTI_MAX];

#define add_optab (optab_table[OTI_add])
#define sub_optab (optab_table[OTI_sub])
#define smul_optab (optab_table[OTI_smul])
#define addv_optab (optab_table[OTI_addv])
#define subv_optab (optab_table[OTI_subv])
#define smul_highpart_optab (optab_table[OTI_smul_highpart])
#define umul_highpa