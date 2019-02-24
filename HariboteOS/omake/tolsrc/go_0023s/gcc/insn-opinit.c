/* Generated automatically by the program `genopinit'
from the machine description file `md'.  */

#include "config.h"
#include "system.h"
#include "rtl.h"
#include "flags.h"
#include "insn-config.h"
#include "recog.h"
#include "expr.h"
#include "optabs.h"
#include "reload.h"

void
init_all_optabs ()
{
  if (HAVE_zero_extendhidi2)
    extendtab[(int) DImode][(int) HImode][1] = CODE_FOR_zero_extendhidi2;
  if (HAVE_zero_extendqidi2)
    extendtab[(int) DImode][(int) QImode][1] = CODE_FOR_zero_extendqidi2;
  if (HAVE_extendhidi2)
    extendtab[(int) DImode][(int) HImode][0] = CODE_FOR_extendhidi2;
  if (HAVE_extendqidi2)
    extendtab[(int) DImode][(int) QImode][0] = CODE_FOR_extendqidi2;
  extendtab[(int) SImode][(int) HImode][0] = CODE_FOR_extendhisi2;
  extendtab[(int) HImode][(int) QImode][0] = CODE_FOR_extendqihi2;
  extendtab[(int) SImode][(int) QImode][0] = CODE_FOR_extendqisi2;
  if (HAVE_floathisf2)
    floattab[(int) SFmode][(int) HImode][0] = CODE_FOR_floathisf2;
  if (HAVE_floathidf2)
    floattab[(int) DFmode][(int) HImode][0] = CODE_FOR_floathidf2;
  if (HAVE_floathixf2)
    floattab[(int) XFmode][(int) HImode][0] = CODE_FOR_floathixf2;
  if (HAVE_floathitf2)
    floattab[(int) TFmode][(int) HImode][0] = CODE_FOR_floathitf2;
  if (HAVE_floatsixf2)
    floattab[(int) XFmode][(int) SImode][0] = CODE_FOR_floatsixf2;
  if (HAVE_floatsitf2)
    floattab[(int) TFmode][(int) SImode][0] = CODE_FOR_floatsitf2;
  if (HAVE_floatdixf2)
    floattab[(int) XFmode][(int) DImode][0] = CODE_FOR_floatdixf2;
  if (HAVE_floatditf2)
    floattab[(int) TFmode][(int) DImode][0] = CODE_FOR_floatditf2;
  if (HAVE_divqi3)
    sdiv_optab->handlers[(int) QImode].insn_code = CODE_FOR_divqi3;
  if (HAVE_udivqi3)
    udiv_optab->handlers[(int) QImode].insn_code = CODE_FOR_udivqi3;
  if (HAVE_divmodhi4)
    sdivmod_optab->handlers[(int) HImode].insn_code = CODE_FOR_divmodhi4;
  if (HAVE_udivmoddi4)
    udivmod_optab->handlers[(int) DImode].insn_code = CODE_FOR_udivmoddi4;
  udivmod_optab->handlers[(int) SImode].insn_code = CODE_FOR_udivmodsi4;
  if (HAVE_sqrtxf2)
    sqrt_optab->handlers[(int) XFmode].insn_code = CODE_FOR_sqrtxf2;
  if (HAVE_sqrttf2)
    sqrt_optab->handlers[(int) TFmode].insn_code = CODE_FOR_sqrttf2;
  if (HAVE_sindf2)
    sin_optab->handlers[(int) DFmode].insn_code = CODE_FOR_sindf2;
  if (HAVE_sinsf2)
    sin_optab->handlers[(int) SFmode].insn_code = CODE_FOR_sinsf2;
  if (HAVE_sinxf2)
    sin_optab->handlers[(int) XFmode].insn_code = CODE_FOR_sinxf2;
  if (HAVE_sintf2)
    sin_optab->handlers[(int) TFmode].insn_code = CODE_FOR_sintf2;
  if (HAVE_cosdf2)
    cos_optab->handlers[(int) DFmode].insn_code = CODE_FOR_cosdf2;
  if (HAVE_cossf2)
    cos_optab->handlers[(int) SFmode].insn_code = CODE_FOR_cossf2;
  if (HAVE_cosxf2)
    cos_optab->handlers[(int) XFmode].insn_code = CODE_FOR_cosxf2;
  if (HAVE_costf2)
    cos_optab->handlers[(int) TFmode].insn_code = CODE_FOR_costf2;
  if (HAVE_addv4sf3)
    addv_optab->handlers[(int) (int) V4SFmode].insn_code =
    add_optab->handlers[(int) (int) V4SFmode].insn_code = CODE_FOR_addv4sf3;
  if (HAVE_subv4sf3)
    subv_optab->handlers[(int) (int) V4SFmode].insn_code =
    sub_optab->handlers[(int) (int) V4SFmode].insn_code = CODE_FOR_subv4sf3;
  if (HAVE_mulv4sf3)
    smulv_optab->handlers[(int) (int) V4SFmode].insn_code =
    smul_optab->handlers[(int) (int) V4SFmode].insn_code = CODE_FOR_mulv4sf3;
  if (HAVE_divv4sf3)
    sdiv_optab->handlers[(int) V4SFmode].insn_code = CODE_FOR_divv4sf3;
  if (HAVE_sqrtv4sf2)
    sqrt_optab->handlers[(int) V4SFmode].insn_code = CODE_FOR_sqrtv4sf2;
  if (HAVE_addv8qi3)
    add_optab->handlers[(int) V8QImode].insn_code = CODE_FOR_addv8qi3;
  if (HAVE_addv4hi3)
    add_optab->handlers[(int) V4HImode].insn_code = CODE_FOR_addv4hi3;
  if (HAVE_addv2si3)
    add_optab->handlers[(int) V2SImode].insn_code = CODE_FOR_addv2si3;
  if (HAVE_subv8qi3)
    sub_optab->handlers[(int) V8QImode].insn_code = CODE_FOR_subv8qi3;
  if (HAVE_subv4hi3)
    sub_optab->handlers[(i