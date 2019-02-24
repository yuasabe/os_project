/* Generated automatically by gengenrtl from rtl.def.  */

/* !kawai! */
#include "config.h"
#include "system.h"
#include "../include/obstack.h"
#include "rtl.h"
#include "ggc.h"
/* end of !kawai! */

extern struct obstack *rtl_obstack;

#define obstack_alloc_rtx(n)					¥
    ((rtx) obstack_alloc (rtl_obstack,				¥
			  sizeof (struct rtx_def)		¥
			  + ((n) - 1) * sizeof (rtunion)))

rtx
gen_rtx_fmt_s (code, mode, arg0)
     RTX_CODE code;
     enum machine_mode mode;
     const char *arg0;
{
  rtx rt;
  rt = ggc_alloc_rtx (1);
  memset (rt, 0, sizeof (struct rtx_def) - sizeof (rtunion));

  PUT_CODE (rt, code);
  PUT_MODE (rt, mode);
  XSTR (rt, 0) = arg0;

  return rt;
}

rtx
gen_rtx_fmt_ee (code, mode, arg0, arg1)
     RTX_CODE code;
     enum machine_mode mode;
     rtx arg0;
     rtx arg1;
{
  rtx rt;
  rt = ggc_alloc_rtx (2);
  memset (rt, 0, sizeof (struct rtx_def) - sizeof (rtunion));

  PUT_CODE (rt, code);
  PUT_MODE (rt, mode);
  XEXP (rt, 0) = arg0;
  XEXP (rt, 1) = arg1;

  return rt;
}

rtx
gen_rtx_fmt_ue (code, mode, arg0, arg1)
     RTX_CODE code;
     enum machine_mode mode;
     rtx arg0;
     rtx arg1;
{
  rtx rt;
  rt = ggc_alloc_rtx (2);
  memset (rt, 0, sizeof (struct rtx_def) - sizeof (rtunion));

  PUT_CODE (rt, code);
  PUT_MODE (rt, mode);
  XEXP (rt, 0) = arg0;
  XEXP (rt, 1) = arg1;

  return rt;
}

rtx
gen_rtx_fmt_iss (code, mode, arg0, arg1, arg2)
     RTX_CODE code;
     enum machine_mode mode;
     int arg0;
     const char *arg1;
     const char *arg2;
{
  rtx rt;
  rt = ggc_alloc_rtx (3);
  memset (rt, 0, sizeof (struct rtx_def) - sizeof (rtunion));

  PUT_CODE (rt, code);
  PUT_MODE (rt, mode);
  XINT (rt, 0) = arg0;
  XSTR (rt, 1) = arg1;
  XSTR (rt, 2) = arg2;

  return rt;
}

rtx
gen_rtx_fmt_is (code, mode, arg0, arg1)
     RTX_CODE code;
     enum machine_mode mode;
     int arg0;
     const char *arg1;
{
  rtx rt;
  rt = ggc_alloc_rtx (2);
  memset (rt, 0, sizeof (struct rtx_def) - sizeof (rtunion));

  PUT_CODE (rt, code);
  PUT_MODE (rt, mode);
  XINT (rt, 0) = arg0;
  XSTR (rt, 1) = arg1;

  return rt;
}

rtx
gen_rtx_fmt_i (code, mode, arg0)
     RTX_CODE code;
     enum machine_mode mode;
     int arg0;
{
  rtx rt;
  rt = ggc_alloc_rtx (1);
  memset (rt, 0, sizeof (struct rtx_def) - sizeof (rtunion));

  PUT_CODE (rt, code);
  PUT_MODE (rt, mode);
  XINT (rt, 0) = arg0;

  return rt;
}

rtx
gen_rtx_fmt_isE (code, mode, arg0, arg1, arg2)
     RTX_CODE code;
     enum machine_mode mode;
     int arg0;
     const char *arg1;
     rtvec arg2;
{
  rtx rt;
  rt = ggc_alloc_rtx (3);
  memset (rt, 0, sizeof (struct rtx_def) - sizeof (rtunion));

  PUT_CODE (rt, code);
  PUT_MODE (rt, mode);
  XINT (rt, 0) = arg0;
  XSTR (rt, 1) = arg1;
  XVEC (rt, 2) = arg2;

  return rt;
}

rtx
gen_rtx_fmt_iE (code, mode, arg0, arg1)
     RTX_CODE code;
     enum machine_mode mode;
     int arg0;
     rtvec arg1;
{
  rtx rt;
  rt = ggc_alloc_rtx (2);
  memset (rt, 0, sizeof (struct rtx_def) - sizeof (rtunion));

  PUT_CODE (rt, code);
  PUT_MODE (rt, mode);
  XINT (rt, 0) = arg0;
  XVEC (rt, 1) = arg1;

  return rt;
}

rtx
gen_rtx_fmt_Ess (code, mode, arg0, arg1, arg2)
     RTX_CODE code;
     enum machine_mode mode;
     rtvec arg0;
     const char *arg1;
     const char *arg2;
{
  rtx rt;
  rt = ggc_alloc_rtx (3);
  memset (rt, 0, sizeof (struct rtx_def) - sizeof (rtunion));

  PUT_CODE (rt, code);
  PUT_MODE (rt, mode);
  XVEC (rt, 0) = arg0;
  XSTR (rt, 1) = arg1;
  XSTR (rt, 2) = arg2;

  return rt;
}

rtx
gen_rtx_fmt_sEss (code, mode, arg0, arg1, arg2, arg3)
     RTX_CODE code;
     enum machine_mode mode;
     const char *arg0;
     rtvec arg1;
     const char *arg2;
     const char *arg3;
{
  rtx rt;
  rt = ggc_alloc_rtx (4);
  memset (rt, 0, sizeof (struct rtx_def) - sizeof (rtunion));

  PUT_CODE (rt, code);
  PUT_MODE (rt, mode);
  XSTR (rt, 0) = arg0;
  XVEC (rt, 1) = arg1;
  XSTR (rt,