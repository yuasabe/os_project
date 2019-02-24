/* "nask.c" */
/* copyright(C) 2003 H.Kawai(川合秀実) */
/*   [OSASK 3978], [OSASK 3979]で光成さんの指摘を大いに参考にしました */
/*	小柳さんのstring0に関する指摘も参考にしました */

#include "../include/stdlib.h"	/* malloc/free */

#define	DEBUG			0

int nask_LABELBUFSIZ = 256 * 1024;

#define	UCHAR			unsigned char

#define	OPCLENMAX		8	/* 足りなくなったら12にしてください */
#define MAX_SECTIONS	8

#define E_LABEL0		16
int nask_L_LABEL0 = 16384; /* externラベルは16300個程度使える */
int nask_maxlabels = 64 * 1024; /* 64K個(LL:88*64k) */

static void setdec(unsigned int i, int n, UCHAR *s);
static void sethex0(unsigned int i, int n, UCHAR *s);

static void *cmalloc(int size)
{
	int i;
	char *p = malloc(size);
//	if (p) {
		for (i = 0; i < size; i++)
			p[i] = 0;
//	}
	return p;
}

struct INST_TABLE {
	UCHAR opecode[OPCLENMAX];
	unsigned int support;
	UCHAR param[8];
};

struct STR_SECTION {
	unsigned int dollar_label0; /* $ */
	unsigned int dollar_label1; /* ..$ */
	unsigned int dollar_label2; /* $$ */
	int total_len;
	UCHAR *p0, *p; /* ソート用のポインタ */
	UCHAR name[17], name_len;
	signed char align0, align1; /* -1は未設定 */
};

struct STR_OUTPUT_SECTION {
	UCHAR *p, *d0, *reloc_p;
	int addr, relocs;
	UCHAR align, flags;
};

extern int nask_errors;

#define SUP_8086		0x000000ff	/* bit 0 */
#define SUP_80186		0x000000fe	/* bit 1 */
#define SUP_80286		0x000000fc	/* bit 2 */
#define SUP_80286P		0x000000a8	/* bit 3 */
#define SUP_i386		0x000000f0	/* bit 4 */
#define SUP_i386P		0x000000a0	/* bit 5 */
#define SUP_i486		0x000000c0	/* bit 6 */
#define SUP_i486P		0x00000080	/* bit 7 */
#define	SUP_Pentium
#define	SUP_Pentium2
#define	SUP_Pentium3
#define	SUP_Pentium4

#define	PREFIX			0x01	/* param[1]がプリフィックス番号 */
#define	NO_PARAM		0x02	/* param[1]の下位4bitがオペコードバイト数 */
#define	OPE_MR			0x03	/* mem/reg,reg型 */ /* [1]:datawidth, [2]:len */
#define	OPE_RM			0x04	/* reg,mem/reg型 */
#define	OPE_M			0x05	/* mem/reg型 */
#define OPE_SHIFT		0x06	/* ROL, ROR, RCL, RCR, SHL, SAL, SHR, SAR */
#define OPE_RET			0x07	/* RET, RETF, RETN */
#define OPE_AAMD		0x08	/* AAM, AAD */
#define OPE_INT			0x09	/* INT */
#define	OPE_PUSH		0x0a	/* INC, DEC, PUSH, POP */
#define	OPE_MOV			0x0b	/* MOV */
#define	OPE_ADD			0x0c	/* ADD, OR, ADC, SBB, AND, SUB, XOR, CMP */
#define	OPE_XCHG		0x0d	/* XCHG */
#define	OPE_INOUT		0x0e	/* IN, OUT */
#define	OPE_IMUL		0x0f	/* IMUL */
#define	OPE_TEST		0x10	/* TEST */
#define	OPE_MOVZX		0x11	/* MOVSX, MOVZX */
#define	OPE_SHLD		0x12	/* SHLD, SHRD */
#define	OPE_LOOP		0x13	/* LOOPcc, JCXZ */
#define	OPE_JCC			0x14	/* Jcc */
#define	OPE_BT			0x15	/* BT, BTC, BTR, BTS */
#define	OPE_ENTER		0x16	/* ENTER */
#define OPE_ALIGN		0x17	/* ALIGN, ALIGNB */
#define	OPE_FPU			0x30
#define	OPE_FPUP		0x31
#define	OPE_FSTSW		0x32
#define	OPE_FXCH		0x33
#define	OPE_ORG			0x3d	/* ORG */
#define	OPE_RESB		0x3e	/* RESB, RESW, RESD, RESQ, REST */
#define	OPE_EQU			0x3f

#define	OPE_JMP			0x40	/* CALL, JMP */
#define OPE_GLOBAL		0x44	/* GLOBAL, EXTERN */
#define	OPE_TIMES		0x47	/* TIMES */
#define	OPE_DB			0x48	/* DB, DW, DD, DQ, DT */
#define	OPE_END			0x49

/* NO_PARAM用 */
#define	OPE16			0x10
#define	OPE32			0x20
#define DEF_DS			0x40
	/* param[1]のbit4 : ope32 */
	/* param[1]のbit5 : ope16 */
	/* param[1]のbit6 : デフォルトプリフィックスDS */
	/* param[1]のbit7 : デフォルトプリフィックスSS */

static UCHAR table_prms[] = {
	0, 0, 0 /* NO_PARAM */, 2 /* OPE_MR */, 2 /* OPE_RM */,
	1 /* OPE_M */, 2 /* OPE_SHIFT */, 9 /* OPE_RET */, 9 /* OPE_AAMD */,
	1 /* OPE_INT */, 1 /* OPE_PUSH */, 2 /* OPE_MOV */, 2 /* OPE_ADD */,
	2 /* OPE_XCHG */, 2 /* OPE_INOUT */, 9 /* OPE_IMUL */, 2 /* OPE_TEST */,
	2 /* OPE_MOVZX */, 3 /* OPE_SHLD */, 9 /* OPE_LOOP */, 1 /* OPE_JCC */,
	2 /* OPE_BT */, 2 /* OPE_ENTER */, 1 /* OPE_ALIGN */, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,	0, 0, 0, 0, 0, 0, 0, 0,
	9 /* OPE_FPU */, 9 /* OPE_FPUP */, 1 /* OPE_FSTSW */, 9 /* OPE_FXCH */,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 1 /* OPE_ORG */, 1 /* OPE_RESB */,
	1 /* OPE_EQU */
};

struct STR_DECODE {
	UCHAR *label, *param;
	struct INST_TABLE *instr;
	unsigned int prm_t[3];
	UCHAR *prm_p[3];
	int prefix;
	int gparam[3], gvalue[3], gp_mem, gp_reg;
	struct STR_SECTION *sectable;
	UCHAR error, flag /* , dollar */;
};
/* flagのbit0はmem/regがregかどうかをあらわす */

struct STR_TERM {
	int term_type;
	int value;
};

struct STR_OFSEXPR {
	int scale[2], disp;
	unsigned char reg[2], dispflag; /* 0xffのとき、unknown, regが127以下なら、スケール無し */
	unsigned char err;
};

struct STR_DEC_EXPR_STATUS {
	unsigned int support;
	int glabel_len;
	UCHAR *glabel;
	signed char datawidth; /* -1(default), 1(byte), 2(word), 4(dword) */
	signed char seg_override; /* -1(default), 0〜5 */
	signed char range; /* -1(default), 0(short), 1(near), 2(far) */
	char nosplit; /* 0(default), 1(nosplit) */
	char use_dollar;  /* 0(no use), 1(use) */
	char option;
	char to_flag;
	unsigned int dollar_label0; /* $ */
	unsigned int dollar_label1; /* ..$ */
	unsigned int dollar_label2; /* $$ */
};

struct STR_STATUS {
	UCHAR *src1; /* ファイル終端ポインタ */
	unsigned int support, file_len;
	char bits, optimize, format, option;
	struct STR_DEC_EXPR_STATUS expr_status;
	struct STR_OFSEXPR ofsexpr;
	struct STR_TERM *expression, *mem_expr;
	UCHAR *file_p;
};

struct STR_IFDEFBUF {
	/* 条件付き定義用バッファ構造体 */
	UCHAR *bp, *bp0, *bp1; /* range-error用バッファ */
	UCHAR vb[12]; /* bit0-4:バイト数, bit7:exprフラグ, bit5-6:レンジチェック */
	int dat[12];
	UCHAR *expr[12];
};

UCHAR *decoder(struct STR_STATUS *status, UCHAR *src, struct STR_DECODE *decode);
UCHAR *putprefix(UCHAR *dest0, UCHAR *dest1, int prefix, int bits, int opt);
void put4b(unsigned int i, UCHAR *p);
unsigned int get4b(UCHAR *p);
struct STR_TERM *decode_expr(UCHAR **ps, UCHAR *s1, struct STR_TERM *expr, int *priority, struct STR_DEC_EXPR_STATUS *status);
void calc_ofsexpr(struct STR_OFSEXPR *ofsexpr, struct STR_TERM **pexpr, char nosplit);
int getparam(UCHAR **ps, UCHAR *s1, int *p, struct STR_TERM *expression, struct STR_TERM *mem_expr, 
	struct STR_OFSEXPR *ofsexpr, struct STR_DEC_EXPR_STATUS *status);
int testmem(struct STR_OFSEXPR *ofsexpr, int gparam, struct STR_STATUS *status, int *prefix);
void putmodrm(struct STR_IFDEFBUF *ifdef, int tmret, int gparam,
	struct STR_STATUS *status, /* struct STR_OFSEXPR *ofsexpr, */ int ttt);
int microcode90(struct STR_IFDEFBUF *ifdef, struct STR_TERM *expr, int *def, signed char dsiz);
int microcode91(struct STR_IFDEFBUF *ifdef, struct STR_TERM *expr, int *def, signed char dsiz);
int microcode94(struct STR_IFDEFBUF *ifdef, struct STR_TERM *expr, int *def);
int defnumexpr(struct STR_IFDEFBUF *ifdef, struct STR_TERM *expr, UCHAR vb, UCHAR def);
int getparam0(UCHAR *s, struct STR_STATUS *status);
int getconst(UCHAR **ps, struct STR_STATUS *status, int *p);
int testmem0(struct STR_STATUS *status, int gparam, int *prefix);
static UCHAR *labelbuf0, *labelbuf;
static UCHAR *locallabelbuf0 /* 256bytes */, *locallabelbuf;
static int nextlabelid;
int label2id(int len, UCHAR *label, int extflag);
UCHAR *id2label(int id);
UCHAR *put_expr(UCHAR *s, struct STR_TERM **pexpr);
UCHAR *flush_bp(int len, UCHAR *buf, UCHAR *dest0, UCHAR *dest1, struct STR_IFDEFBUF *ifdef);
struct STR_TERM *rel_expr(struct STR_TERM *expr, struct STR_DEC_EXPR_STATUS *status);
UCHAR *LL_skip_expr(UCHAR *p);
UCHAR *LL_skipcode(UCHAR *p);

#define	defnumconst(ifdef, imm, virbyte, typecode) ifdef->vb[(virbyte) & 0x07] = typecode; ifdef->dat[(virbyte) & 0x07] = imm

/* リマークNL(f8) : ラインスタート, 4バイトのレングス, 4バイトのポインタ
	バイト列を並べる */
/* リマークADR(e0) : アドレス出力 */
/* リマークBY(e1) : 1バイト出力 */
/* リマークWD(e2) : 2バイト出力 */
/* リマーク3B(e3) : 3バイト出力 */
/* リマークDW(e4) : 4バイト出力 */
/* リマーク[BY](e5) : 1バイト出力[]つき */
/* リマーク[WD](e6) : 2バイト出力[]つき */
/* リマーク[3B](e7) : 3バイト出力[]つき */
/* リマーク[DW](e8) : 4バイト出力[]つき */

#define	REM_ADDR		0xe0
//#define	REM_BYTE		0xe1	/* 廃止 */
//#define	REM_WORD		0xe2	/* 廃止 */
//#define	REM_DWRD		0xe4	/* 廃止 */
#define	REM_ADDR_ERR	0xe5
#define	REM_RANGE_ERR	0xe8
#define REM_3B			0xf1
#define REM_4B			0xf2
#define REM_8B			0xf6
#define	SHORT_DB0		0x30
#define	SHORT_DB1		0x31
#define	SHORT_DB2		0x32
#define	SHORT_DB4		0x34

#define	EXPR_MAXSIZ		2048
#define	EXPR_MAXLEN		1000

UCHAR *skipspace(UCHAR *s, UCHAR *t)
{
	while (s < t && *s != '¥n' && *s <= ' ')
		s++;
	return s;
}

UCHAR *putimm(int i, UCHAR *p)
/* 最大6バイト出力 */
{
	UCHAR c = 6;
	if (i >= 0) {
		if (i <= 0xff)
			c = 0x00;
		else if (i <= 0xffff)
			c = 0x02;
		else if (i <= 0xffffff)
			c = 0x04;
	} else {
		if (i >= -0x100)
			c = 0x01;
		else if (i >= -0x10000)
			c = 0x03;
		else if (i >= -0x1000000)
			c = 0x05;
	}
	p[0] = c;
	p[1] = i & 0xff;
	c >>= 1;
	p += 2;
	while (c) {
		i >>= 8;
		c--;
		*p++ = i & 0xff;
	}
	return p;
}

UCHAR *nask(UCHAR *src0, UCHAR *src1, UCHAR *dest0, UCHAR *dest1)
/* dest1を返す(NULLならあふれた) */
{
	int i, j, k, prefix_def, tmret;
	UCHAR buf[2 * 8], *bp; /* bufは適当にmallocした方がいいかも */
	UCHAR *src, c, *s, *labelflags, *dest00 = dest0;
	struct STR_STATUS *status;
	struct STR_DECODE *decode;
	struct INST_TABLE *itp;
	struct STR_IFDEFBUF *ifdef;
	struct STR_TERM *expr;
	static int tbl_o16o32[4] =
		{ 0, 0x10000000 /* O16(暗黙) */, 0, 0x20000000 /* O32(暗黙) */ };
	struct STR_SECTION *sectable, *section;
	nextlabelid = nask_L_LABEL0;
	status = malloc(sizeof (*status));
	decode = malloc(sizeof (*decode));
	ifdef = malloc(sizeof (*ifdef));
	status->expression = malloc(EXPR_MAXLEN * sizeof (struct STR_TERM));
	status->mem_expr = malloc(EXPR_MAXLEN * sizeof (struct STR_TERM));
	sectable = cmalloc(MAX_SECTIONS * sizeof (struct STR_SECTION));
	ifdef->bp0 = malloc(256);
	ifdef->bp1 = ifdef->bp0 + 256;
	labelbuf = labelbuf0 = malloc(nask_LABELBUFSIZ);
	locallabelbuf = locallabelbuf0 = malloc(256);
	for (i = 0; i < 9; i++)
		ifdef->expr[i] = malloc(EXPR_MAXSIZ);
	labelflags = malloc(nask_maxlabels);
	for (i = 0; i < nask_maxlabels; i++)
		labelflags[i] = 0;
	for (i = 0; i < MAX_SECTIONS; i++) {
	//	sectable[i].name[0] = '¥0';
	//	sectable[i].total_len = 0;
		sectable[i].align0 = -1;
		sectable[i].align1 = 1;
		sectable[i].dollar_label2 = 0xffffffff;
	}
	decode->sectable = section = sectable;
	section->name[0] = '.';
	section->name[1] = '.';
	section->name[2] = '¥0';
	section->p = dest0;

	status->src1 = src1;
	status->support = status->expr_status.support = 1; /* 1:8086 */
	status->bits = 16;
	status->optimize = 0;
	status->format = 0; /* BIN */
	status->option = 0; /* ほぼNASM互換 */
	status->expr_status.option = 0;
	status->file_len = 0;

	if (dest0 + 5 > dest1)
		dest0 = NULL;
	if (dest0 == NULL)
		goto overrun;

	dest0[0] = REM_3B;
	dest0[1] = 0; /* start section */
	dest0[2] = 0;
	dest0[3] = 0x68; /* 68-00 Intel-endian */
	dest0[4] = 0x00;
//	dest0[5] = 0x58; /* ORG */
//	dest0[6] = 0x00; /* 0 */
//	dest0[7] = 0x00;
	dest0 += 5;

	status->expr_status.dollar_label2 = 0xffffffff;
	while (src0 < src1) {
		if (status->expr_status.dollar_label2 == 0xffffffff) {
			status->expr_status.dollar_label2 = nextlabelid++;
			status->expr_status.dollar_label1 = status->expr_status.dollar_label2;
		}
		status->expr_status.dollar_label0 = status->expr_status.dollar_label1;
		status->expr_status.dollar_label1 = 0xffffffff;
		bp = buf;
		ifdef->vb[8] = 0; /* for TIMES */
		src = decoder(status, src0, decode);
		/* ラインスタート出力 */
		/* f7, src - src0, src0 */
		if (dest0 + 9 + 6 /* $の分 */ > dest1)
			dest0 = NULL;
		if (dest0 == NULL)
			goto overrun;
		dest0[0] = 0xf7; /* line start */
		put4b(src - src0, &dest0[1]);
		put4b((int) src0, &dest0[5]);
		dest0 += 9;
		ifdef->bp = ifdef->bp0;
	//	if (decode->dollar != 0 && status->expr_status.dollar_label0 == 0xffffffff)
	//		status->expr_status.dollar_label0 = nextlabelid++;
		if ((i = status->expr_status.dollar_label0) != 0xffffffff) {
			if (labelflags[i] == 0) {
				dest0[0] = 0x0e;
				labelflags[i] = 0x01;
				dest0 = putimm(i, &dest0[1]);
			}
		}
		if (decode->label) {
			/* ラベル定義 */
			bp[0] = 0x0e; /* プログラムカウンタをラベル定義するコマンド */
 			if (decode->instr != NULL && decode->instr->param[0] == OPE_EQU)
				bp[0] = 0x2d; /* EQU */
			s = decode->label;
			do {
				c = *s;
				if (c <= ' ')
					break;
				if (c == ':')
					break;
				if (c == ';')
					break;
				if (c == ',')
					break;
			} while (++s < src1);
			i = label2id(s - decode->label, decode->label, 0);
			if (labelflags[i]) {
				*bp++ = 0xe7;
				c = 0; /* mod nnn r/m なし */
				goto outbp;
			}
			labelflags[i] = 0x01;
			c = bp[0];
			bp = putimm(i, &bp[1]);
			if (c == 0x0e) {
				if ((dest0 = flush_bp(bp - buf, buf, dest0, dest1, ifdef)) == NULL)
					goto overrun;
				bp = buf;
			}
			if (*decode->label != '.') {
				if (!(decode->instr != NULL && decode->instr->param[0] == OPE_EQU)) {
					i = s - decode->label;
					locallabelbuf = locallabelbuf0;
					s = decode->label;
					while (i) {
						i--;
						*locallabelbuf++ = *s++;
					}
				}
			}
		}
times_skip:
		if (decode->error) {
err:
			/* エラー出力 */
			buf[0] = decode->error | 0xe0;
			bp = buf + 1;
			c = 0; /* mod nnn r/m なし */
			goto outbp;
		}
		c = 0; /* mod nnn r/m なし */
		prefix_def = status->bits; /* デフォルト状態 */
		if ((itp = decode->instr) != 0) {
			switch (itp->param[0]) {
			case NO_PARAM:
				/* プリフィックス */
				j = itp->param[1];
				if (j & OPE16)
					decode->prefix |= 0x10000000; /* O16(暗黙) */
				if (j & OPE32)
					decode->prefix |= 0x20000000; /* O32(暗黙) */
				if (j & DEF_DS)
					prefix_def |= 0x01; /* DS */
				for (i = 0; i < (j & 0x0f); i++) {
					bp[0] = SHORT_DB1; /* 0x31 */
					bp[1] = itp->param[2 + i];
					bp += 2;
				}
			//	c = 0; /* mod nnn r/m なし */
				break;

			case OPE_M:
			ope_m:
				if ((i = decode->gparam[0]) & 0xe0) /* regでもmemでもない || rangeがついたらエラー */
					goto err4; /* データタイプエラー */
				decode->flag = 0;
				if ((i & 0x10) == 0) {
					decode->flag = 1;
					if (decode->gvalue[0] >= 24) /* regだがreg8/reg16/reg32ではない */
						goto err4;
				}
				decode->gp_mem = i;
				decode->gp_reg = itp->param[1] << (9 - 4);
				i &= 0x0f;
				goto ope_mr_check0;

			case OPE_MR:
				if ((j = decode->gparam[0]) & 0xe0) /* regでもmemでもない || rangeがついたらエラー */
					goto err4;
				decode->flag = 0;
				if ((j & 0x10) == 0) {
					decode->flag = 1;
					if (decode->gvalue[0] >= 24) /* regだがreg8/reg16/reg32ではない */
						goto err4;
				}
				if (decode->gparam[1] & 0x1f0) /* regではない || rangeがついたらエラー || use $もエラー */
					goto err4;
				if (decode->gvalue[1] >= 24) /* regだがreg8/reg16/reg32ではない */
					goto err4;
				decode->gp_reg = decode->gparam[1];
				if ((j & 0x0f) == 0x0f && (itp->param[1] & 0x80) != 0) {
					/* memのデータサイズが不定 && 第二オペランドにsame0指定あり */
					j = (j & ‾0x0f) | (decode->gparam[1] & 0x0f);
				}
				decode->gp_mem = decode->gparam[0] = j;
				goto ope_mr2;

			case OPE_RM:
				if (decode->gparam[0] & 0x1f0) /* regではない || rangeがついたらエラー || use $もエラー */
					goto err4;
				if (decode->gvalue[0] >= 24) /* regだがreg8/reg16/reg32ではない */
					goto err4;
				decode->gp_reg = decode->gparam[0];
				if ((j = decode->gparam[1]) & 0xe0) /* regでもmemでもない || rangeがついたらエラー */
					goto err4;
				decode->flag = 0;
				if ((j & 0x10) == 0) {
					decode->flag = 1;
					if (decode->gvalue[1] >= 24) /* regだがreg8/reg16/reg32ではない */
						goto err4;
				}
				if ((j & 0x0f) == 0x0f && (itp->param[1] & 0x80) != 0) {
					/* memのデータサイズが不定 && 第二オペランドにsame0指定あり */
					j = (j & ‾0x0f) | (decode->gparam[0] & 0x0f);
				}
				decode->gp_mem = decode->gparam[1] = j;

		ope_mr2:
				/* オペランドサイズチェック */
				k = itp->param[1] >> 4;
				i = decode->gparam[0] & 0x0f;
				j = decode->gparam[1] & 0x0f;
				if (k == 0)
					goto ope_mr_check0;
				if (k & 8) {
					if (i == j)
						goto ope_mr_check0;
				}
				if (j > 4) {
			err3:
					decode->error = 3; /* data size error */
					goto err;
				}
			//	if (j == 3)
			//		goto err3;
				if ((k & j) == 0)
					goto err3;

		ope_mr_check0:
				k = itp->param[1] & 0x0f;
				if (k == 0)
					goto ope_mr_mem;
				if (k & 8) {
					if (i == 0xf)
						goto ope_mr_mem;
				}
				if (i > 4)
					goto err3;
			//	if (i == 3)
			//		goto err3;
				if ((k & i) == 0)
					goto err3;
		ope_mr_mem:
				j = decode->flag & 1;
				if (itp->param[2] & 0x40) { /* no-reg */
					if (j != 0) {
			err4:
						decode->error = 4; /* data type error */
						goto err;
					}
				}
				if (itp->param[2] & (UCHAR) 0x80) { /* no-mem */
					if (j == 0)
						goto err4;
				}
			//	if (j == 0) {
			//		tmret = testmem0(status, decode->gp_mem, &decode->prefix);
			//		if (tmret == 0)
			//			goto err5; /* addressing error */
			//		prefix_def |= tmret & 0x03;
			//	}
				j = itp->param[2] & 0x07;
				for (i = 0; i < j; i++) {
					bp[0] = SHORT_DB1; /* 0x31 */
					bp[1] = itp->param[3 + i];
					bp += 2;
				}
				if ((itp->param[2] & 0x30) != 0x20) {
					/* データサイズを確定 */
					i = decode->gparam[0];
					if (itp->param[2] & 0x08)
						i = decode->gparam[1];
					i &= 0x0f;
					if ((itp->param[2] & 0x20) == 0) {
						decode->prefix |= (tbl_o16o32 - 1)[i];
					//	if (i == 2)
					//		decode->prefix |= 0x10000000; /* O16(暗黙) */
					//	if (i == 4)
					//		decode->prefix |= 0x20000000; /* O32(暗黙) */
					}
					if (itp->param[2] & 0x10) {
						if (i != 1)
							bp[-1] |= 0x01;
					}
				}
				if (status->optimize >= 1) {
					if (itp->param[3] == 0x8d) /* LEA */
						decode->prefix &= ‾0x07e0; /* bit5-10 */
				}
				bp[0] = 0x78; /* mod nnn r/m */
				bp[1] = 0x79; /* sib */
				bp[2] = 0x7a; /* disp */
				bp += 3;
	setc:
				c = 3 ^ decode->flag; /* mod nnn r/m あり */ 				
				break;

			case OPE_SHIFT: /* mem/reg, imm8|CL */
				if ((j = decode->gparam[0]) & 0xe0) /* regでもmemでもない || rangeがついたらエラー */
					goto err4;
				decode->gp_mem = j;
				decode->gp_reg = itp->param[1] << 9; /* TTT */
				decode->flag = 0; /* mem */
				if ((j & 0x10) == 0) {
					/* reg */
					decode->flag = 1;
					if (decode->gvalue[0] >= 24) /* regだがreg8/reg16/reg32ではない */
						goto err4;
				}
				/* データサイズを確定 */
				i = decode->gparam[0] & 0x0f;
				if (i == 0)
					goto err3;
				if (i > 4)
					goto err3;
				decode->prefix |= (tbl_o16o32 - 1)[i];
			//	if (i == 2)
			//		decode->prefix |= 0x10000000; /* O16(暗黙) */
			//	if (i == 4)
			//		decode->prefix |= 0x20000000; /* O32(暗黙) */
				j = 0;
				if (i != 1)
					j++; /* j = 1; */
				if (decode->gparam[1] == 0x2201) { /* CL */
					bp[0] = SHORT_DB1; /* 0x31 */
					bp[1] = 0xd2 | j;
					bp[2] = 0x78; /* mod nnn r/m */
					bp[3] = 0x79; /* sib */
					bp[4] = 0x7a; /* disp */
				} else {
					static int mcode[] = {
						0x0154, SHORT_DB1, 0xc0, 0x98, 0, /* default */
						0x01,   SHORT_DB1, 0xd0, SHORT_DB0, 0 /* if == 1 */
					};
					mcode[2] = 0xc0 | j;
					mcode[7] = 0xd0 | j;
					if ((decode->gparam[1] & 0xf0) != 0x20) /* immではない || rangeがついていた */
						goto err4; /* data type error */
					if ((decode->gparam[1] & 0x0f) == 0x0f) {
						if (microcode94(ifdef, status->expression, mcode))
							goto err2;
					} else if ((decode->gparam[1] & 0x0f) == 0x01) {
						defnumconst(ifdef, 0xc0 | j, 0x74, 0x01 /* UCHAR, const */);
						if (defnumexpr(ifdef, status->expression, 0x75 & 0x07, 0x98 & 0x07))
							goto err2;
					} else
						goto err3; /* WORDやDWORDが指定された */
					bp[0] = 0x7c; /* 1100000w || 1101000w */
					bp[1] = 0x78; /* mod nnn r/m */
					bp[2] = 0x79; /* sib */
					bp[3] = 0x7a; /* disp */
					bp[4] = 0x7d; /* imm8 || none */
				}
				bp += 5;
			//	c = 3 ^ decode->flag; /* mod nnn r/m あり */ 				
			//	break;
				goto setc;

			case OPE_RET: /* RET, RETF, RETN */
				bp[0] = SHORT_DB1; /* 0x31 */
			//	c = 0; /* mod nnn r/m なし */
				if (decode->flag == 0) {
					/* オペランドなし */
				//	bp[0] = SHORT_DB1; /* 0x31 */
					bp[1] = itp->param[1] | 0x01;
					bp += 2;
					break;
				}
				if (decode->flag > 1)
					goto err2; /* パラメータエラー */
				if ((decode->gparam[0] & 0xf0) != 0x20)
					goto err2; /* immではない || rangeがついていた */
				if ((decode->gparam[0] & 0x0f) == 2)
					goto OPE_RET_notopt; /* WORD指定あり */
				if ((decode->gparam[0] & 0x0f) != 0x0f)
					goto err3; /* BYTEやDWORDが指定された */
				if (status->optimize == 0) {
					/* 最適化しない */
		OPE_RET_notopt:
					if (defnumexpr(ifdef, status->expression, 0x75 & 0x07, 0x9a & 0x07))
						goto err2;
				//	bp[0] = SHORT_DB1; /* 0x31 */
					bp[1] = itp->param[1];
					bp += 2;
				} else {
					static int mcode[] = {
						0x0154, SHORT_DB1, 0, 0x9a, 0, /* default */
						0x00,   SHORT_DB1, 0, SHORT_DB0, 0 /* if == 0 */
					};
					mcode[2] = itp->param[1];
					mcode[7] = itp->param[1] | 0x01;
					/* 最適化する */
					if (microcode94(ifdef, status->expression, mcode))
						goto err2;
					*bp++ = 0x7c; /* 自動選択されたオペコード */
				}
				*bp++ = 0x7d; /* imm16 || none */
				break;

			case OPE_AAMD: /* AAM, AAD */
				if (decode->flag == 0) {
					defnumconst(ifdef, itp->param[2], 0x74, 0x01 /* UCHAR, const */);
				} else if (decode->flag == 1) {
					if ((decode->gparam[0] & 0xf0) != 0x20)
						goto err2; /* immではない || rangeがついていた */
					if ((decode->gparam[0] & 0x0f) != 0x01 && (decode->gparam[0] & 0x0f) != 0x0f)
						goto err3; /* WORDやDWORDがついていた */
					if (defnumexpr(ifdef, status->expression, 0x74 & 0x07, 0x98 & 0x07))
						goto err2;
				} else
					goto err2; /* パラメータエラー */
				bp[0] = SHORT_DB1; /* 0x31 */
				bp[1] = itp->param[1];
				bp[2] = 0x7c; /* オペランド(デフォルト:itp->param[2]) */
				bp += 3;
			//	c = 0; /* mod nnn r/m なし */
				break;

			case OPE_INT: /* INT */
				if ((decode->gparam[0] & 0xf0) != 0x20)
					goto err2; /* immではない || rangeがついていた */
				if ((decode->gparam[0] & 0x0f) == 1)
					goto OPE_INT_notopt; /* BYTE指定あり */
				if ((decode->gparam[0] & 0x0f) != 0x0f)
					goto err3; /* WORDやDWORDがついていた */
				if (status->optimize == 0) {
		OPE_INT_notopt:
					/* 最適化しない */
					if (defnumexpr(ifdef, status->expression, 0x75 & 0x07, 0x98 & 0x07))
						goto err2;
					bp[0] = SHORT_DB1; /* 0x31 */
					bp[1] = 0xcd;
					bp += 2;
				} else {
					static int mcode[] = {
						0x0154, SHORT_DB1, 0xcd, 0x98, 0, /* default */
						0x03,   SHORT_DB1, 0xcc, SHORT_DB0, 0 /* if == 3 */
					};
					/* 最適化する */
					if (microcode94(ifdef, status->expression, mcode))
						goto err2;
					*bp++ = 0x7c; /* 自動選択されたオペコード */
				}
				*bp++ = 0x7d; /* imm8 || none */
			//	c = 0; /* mod nnn r/m なし */
				break;

			case OPE_PUSH: /* PUSH, POP, INC, DEC */
				if (decode->gparam[0] & 0xc0)
					goto err2; /* rangeがついていた */
			//	c = 0; /* mod nnn r/m なし */
				decode->gp_mem = decode->gparam[0];
				decode->gp_reg = (itp->param[1] & 0x07) << 9;
				bp[0] = SHORT_DB1; /* 0x31 */
				switch (decode->gparam[0] & 0x30) {
				case 0x00: /* reg */
					if (decode->gvalue[0] < 16) {
						/* reg16/reg32 */
						decode->prefix |= (tbl_o16o32 - 1)[decode->gparam[0] & 0x0f];
					//	i = 0x10000000; /* O16(暗黙) */
					//	if (decode->gvalue[0] < 8) {
					//	//	i = 0x20000000; /* O32(暗黙) */
					//		i <<= 1;
					//	}
					//	decode->prefix |= i;
						bp[1] = itp->param[2] | (decode->gvalue[0] & 0x07);
						bp += 2;
						goto outbp;
					}
					if (decode->gvalue[0] < 24) {
						/* reg8 */
						if (itp->param[1] & 0x08)
							goto err3; /* PUSH, POP */
						bp[1] = itp->param[3];
						c = 2; /* mod nnn r/m あり */
			ope_push_mem:
						bp[2] = 0x78;
						bp[3] = 0x79;
						bp[4] = 0x7a;
						bp += 5;
						goto outbp;
					}
					if ((itp->param[1] & 0x08) == 0)
						goto err2; /* INC, DEC */
					if (decode->gvalue[0] < 28) {
						/* ES, CS, SS, DS */
						/* NASKは"POP CS"をエラーにしない(8086のため) */
						bp[1] = itp->param[4] | (decode->gvalue[0] & 0x03) << 3;
						bp += 2;
						goto outbp;
					}
					if (decode->gvalue[0] < 30) {
						/* FS, GS */
						bp[1] = 0x0f;
						bp[2] = SHORT_DB1; /* 0x31 */
						bp[3] = itp->param[5] | (decode->gvalue[0] & 0x03) << 3;
						bp += 4;
						goto outbp;
					}
					goto err2;
				case 0x10: /* mem */
				//	tmret = testmem0(status, decode->gp_mem, &decode->prefix);
				//	if (tmret == 0)
				//		goto err5; /* addressing error */
				//	prefix_def |= tmret & 0x03;
					c = decode->gparam[0] & 0x0f;
					bp[1] = 0;
					if (itp->param[1] & 0x08) {
						/* PUSH, POP */
						if (c == 0x01)
							goto err3;
						if (c == 0x0f)
							c = 1;
					} else {
						/* INC, DEC */
						if (c == 0x0f)
							goto err3;
						if (c != 1)
							bp[1] = 1;
					}
					bp[1] |= itp->param[3];
					decode->prefix |= (tbl_o16o32 - 1)[c];
					c = 3; /* mod nnn r/m あり */
					goto ope_push_mem;
				case 0x20: /* imm */
					if ((itp->param[1] & 0x10) == 0)
						goto err2;
					/* PUSH */
					{
						static int mcode[] = {
							0x54,	0x01 /* UCHAR, const */, 0x68 /* 16bit/32bit */,
									0x01 /* UCHAR, const */, 0x6a /* 8bit */
						};
						c = decode->gparam[0] & 0x0f;
						mcode[0] = 0x54; /* word/byte mode */
						if (c <= 4)
							decode->prefix |= (tbl_o16o32 - 1)[c];
						if (c == 4 || c == 0x0f && (prefix_def & 32) != 0)
							mcode[0] = 0x54 | 8 /* D-bit */;
						if ((decode->error = microcode90(ifdef, status->expression, mcode, c)) != 0)
							goto err;
						bp[0] = 0x7d;
						bp[1] = 0x7c;
						bp += 2;
						c = 0; /* mod nnn r/m なし */
						goto outbp;
					}
				}
				goto err2;

			case OPE_MOV: /* MOV */
				if (decode->gparam[0] & 0xc0)
					goto err4; /* rangeがついている, data type error */
				if (decode->gparam[1] & 0xc0)
					goto err4; /* rangeがついている, data type error */
				if ((decode->gparam[1] & 0x30) == 0x20) {
					/* imm */
					static char typecode[4] = { 0x9e & 0x07, 0x9b & 0x07, 0, 0x9d & 0x07 };
					c = decode->gparam[0] & decode->gparam[1] & 0x0f;
					if (c == 0)
						goto err3;
					if (c == 0x0f)
						goto err3;
					decode->prefix |= (tbl_o16o32 - 1)[c];
					if (defnumexpr(ifdef, status->expression, 0x74 & 0x07, typecode[c - 1]))
						goto err2; /* parameter error */
					bp[0] = SHORT_DB1; /* 0x31 */
					if ((j = decode->gparam[0] & 0x30) == 0x00) {
						if (decode->gvalue[0] >= 24)
							goto err4; /* data type error */
						bp[1] = 0xb0 | (decode->gvalue[0] & 0x07);
						bp[2] = 0x7c; /* imm */
						if (c != 1)
							bp[1] |= 0x08;
						bp += 3;
						c = 0; /* mod nnn r/m なし */
						goto outbp;
					}
					if (j != 0x10)
						goto err2;
					/* mem,imm */
					decode->gp_mem = decode->gparam[0];
				//	tmret = testmem0(status, decode->gp_mem = decode->gparam[0], &decode->prefix);
				//	if (tmret == 0)
				//		goto err5; /* addressing error */
				//	prefix_def |= tmret & 0x03;
				//	decode->flag = 0;
					bp[1] = 0xc6;
					decode->gp_reg = 0x00 << 9;
					bp[2] = 0x78;
					bp[3] = 0x79;
					bp[4] = 0x7a;
					bp[5] = 0x7c; /* imm */
					if (c != 1)
						bp[1] |= 0x01;
					bp += 6;
					c = 3; /* mod nnn r/m あり */
					goto outbp;
				}
				i = 0; /* direction-bit */
				if ((decode->gparam[1] & 0x30) == 0x10)
					goto mov_swap; /* memory */
				if ((decode->gparam[0] & 0x30) == 0x00 && decode->gvalue[0] >= 24) {
		mov_swap:
					i++;
				}
				tmret = 0;
				decode->flag = 1;
				if (((decode->gp_mem = decode->gparam[i]) & 0x30) == 0x10) {
					/* memory */
					tmret = testmem0(status, decode->gp_mem, &decode->prefix);
					if (tmret == 0)
						goto err5; /* addressing error */
				//	prefix_def |= tmret & 0x03;
					decode->flag = 0;
				} else if ((decode->gp_mem & 0x30) != 0x00)
					goto err4; /* immが来てはいけない */
				else if ((decode->gp_mem >> 9) >= 24)
					goto err4; /* reg8/reg16/reg32以外が来てはいけない */
				j = decode->gp_reg = decode->gparam[i ^ 1];
				c = decode->gp_mem & 0x0f;
				if ((j & 0x30) != 0x00)
					goto err4; /* regではない, data type error */
				if (24 <= (j >> 9) && (j >> 9) < 30 && decode->flag != 0) {
					if (c == 1)
						goto err3; /* data size error */
				} else if (c != 0x0f && (j & 0x0f) != c)
					goto err3; /* data size error */
				decode->gparam[i] = (decode->gparam[i] & ‾0x0f) | (j & 0x0f);
				if (j == 0x0004 /* EAX */ || j == 0x1002 /* AX */ || j == 0x2001 /* AL */) {
					if (tmret & 0x08) { /* disp-only */
						decode->prefix |= (tbl_o16o32 - 1)[j & 0x0f];
						c = 0xa0 | (i ^ 1) << 1;
						if (j != 0x2001 /* AL */)
							c |= 0x01;
						bp[0] = SHORT_DB1; /* 0x31 */
						bp[1] = c;
						bp[2] = 0x7a; /* disp */
						bp += 3;
					//	c = 3 ^ decode->flag; /* mod nnn r/m あり */
					//	goto outbp;
						goto setc;
					}
				}
				if ((j = decode->gp_reg >> 9) < 24) { /* mem/reg,reg */
					itp->param[2] = 0x11; /* w0 */
					itp->param[3] = 0x88 | i << 1;
					goto ope_mr_mem;
				}
				if (j < 30) { /* mem/reg,sreg */
					if (i == 0 && decode->flag != 0) {
						/* (i == 0)かつregなら、O16/O32あり */
						decode->prefix |= (tbl_o16o32 - 1)[c];
					}
					itp->param[2] = 0x21; /* no-w no-o16/o32 */
					itp->param[3] = 0x8c | i << 1;
					goto ope_mr_mem;
				}
				if (j < 40)
					goto err2; /* そんなレジスタは知らないので、パラメータエラー */
				if (j < 64) {
					c = (j - 40) >> 3;
					if (c == 2)
						c = 4;
					c |= 0x20; /* 20, 21, 24 */
					itp->param[2] = 0xa2; /* reg-only no-w no-o16/o32 no-mem */
					itp->param[3] = 0x0f;
					itp->param[4] = c | i << 1;
					goto ope_mr_mem;
				}
	err2:
				decode->error = 2; /* パラメータエラー */
				goto err;

			case OPE_ADD: /* ADD */
				itp->param[3] &= 0x38;
				if (decode->gparam[0] & 0xc0)
					goto err4; /* rangeがついている, data type error */
				if (decode->gparam[1] & 0xc0)
					goto err4; /* rangeがついている, data type error */
				if ((decode->gparam[1] & 0x30) == 0x20) {
					/* imm */
					if ((c = decode->gparam[0] & 0x0f) == 0x0f)
						goto err3; /* data size error */
					if (c > 4)
						goto err3; /* data size error */
					if ((decode->gparam[1] & 0x0f) != 0x0f) {
						if (c < (decode->gparam[1] & 0x0f))
							goto err3; /* data size error */
					}
					decode->prefix |= (tbl_o16o32 - 1)[c];
					if ((j = decode->gparam[0]) & 0x20) /* regでもmemでもないならエラー */
						goto err4;
					decode->flag = 0;
					if ((j & 0x10) == 0) {
						decode->flag = 1;
						if (decode->gvalue[0] >= 24) /* regだがreg8/reg16/reg32ではない */
							goto err4;
						if ((decode->gvalue[0] & 0x07) == 0) {
							/* EAX, AX, AL */
							static int mcode[] = {
								0x5c,	0x01 /* UCHAR, const */, 0x05, 0x00 /* null */,         0x00 /* 32bit */,
										0x01 /* UCHAR, const */, 0x83, 0x01 /* UCHAR, const */, 0xc0 /* 8bit */
							};
							if (c <= 2) {
								/* AL, AXなので話は簡単 */
								bp[0] = SHORT_DB1;
								bp[1] = itp->param[3] | 0x04;
								if (c == 2)
									bp[1] |= 0x01;
								bp[2] = 0x7c;
								bp += 3;
							//	c == 1 >> 9e(6);
							//	c == 2 >> 9b(3);
							//	9 - c * 3
								if (defnumexpr(ifdef, status->expression, 0x7c & 0x07, 9 - c * 3))
									goto err2; /* パラメータエラー */
								c = 0; /* mod nnn r/m なし */
								goto outbp;
							}
							/* EAX */
							mcode[2] = itp->param[3] | 0x05;
							mcode[8] = itp->param[3] | 0xc0;

							bp[0] = 0x7d;
							bp[1] = 0x7e;
							bp[2] = 0x7c; /* imm */
							bp += 3;

							if (microcode91(ifdef, status->expression, mcode, decode->gparam[1] & 0x0f))
								goto err2; /* パラメータエラー */
							c = 0; /* mod nnn r/m なし */
							goto outbp;
						}
					}
					decode->gp_mem = decode->gparam[0];
					decode->gp_reg = itp->param[3] << (9 - 3);
					if (c == 1) {
						/* 1バイトなので話は簡単 */
						bp[0] = SHORT_DB1;
						bp[1] = 0x80;
						bp += 2;
						if (defnumexpr(ifdef, status->expression, 0x7c & 0x07, 0x9e & 0x07))
							goto err2; /* パラメータエラー */
					} else {
						static int mcode[] = {
							0x54,	0x01 /* UCHAR, const */, 0x81 /* 16bit/32bit */,
									0x01 /* UCHAR, const */, 0x83 /* 8bit */
						};
						mcode[0] = 0x54; /* 16bit */
						if (c == 4)
							mcode[0] = 0x5c; /* 32bit */
						*bp++ = 0x7d;
						if (microcode90(ifdef, status->expression, mcode, decode->gparam[1] & 0x0f))
							goto err2; /* パラメータエラー */
					}
					bp[0] = 0x78;
					bp[1] = 0x79;
					bp[2] = 0x7a;
					bp[3] = 0x7c;
					bp += 4;
				//	c = 3 ^ decode->flag; /* mod nnn r/m あり */
				//	goto outbp;
					goto setc;
				}
				i = 0; /* direction-bit */
				if ((decode->gparam[1] & 0x30) == 0x10)
					i++;
				if ((j = decode->gparam[i]) & 0x20) /* regでもmemでもないならエラー */
					goto err4;
				decode->flag = 0;
				if ((j & 0x10) == 0) {
					decode->flag = 1;
					if (decode->gvalue[i] >= 24) /* regだがreg8/reg16/reg32ではない */
						goto err4;
				}
				if (decode->gparam[i ^ 1] & 0x30) /* regではないならエラー */
					goto err4;
				if (decode->gvalue[i ^ 1] >= 24) /* regだがreg8/reg16/reg32ではない */
					goto err4;
				decode->gp_reg = decode->gparam[i ^ 1];
				if ((j & 0x0f) == 0x0f /* && (itp->param[1] & 0x80) != 0 */ ) {
					/* memのデータサイズが不定 && 第二オペランドにsame0指定あり */
					j = (j & ‾0x0f) | (decode->gp_reg & 0x0f);
				}
				decode->gp_mem = decode->gparam[i] = j;
				itp->param[3] = (itp->param[3] & ‾0x02) | i << 1;
				goto ope_mr2;

			case OPE_XCHG: /* XCHG */
				/* メモリを第1オペランドへ。EAXを第2オペランドへ */
				/* そして、reg16/reg32, eAXなら特別形式 */
				/* それ以外はMR型 */
				i = 0;
				if ((decode->gparam[1] & 0x30) == 0x10)
					goto xchg_swap; /* memory */
				if (decode->gparam[0] == 0x0004 /* EAX */ || decode->gparam[0] == 0x1002 /* AX */) {
		xchg_swap:
					i++;
				}
				if ((j = decode->gparam[i]) & 0xe0) /* regでもmemでもない || rangeがついたらエラー */
					goto err4;
				decode->flag = 0;
				if ((j & 0x10) == 0) {
					decode->flag = 1;
					if (decode->gvalue[i] >= 24) /* regだがreg8/reg16/reg32ではない */
						goto err4;
				}
				if (decode->gparam[i ^ 1] & 0x1f0) /* regではない || rangeがついたらエラー || use $もエラー */
					goto err4;
				if (decode->gvalue[i ^ 1] >= 24) /* regだがreg8/reg16/reg32ではない */
					goto err4;
				decode->gp_reg = decode->gparam[i ^ 1];
				if ((j & 0x0f) == 0x0f /* && (itp->param[1] & 0x80) != 0 */) {
					/* memのデータサイズが不定 && 第二オペランドにsame0指定あり */
					j = (j & ‾0x0f) | (decode->gp_reg & 0x0f);
				}
				decode->gp_mem = decode->gparam[i] = j;
				if ((decode->gp_reg & 0x0f) != (decode->gp_mem & 0x0f))
					goto err3; /* data size error */
				if (decode->gp_reg == 0x0004 /* EAX */ ||  decode->gp_reg == 0x1002 /* AX */) {
					if (decode->flag) {
						bp[0] = SHORT_DB1; /* 0x31 */
						bp[1] = 0x90 | ((decode->gp_mem >> 9) & 0x07);
						bp += 2;
						decode->prefix |= (tbl_o16o32 - 1)[decode->gp_reg & 0x0f];
					//	c = 0; /* mod nnn r/m なし */
						goto outbp;
					}
				}
				goto ope_mr2;

			case OPE_INOUT: /* IN, OUT */
				j = 0;
				if ((c = itp->param[1]) == 0xe6 /* OUT */)
					j++; /* j = 1; */
				i = 0x10000000; /* O16(暗黙) */
				if (decode->gparam[j] == 0x0004 || decode->gparam[j] == 0x1002) {
					/* EAX か AX */
					decode->prefix |= (tbl_o16o32 - 1)[decode->gparam[j] & 0x0f];
					c++;
				} else if (decode->gparam[j] != 0x2001)
					goto err2; /* パラメータエラー */
				j = getparam0(decode->prm_p[j ^ 0x01], status);
				bp[0] = SHORT_DB1; /* 0x31 */
				if (j == 0x1402) { /* DX */
					bp[1] = c | 0x08;
					bp += 2;
				} else {
					bp[1] = c;
					bp[2] = 0x7c;
					bp += 3;
					c = j & 0x0f;
					if (c != 0xf && c != 0x01)
						goto err3; /* data size error */
					if (j & 0xc0)
						goto err2; /* パラメータエラー(range検出) */
					if ((j & 0x30) != 0x20)
						goto err2; /* パラメータエラー(reg/mem検出) */
					if (defnumexpr(ifdef, status->expression, 0x7c & 0x07, 0x98 & 0x07))
						goto err2; /* パラメータエラー */
				}
				c = 0; /* mod nnn r/m なし */
				goto outbp;

			case OPE_IMUL:
				/* mem/reg			1111011w   mod-101-r/m */
				/* reg,mem/reg		00001111   10101111   mod-reg-r/m */
				/* reg,mem/reg,imm	011010s1   mod-reg-r/m   imm */
				/* reg,imm >> reg,reg,immに読み替え */
				if (decode->flag == 0)
					goto err2; /* parameter error */
				if (decode->flag > 3)
					goto err2; /* parameter error */
				if (decode->flag == 1)
					goto ope_m;
				if (decode->gparam[0] & 0xf1) /* 偶数サイズのregのみ */
					goto err4; /* data type error */
				if (decode->gvalue[0] >= 24)
					goto err4; /* data type error */
				decode->gp_reg = decode->gparam[0];
				if (decode->flag == 2) {
					if ((decode->gparam[1] & 0x20) == 0) {
						/* mem/reg */
						bp[0] = SHORT_DB1; /* 0x31 */
						bp[1] = 0x0f;
						bp[2] = SHORT_DB1; /* 0x31 */
						bp[3] = 0xaf;
						bp[4] = 0x78; /* mod nnn r/m */
						bp[5] = 0x79; /* sib */
						bp[6] = 0x7a; /* disp */
						bp += 7;
		imul2:
						if (decode->gparam[1] & 0xe0) /* rangeがついていた || imm */
							goto err4; /* data type error */
						decode->gp_mem = decode->gparam[1];
						decode->flag = 0; /* mem */
						if ((decode->gp_mem & 0x10) == 0) {
							/* reg */
							decode->flag = 1;
							if (decode->gvalue[1] >= 24) /* regだがreg8/reg16/reg32ではない */
								goto err4;
						}
						if ((j = decode->gp_mem & decode->gp_reg & 0x0f) == 0)
							goto err3; /* data size error */
						decode->prefix |= (tbl_o16o32 - 1)[j];
					//	c = 3 ^ decode->flag; /* mod nnn r/m あり */ 				
					//	goto outbp;
						goto setc;
					}
					/* imm */
					decode->gparam[2] = decode->gparam[1];
					decode->gparam[1] = decode->gparam[0];
					decode->gvalue[1] = decode->gvalue[0];
				//	decode->flag = 3;
				}
				{
					/* reg,mem/reg,imm型 */
					static int mcode[] = {
						0x54,	0x01 /* UCHAR, const */, 0x69 /* 16bit/32bit */,
								0x01 /* UCHAR, const */, 0x6b /* 8bit */
					};
					if ((decode->gparam[2] & 0xf0) != 0x20) /* not imm */
						goto err4; /* data type error */
					mcode[0] &= 0x54;
					if (decode->gp_reg & 4)
						mcode[0] |= 0x5c;
					if (microcode90(ifdef, status->expression, mcode, decode->gparam[2] & 0x0f))
						goto err2; /* パラメータエラー */
					bp[0] = 0x7d; /* 011010s1 */
					bp[1] = 0x78; /* mod nnn r/m */
					bp[2] = 0x79; /* sib */
					bp[3] = 0x7a; /* disp */
					bp[4] = 0x7c; /* imm */
					bp += 5;
					goto imul2;
				}

			case OPE_TEST: /* mem/reg, mem/reg|imm8 */
				decode->gp_mem = decode->gparam[0];
				decode->gp_reg = decode->gparam[1];
				if ((decode->gp_reg & 0x30) == 0x10) {
					decode->gp_mem = decode->gparam[1];
					decode->gp_reg = decode->gparam[0];
				}
				if ((j = decode->gp_mem) & 0xe0) /* regでもmemでもない || rangeがついたらエラー */
					goto err4; /* data type error */
				decode->flag = 0; /* mem */
				if ((j & 0x10) == 0) {
					/* reg */
					decode->flag = 1;
					if ((j >> 9) >= 24) /* regだがreg8/reg16/reg32ではない */
						goto err4;
				}
				j &= 0x0f;
				bp[0] = SHORT_DB1; /* 0x31 */
				s = bp;
				if ((i = decode->gp_reg & 0xf0) == 0x00) {
					/* mem/reg,reg */
					j &= decode->gp_reg;
					if ((decode->gp_reg >> 9) >= 24) /* regだがreg8/reg16/reg32ではない */
						goto err4; /* data type error */
					bp[1] = 0x84;
					bp[2] = 0x78; /* mod nnn r/m */
					bp[3] = 0x79; /* sib */
					bp[4] = 0x7a; /* disp */
					bp += 5;
				} else {
					static UCHAR table[] = {
						0x9e /* dummy */ & 0x07, 0x9e /* byte */ & 0x07, 0x9b /* word */ & 0x07,
						0x9b /* dummy */ & 0x07, 0x9d /* dword */ & 0x07
					};
					if (i != 0x20) /* immではないか、rangeがついていた */
						goto err4; /* data type error */
					if ((decode->gp_mem & 0x0ef0) == 0) {
						/* EAX, AX, AL */
						bp[1] = 0xa8;
						bp += 2;
					} else {
						if (j > 4)
							goto err3; /* data size error */
						bp[1] = 0xf6;
						decode->gp_reg = 0 << 9;
						bp[2] = 0x78; /* mod nnn r/m */
						bp[3] = 0x79; /* sib */
						bp[4] = 0x7a; /* disp */
						bp += 5;
					}
					*bp++ = 0x7c;
					if (defnumexpr(ifdef, status->expression, 0x7c & 0x07, table[j]))
						goto err2; /* パラメータエラー */
				}
				if (j == 0)
					goto err3; /* data size error */
				if (j != 1)
					s[1] |= 0x01;
				decode->prefix |= (tbl_o16o32 - 1)[j];
			//	c = 3 ^ decode->flag; /* mod nnn r/m あり */ 				
			//	goto outbp;
				goto setc;

			case OPE_MOVZX:
				if (decode->gparam[0] & 0xf9) /* regでない || rangeがついたらエラー || reg8 */
					goto err4; /* data type error */
				if (decode->gvalue[0] >= 24)
					goto err4; /* data type error */
				if (decode->gparam[1] & 0xe4) /* regでもmemでもない || rangeがついたらエラー || dwordかサイズ不定 */
					goto err4; /* data type error */
				decode->flag = 0; /* mem */
				if ((decode->gparam[1] & 0x10) == 0) {
					/* reg */
					decode->flag = 1;
					if (decode->gvalue[1] >= 24) /* regだがreg8/reg16/reg32ではない */
						goto err4;
				}
				decode->gp_reg = decode->gparam[0];
				decode->gp_mem = decode->gparam[1];
				decode->prefix |= (tbl_o16o32 - 1)[decode->gparam[0] & 0x0f];
				bp[0] = SHORT_DB1; /* 0x31 */
				bp[1] = 0x0f;
				bp[2] = SHORT_DB1; /* 0x31 */
				bp[3] = itp->param[1] ^ (decode->gparam[1] & 0x01);
				bp[4] = 0x78; /* mod nnn r/m */
				bp[5] = 0x79; /* sib */
				bp[6] = 0x7a; /* disp */
				bp += 7;
				goto setc;

			case OPE_SHLD: /* mem/reg, reg, imm8|CL */
				if ((j = decode->gparam[0]) & 0xe0) /* regでもmemでもない || rangeがついたらエラー */
					goto err4; /* data type error */
				decode->gp_mem = j;
				decode->gp_reg = decode->gparam[1];
				if ((decode->gparam[1] & 0xf0) != 0x00) /* regではない || rangeがついた */
					goto err4; /* data type error */
				decode->flag = 0; /* mem */
				if ((j & 0x10) == 0) {
					/* reg */
					decode->flag = 1;
					if (decode->gvalue[0] >= 24) /* regだがreg8/reg16/reg32ではない */
						goto err4;
				}
				/* データサイズを確定 */
				i = decode->gparam[1] & 0x0f;
				if (i <= 1)
					goto err3; /* data size error */
				if (i > 4)
					goto err3; /* data size error */
				if ((decode->gparam[0] & i) == 0)
					goto err3; /* data size error */
				decode->prefix |= (tbl_o16o32 - 1)[i];
				bp[0] = SHORT_DB1; /* 0x31 */
				bp[1] = 0x0f;
				bp[2] = SHORT_DB1; /* 0x31 */
				bp[3] = itp->param[1];
				bp[4] = 0x78; /* mod nnn r/m */
				bp[5] = 0x79; /* sib */
				bp[6] = 0x7a; /* disp */
				if (decode->gparam[2] == 0x2201) { /* CL */
					bp[3] |= 0x01;
					bp += 7;
				} else {
					if ((decode->gparam[2] & 0xf0) != 0x20) /* immではない || rangeがついていた */
						goto err4; /* data type error */
					bp[7] = 0x7c; /* imm8 */
					if (defnumexpr(ifdef, status->expression, 0x7c & 0x07, 0x98 & 0x07 /* UCHAR */))
						goto err2; /* パラメータエラー */
					bp += 8;
				}
			//	c = 3 ^ decode->flag; /* mod nnn r/m あり */ 				
			//	goto outbp;
				goto setc;

			case OPE_LOOP:
				if (itp->param[2]) {
					if (decode->flag != 1)
						goto err2; /* parameter error */
					decode->prefix |= (tbl_o16o32 - 1)[itp->param[2] >> 3];
				} else {
					if (decode->flag == 2) {
						i = decode->gparam[1];
						if (i == 0x0204 /* ECX */ || i == 0x1202 /* CX */)
							decode->prefix |= (tbl_o16o32 - 1)[i & 0x07];
						else
							goto err4; /* data type error */
					} else if (decode->flag != 1)
						goto err2;
				}
				if ((decode->gparam[0] & 0x30) != 0x20) /* immではない */
					goto err4;
				c = decode->gparam[0] & 0xc0;
				if (c == 0x40) /* NEAR */
					goto err4;
				if (c == 0x80) /* FAR */
					goto err4;
				c = decode->gparam[0] & 0x0f;
				if (c