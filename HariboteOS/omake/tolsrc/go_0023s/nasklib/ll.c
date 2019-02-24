/* "LL.c":Lilte-Linker */
/* Copyright(C) 2002 H.Kawai */

/* 方針：遅くてもいいから、とりあえず動くこと！ */
/* アルゴリズムに凝るのは、動くようになってから */

#define	DEBUG			0

#include "../include/stdlib.h"

#if (DEBUG)
	#include "../include/stdio.h"
#endif

#define	INVALID_DELTA		0x40000000

extern int nask_maxlabels;

#define	MAXSIGMAS				  4

#define	UCHAR			unsigned char

struct STR_SIGMA {
	int scale;
	unsigned int subsect, terms;
};

struct STR_VALUE {
	int min;
	unsigned int delta, flags;
	int scale[2];
	unsigned int label[2];
	struct STR_SIGMA sigma[MAXSIGMAS];
};

#define	VFLG_ERROR			0x01	/* エラー */
#define	VFLG_SLFREF			0x02	/* 自己参照エラー */
#define	VFLG_UNDEF			0x04	/* 未定義エラー */
#define	VFLG_EXTERN			0x10	/* 外部ラベル */
#define	VFLG_CALC			0x20	/* 計算中 */
#define	VFLG_ENABLE			0x40	/* STR_LABELで有効なことを示す */

struct STR_LABEL {
	struct STR_VALUE value;
	UCHAR *define; /* これがNULLだと、extlabel */
};

struct STR_SUBSECTION {
	unsigned int min, delta, unsolved; /* unsolved == 0 なら最適化の必要なし */
	UCHAR *sect0, *sect1;
};

static struct STR_LABEL *label0;
static struct STR_SUBSECTION *subsect0;

unsigned int solve_subsection(struct STR_SUBSECTION *subsect, char force);
UCHAR *skip_mc30(UCHAR *s, UCHAR *bytes, char flag);
void calcsigma(struct STR_VALUE *value);
void addsigma(struct STR_VALUE *value, struct STR_SIGMA sigma);
UCHAR *LL_skip_expr(UCHAR *p);
UCHAR *LL_skip_mc30(UCHAR *s, UCHAR *bytes, char flag);
UCHAR *LL_skipcode(UCHAR *p);

void init_value(struct STR_VALUE *value)
{
	int i;
	value->min = value->delta = value->scale[0] = value->scale[1] = 0;
	value->flags = 0;
	value->label[0] = value->label[1] = -1;
	for (i = 0; i < MAXSIGMAS; i++) {
		value->sigma[i].scale = 0;
		value->sigma[i].subsect = 0;
		value->sigma[i].terms = 0;
	}
	return;
}

unsigned int get_id(int len, UCHAR **ps, int i)
/* len = 0〜3 */
{
	union UINT_UCHAR {
		unsigned int i;
		UCHAR c[4];
	} u;
	UCHAR *s = *ps;
	u.i = i;
	i = 0;
	do {
		u.c[i++] = *s++;
	} while (--len >= 0);
	*ps = s;
	return u.i;
}

void calc_value0(struct STR_VALUE *value, UCHAR **pexpr);

void calc_value(struct STR_VALUE *value, UCHAR **pexpr)
{
	calc_value0(value, pexpr);
	if (value->sigma[0].scale)
		calcsigma(value);
	return;
}

void enable_label(struct STR_LABEL *label)
{
	struct STR_VALUE value;
	UCHAR *t;

//	if (label->value.flags & VFLG_CALC) {
//		label->value.flags |= VFLG_SLFREF;
//		return;
//	}
//	init_value(&value);
//	value.label[0] = label - label0; /* for EXTERN */
//	value.scale[0] = 1;
//	value.flags |= label->value.flags & VFLG_EXTERN;
	if ((t = label->define) != NULL) {
		label->value.flags |= VFLG_CALC;
		calc_value0(&value, &t);
		if (value.flags & VFLG_SLFREF) {
			init_value(&value);
			value.flags |= VFLG_SLFREF;
		}
	} else /* if ((label->value.flags & VFLG_EXTERN) == 0) */ {
		/* EXTERNの時はすぐにENABLEになるので、ここに来るはずない */
		init_value(&value);
		value.flags |= VFLG_UNDEF;
	}
	label->value = value;
	label->value.flags |= VFLG_ENABLE;
	return;
}

void calc_value0(struct STR_VALUE *value, UCHAR **pexpr)
{
	UCHAR *expr = *pexpr, c, *t;
	int i, j, k;
	struct STR_VALUE tmp, tmp2;
	struct STR_SUBSECTION *subsect;
	c = *expr++;
	init_value(value);
	if (c <= 6) {
		/* 定数 */
		i = 0;
		if (c & 0x01)
			i--;
		value->min = get_id(c >> 1, &expr, i);
		goto fin;
	}
	if (c == 0x07) {
		/* 定数1 */
		value->min = 1;
		goto fin;
	}
	if (c <= 0x0b) {
		/* ラベル引用 */
		struct STR_LABEL *label;
		label = &label0[i = get_id(c - 8, &expr, 0)];
		if ((label->value.flags & VFLG_ENABLE) == 0) {
			if (label->value.flags & VFLG_CALC) {
				value->flags |= VFLG_SLFREF;
				goto fin;
			}
			enable_label(label);
		}
		*value = label->value;
		goto fin;
	}
	#if (DEBUG)
		if (c < 0x10)
			goto dberr;
	#endif
	if (c < 0x20) {
		/* 演算子 */
		calc_value0(value, &expr);
		if (c == 0x10)
			goto fin; /* 単項演算子 + */
		if (c == 0x11) {
			/* 単項演算子 - */
			value->min = - (int) (value->min + value->delta);
	minus2:
			value->scale[0] *= -1;
 			value->scale[1] *= -1;
			for (i = 0; i < MAXSIGMAS; i++)
				value->sigma[i].scale *= -1;
			goto fin;
		}
		if (c == 0x12) {
			/* 単項演算子 ‾ */
			/* ‾a == - a - 1 */
			value->min = - (int) (value->min + value->delta) - 1;
			goto minus2;
		}
		calc_value0(&tmp, &expr);
		value->flags |= tmp.flags;
		if (c == 0x13) {
			/* 二項演算子 + */
add2:
			for (i = 0; i < 2; i++) {
				if (value->scale[i] == 0)
					break;
			}
			for (j = 0; j < 2; j++) {
				if (tmp.scale[j] == 0)
					break;
				for (k = 0; k < i; k++) {
					if (value->label[k] == tmp.label[j]) {
						/* 同類項検出 */
						if ((value->scale[k] += tmp.scale[j]) == 0) {
							/* 項の消滅 */
							i--;
							value->label[k] = 0xffffffff;
							if (k)
								goto next_product;
							value->scale[0] = value->scale[1];
							value->label[0] = value->label[1];
							value->scale[1] = 0;
							value->label[1] = 0xffffffff;
						}
						goto next_product;
					}
				}
				/* 同類項が無かったので、項の追加 */
				if (i >= 2) {
					value->flags |= VFLG_ERROR;
					goto fin;
				}
				value->scale[i] = tmp.scale[j];
				value->label[i] = tmp.label[j];
				i++;
next_product:
				;
			}
			for (j = 0; j < MAXSIGMAS; j++) {
				if (tmp.sigma[j].scale == 0)
					break;
				addsigma(value, tmp.sigma[j]);
			}
			value->min += tmp.min;
			value->delta += tmp.delta;
			goto fin;
		}
		if (c == 0x14) {
			/* 二項演算子 - */
			tmp.min = - (int) (tmp.min + tmp.delta);
			tmp.scale[0] *= -1;
 			tmp.scale[1] *= -1;
			for (i = 0; i < MAXSIGMAS; i++)
				tmp.sigma[i].scale *= -1;
			goto add2;
		}
		if (c == 0x15) { /* "*" */
			if (value->scale[0])
				goto no_exchange;
			if (tmp.scale[0])
				goto exchange;
		}
		if (c == 0x15 || 0x1d <= c && c <= 0x1f) { /* "*", "&", "|", "^" */
			if (value->delta)
				goto no_exchange;
			if (tmp.delta)
				goto exchange;
			if (value->sigma[0].scale)
				goto no_exchange;
			if (tmp.sigma[0].scale) {
		exchange:
				/* 項の交換 */
				/* 不定値を1項目に持ってくるために交換 */
				tmp.flags = value->flags;
				tmp2 = *value;
				*value = tmp;
				tmp = tmp2;
			}
		}
	no_exchange:
		calcsigma(&tmp);
		if (tmp.delta) {
			/* 不定値を使うために、値がまだ決定できない */
			value->flags |= VFLG_ERROR;
			goto fin;
		}
		if (c == 0x15) {
			/* 二項演算子 * (signed) */
			if (tmp.scale[0]) {
	error:
				/* 両方ともlabelを含んでいればエラー */
				value->flags |= VFLG_ERROR;
				goto fin;
			}
multiply2:
			if (tmp.min == 0) {
				init_value(value);
				goto fin;
			}
			value->scale[0] *= tmp.min;
			value->scale[1] *= tmp.min;
			for (i = 0; i < MAXSIGMAS; i++)
				value->sigma[i].scale *= tmp.min;
			i = (signed int) value->min * (signed int) tmp.min;
			j = (signed int) (value->min + value->delta) * (signed int) tmp.min;
minmax:
			value->min = i;
			value->delta = j - i;
			if (i > j) {
				value->min = j;
				value->delta = i - j;
			}
			goto fin;
		}
		/* これ以降はtmp側のlabelはキャンセルされる */
		if (c == 0x16) {
			/* 二項演算子 << */
			tmp.min = 1 << tmp.min;
			goto multiply2;
		}
		if (c == 0x17) {
			/* 二項演算子 / (unsigned) */
divu2:
			if (tmp.min == 0)
				goto error;
divu_ij:
			i = (unsigned int) value->min / (unsigned int) tmp.min;
			j = (unsigned int) (value->min + value->delta) / (unsigned int) tmp.min;
			for (k = 0; k < MAXSIGMAS; k++) {
				if (value->sigma[k].scale % tmp.min) {
					calcsigma(value);
					goto divu_ij;
				}
			}
divall:
			if (value->scale[0] % tmp.min)
				goto error;
			if (value->scale[1] % tmp.min)
				goto error;
			value->scale[0] /= tmp.min;
			value->scale[1] /= tmp.min;
			for (k = 0; k < MAXSIGMAS; k++)
				value->sigma[k].scale /= tmp.min;
			goto minmax;
		}
		if (c == 0x19) {
			/* 二項演算子 / (signed) */
divs2:
			if (tmp.min == 0)
				goto error;
divs_ij:
			i = (signed int) value->min / (signed int) tmp.min;
			j = (signed int) (value->min + value->delta) / (signed int) tmp.min;
			for (k = 0; k < MAXSIGMAS; k++) {
				if (value->sigma[k].scale % tmp.min) {
					calcsigma(value);
					goto divs_ij;
				}
			}
			goto divall;
		}
		if (c == 0x1b) {
			/* 二項演算子 >> (unsigned) */
			tmp.min = 1 << tmp.min;
			goto divu2;
		}
		if (c == 0x1c) {
			/* 二項演算子 >> (signed) */
			tmp.min = 1 << tmp.min;
			goto divs2;
		}
		value->scale[0] = value->scale[1] = 0; /* 自動キャンセル */
		calcsigma(value);
		if (c == 0x18) {
			/* 二項演算子 % (unsigned) */
			if (tmp.min == 0)
				goto error;
			value->min = (unsigned int) value->min % (unsigned int) tmp.min;
			if (value->delta) {
				value->min = 0;
				value->delta = tmp.min - 1;
			}
			goto fin;
		}
		if (c == 0x1a) {
			/* 二項演算子 % (signed) */
			if (tmp.min == 0)
				goto error;
			value->min = (signed int) value->min % (signed int) tmp.min;
			if (value->delta) {
				value->delta = tmp.min - 1;
				if (value->min >= 0) {
					value->min = 0;
				} else if ((signed int) (value->min + value->delta) <= 0) {
					value->min = 1 - tmp.min;
				} else {
					value->min = 1 - tmp.min;
					value->delta *= 2;
				}
			}
			goto fin;
		}
		if (c == 0x1d) {
			/* 二項演算子 & (unsigned) */
			if (tmp.min == -1)
				goto fin; /* 素通り */
			if (value->delta == 0) {
				value->min &= tmp.min;
				goto fin;
			}
			if (tmp.min & 0x80000000)
				goto error; /* 手が付けられない */
			/* 本当はもっと細かくできるが、面倒なのでやってない */
			value->min = 0;
			value->delta = tmp.min;
			goto fin;
		}
		if (c == 0x1e) {
			/* 二項演算子 | (unsigned) */
			if (tmp.min == 0)
				goto fin; /* 素通り */
			if (value->delta == 0) {
				value->min |= tmp.min;
				goto fin;
			}
			if (tmp.min & 0x80000000)
				goto error; /* 手が付けられない */
			value->delta += tmp.min;
			if ((value->delta & 0x80000000) == 0)
				goto fin;
			value->delta = 0;
			goto error;
		}
	//	if (c == 0x1f) {
			/* 二項演算子 ^ (unsigned) */
			if (tmp.min == 0)
				goto fin; /* 素通り */
			if (value->delta == 0) {
				value->min ^= tmp.min;
				goto fin;
			}
			if (tmp.min & 0x80000000)
				goto error; /* 手が付けられない */
			if ((signed int) value->min > (signed int) 0 && value->min >= tmp.min) {
				value->min -= tmp.min;
				value->delta += tmp.min;
				goto fin;
			}
			goto error; /* 手が付けられない */
	//	}
	}
	#if (DEBUG)
		if (c < 0x80)
			goto dberr;
	#endif
#if 0
	if (c == 0x80) {
		/* subsect size sum */
		/* sumは、deltaが2GBならエラーにしてしまう(面倒なので) */
		calc_value(&tmp, &expr);
		i = tmp.min;
		calc_value(&tmp, &expr);
		subsect = subsect0 + tmp.min;
		do {
			value->min += subsect->min;
			value->delta += subsect->delta;
			if (subsect->delta >= INVALID_DELTA)
				value->flags |= VFLG_ERROR;
		} while (--i);
		goto fin;
	}
#endif
	#if (DEBUG)
dberr:
		fprintf(stderr, "calc_value:%02x¥n", c);
		GOL_sysabort(GO_TERM_BUGTRAP);
	#endif

fin:
	*pexpr = expr;
	return;
}

static UCHAR *skip_expr(UCHAR *expr)
{
	return LL_skip_expr(expr);
}





/* ラベルの定義方法:
	一般式
	80 variable-sum, 0000bbaa(aa:項数-1, bb:最初の番号), 
	84〜87 sum, i - 1, expr, expr, ...

  ・80〜8f:LLが内部処理用に使う
	80〜83:variable参照(1〜4バイト)
	88〜8f:sum(variable), (1〜4, 1〜4) : 最初は項数-1, 次は最初の番号
		{ "|>", 12, 18 }, { "&>", 12, 17 },
		{ "<<", 12, 16 }, { ">>", 12, 17 },
		{ "//", 14,  9 }, { "%%", 14, 10 },
		{ "+",  13,  4 }, { "-",  13,  5 },
		{ "*",  14,  6 }, { "/",  14,  7 },
		{ "%",  14,  8 }, { "^",   7, 14 },
		{ "&",   8, 12 }, { "|",   6, 13 },
		{ "",    0,  0 }

	s+
	s-
	s‾

	+, -, *, <<, /u, %u, /s, %s
	>>u, >>s, &, |, ^

*/






#if 0

	< 文法 >

最初はヘッダ。
・ヘッダサイズ(DW) = 12
・バージョンコード(DW)
・ラベル総数(DW)


  ・38:式の値をDBにして設置
  ・39:式の値をDWにして設置
  ・3a:式の値を3バイトで設置
  ・3b:式の値をDDにして設置
  以下、・3fまである。
  ・40〜47:ブロックコマンド。if文などの後続文をブロック化する(2〜9)。
  ・48:バイトブロック（ブロック長がバイトで後続）。
  ・49:ワードブロック。
  ・4a:バイトブロック。
  ・4b:ダブルワードブロック。
  ・4c:排他的if開始。
  ・4d:選択的if開始。
  ・4e:選択的バウンダリーif開始。変数設定の直後、バウンダリー値が続く。
  ・4f:endif。
  排他的ifは、endifが来るまでいくつも並べられる。endifが来るまで、
  全てelse-ifとして扱われる。最後にelseを作りたければ、条件を定数1にせよ。
  ・ターミネーターはラベル定義で0xffffffff。

  ・58:ORG

  ・60:アライン。バイトの埋め方は個別に設定する。・・・これは排他的ifでも記述できる。

  ・70〜77:可変長バイト宣言(文法上では40〜4bが後続することを許すが、サポートしていない。許されるのは30〜3f)
  ・78〜7f:可変長バイト参照

  ・80〜8f:LLが内部処理用に使う
	80〜83:variable参照(1〜4バイト)
	88〜8f:sum(variable), (1〜4, 1〜4) : 最初は項数-1, 次は最初の番号


  if文中では、可変長バイト宣言しかできない。

	ORGについて。本来引数は式であってよいが、このバージョンでは定数式を仮定している。




#endif

/* ibuf, obufの基礎構造 */
/* シグネチャー8バイト, 総長4バイト, リザーブ4バイト */
/* メインデータレングス4B, メインデータスタート4B */

/* ↑こういうややこしいことはやらない */
/* スキップコマンドを作って対処する */

static UCHAR table98typlen[] = { 0x38, 0x38, 0x39, 0x39, 0x3b, 0x3b, 0x38 };
static UCHAR table98range [] = { 0x00, 0x02, 0x00, 0x03, 0x00, 0x03, 0x03 };

struct STR_LL_VB {
	UCHAR *expr, typlen, range;
};

UCHAR *LL_define_VB(struct STR_LL_VB *virtualbyte, UCHAR *s)
{
	UCHAR c;
	virtualbyte->typlen = c = *s++;
	#if (DEBUG)
		if (c < 0x30)
			goto dberr0;
	#endif
	if (c < 0x38) {
		virtualbyte->expr = s;
		s += c & 0x07;
		goto fin;
	}
	if (c < 0x3c)
		virtualbyte->range = *s++;
	else {
		#if (DEBUG)
			if (c < 0x98)
				goto dberr0;
			if (c >= 0x9f)
				goto dberr0;
		#endif
		virtualbyte->typlen = table98typlen[c & 0x07];
		virtualbyte->range  = table98range [c & 0x07];
	}
	virtualbyte->expr = s;
	s = skip_expr(s);
fin:
	return s;	

	#if (DEBUG)
dberr0:
		fprintf(stderr, "LL_define_VB:%02x¥n", c);
		GOL_sysabort(GO_TERM_BUGTRAP);
		return NULL;
	#endif
}

UCHAR *lccbug_LL_mc90_func(UCHAR *src0, struct STR_LL_VB *vbb, struct STR_LL_VB *vbc, char len, char c)
{
	do {
		if (c != 0) {
			src0 = skip_mc30(src0, NULL, 1);
			if (vbc)
				src0 = skip_mc30(src0, NULL, 1);
		} else {
			src0 = LL_define_VB(vbb, src0);
			if (vbc)
				src0 = LL_define_VB(vbc, src0);
		}
		c--;
	} while (--len);
	return src0;
}

UCHAR *LL(UCHAR *src0, UCHAR *src1, UCHAR *dest0, UCHAR *dest1)
/* なおsrcに書き込みをするので要注意 */
/* 新dest1を返す */
{
	struct STR_SUBSECTION *subsect1, *subsect;
	unsigned int l0, min, max, unsolved, unsolved0;
	struct STR_LABEL *label;
	struct STR_VALUE *value, *labelvalue;
	struct STR_LL_VB virtualbyte[8], *vba, *vbb, *vbc;
	signed int times_count;
	UCHAR *s, *times_src0, *times_dest0, c, len, range;

	label0 = malloc(nask_maxlabels * sizeof (struct STR_LABEL));
	subsect0 = malloc(nask_maxlabels /* == MAXSUBSETCS */ * sizeof (struct STR_SUBSECTION));
	value = malloc(sizeof (struct STR_VALUE));
	labelvalue = malloc(sizeof (struct STR_VALUE));

	/* ラベル式を登録する */
//	l0 = 0; /* 現在のアドレスはl0からlc個の和 */
//	lc = 0;
//	min = max = 0; /* 次に生成するのは、variable0[l0 + lc] */

	label = label0;
	for (unsolved = nask_maxlabels; unsolved > 0; label++, unsolved--) {
		label->value.flags = 0; /* enableを消すため */
		label->define = NULL;
	}

	/* 切り分ける */
	/* ラベルを検出する */
	subsect = subsect0;
	subsect->sect0 = src0;
	init_value(labelvalue); /* ラベル保持 */
	while (src0 < src1) {
		c = *src0;
		if (c == 0x2d) {
			/* EQU */
			src0++;
			calc_value(value, &src0);
			label = &label0[value->min];
			label->define = src0;
			src0 = skip_expr(src0);
			continue;
		}
		if (c == 0x0e) {
			/* ラベル定義 */
			struct STR_SIGMA sigma;
			sigma.scale = 1;
			sigma.subsect = subsect - subsect0;
			sigma.terms = 1;
			subsect->sect1 = src0;
			subsect++;
			subsect->sect0 = src0;
			src0++;
			calc_value(value, &src0);
			label = &label0[value->min];
			label->define = src0;
			addsigma(labelvalue, sigma);
			label->value = *labelvalue;
			label->value.flags |= VFLG_ENABLE;
			continue;
		}
		if (c == 0x2c) {
			/* externラベル宣言 */
			src0++;
			calc_value(value, &src0);
			label = &label0[value->min];
			init_value(&label->value);
		//	label->define = NULL;
			label->value.flags |= VFLG_EXTERN | VFLG_ENABLE;
			label->value.label[0] = value->min;
			label->value.scale[0] = 1;
			continue;
		}
		if (c == 0x58) {
			/* ORG */
			subsect->sect1 = src0;
			subsect++;
			subsect->sect0 = src0;
			src0++;
			calc_value(labelvalue, &src0);
			continue;
		}
		src0 = LL_skipcode(src0);
	}
	subsect->sect1 = src1;
	subsect1 = subsect + 1;

	for (subsect = subsect0; subsect < subsect1; subsect++) {
		subsect->min = 0;
		subsect->delta = INVALID_DELTA;
		subsect->unsolved = 1;
	}
	len = 0;
	c = 0;
	unsolved0 = 0x7fffffff;
	do {
		for (subsect = subsect0; subsect < subsect1; subsect++) {
			if (subsect->unsolved)
				solve_subsection(subsect, len);
		}
		unsolved = 0;
		for (subsect = subsect1 - 1; subsect >= subsect0; subsect--) {
			if (subsect->unsolved)
				unsolved += solve_subsection(subsect, len);
		}
		c++;
		if (unsolved0 > unsolved) {
			c = 0;
			unsolved0 = unsolved;
		}
		if (c >= 3) {
			len++;
			c = 0;
		}
	} while (unsolved);

	for (subsect = subsect0; subsect < subsect1; subsect++) {
		times_count = 0;
		for (src0 = subsect->sect0; src0 < subsect->sect1; ) {
			c = *src0++;
			if (dest0 + 64 >= dest1) {
	error:
				return NULL; /* buffer overrun */
			}
			if (c == 0x2d) {
				/* EQU */
				calc_value(value, &src0);
				label = &label0[value->min];
				if ((label->value.flags & VFLG_ENABLE) == 0)
					enable_label(label);
				*value = label->value;
				calcsigma(value);
				c = 0x0c;
				if (value->scale[0])
					c++;
				*dest0++ = c;
				len = 4;
				src0 = skip_expr(src0);
				goto putvalue0;
			}
			if (c == 0x0e) {
				src0 = skip_expr(src0);
				continue;
			}
			if (c == 0x0f) {
				dest0[0] = 0xf6; /* REM_8B */
				dest0[1] = *src0++;
				c = *src0++;
				label = &label0[get_id(c - 8, &src0, 0)];
				if ((label->value.flags & VFLG_ENABLE) == 0)
					enable_label(label);
				*value = label->value;
				calcsigma(value);
				dest0[2] = 0; /* base extern label (none) */
				dest0[3] = 0;
				dest0[4] = value->min & 0xff;
				dest0[5] = (value->min >>  8) & 0xff;
				dest0[6] = (value->min >> 16) & 0xff;
				dest0[7] = (value->min >> 24) & 0xff;
				dest0 += 8;
				if (value->scale[0]) {
					dest0[2 - 8] = value->label[0] & 0xff;
					dest0[3 - 8] = (value->label[0] >> 8) & 0xff;
				//	if (value->scale[0] != 1 || value->scale[1] != 0) {
				//		/* エラーを出力 */
				//	}
				}
				continue;
			}
			if (c == 0x2c) {
				dest0[0] = 0x2c;
				dest0[1] = src0[0];
				dest0[2] = src0[1];
				dest0[3] = src0[2];
				src0 += 3;
				dest0 += 4;
				continue;
			}
			#if (DEBUG)
				if (c < 0x30)
					goto dberr0;
			#endif
			if (c < 0x37) {
				/* 30〜37:ショートDB(定数、0〜7バイト) */
				len = c & 0x07;
	copy:
				*dest0++ = c;
				if (len) {
					times_count -= len;
					do {
						*dest0++ = *src0++;
					} while (--len);
	check_times_count:
					if (times_count == 0) {
	times_expand:
						s = skip_expr(times_src0 + 4); /* len */
						calc_value(value, &s); /* 繰り返し回数 */
						c = 0;
						value->min = times_src0[0] | times_src0[1] << 8
							| times_src0[2] << 16 | times_src0[3] << 24;
						/* ↑forceなどで食い違いが生じないようにするため */
						if (value->delta) {
							/* 不定値エラー */
							c = 1;
							*dest0++ = 0xe9;
						}
						if (value->flags & VFLG_UNDEF) {
							/* 未定義エラー */
							c = 1;
							*dest0++ = 0xec;
						}
						if (value->flags & VFLG_SLFREF) {
							/* 自己参照エラー */
							c = 1;
							*dest0++ = 0xea;
						}
						if (value->flags & VFLG_ERROR) {
							/* ラベル値一般エラー */
							c = 1;
							*dest0++ = 0xeb;
						}
						if (c != 0 || (unsigned int) value->min >= 0xfffffff0) {
							*dest0++ = 0xe6; /* TIMES error */
							dest0 = times_dest0; /* 繰り返し回数0とみなす */
							continue;
						}
						value->delta = dest0 - times_dest0;
						if (dest0 + value->delta * value->min + 64 >= dest1)
							goto error; /* 1回分余計に比較しているが、ま、いいか */
						for (dest0 = times_src0 = times_dest0; value->min > 0; value->min--) {
							s = times_src0;
							for (times_count = value->delta; times_count > 0; times_count--)
								*dest0++ = *s++;
						}
					}
	times_count0:
					times_count = 0;
				}
				continue;
			}
			if (c < 0x3c) {
				/* 式の値を置く */
				len = c - 0x37;
				range = *src0++;
				calc_value(value, &src0);
	putvalue:
				if (value->scale[0] && len == 4 && value->delta == 0
					&& (value->flags & (VFLG_UNDEF | VFLG_SLFREF | VFLG_ERROR)) == 0) {
					/* 要リロケーション(1か-1) */
					if (value->delta == 0 && value->flags)
					if (value->scale[0] == -1) {
						value->label[0] = value->label[1];
						value->scale[1] = -1;
					}
					/* ここで想定すべきケース */
					/* scale[0] == 1, scale[1] ==  0 : 絶対補正 */
					/* scale[0] == 1, scale[1] == -1 : 相対補正 */
					/* 0x2e : 絶対補正要求, 0x2f : 相対補正要求 */
					dest0[0] = 0x2e - value->scale[1];
					dest0[1] = 0x09;
					dest0[2] = value->label[0] & 0xff;
					dest0[3] = (value->label[0] >> 8) & 0xff;
					dest0 += 4;
				}
				*dest0++ = 0x30 | len; /* 31〜34 */
				/* レンジチェックなどをやる */
				/* scaleチェックもやって、必要ならリンクに必要な情報をつける */
	putvalue0:
				/* lenバイトを置く */
				dest0[0] = value->min         & 0xff;
				dest0[1] = (value->min >>  8) & 0xff;
				dest0[2] = (value->min >> 16) & 0xff;
				dest0[3] = (value->min >> 24) & 0xff;
				dest0 += len;
				if (value->delta) {
					/* 不定値エラー */
					*dest0++ = 0xe9;
				}
				if (value->flags & VFLG_UNDEF) {
					/* 未定義エラー */
					*dest0++ = 0xec;
				}
				if (value->flags & VFLG_SLFREF) {
					/* 自己参照エラー */
					*dest0++ = 0xea;
				}
				if (value->flags & VFLG_ERROR) {
					/* ラベル値一般エラー */
					*dest0++ = 0xeb;
				}
				if ((times_count -= len) == 0)
					goto times_expand;
				goto times_count0;
			//	times_count = 0;
			//	continue;
			}
			if (c == 0x58) {
				/* ORG */
				calc_value(value, &src0);
				*dest0++ = 0x5a;
				len = 4;
				goto putvalue0;
			}
			if (c == 0x59) {
				/* TIMES */
				times_dest0 = dest0;
				times_src0 = src0;
				src0 += 4;
				calc_value(value, &src0); /* len */
				/* lenの部分にエラーはないということになっている(手抜き) */
				times_count = value->min;
				src0 = skip_expr(src0);
				continue;
			}
			if (c == 0x68) {
				/* endian */
				src0++;
				continue;
			}
			#if (DEBUG)
				if (c < 0x70)
					goto dberr0;
			#endif
			if (c < 0x78) {
				/* 0x70〜0x77:virtual byte定義 */
				src0 = LL_define_VB(&virtualbyte[c & 0x07], src0);
				continue;
			}
			if (c < 0x80) {
				/* 0x78〜0x7f:virtual byte展開 */
				vba = &virtualbyte[c & 0x07];
				s = vba->expr;
				if (vba->typlen < 0x38) {
					len = vba->typlen & 0x07;
					*dest0++ = vba->typlen;
					if (len) {
						times_count -= len;
						do {
							*dest0++ = *s++;
						} while (--len);
						goto check_times_count;
					}
					continue;
				}
				len = vba->typlen - 0x37;
				range = vba->range;
				calc_value(value, &s);
				goto putvalue;
			}
			#if (DEBUG)
				if (c < 0x90)
					goto dberr0;
			#endif
			if (c <= 0x91) {
				/* microcode90 */
				/* microcode91 */
				len = src0[2]; /* z-bbb-d-aaa */
				vba = &virtualbyte[len & 0x07];
				vbb = &virtualbyte[(len >> 4) & 0x07];
				vbc = vbb + 1;
				if (c == 0x90)
					vbc = NULL;
				vba->typlen = 0x37 + (src0[1] & 0x0f);
				vba->range = 0x03; /* non-overflow */
				if (vba->typlen == 1)
					vba->range = 0x02; /* normal-signed */
				c = src0[0] & 0x0f;
				len = ((len /* z-bbb-d-aaa */ >> 7) & 0x01) + 2;
				vba->expr = src0 + 3;
				src0 = skip_expr(vba->expr);

				src0 = lccbug_LL_mc90_func(src0, vbb, vbc, len, c);
#if 0
				do {
					if (c != 0) {
						src0 = skip_mc30(src0, NULL, 1);
						if (vbc)
							src0 = skip_mc30(src0, NULL, 1);
					} else {
						src0 = LL_define_VB(vbb, src0);
						if (vbc)
							src0 = LL_define_VB(vbc, src0);
					}
					c--;
				} while (--len);
#endif
				continue;
			}
			#if (DEBUG)
				if (c < 0x94)
					goto dberr0;
			#endif
			if (c <= 0x95) {
				/* microcode94 */
				/* microcode95 */
				len = src0[2]; /* z-bbb-d-aaa */
				vba = &virtualbyte[len & 0x07];
				vbb = &virtualbyte[(len >> 4) & 0x07];
				vbc = vbb + 1;
				if (c == 0x94)
					vbc = NULL;
				vba->expr = src0 + 3;
				c = src0[0] & 0x0f;
				src0 = skip_expr(vba->expr);
				len = *src0++;
				goto outmc94_skip;
				do {
					c--;
					src0 = skip_expr(src0);
			outmc94_skip:
					if (c == 0) {
						range = *src0++;
						if (range < 0x3c) {
							vba->typlen = range;
							vba->range = *src0++;
						} else {
							vba->typlen = table98typlen[range & 0x07];
							vba->range = table98range[range & 0x07];
						}
						src0 = LL_define_VB(vbb, src0);
					} else {
						src0 = skip_mc30(src0, NULL, 0);
						src0 = skip_mc30(src0, NULL, 1);
					}
					if (vbc) {
						if (c == 0)
							src0 = LL_define_VB(vbc, src0);
						else
							src0 = skip_mc30(src0, NULL, 1);
					}
				} while (--len);
				continue;
			}
			#if (DEBUG)
				if (c < 0x98)
					goto dberr0;
			#endif
			if (c < 0x9f) {
				len = table98typlen[c & 0x07] & 0x07;
				range = table98range[c & 0x07];
				calc_value(value, &src0);
				goto putvalue;
			}
			#if (DEBUG)
				if (c < 0xe0)
					goto dberr0;
			#endif
			if (c < 0xf0) {
				/* 1バイトリマーク */
				*dest0++ = c;
				continue;
			}
			if (c < 0xf8) {
				/* nバイトリマーク */
				len = c - 0xef;
				goto copy;
			}
			#if (DEBUG)
	dberr0:
				fprintf(stderr, "LL:%02x¥n", c);
				GOL_sysabort(GO_TERM_BUGTRAP);
			#endif
		}
	}

	free(label0);
	free(subsect0);
	free(value);
	free(labelvalue);

	return dest0;
}


UCHAR *skip_mc30(UCHAR *s, UCHAR *bytes, char flag)
{
	return LL_skip_mc30(s, bytes, flag);
}

unsigned int solve_subsection(struct STR_SUBSECTION *subsect, char force)
/* unsolved数を返す */
/* subsect->unsolved == 0 なら呼ばないこと */
{
	struct {
		signed char min, max;
	} vb[8], *vba, *vbb, *vbc;
	unsigned int unsolved = 0, min = 0, delta = 0;
	int i;
	struct STR_VALUE value, tmp;
	UCHAR *s, *t, c, d, e, f, g, h;

	for (s = subsect->sect0; s < subsect->sect1; ) {
		c = *s++;
		if (c == 0x2d) {
			/* EQUラベル定義 */
			s = skip_expr(s);
			s = skip_expr(s);
			continue;
		}
		if (c == 0x0e) {
			/* 非EQUラベルの定義 */
			/* 今は手抜きなので取り合えず無視 */
			s = skip_expr(s);
			continue;
			/* 本来はこれは来ない(？) */
		}
		if (c == 0x0f) {
			s += s[1] - (0x08 - 3); /* GLOBAL用変換 */
			continue;
		}
		if (c == 0x2c) {
			s = skip_expr(s);
			continue;
		}
		#if (DEBUG)
			if (c < 0x30)
				goto dberr0;
		#endif
		if (c < 0x3c) {
skipmc30:
			/* 30〜37:ショートDB(定数、0〜7バイト) */
			/* 式をDB〜DDで置く */
			s = skip_mc30(s - 1, &c, 1);
			min += c;
			continue;
		}
		if (c == 0x58) {
			/* ORG */
			s = skip_expr(s);
			continue;
		}
		if (c == 0x59) {
			/* TIMES */
			t = s;
			i = s[0] | s[1] << 8 | s[2] << 16 | s[3] << 24;
			s += 4;
			calc_value(&value, &s); /* len */
			/* lenにエラーはないという前提(プログラムカウンタを含まない定数式) */
			if ((unsigned int) i < 0xfffffff0) {
				min += (i - 1) * value.min; /* iは繰り返し回数 */
				s = skip_expr(s);
				continue;
			}
			i = value.min;
			calc_value(&value, &s); /* 繰り返し回数 */
			if (value.flags & (VFLG_SLFREF | VFLG_UNDEF)) /* 自己参照エラー */
				goto mc59_force;
			if (value.flags & VFLG_ERROR) {
				/* ラベル値一般エラー