/* Generated automatically by gengenrtl from rtl.def.  */

#ifndef GCC_GENRTL_H
#define GCC_GENRTL_H

extern rtx gen_rtx_fmt_s	PARAMS ((RTX_CODE, enum machine_mode mode,
				       const char *arg0));
extern rtx gen_rtx_fmt_ee	PARAMS ((RTX_CODE, enum machine_mode mode,
				       rtx arg0, rtx arg1));
extern rtx gen_rtx_fmt_ue	PARAMS ((RTX_CODE, enum machine_mode mode,
				       rtx arg0, rtx arg1));
extern rtx gen_rtx_fmt_iss	PARAMS ((RTX_CODE, enum machine_mode mode,
				       int arg0, const char *arg1,
				       const char *arg2));
extern rtx gen_rtx_fmt_is	PARAMS ((RTX_CODE, enum machine_mode mode,
				       int arg0, const char *arg1));
extern rtx gen_rtx_fmt_i	PARAMS ((RTX_CODE, enum machine_mode mode,
				       int arg0));
extern rtx gen_rtx_fmt_isE	PARAMS ((RTX_CODE, enum machine_mode mode,
				       int arg0, const char *arg1,
				       rtvec arg2));
extern rtx gen_rtx_fmt_iE	PARAMS ((RTX_CODE, enum machine_mode mode,
				       int arg0, rtvec arg1));
extern rtx gen_rtx_fmt_Ess	PARAMS ((RTX_CODE, enum machine_mode mode,
				       rtvec arg0, const char *arg1,
				       const char *arg2));
extern rtx gen_rtx_fmt_sEss	PARAMS ((RTX_CODE, enum machine_mode mode,
				       const char *arg0, rtvec arg1,
				       const char *arg2, const char *arg3));
extern rtx gen_rtx_fmt_eE	PARAMS ((RTX_CODE, enum machine_mode mode,
				       rtx arg0, rtvec arg1));
extern rtx gen_rtx_fmt_E	PARAMS ((RTX_CODE, enum machine_mode mode,
				       rtvec arg0));
extern rtx gen_rtx_fmt_e	PARAMS ((RTX_CODE, enum machine_mode mode,
				       rtx arg0));
extern rtx gen_rtx_fmt_sse	PARAMS ((RTX_CODE, enum machine_mode mode,
				       const char *arg0, const char *arg1,
				       rtx arg2));
extern rtx gen_rtx_fmt_ss	PARAMS ((RTX_CODE, enum machine_mode mode,
				       const char *arg0, const char *arg1));
extern rtx gen_rtx_fmt_sE	PARAMS ((RTX_CODE, enum machine_mode mode,
				       const char *arg0, rtvec arg1));
extern rtx gen_rtx_fmt_iuueiee	PARAMS ((RTX_CODE, enum machine_mode mode,
				       int arg0, rtx arg1, rtx arg2,
				       rtx arg3, int arg4, rtx arg5,
				       rtx arg6));
extern rtx gen_rtx_fmt_iuueiee0	PARAMS ((RTX_CODE, enum machine_mode mode,
				       int arg0, rtx arg1, rtx arg2,
				       rtx arg3, int arg4, rtx arg5,
				       rtx arg6));
extern rtx gen_rtx_fmt_iuueieee	PARAMS ((RTX_CODE, enum machine_mode mode,
				       int arg0, rtx arg1, rtx arg2,
				       rtx arg3, int arg4, rtx arg5,
				       rtx arg6, rtx arg7));
extern rtx gen_rtx_fmt_iuu	PARAMS ((RTX_CODE, enum machine_mode mode,
				       int arg0, rtx arg1, rtx arg2));
extern rtx gen_rtx_fmt_iuu00iss	PARAMS ((RTX_CODE, enum machine_mode mode,
				       int arg0, rtx arg1, rtx arg2,
				       int arg3, const char *arg4,
				       const char *arg5));
extern rtx gen_rtx_fmt_ssiEEsi	PARAMS ((RTX_CODE, enum machine_mode mode,
				       const char *arg0, const char *arg1,
				       int arg2, rtvec arg3, rtvec arg4,
				       const char *arg5, int arg6));
extern rtx gen_rtx_fmt_Ei	PARAMS ((RTX_CODE, enum machine_mode mode,
				       rtvec arg0, int arg1));
extern rtx gen_rtx_fmt_eEee0	PARAMS ((RTX_CODE, enum machine_mode mode,
				       rtx arg0, rtvec arg1, rtx arg2,
				       rtx arg3));
extern rtx gen_rtx_fmt_eee	PARAMS ((RTX_CODE, enum machine_mode mode,
				       rtx arg0, rtx arg1, rtx arg2));
extern rtx gen_rtx_fmt_	PARAMS ((RTX_CODE, enum machine_mode mode));
extern rtx gen_rtx_fmt_w	PARAMS ((RTX_CODE, enum machine_mode mode,
				       HOST_WIDE_INT arg0));
extern rtx gen_rtx_fmt_0wwwww	PARAMS ((RTX_CODE, enum machine_mode mode,
				       HOST_WIDE_INT arg0,
				       HOST_WIDE_INT arg1,
				       HOST_WIDE_INT arg2,
				       HOST_WIDE_INT arg3,
				       HOST_WIDE_INT arg4));
extern rtx gen_rtx_fmt_0	PARAMS ((RTX_CODE, enum machine_mode mode));
extern rtx gen_rtx_fmt_i0	PARAMS ((RTX_CODE, enum machine_mode mode,
				       int arg0));
extern rtx gen_rtx_fmt_ei	PARAMS ((RTX_CODE, enum machine_mode mode,
				       rtx arg0, int a