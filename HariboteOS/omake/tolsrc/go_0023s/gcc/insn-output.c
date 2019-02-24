/* Generated automatically by the program `genoutput'
   from the machine description file `md'.  */

#include "config.h"
#include "system.h"
#include "flags.h"
#include "ggc.h"
#include "rtl.h"
#include "expr.h"
#include "insn-codes.h"
#include "tm_p.h"
#include "function.h"
#include "regs.h"
#include "hard-reg-set.h"
#include "real.h"
#include "insn-config.h"

#include "conditions.h"
#include "insn-attr.h"

#include "recog.h"

#include "toplev.h"
#include "output.h"

static const char * const output_0[] = {
  "test{q}¥t{%0, %0|%0, %0}",
  "cmp{q}¥t{%1, %0|%0, %1}",
};

static const char * const output_3[] = {
  "test{l}¥t{%0, %0|%0, %0}",
  "cmp{l}¥t{%1, %0|%0, %1}",
};

static const char * const output_6[] = {
  "test{w}¥t{%0, %0|%0, %0}",
  "cmp{w}¥t{%1, %0|%0, %1}",
};

static const char * const output_9[] = {
  "test{b}¥t{%0, %0|%0, %0}",
  "cmp{b}¥t{$0, %0|%0, 0}",
};

static const char *output_18 PARAMS ((rtx *, rtx));

static const char *
output_18 (operands, insn)
     rtx *operands ATTRIBUTE_UNUSED;
     rtx insn ATTRIBUTE_UNUSED;
{
{
  if (find_regno_note (insn, REG_DEAD, REGNO (operands[1])))
    return "ftst¥n¥tfnstsw¥t%0¥n¥tfstp¥t%y0";
  else
    return "ftst¥n¥tfnstsw¥t%0";
}
}

static const char *output_19 PARAMS ((rtx *, rtx));

static const char *
output_19 (operands, insn)
     rtx *operands ATTRIBUTE_UNUSED;
     rtx insn ATTRIBUTE_UNUSED;
{
 return output_fp_compare (insn, operands, 0, 0);
}

static const char *output_20 PARAMS ((rtx *, rtx));

static const char *
output_20 (operands, insn)
     rtx *operands ATTRIBUTE_UNUSED;
     rtx insn ATTRIBUTE_UNUSED;
{
 return output_fp_compare (insn, operands, 2, 0);
}

static const char *output_21 PARAMS ((rtx *, rtx));

static const char *
output_21 (operands, insn)
     rtx *operands ATTRIBUTE_UNUSED;
     rtx insn ATTRIBUTE_UNUSED;
{
 return output_fp_compare (insn, operands, 0, 0);
}

static const char *output_22 PARAMS ((rtx *, rtx));

static const char *
output_22 (operands, insn)
     rtx *operands ATTRIBUTE_UNUSED;
     rtx insn ATTRIBUTE_UNUSED;
{
 return output_fp_compare (insn, operands, 2, 0);
}

static const char *output_23 PARAMS ((rtx *, rtx));

static const char *
output_23 (operands, insn)
     rtx *operands ATTRIBUTE_UNUSED;
     rtx insn ATTRIBUTE_UNUSED;
{
 return output_fp_compare (insn, operands, 0, 0);
}

static const char *output_24 PARAMS ((rtx *, rtx));

static const char *
output_24 (operands, insn)
     rtx *operands ATTRIBUTE_UNUSED;
     rtx insn ATTRIBUTE_UNUSED;
{
 return output_fp_compare (insn, operands, 0, 0);
}

static const char *output_25 PARAMS ((rtx *, rtx));

static const char *
output_25 (operands, insn)
     rtx *operands ATTRIBUTE_UNUSED;
     rtx insn ATTRIBUTE_UNUSED;
{
 return output_fp_compare (insn, operands, 2, 0);
}

static const char *output_26 PARAMS ((rtx *, rtx));

static const char *
output_26 (operands, insn)
     rtx *operands ATTRIBUTE_UNUSED;
     rtx insn ATTRIBUTE_UNUSED;
{
 return output_fp_compare (insn, operands, 2, 0);
}

static const char *output_27 PARAMS ((rtx *, rtx));

static const char *
output_27 (operands, insn)
     rtx *operands ATTRIBUTE_UNUSED;
     rtx insn ATTRIBUTE_UNUSED;
{
 return output_fp_compare (insn, operands, 0, 1);
}

static const char *output_28 PARAMS ((rtx *, rtx));

static const char *
output_28 (operands, insn)
     rtx *operands ATTRIBUTE_UNUSED;
     rtx insn ATTRIBUTE_UNUSED;
{
 return output_fp_compare (insn, operands, 2, 1);
}

static const char *output_32 PARAMS ((rtx *, rtx));

static const char *
output_32 (operands, insn)
     rtx *operands ATTRIBUTE_UNUSED;
     rtx insn ATTRIBUTE_UNUSED;
{
 return output_fp_compare (insn, operands, 1, 0);
}

static const char *output_33 PARAMS ((rtx *, rtx));

static const char *
output_33 (operands, insn)
     rtx *operands ATTRIBUTE_UNUSED;
     rtx insn ATTRIBUTE_UNUSED;
{
 return output_fp_compare (insn, operands, 1, 0);
}

static const char *output_34 PARAMS ((rtx *, rtx));

static const char *
output_34 (operands, insn)
     rtx *operands ATTRIBUTE_UNUSED;
     rtx insn ATTRIBUTE_UNUSED;
{
 return output_fp_compare (insn, operands, 1, 0);
}

static const char *output_35 PARAMS ((rtx *, rtx));

static const char *
output_35 (operands, insn)
     rtx *operands ATTRIBUTE_UNUSED;
     rtx insn ATTRIBUTE_UNUSED;
{
 return output_fp_compare (insn, operands, 1, 1);
}

static const char *output_36 PARAMS ((rtx *, rtx));

static const char *
output_36 (operands, insn)
     rtx *operands ATTRIBUTE_UNUSED;
     rtx insn ATTRIBUTE_UNUSED;
{
 return output_fp_compare (insn, operands, 1, 1);
}

static const char *output_37 PARAMS ((rtx *, rtx));

static const char *
output_37 (operands, insn)
     rtx *operands ATTRIBUTE_UNUSED;
     rtx insn ATTRIBUTE_UNUSED;
{
 return output_fp_compare (insn, operands, 1, 1);
}

static const char *output_44 PARAMS ((rtx *, rtx));

static const char *
output_44 (operands, insn)
     rtx *operands ATTRIBUTE_UNUSED;
     rtx insn ATTRIBUTE_UNUSED;
{
{
  operands[1] = constm1_rtx;
  return "or{l}¥t{%1, %0|%0, %1}";
}
}

static const char *output_45 PARAMS ((rtx *, rtx));

static const char *
output_45 (operands, insn)
     rtx *operands ATTRIBUTE_UNUSED;
     rtx insn ATTRIBUTE_UNUSED;
{
{
  switch (get_attr_type (insn))
    {
    case TYPE_SSE:
      if (get_attr_mode (insn) == TImode)
        return "movdqa¥t{%1, %0|%0, %1}";
      return "movd¥t{%1, %0|%0, %1}";

    case TYPE_MMX:
      if (get_attr_mode (insn) == DImode)
	return "movq¥t{%1, %0|%0, %1}";
      return "movd¥t{%1, %0|%0, %1}";

    case TYPE_LEA:
      return "lea{l}¥t{%1, %0|%0, %1}";

    default:
      if (flag_pic && SYMBOLIC_CONST (operands[1]))
	abort();
      return "mov{l}¥t{%1, %0|%0, %1}";
    }
}
}

static const char * const output_46[] = {
  "movabs{l}¥t{%1, %P0|%P0, %1}",
  "mov{l}¥t{%1, %a0|%a0, %1}",
  "movabs{l}¥t{%1, %a0|%a0, %1}",
};

static const char * const output_47[] = {
  "movabs{l}¥t{%P1, %0|%0, %P1}",
  "mov{l}¥t{%a1, %0|%0, %a1}",
};

static const char * const output_49[] = {
  "push{w}¥t{|WORD PTR }%1",
  "push{w}¥t%1",
};

static const char *output_51 PARAMS ((rtx *, rtx));

static const char *
output_51 (operands, insn)
     rtx *operands ATTRIBUTE_UNUSED;
     rtx insn ATTRIBUTE_UNUSED;
{
{
  switch (get_attr_type (insn))
    {
    case TYPE_IMOVX:
      /* movzwl is faster than movw on p2 due to partial word stalls,
	 though not as fast as an aligned movl.  */
      return "movz{wl|x}¥t{%1, %k0|%k0, %1}";
    default:
      if (get_attr_mode (insn) == MODE_SI)
        return "mov{l}¥t{%k1, %k0|%k0, %k1}";
      else
        return "mov{w}¥t{%1, %0|%0, %1}";
    }
}
}

static const char * const output_52[] = {
  "movabs{w}¥t{%1, %P0|%P0, %1}",
  "mov{w}¥t{%1, %a0|%a0, %1}",
  "movabs{w}¥t{%1, %a0|%a0, %1}",
};

static const char * const output_53[] = {
  "movabs{w}¥t{%P1, %0|%0, %P1}",
  "mov{w}¥t{%a1, %0|%0, %a1}",
};

static const char * const output_58[] = {
  "push{w}¥t{|word ptr }%1",
  "push{w}¥t%w1",
};

static const char *output_60 PARAMS ((rtx *, rtx));

static const char *
output_60 (operands, insn)
     rtx *operands ATTRIBUTE_UNUSED;
     rtx insn ATTRIBUTE_UNUSED;
{
{
  switch (get_attr_type (insn))
    {
    case TYPE_IMOVX:
      if (!ANY_QI_REG_P (operands[1]) && GET_CODE (operands[1]) != MEM)
	abort ();
      return "movz{bl|x}¥t{%1, %k0|%k0, %1}";
    default:
      if (get_attr_mode (insn) == MODE_SI)
        return "mov{l}¥t{%k1, %k0|%k0, %k1}";
      else
        return "mov{b}¥t{%1, %0|%0, %1}";
    }
}
}

static const char *output_66 PARAMS ((rtx *, rtx));

static const char *
output_66 (operands, insn)
     rtx *operands ATTRIBUTE_UNUSED;
     rtx insn ATTRIBUTE_UNUSED;
{
{
  switch (get_attr_type (insn))
    {
    case TYPE_IMOVX:
      return "movs{bl|x}¥t{%h1, %k0|%k0, %h1}";
    default:
      return "mov{b}¥t{%h1, %0|%0, %h1}";
    }
}
}

static const char *output_67 PARAMS ((rtx *, rtx));

static const char *
output_67 (operands, insn)
     rtx *operands ATTRIBUTE_UNUSED;
     rtx insn ATTRIBUTE_UNUSED;
{
{
  switch (get_attr_type (insn))
    {
    case TYPE_IMOVX:
      return "movs{bl|x}¥t{%h1, %k0|%k0, %h1}";
    default:
      return "mov{b}¥t{%h1, %0|%0, %h1}";
    }
}
}

static const char * const output_68[] = {
  "movabs{b}¥t{%1, %P0|%P0, %1}",
  "mov{b}¥t{%1, %a0|%a0, %1}",
  "movabs{b}¥t{%1, %a0|%a0, %1}",
};

static const char * const output_69[] = {
  "movabs{b}¥t{%P1, %0|%0, %P1}",
  "mov{b}¥t{%a1, %0|%0, %a1}",
};

static const char *output_71 PARAMS ((rtx *, rtx));

static const char *
output_71 (operands, insn)
     rtx *operands ATTRIBUTE_UNUSED;
     rtx insn ATTRIBUTE_UNUSED;
{
{
  switch (get_attr_type (insn))
    {
    case TYPE_IMOVX:
      return "movz{bl|x}¥t{%h1, %k0|%k0, %h1}";
    default:
      return "mov{b}¥t{%h1, %0|%0, %h1}";
    }
}
}

static const char *output_72 PARAMS ((rtx *, rtx));

static const char *
output_72 (operands, insn)
     rtx *operands ATTRIBUTE_UNUSED;
     rtx insn ATTRIBUTE_UNUSED;
{
{
  switch (get_attr_type (insn))
    {
    case TYPE_IMOVX:
      return "movz{bl|x}¥t{%h1, %k0|%k0, %h1}";
    default:
      return "mov{b}¥t{%h1, %0|%0, %h1}";
    }
}
}

static const char * const output_77[] = {
  "push{q}¥t%1",
  "#",
};

static const char *output_82 PARAMS ((rtx *, rtx));

static const char *
output_82 (operands, insn)
     rtx *operands ATTRIBUTE_UNUSED;
     rtx insn ATTRIBUTE_UNUSED;
{
{
  operands[1] = constm1_rtx;
  return "or{q}¥t{%1, %0|%0, %1}";
}
}

static const char * const output_83[] = {
  "#",
  "#",
  "movq¥t{%1, %0|%0, %1}",
  "movq¥t{%1, %0|%0, %1}",
  "movq¥t{%1, %0|%0, %1}",
  "movdqa¥t{%1, %0|%0, %1}",
  "movq¥t{%1, %0|%0, %1}",
};

static const char *output_84 PARAMS ((rtx *, rtx));

static const char *
output_84 (operands, insn)
     rtx *operands ATTRIBUTE_UNUSED;
     rtx insn ATTRIBUTE_UNUSED;
{
{
  switch (get_attr_type (insn))
    {
    case TYPE_SSE:
      if (register_operand (operands[0], DImode)
	  && register_operand (operands[1], DImode))
	  return "movdqa¥t{%1, %0|%0, %1}";
      /* FALLTHRU */
    case TYPE_MMX:
      return "movq¥t{%1, %0|%0, %1}";
    case TYPE_MULTI:
      return "#";
    case TYPE_LEA:
      return "lea{q}¥t{%a1, %0|%0, %a1}";
    default:
      if (flag_pic && SYMBOLIC_CONST (operands[1]))
	abort ();
      if (get_attr_mode (insn) == MODE_SI)
	return "mov{l}¥t{%k1, %k0|%k0, %k1}";
      else if (which_alternative == 2)
	return "movabs{q}¥t{%1, %0|%0, %1}";
      else
	return "mov{q}¥t{%1, %0|%0, %1}";
    }
}
}

static const char * const output_85[] = {
  "movabs{q}¥t{%1, %P0|%P0, %1}",
  "mov{q}¥t{%1, %a0|%a0, %1}",
};

static const char * const output_86[] = {
  "movabs{q}¥t{%P1, %0|%0, %P1}",
  "mov{q}¥t{%a1, %0|%0, %a1}",
};

static const char *output_88 PARAMS ((rtx *, rtx));

static const char *
output_88 (operands, insn)
     rtx *operands ATTRIBUTE_UNUSED;
     rtx insn ATTRIBUTE_UNUSED;
{
{
  switch (which_alternative)
    {
    case 0:
      /* %%% We loose REG_DEAD notes for controling pops if we split late.  */
      operands[0] = gen_rtx_MEM (SFmode, stack_pointer_rtx);
      operands[2] = stack_pointer_rtx;
      operands[3] = GEN_INT (4);
      if (find_regno_note (insn, REG_DEAD, REGNO (operands[1])))
	return "sub{l}¥t{%3, %2|%2, %3}¥n¥tfstp%z0¥t%y0";
      else
	return "sub{l}¥t{%3, %2|%2, %3}¥n¥tfst%z0¥t%y0";

    case 1:
      return "push{l}¥t%1";
    case 2:
      return "#";

    default:
      abort ();
    }
}
}

static const char *output_89 PARAMS ((rtx *, rtx));

static const char *
output_89 (operands, insn)
     rtx *operands ATTRIBUTE_UNUSED;
     rtx insn ATTRIBUTE_UNUSED;
{
{
  switch (which_alternative)
    {
    case 0:
      /* %%% We loose REG_DEAD notes for controling pops if we split late.  */
      operands[0] = gen_rtx_MEM (SFmode, stack_pointer_rtx);
      operands[2] = stack_pointer_rtx;
      operands[3] = GEN_INT (8);
      if (find_regno_note (insn, REG_DEAD, REGNO (operands[1])))
	return "sub{q}¥t{%3, %2|%2, %3}¥n¥tfstp%z0¥t%y0";
      else
	return "sub{q}¥t{%3, %2|%2, %3}¥n¥tfst%z0¥t%y0";

    case 1:
      return "push{q}¥t%q1";

    case 2:
      return "#";

    default:
      abort ();
    }
}
}

static const char *output_90 PARAMS ((rtx *, rtx));

static const char *
output_90 (operands, insn)
     rtx *operands ATTRIBUTE_UNUSED;
     rtx insn ATTRIBUTE_UNUSED;
{
{
  switch (which_alternative)
    {
    case 0:
      if (REG_P (operands[1])
          && find_regno_note (insn, REG_DEAD, REGNO (operands[1])))
        return "fstp¥t%y0";
      else if (STACK_TOP_P (operands[0]))
        return "fld%z1¥t%y1";
      else
        return "fst¥t%y0";

    case 1:
      if (find_regno_note (insn, REG_DEAD, REGNO (operands[1])))
        return "fstp%z0¥t%y0";
      else
        return "fst%z0¥t%y0";

    case 2:
      switch (standard_80387_constant_p (operands[1]))
        {
        case 1:
	  return "fldz";
	case 2:
	  return "fld1";
	}
      abort();

    case 3:
    case 4:
      return "mov{l}¥t{%1, %0|%0, %1}";
    case 5:
      if (TARGET_SSE2)
	return "pxor¥t%0, %0";
      else
	return "xorps¥t%0, %0";
    case 6:
      if (TARGET_PARTIAL_REG_DEPENDENCY)
	return "movaps¥t{%1, %0|%0, %1}";
      else
	return "movss¥t{%1, %0|%0, %1}";
    case 7:
    case 8:
      return "movss¥t{%1, %0|%0, %1}";

    case 9:
    case 10:
      return "movd¥t{%1, %0|%0, %1}";

    case 11:
      return "movq¥t{%1, %0|%0, %1}";

    default:
      abort();
    }
}
}

static const char *output_91 PARAMS ((rtx *, rtx));

static const char *
output_91 (operands, insn)
     rtx *operands ATTRIBUTE_UNUSED;
     rtx insn ATTRIBUTE_UNUSED;
{
{
  if (STACK_TOP_P (operands[0]))
    return "fxch¥t%1";
  else
    return "fxch¥t%0";
}
}

static const char *output_92 PARAMS ((rtx *, rtx));

static const char *
output_92 (operands, insn)
     rtx *operands ATTRIBUTE_UNUSED;
     rtx insn ATTRIBUTE_UNUSED;
{
{
  switch (which_alternative)
    {
    case 0:
      /* %%% We loose REG_DEAD notes for controling pops if we split late.  */
      operands[0] = gen_rtx_MEM (DFmode, stack_pointer_rtx);
      operands[2] = stack_pointer_rtx;
      operands[3] = GEN_INT (8);
      if (find_regno_note (insn, REG_DEAD, REGNO (operands[1])))
	return "sub{l}¥t{%3, %2|%2, %3}¥n¥tfstp%z0¥t%y0";
      else
	return "sub{l}¥t{%3, %2|%2, %3}¥n¥tfst%z0¥t%y0";

    case 1:
    case 2:
    case 3:
      return "#";

    default:
      abort ();
    }
}
}

static const char *output_93 PARAMS ((rtx *, rtx));

static const char *
output_93 (operands, insn)
     rtx *operands ATTRIBUTE_UNUSED;
     rtx insn ATTRIBUTE_UNUSED;
{
{
  switch (which_alternative)
    {
    case 0:
      /* %%% We loose REG_DEAD notes for controling pops if we split late.  */
      operands[0] = gen_rtx_MEM (DFmode, stack_pointer_rtx);
      operands[2] = stack_pointer_rtx;
      operands[3] = GEN_INT (8);
      if (TARGET_64BIT)
	if (find_regno_note (insn, REG_DEAD, REGNO (operands[1])))
	  return "sub{q}¥t{%3, %2|%2, %3}¥n¥tfstp%z0¥t%y0";
	else
	  return "sub{q}¥t{%3, %2|%2, %3}¥n¥tfst%z0¥t%y0";
      else
	if (find_regno_note (insn, REG_DEAD, REGNO (operands[1])))
	  return "sub{l}¥t{%3, %2|%2, %3}¥n¥tfstp%z0¥t%y0";
	else
	  return "sub{l}¥t{%3, %2|%2, %3}¥n¥tfst%z0¥t%y0";


    case 1:
    case 2:
      return "#";

    default:
      abort ();
    }
}
}

static const char *output_94 PARAMS ((rtx *, rtx));

static const char *
output_94 (operands, insn)
     rtx *operands ATTRIBUTE_UNUSED;
     rtx insn ATTRIBUTE_UNUSED;
{
{
  switch (which_alternative)
    {
    case 0:
      if (REG_P (operands[1])
          && find_regno_note (insn, REG_DEAD, REGNO (operands[1])))
        return "fstp¥t%y0";
      else if (STACK_TOP_P (operands[0]))
        return "fld%z1¥t%y1";
      else
        return "fst¥t%y0";

    case 1:
      if (find_regno_note (insn, REG_DEAD, REGNO (operands[1])))
        return "fstp%z0¥t%y0";
      else
        return "fst%z0¥t%y0";

    case 2:
      switch (standard_80387_constant_p (operands[1]))
        {
        case 1:
	  return "fldz";
	case 2:
	  return "fld1";
	}
      abort();

    case 3:
    case 4:
      return "#";
    case 5:
      return "pxor¥t%0, %0";
    case 6:
      if (TARGET_PARTIAL_REG_DEPENDENCY)
	return "movapd¥t{%1, %0|%0, %1}";
      else
	return "movsd¥t{%1, %0|%0, %1}";
    case 7:
    case 8:
        return "movsd¥t{%1, %0|%0, %1}";

    default:
      abort();
    }
}
}

static const char *output_95 PARAMS ((rtx *, rtx));

static const char *
output_95 (operands, insn)
     rtx *operands ATTRIBUTE_UNUSED;
     rtx insn ATTRIBUTE_UNUSED;
{
{
  switch (which_alternative)
    {
    case 0:
      if (REG_P (operands[1])
          && find_regno_note (insn, REG_DEAD, REGNO (operands[1])))
        return "fstp¥t%y0";
      else if (STACK_TOP_P (operands[0]))
        return "fld%z1¥t%y1";
      else
        return "fst¥t%y0";

    case 1:
      if (find_regno_note (insn, REG_DEAD, REGNO (operands[1])))
        return "fstp%z0¥t%y0";
      else
        return "fst%z0¥t%y0";

    case 2:
      switch (standard_80387_constant_p (operands[1]))
        {
        case 1:
	  return "fldz";
	case 2:
	  return "fld1";
	}
      abort();

    case 3:
    case 4:
      return "#";

    case 5:
      return "pxor¥t%0, %0";
    case 6:
      if (TARGET_PARTIAL_REG_DEPENDENCY)
	return "movapd¥t{%1, %0|%0, %1}";
      else
	return "movsd¥t{%1, %0|%0, %1}";
    case 7:
    case 8:
      return "movsd¥t{%1, %0|%0, %1}";

    default:
      abort();
    }
}
}

static const char *output_96 PARAMS ((rtx *, rtx));

static const char *
output_96 (operands, insn)
     rtx *operands ATTRIBUTE_UNUSED;
     rtx insn ATTRIBUTE_UNUSED;
{
{
  if (STACK_TOP_P (operands[0]))
    return "fxch¥t%1";
  else
    return "fxch¥t%0";
}
}

static const char *output_97 PARAMS ((rtx *, rtx));

static const char *
output_97 (operands, insn)
     rtx *operands ATTRIBUTE_UNUSED;
     rtx insn ATTRIBUTE_UNUSED;
{
{
  switch (which_alternative)
    {
    case 0:
      /* %%% We loose REG_DEAD notes for controling pops if we split late.  */
      operands[0] = gen_rtx_MEM (XFmode, stack_pointer_rtx);
      operands[2] = stack_pointer_rtx;
      operands[3] = GEN_INT (12);
      if (find_regno_note (insn, REG_DEAD, REGNO (operands[1])))
	return "sub{l}¥t{%3, %2|%2, %3}¥n¥tfstp%z0¥t%y0";
      else
	return "sub{l}¥t{%3, %2|%2, %3}¥n¥tfst%z0¥t%y0";

    case 1:
    case 2:
      return "#";

    default:
      abort ();
    }
}
}

static const char *output_98 PARAMS ((rtx *, rtx));

static const char *
output_98 (operands, insn)
     rtx *operands ATTRIBUTE_UNUSED;
     rtx insn ATTRIBUTE_UNUSED;
{
{
  switch (which_alternative)
    {
    case 0:
      /* %%% We loose REG_DEAD notes for controling pops if we split late.  */
      operands[0] = gen_rtx_MEM (XFmode, stack_pointer_rtx);
      operands[2] = stack_pointer_rtx;
      operands[3] = GEN_INT (16);
      if (find_regno_note (insn, REG_DEAD, REGNO (operands[1])))
	return "sub{l}¥t{%3, %2|%2, %3}¥n¥tfstp%z0¥t%y0";
      else
	return "sub{l}¥t{%3, %2|%2, %3}¥n¥tfst%z0¥t%y0";

    case 1:
    case 2:
      return "#";

    default:
      abort ();
    }
}
}

static const char *output_99 PARAMS ((rtx *, rtx));

static const char *
output_99 (operands, insn)
     rtx *operands ATTRIBUTE_UNUSED;
     rtx insn ATTRIBUTE_UNUSED;
{
{
  switch (which_alternative)
    {
    case 0:
      /* %%% We loose REG_DEAD notes for controling pops if we split late.  */
      operands[0] = gen_rtx_MEM (XFmode, stack_pointer_rtx);
      operands[2] = stack_pointer_rtx;
      operands[3] = GEN_INT (12);
      if (find_regno_note (insn, REG_DEAD, REGNO (operands[1])))
	return "sub{l}¥t{%3, %2|%2, %3}¥n¥tfstp%z0¥t%y0";
      else
	return "sub{l}¥t{%3, %2|%2, %3}¥n¥tfst%z0¥t%y0";

    case 1:
      return "#";

    default:
      abort ();
    }
}
}

static const char *output_100 PARAMS ((rtx *, rtx));

static const char *
output_100 (operands, insn)
     rtx *operands ATTRIBUTE_UNUSED;
     rtx insn ATTRIBUTE_UNUSED;
{
{
  switch (which_alternative)
    {
    case 0:
      /* %%% We loose REG_DEAD notes for controling pops if we split late.  */
      operands[0] = gen_rtx_MEM (XFmode, stack_pointer_rtx);
      operands[2] = stack_pointer_rtx;
      operands[3] = GEN_INT (16);
      if (TARGET_64BIT)
	if (find_regno_note (insn, REG_DEAD, REGNO (operands[1])))
	  return "sub{q}¥t{%3, %2|%2, %3}¥n¥tfstp%z0¥t%y0";
	else
	  return "sub{q}¥t{%3, %2|%2, %3}¥n¥tfst%z0¥t%y0";
      else
	if (find_regno_note (insn, REG_DEAD, REGNO (operands[1])))
	  return "sub{l}¥t{%3, %2|%2, %3}¥n¥tfstp%z0¥t%y0";
	else
	  return "sub{l}¥t{%3, %2|%2, %3}¥n¥tfst%z0¥t%y0";

    case 1:
      return "#";

    default:
      abort ();
    }
}
}

static const char *output_101 PARAMS ((rtx *, rtx));

static const char *
output_101 (operands, insn)
     rtx *operands ATTRIBUTE_UNUSED;
     rtx insn ATTRIBUTE_UNUSED;
{
{
  switch (which_alternative)
    {
    case 0:
      if (REG_P (operands[1])
          && find_regno_note (insn, REG_DEAD, REGNO (operands[1])))
        return "fstp¥t%y0";
      else if (STACK_TOP_P (operands[0]))
        return "fld%z1¥t%y1";
      else
        return "fst¥t%y0";

    case 1:
      /* There is no non-popping store to memory for XFmode.  So if
	 we need one, follow the store with a load.  */
      if (! find_regno_note (insn, REG_DEAD, REGNO (operands[1])))
        return "fstp%z0¥t%y0¥n¥tfld%z0¥t%y0";
      else
        return "fstp%z0¥t%y0";

    case 2:
      switch (standard_80387_constant_p (operands[1]))
        {
        case 1:
	  return "fldz";
	case 2:
	  return "fld1";
	}
      break;

    case 3: case 4:
      return "#";
    }
  abort();
}
}

static const char *output_102 PARAMS ((rtx *, rtx));

static const char *
output_102 (operands, insn)
     rtx *operands ATTRIBUTE_UNUSED;
     rtx insn ATTRIBUTE_UNUSED;
{
{
  switch (which_alternative)
    {
    case 0:
      if (REG_P (operands[1])
          && find_regno_note (insn, REG_DEAD, REGNO (operands[1])))
        return "fstp¥t%y0";
      else if (STACK_TOP_P (operands[0]))
        return "fld%z1¥t%y1";
      else
        return "fst¥t%y0";

    case 1:
      /* There is no non-popping store to memory for XFmode.  So if
	 we need one, follow the store with a load.  */
      if (! find_regno_note (insn, REG_DEAD, REGNO (operands[1])))
        return "fstp%z0¥t%y0¥n¥tfld%z0¥t%y0";
      else
        return "fstp%z0¥t%y0";

    case 2:
      switch (standard_80387_constant_p (operands[1]))
        {
        case 1:
	  return "fldz";
	case 2:
	  return "fld1";
	}
      break;

    case 3: case 4:
      return "#";
    }
  abort();
}
}

static const char *output_103 PARAMS ((rtx *, rtx));

static const char *
output_103 (operands, insn)
     rtx *operands ATTRIBUTE_UNUSED;
     rtx insn ATTRIBUTE_UNUSED;
{
{
  switch (which_alternative)
    {
    case 0:
      if (REG_P (operands[1])
          && find_regno_note (insn, REG_DEAD, REGNO (operands[1])))
        return "fstp¥t%y0";
      else if (STACK_TOP_P (operands[0]))
        return "fld%z1¥t%y1";
      else
        return "fst¥t%y0";

    case 1:
      /* There is no non-popping store to memory for XFmode.  So if
	 we need one, follow the store with a load.  */
      if (! find_regno_note (insn, REG_DEAD, REGNO (operands[1])))
        return "fstp%z0¥t%y0¥n¥tfld%z0¥t%y0";
      else
        return "fstp%z0¥t%y0";

    case 2:
      switch (standard_80387_constant_p (operands[1]))
        {
        case 1:
	  return "fldz";
	case 2:
	  return "fld1";
	}
      break;

    case 3: case 4:
      return "#";
    }
  abort();
}
}

static const char *output_104 PARAMS ((rtx *, rtx));

static const char *
output_104 (operands, insn)
     rtx *operands ATTRIBUTE_UNUSED;
     rtx insn ATTRIBUTE_UNUSED;
{
{
  switch (which_alternative)
    {
    case 0:
      if (REG_P (operands[1])
          && find_regno_note (insn, REG_DEAD, REGNO (operands[1])))
        return "fstp¥t%y0";
      else if (STACK_TOP_P (operands[0]))
        return "fld%z1¥t%y1";
      else
        return "fst¥t%y0";

    case 1:
      /* There is no non-popping store to memory for XFmode.  So if
	 we need one, follow the store with a load.  */
      if (! find_regno_note (insn, REG_DEAD, REGNO (operands[1])))
        return "fstp%z0¥t%y0¥n¥tfld%z0¥t%y0";
      else
        return "fstp%z0¥t%y0";

    case 2:
      switch (standard_80387_constant_p (operands[1]))
        {
        case 1:
	  return "fldz";
	case 2:
	  return "fld1";
	}
      break;

    case 3: case 4:
      return "#";
    }
  abort();
}
}

static const char *output_105 PARAMS ((rtx *, rtx));

static const char *
output_105 (operands, insn)
     rtx *operands ATTRIBUTE_UNUSED;
     rtx insn ATTRIBUTE_UNUSED;
{
{
  if (STACK_TOP_P (operands[0]))
    return "fxch¥t%1";
  else
    return "fxch¥t%0";
}
}

static const char *output_106 PARAMS ((rtx *, rtx));

static const char *
output_106 (operands, insn)
     rtx *operands ATTRIBUTE_UNUSED;
     rtx insn ATTRIBUTE_UNUSED;
{
{
  if (STACK_TOP_P (operands[0]))
    return "fxch¥t%1";
  else
    return "fxch¥t%0";
}
}

static const char * const output_116[] = {
  "mov¥t{%k1, %k0|%k0, %k1}",
  "#",
};

static const char * const output_117[] = {
  "movz{wl|x}¥t{%1, %k0|%k0, %1} ",
  "movz{wq|x}¥t{%1, %0|%0, %1}",
};

static const char * const output_118[] = {
  "movz{bl|x}¥t{%1, %k0|%k0, %1} ",
  "movz{bq|x}¥t{%1, %0|%0, %1}",
};

static const char * const output_120[] = {
  "{cltq|cdqe}",
  "movs{lq|x}¥t{%1,%0|%0, %1}",
};

static const char *output_123 PARAMS ((rtx *, rtx));

static const char *
output_123 (operands, insn)
     rtx *operands ATTRIBUTE_UNUSED;
     rtx insn ATTRIBUTE_UNUSED;
{
{
  switch (get_attr_prefix_0f (insn))
    {
    case 0:
      return "{cwtl|cwde}";
    default:
      return "movs{wl|x}¥t{%1,%0|%0, %1}";
    }
}
}

static const char *output_124 PARAMS ((rtx *, rtx));

static const char *
output_124 (operands, insn)
     rtx *operands ATTRIBUTE_UNUSED;
     rtx insn ATTRIBUTE_UNUSED;
{
{
  switch (get_attr_prefix_0f (insn))
    {
    case 0:
      return "{cwtl|cwde}";
    default:
      return "movs{wl|x}¥t{%1,%k0|%k0, %1}";
    }
}
}

static const char *output_125 PARAMS ((rtx *, rtx));

static const char *
output_125 (operands, insn)
     rtx *operands ATTRIBUTE_UNUSED;
     rtx insn ATTRIBUTE_UNUSED;
{
{
  switch (get_attr_prefix_0f (insn))
    {
    case 0:
      return "{cbtw|cbw}";
    default:
      return "movs{bw|x}¥t{%1,%0|%0, %1}";
    }
}
}

static const char *output_133 PARAMS ((rtx *, rtx));

static const char *
output_133 (operands, insn)
     rtx *operands ATTRIBUTE_UNUSED;
     rtx insn ATTRIBUTE_UNUSED;
{
{
  switch (which_alternative)
    {
    case 0:
      if (REG_P (operands[1])
          && find_regno_note (insn, REG_DEAD, REGNO (operands[1])))
        return "fstp¥t%y0";
      else if (STACK_TOP_P (operands[0]))
        return "fld%z1¥t%y1";
      else
        return "fst¥t%y0";

    case 1:
      if (find_regno_note (insn, REG_DEAD, REGNO (operands[1])))
        return "fstp%z0¥t%y0";

      else
        return "fst%z0¥t%y0";
    case 2:
      return "cvtss2sd¥t{%1, %0|%0, %1}";

    default:
      abort ();
    }
}
}

static const char *output_135 PARAMS ((rtx *, rtx));

static const char *
output_135 (operands, insn)
     rtx *operands ATTRIBUTE_UNUSED;
     rtx insn ATTRIBUTE_UNUSED;
{
{
  switch (which_alternative)
    {
    case 0:
      if (REG_P (operands[1])
          && find_regno_note (insn, REG_DEAD, REGNO (operands[1])))
        return "fstp¥t%y0";
      else if (STACK_TOP_P (operands[0]))
        return "fld%z1¥t%y1";
      else
        return "fst¥t%y0";

    case 1:
      /* There is no non-popping store to memory for XFmode.  So if
	 we need one, follow the store with a load.  */
      if (! find_regno_note (insn, REG_DEAD, REGNO (operands[1])))
        return "fstp%z0¥t%y0¥n¥tfld%z0¥t%y0";
      else
        return "fstp%z0¥t%y0";

    default:
      abort ();
    }
}
}

static const char *output_136 PARAMS ((rtx *, rtx));

static const char *
output_136 (operands, insn)
     rtx *operands ATTRIBUTE_UNUSED;
     rtx insn ATTRIBUTE_UNUSED;
{
{
  switch (which_alternative)
    {
    case 0:
      if (REG_P (operands[1])
          && find_regno_note (insn, REG_DEAD, REGNO (operands[1])))
        return "fstp¥t%y0";
      else if (STACK_TOP_P (operands[0]))
        return "fld%z1¥t%y1";
      else
        return "fst¥t%y0";

    case 1:
      /* There is no non-popping store to memory for XFmode.  So if
	 we need one, follow the store with a load.  */
      if (! find_regno_note (insn, REG_DEAD, REGNO (operands[1])))
        return "fstp%z0¥t%y0¥n¥tfld%z0¥t%y0";
      else
        return "fstp%z0¥t%y0";

    default:
      abort ();
    }
}
}

static const char *output_137 PARAMS ((rtx *, rtx));

static const char *
output_137 (operands, insn)
     rtx *operands ATTRIBUTE_UNUSED;
     rtx insn ATTRIBUTE_UNUSED;
{
{
  switch (which_alternative)
    {
    case 0:
      if (REG_P (operands[1])
          && find_regno_note (insn, REG_DEAD, REGNO (operands[1])))
        return "fstp¥t%y0";
      else if (STACK_TOP_P (operands[0]))
        return "fld%z1¥t%y1";
      else
        return "fst¥t%y0";

    case 1:
      /* There is no non-popping store to memory for XFmode.  So if
	 we need one, follow the store with a load.  */
      if (! find_regno_note (insn, REG_DEAD, REGNO (operands[1])))
        return "fstp%z0¥t%y0¥n¥tfld%z0¥t%y0";
      else
        return "fstp%z0¥t%y0";

    default:
      abort ();
    }
}
}

static const char *output_138 PARAMS ((rtx *, rtx));

static const char *
output_138 (operands, insn)
     rtx *operands ATTRIBUTE_UNUSED;
     rtx insn ATTRIBUTE_UNUSED;
{
{
  switch (which_alternative)
    {
    case 0:
      if (REG_P (operands[1])
          && find_regno_note (insn, REG_DEAD, REGNO (operands[1])))
        return "fstp¥t%y0";
      else if (STACK_TOP_P (operands[0]))
        return "fld%z1¥t%y1";
      else
        return "fst¥t%y0";

    case 1:
      /* There is no non-popping store to memory for XFmode.  So if
	 we need one, follow the store with a load.  */
      if (! find_regno_note (insn, REG_DEAD, REGNO (operands[1])))
        return "fstp%z0¥t%y0¥n¥tfld%z0¥t%y0";
      else
        return "fstp%z0¥t%y0";

    default:
      abort ();
    }
}
}

static const char *output_139 PARAMS ((rtx *, rtx));

static const char *
output_139 (operands, insn)
     rtx *operands ATTRIBUTE_UNUSED;
     rtx insn ATTRIBUTE_UNUSED;
{
{
  switch (which_alternative)
    {
    case 0:
      if (find_regno_note (insn, REG_DEAD, REGNO (operands[1])))
	return "fstp%z0¥t%y0";
      else
	return "fst%z0¥t%y0";
    default:
      abort ();
    }
}
}

static const char *output_140 PARAMS ((rtx *, rtx));

static const char *
output_140 (operands, insn)
     rtx *operands ATTRIBUTE_UNUSED;
     rtx insn ATTRIBUTE_UNUSED;
{
{
  switch (which_alternative)
    {
    case 0:
      if (find_regno_note (insn, REG_DEAD, REGNO (operands[1])))
	return "fstp%z0¥t%y0";
      else
	return "fst%z0¥t%y0";
    case 4:
      return "cvtsd2ss¥t{%1, %0|%0, %1}";
    default:
      abort ();
    }
}
}

static const char *output_141 PARAMS ((rtx *, rtx));

static const char *
output_141 (operands, insn)
     rtx *operands ATTRIBUTE_UNUSED;
     rtx insn ATTRIBUTE_UNUSED;
{
{
  switch (which_alternative)
    {
    case 0:
