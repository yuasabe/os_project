/* Generated automatically by the program `genemit'
from the machine description file `md'.  */

#include "config.h"
#include "system.h"
#include "rtl.h"
#include "tm_p.h"
#include "function.h"
#include "expr.h"
#include "optabs.h"
#include "real.h"
#include "flags.h"
#include "output.h"
#include "insn-config.h"
#include "hard-reg-set.h"
#include "recog.h"
#include "resource.h"
#include "reload.h"
#include "toplev.h"
#include "ggc.h"

#define FAIL return (end_sequence (), _val)
#define DONE return (_val = gen_sequence (), end_sequence (), _val)
rtx
gen_cmpdi_ccno_1_rex64 (operand0, operand1)
     rtx operand0;
     rtx operand1;
{
  return gen_rtx_SET (VOIDmode,
	gen_rtx_REG (VOIDmode,
	17),
	gen_rtx_COMPARE (VOIDmode,
	operand0,
	operand1));
}

rtx
gen_cmpdi_1_insn_rex64 (operand0, operand1)
     rtx operand0;
     rtx operand1;
{
  return gen_rtx_SET (VOIDmode,
	gen_rtx_REG (VOIDmode,
	17),
	gen_rtx_COMPARE (VOIDmode,
	operand0,
	operand1));
}

rtx
gen_cmpqi_ext_3_insn (operand0, operand1)
     rtx operand0;
     rtx operand1;
{
  return gen_rtx_SET (VOIDmode,
	gen_rtx_REG (VOIDmode,
	17),
	gen_rtx_COMPARE (VOIDmode,
	gen_rtx_SUBREG (QImode,
	gen_rtx_ZERO_EXTRACT (SImode,
	operand0,
	GEN_INT (8),
	GEN_INT (8)),
	0),
	operand1));
}

rtx
gen_cmpqi_ext_3_insn_rex64 (operand0, operand1)
     rtx operand0;
     rtx operand1;
{
  return gen_rtx_SET (VOIDmode,
	gen_rtx_REG (VOIDmode,
	17),
	gen_rtx_COMPARE (VOIDmode,
	gen_rtx_SUBREG (QImode,
	gen_rtx_ZERO_EXTRACT (SImode,
	operand0,
	GEN_INT (8),
	GEN_INT (8)),
	0),
	operand1));
}

rtx
gen_x86_fnstsw_1 (operand0)
     rtx operand0;
{
  return gen_rtx_SET (VOIDmode,
	operand0,
	gen_rtx_UNSPEC (HImode,
	gen_rtvec (1,
		gen_rtx_REG (VOIDmode,
	18)),
	9));
}

rtx
gen_x86_sahf_1 (operand0)
     rtx operand0;
{
  return gen_rtx_SET (VOIDmode,
	gen_rtx_REG (CCmode,
	17),
	gen_rtx_UNSPEC (CCmode,
	gen_rtvec (1,
		operand0),
	10));
}

rtx
gen_popsi1 (operand0)
     rtx operand0;
{
  return gen_rtx_PARALLEL (VOIDmode, gen_rtvec (2,
		gen_rtx_SET (VOIDmode,
	operand0,
	gen_rtx_MEM (SImode,
	gen_rtx_REG (SImode,
	7))),
		gen_rtx_SET (VOIDmode,
	gen_rtx_REG (SImode,
	7),
	gen_rtx_PLUS (SImode,
	gen_rtx_REG (SImode,
	7),
	GEN_INT (4)))));
}

rtx
gen_movsi_insv_1 (operand0, operand1)
     rtx operand0;
     rtx operand1;
{
  return gen_rtx_SET (VOIDmode,
	gen_rtx_ZERO_EXTRACT (SImode,
	operand0,
	GEN_INT (8),
	GEN_INT (8)),
	operand1);
}

rtx
gen_pushdi2_rex64 (operand0, operand1)
     rtx operand0;
     rtx operand1;
{
  return gen_rtx_SET (VOIDmode,
	operand0,
	operand1);
}

rtx
gen_popdi1 (operand0)
     rtx operand0;
{
  return gen_rtx_PARALLEL (VOIDmode, gen_rtvec (2,
		gen_rtx_SET (VOIDmode,
	operand0,
	gen_rtx_MEM (DImode,
	gen_rtx_REG (DImode,
	7))),
		gen_rtx_SET (VOIDmode,
	gen_rtx_REG (DImode,
	7),
	gen_rtx_PLUS (DImode,
	gen_rtx_REG (DImode,
	7),
	GEN_INT (8)))));
}

rtx
gen_swapxf (operand0, operand1)
     rtx operand0;
     rtx operand1;
{
  return gen_rtx_PARALLEL (VOIDmode, gen_rtvec (2,
		gen_rtx_SET (VOIDmode,
	operand0,
	operand1),
		gen_rtx_SET (VOIDmode,
	operand1,
	operand0)));
}

rtx
gen_swaptf (operand0, operand1)
     rtx operand0;
     rtx operand1;
{
  return gen_rtx_PARALLEL (VOIDmode, gen_rtvec (2,
		gen_rtx_SET (VOIDmode,
	operand0,
	operand1),
		gen_rtx_SET (VOIDmode,
	operand1,
	operand0)));
}

rtx
gen_zero_extendhisi2_and (operand0, operand1)
     rtx operand0;
     rtx operand1;
{
  return gen_rtx_PARALLEL (VOIDmode, gen_rtvec (2,
		gen_rtx_SET (VOIDmode,
	operand0,
	gen_rtx_ZERO_EXTEND (SImode,
	operand1)),
		gen_rtx_CLOBBER (VOIDmode,
	gen_rtx_REG (CCmode,
	17))));
}

rtx
gen_zero_extendsidi2_32 (operand0, operand1)
     rtx operand0;
     rtx operand1;
{
  return gen_rtx_PARALLEL (VOIDmode, gen_rtvec (2,
		gen_rtx_SET (VOIDmode,
	operand0,
	gen_rtx_ZERO_EXTEND (DImode,
	operand1)),
		gen_rtx_CLOBBER (VOIDmode,
	gen_rtx_REG (CCmode,
	17))));
}

rtx
gen_zero_extendsidi2_rex64 (operand0, operand1)
     rtx operand0;
     rtx operand1;
{
  return gen_rtx_SET (VOIDmode,
	operand0,
	gen_rtx_ZERO_EXTEND (DImode,
	operand1));
}

rtx
gen_zero_extendhidi2 (operand0, operand1)
     rtx operand0;
     rtx operand1;
{
  return gen_rtx_SET (VOIDmode,
	operand0,
	gen_rtx_ZERO_EXTEND (DImode,
	operand1));
}

rtx
gen_zero_extendqidi2 (operand0, operand1)
     rtx operand0;
     rtx operand1;
{
  return gen_rtx_SET (VOIDmode,
	operand0,
	gen_rtx_ZERO_EXTEND (DImode,
	operand1));
}

rtx
gen_extendsidi2_rex64 (operand0, operand1)
     rtx operand0;
     rtx operand1;
{
  return gen_rtx_SET (VOIDmode,
	operand0,
	gen_rtx_SIGN_EXTEND (DImode,
	operand1));
}

rtx
gen_extendhidi2 (operand0, operand1)
     rtx operand0;
     rtx operand1;
{
  return gen_rtx_SET (VOIDmode,
	operand0,
	gen_rtx_SIGN_EXTEND (DImode,
	operand1));
}

rtx
gen_extendqidi2 (operand0, operand1)
     rtx operand0;
     rtx operand1;
{
  return gen_rtx_SET (VOIDmode,
	operand0,
	gen_rtx_SIGN_EXTEND (DImode,
	operand1));
}

rtx
gen_extendhisi2 (operand0, operand1)
     rtx operand0;
     rtx operand1;
{
  return gen_rtx_SET (VOIDmode,
	operand0,
	gen_rtx_SIGN_EXTEND (SImode,
	operand1));
}

rtx
gen_extendqihi2 (operand0, operand1)
     rtx operand0;
     rtx operand1;
{
  return gen_rtx_SET (VOIDmode,
	operand0,
	gen_rtx_SIGN_EXTEND (HImode,
	operand1));
}

rtx
gen_extendqisi2 (operand0, operand1)
     rtx operand0;
     rtx operand1;
{
  return gen_rtx_SET (VOIDmode,
	operand0,
	gen_rtx_SIGN_EXTEND (SImode,
	operand1));
}

rtx
gen_truncdfsf2_3 (operand0, operand1)
     rtx operand0;
     rtx operand1;
{
  return gen_rtx_SET (VOIDmode,
	operand0,
	gen_rtx_FLOAT_TRUNCATE (SFmode,
	operand1));
}

rtx
gen_truncdfsf2_sse_only (operand0, operand1)
     rtx operand0;
     rtx operand1;
{
  return gen_rtx_SET (VOIDmode,
	operand0,
	gen_rtx_FLOAT_TRUNCATE (SFmode,
	operand1));
}

rtx
gen_fix_truncdi_nomemory (operand0, operand1, operand2, operand3, operand4)
     rtx operand0;
     rtx operand1;
     rtx operand2;
     rtx operand3;
     rtx operand4;
{
  return gen_rtx_PARALLEL (VOIDmode, gen_rtvec (5,
		gen_rtx_SET (VOIDmode,
	operand0,
	gen_rtx_FIX (DImode,
	operand1)),
		gen_rtx_USE (VOIDmode,
	operand2),
		gen_rtx_USE (VOIDmode,
	operand3),
		gen_rtx_CLOBBER (VOIDmode,
	operand4),
		gen_rtx_CLOBBER (VOIDmode,
	gen_rtx_SCRATCH (DFmode))));
}

rtx
gen_fix_truncdi_memory (operand0, operand1, operand2, operand3)
     rtx operand0;
     rtx operand1;
     rtx operand2;
     rtx operand3;
{
  return gen_rtx_PARALLEL (VOIDmode, gen_rtvec (4,
		gen_rtx_SET (VOIDmode,
	operand0,
	gen_rtx_FIX (DImode,
	operand1)),
		gen_rtx_USE (VOIDmode,
	operand2),
		gen_rtx_USE (VOIDmode,
	operand3),
		gen_rtx_CLOBBER (VOIDmode,
	gen_rtx_SCRATCH (DFmode))));
}

rtx
gen_fix_truncsfdi_sse (operand0, operand1)
     rtx operand0;
     rtx operand1;
{
  return gen_rtx_SET (VOIDmode,
	operand0,
	gen_rtx_FIX (DImode,
	operand1));
}

rtx
gen_fix_truncdfdi_sse (operand0, operand1)
     rtx operand0;
     rtx operand1;
{
  return gen_rtx_SET (VOIDmode,
	operand0,
	gen_rtx_FIX (DImode,
	operand1));
}

rtx
gen_fix_truncsi_nomemory (operand0, operand1, operand2, operand3, operand4)
     rtx operand0;
     rtx operand1;
     rtx operand2;
     rtx operand3;
     rtx operand4;
{
  return gen_rtx_PARALLEL (VOIDmode, gen_rtvec (4,
		gen_rtx_SET (VOIDmode,
	operand0,
	gen_rtx_FIX (SImode,
	operand1)),
		gen_rtx_USE (VOIDmode,
	operand2),
		gen_rtx_USE (VOIDmode,
	operand3),
		gen_rtx_CLOBBER (VOIDmode,
	operand4)));
}

rtx
gen_fix_truncsi_memory (operand0, operand1, operand2, operand3)
     rtx operand0;
     rtx operand1;
     rtx operand2;
     rtx operand3;
{
  return gen_rtx_PARALLEL (VOIDmode, gen_rtvec (3,
		gen_rtx_SET (VOIDmode,
	operand0,
	gen_rtx_FIX (SImode,
	operand1)),
		gen_rtx_USE (VOIDmode,
	operand2),
		gen_rtx_USE (VOIDmode,
	operand3)));
}

rtx
gen_fix_truncsfsi_sse (operand0, operand1)
     rtx operand0;
     rtx operand1;
{
  return gen_rtx_SET (VOIDmode,
	operand0,
	gen_rtx_FIX (SImode,
	operand1));
}

rtx
gen_fix_truncdfsi_sse (operand0, operand1)
     rtx operand0;
     rtx operand1;
{
  return gen_rtx_SET (VOIDmode,
	operand0,
	gen_rtx_FIX (SImode,
	operand1));
}

rtx
gen_fix_trunchi_nomemory (operand0, operand1, operand2, operand3, operand4)
     rtx operand0;
     rtx operand1;
     rtx operand2;
     rtx operand3;
     rtx operand4;
{
  return gen_rtx_PARALLEL (VOIDmode, gen_rtvec (4,
		gen_rtx_SET (VOIDmode,
	operand0,
	gen_rtx_FIX (HImode,
	operand1)),
		gen_rtx_USE (VOIDmode,
	operand2),
		gen_rtx_USE (VOIDmode,
	operand3),
		gen_rtx_CLOBBER (VOIDmode,
	operand4)));
}

rtx
gen_fix_trunchi_memory (operand0, operand1, operand2, operand3)
     rtx operand0;
     rtx operand1;
     rtx operand2;
     rtx operand3;
{
  return gen_rtx_PARALLEL (VOIDmode, gen_rtvec (3,
		gen_rtx_SET (VOIDmode,
	operand0,
	gen_rtx_FIX (HImode,
	operand1)),
		gen_rtx_USE (VOIDmode,
	operand2),
		gen_rtx_USE (VOIDmode,
	operand3)));
}

rtx
gen_x86_fnstcw_1 (operand0)
     rtx operand0;
{
  return gen_rtx_SET (VOIDmode,
	operand0,
	gen_rtx_UNSPEC (HImode,
	gen_rtvec (1,
		gen_rtx_REG (HImode,
	18)),
	11));
}

rtx
gen_x86_fldcw_1 (operand0)
     rtx operand0;
{
  return gen_rtx_SET (VOIDmode,
	gen_rtx_REG (HImode,
	18),
	gen_rtx_UNSPEC (HImode,
	gen_rtvec (1,
		operand0),
	12));
}

rtx
gen_floathisf2 (operand0, operand1)
     rtx operand0;
     rtx operand1;
{
  return gen_rtx_SET (VOIDmode,
	operand0,
	gen_rtx_FLOAT (SFmode,
	operand1));
}

rtx
gen_floathidf2 (operand0, operand1)
     rtx operand0;
     rtx operand1;
{
  return gen_rtx_SET (VOIDmode,
	operand0,
	gen_rtx_FLOAT (DFmode,
	operand1));
}

rtx
gen_floathixf2 (operand0, operand1)
     rtx operand0;
     rtx operand1;
{
  return gen_rtx_SET (VOIDmode,
	operand0,
	gen_rtx_FLOAT (XFmode,
	operand1));
}

rtx
gen_floathitf2 (operand0, operand1)
     rtx operand0;
     rtx operand1;
{
  return gen_rtx_SET (VOIDmode,
	operand0,
	gen_rtx_FLOAT (TFmode,
	operand1));
}

rtx
gen_floatsixf2 (operand0, operand1)
     rtx operand0;
     rtx operand1;
{
  return gen_rtx_SET (VOIDmode,
	operand0,
	gen_rtx_FLOAT (XFmode,
	operand1));
}

rtx
gen_floatsitf2 (operand0, operand1)
     rtx operand0;
     rtx operand1;
{
  return gen_rtx_SET (VOIDmode,
	operand0,
	gen_rtx_FLOAT (TFmode,
	operand1));
}

rtx
gen_floatdixf2 (operand0, operand1)
     rtx operand0;
     rtx operand1;
{
  return gen_rtx_SET (VOIDmode,
	operand0,
	gen_rtx_FLOAT (XFmode,
	operand1));
}

rtx
gen_floatditf2 (operand0, operand1)
     rtx operand0;
     rtx operand1;
{
  return gen_rtx_SET (VOIDmode,
	operand0,
	gen_rtx_FLOAT (TFmode,
	operand1));
}

rtx
gen_addqi3_cc (operand0, operand1, operand2)
     rtx operand0;
     rtx operand1;
     rtx operand2;
{
  return gen_rtx_PARALLEL (VOIDmode, gen_rtvec (2,
		gen_rtx_SET (VOIDmode,
	gen_rtx_REG (CCmode,
	17),
	gen_rtx_UNSPEC (CCmode,
	gen_rtvec (2,
		operand1,
		operand2),
	12)),
		gen_rtx_SET (VOIDmode,
	operand0,
	gen_rtx_PLUS (QImode,
	operand1,
	operand2))));
}

rtx
gen_addsi_1_zext (operand0, operand1, operand2)
     rtx operand0;
     rtx operand1;
     rtx operand2;
{
  return gen_rtx_PARALLEL (VOIDmode, gen_rtvec (2,
		gen_rtx_SET (VOIDmode,
	operand0,
	gen_rtx_ZERO_EXTEND (DImode,
	gen_rtx_PLUS (SImode,
	operand1,
	operand2))),
		gen_rtx_CLOBBER (VOIDmode,
	gen_rtx_REG (CCmode,
	17))));
}

rtx
gen_addqi_ext_1 (operand0, operand1, operand2)
     rtx operand0;
     rtx operand1;
     rtx operand2;
{
  return gen_rtx_PARALLEL (VOIDmode, gen_rtvec (2,
		gen_rtx_SET (VOIDmode,
	gen_rtx_ZERO_EXTRACT (SImode,
	operand0,
	GEN_INT (8),
	GEN_INT (8)),
	gen_rtx_PLUS (SImode,
	gen_rtx_ZERO_EXTRACT (SImode,
	operand1,
	GEN_INT (8),
	GEN_INT (8)),
	operand2)),
		gen_rtx_CLOBBER (VOIDmode,
	gen_rtx_REG (CCmode,
	17))));
}

rtx
gen_subdi3_carry_rex64 (operand0, operand1, operand2)
     rtx operand0;
     rtx operand1;
     rtx operand2;
{
  return gen_rtx_PARALLEL (VOIDmode, gen_rtvec (2,
		gen_rtx_SET (VOIDmode,
	operand0,
	gen_rtx_MINUS (DImode,
	operand1,
	gen_rtx_PLUS (DImode,
	gen_rtx_LTU (DImode,
	gen_rtx_REG (CCmode,
	17),
	const0_rtx),
	operand2))),
		gen_rtx_CLOBBER (VOIDmode,
	gen_rtx_REG (CCmode,
	17))));
}

rtx
gen_subsi3_carry (operand0, operand1, operand2)
     rtx operand0;
     rtx operand1;
     rtx operand2;
{
  return gen_rtx_PARALLEL (VOIDmode, gen_rtvec (2,
		gen_rtx_SET (VOIDmode,
	operand0,
	gen_rtx_MINUS (SImode,
	operand1,
	gen_rtx_PLUS (SImode,
	gen_rtx_LTU (SImode,
	gen_rtx_REG (CCmode,
	17),
	const0_rtx),
	operand2))),
		gen_rtx_CLOBBER (VOIDmode,
	gen_rtx_REG (CCmode,
	17))));
}

rtx
gen_subsi3_carry_zext (operand0, operand1, operand2)
     rtx operand0;
     rtx operand1;
     rtx operand2;
{
  return gen_rtx_PARALLEL (VOIDmode, gen_rtvec (2,
		gen_rtx_SET (VOIDmode,
	operand0,
	gen_rtx_ZERO_EXTEND (DImode,
	gen_rtx_MINUS (SImode,
	operand1,
	gen_rtx_PLUS (SImode,
	gen_rtx_LTU (SImode,
	gen_rtx_REG (CCmode,
	17),
	const0_rtx),
	operand2)))),
		gen_rtx_CLOBBER (VOIDmode,
	gen_rtx_REG (CCmode,
	17))));
}

rtx
gen_divqi3 (operand0, operand1, operand2)
     rtx operand0;
     rtx operand1;
     rtx operand2;
{
  return gen_rtx_PARALLEL (VOIDmode, gen_rtvec (2,
		gen_rtx_SET (VOIDmode,
	operand0,
	gen_rtx_DIV (QImode,
	operand1,
	operand2)),
		gen_rtx_CLOBBER (VOIDmode,
	gen_rtx_REG (CCmode,
	17))));
}

rtx
gen_udivqi3 (operand0, operand1, operand2)
     rtx operand0;
     rtx operand1;
     rtx operand2;
{
  return gen_rtx_PARALLEL (VOIDmode, gen_rtvec (2,
		gen_rtx_SET (VOIDmode,
	operand0,
	gen_rtx_UDIV (QImode,
	operand1,
	operand2)),
		gen_rtx_CLOBBER (VOIDmode,
	gen_rtx_REG (CCmode,
	17))));
}

rtx
gen_divmodhi4 (operand0, operand1, operand2, operand3)
     rtx operand0;
     rtx operand1;
     rtx operand2;
     rtx operand3;
{
  return gen_rtx_PARALLEL (VOIDmode, gen_rtvec (3,
		gen_rtx_SET (VOIDmode,
	operand0,
	gen_rtx_DIV (HImode,
	operand1,
	operand2)),
		gen_rtx_SET (VOIDmode,
	operand3,
	gen_rtx_MOD (HImode,
	operand1,
	operand2)),
		gen_rtx_CLOBBER (VOIDmode,
	gen_rtx_REG (CCmode,
	17))));
}

rtx
gen_udivmoddi4 (operand0, operand1, operand2, operand3)
     rtx operand0;
     rtx operand1;
     rtx operand2;
     rtx operand3;
{
  return gen_rtx_PARALLEL (VOIDmode, gen_rtvec (3,
		gen_rtx_SET (VOIDmode,
	operand0,
	gen_rtx_UDIV (DImode,
	operand1,
	operand2)),
		gen_rtx_SET (VOIDmode,
	operand3,
	gen_rtx_UMOD (DImode,
	operand1,
	operand2)),
		gen_rtx_CLOBBER (VOIDmode,
	gen_rtx_REG (CCmode,
	17))));
}

rtx
gen_udivmodsi4 (operand0, operand1, operand2, operand3)
     rtx operand0;
     rtx operand1;
     rtx operand2;
     rtx operand3;
{
  return gen_rtx_PARALLEL (VOIDmode, gen_rtvec (3,
		gen_rtx_SET (VOIDmode,
	operand0,
	gen_rtx_UDIV (SImode,
	operand1,
	operand2)),
		gen_rtx_SET (VOIDmode,
	operand3,
	gen_rtx_UMOD (SImode,
	operand1,
	operand2)),
		gen_rtx_CLOBBER (VOIDmode,
	gen_rtx_REG (CCmode,
	17))));
}

rtx
gen_testsi_1 (operand0, operand1)
     rtx operand0;
     rtx operand1;
{
  return gen_rtx_SET (VOIDmode,
	gen_rtx_REG (VOIDmode,
	17),
	gen_rtx_COMPARE (VOIDmode,
	gen_rtx_AND (SImode,
	operand0,
	operand1),
	const0_rtx));
}

rtx
gen_andqi_ext_0 (operand0, operand1, operand2)
     rtx operand0;
     rtx operand1;
     rtx operand2;
{
  return gen_rtx_PARALLEL (VOIDmode, gen_rtvec (2,
		gen_rtx_SET (VOIDmode,
	gen_rtx_ZERO_EXTRACT (SImode,
	operand0,
	GEN_INT (8),
	GEN_INT (8)),
	gen_rtx_AND (SImode,
	gen_rtx_ZERO_EXTRACT (SImode,
	operand1,
	GEN_INT (8),
	GEN_INT (8)),
	operand2)),
		gen_rtx_CLOBBER (VOIDmode,
	gen_rtx_REG (CCmode,
	17))));
}

rtx
gen_negsf2_memory (operand0, operand1)
     rtx operand0;
  