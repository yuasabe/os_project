#include <setjmp.h>
#include <stdlib.h>		/* malloc */
#include <stdio.h>		/* NULL */

typedef unsigned int UINT32;
typedef unsigned char UCHAR;
typedef UINT32 tek_TPRB;

int tek_checkformat(int siz, UCHAR *p); /* 展開後のサイズを返す */
	/* -1:非osacmp */
	/* -2:osacmpだが対応できない */

int tek_decode(int siz, UCHAR *p, UCHAR *q); /* 成功したら0 */
	/* 正の値はフォーマットの異常・未対応、負の値はメモリ不足 */

static unsigned int tek_getnum_s7s(UCHAR **pp);
static int tek_lzrestore_tek5(int srcsiz, UCHAR *src, int outsiz, UCHAR *outbuf);
static int tek_decmain5(int *work, UCHAR *src, int osiz, UCHAR *q, int lc, int pb, int lp);

int tek_checkformat(int siz, UCHAR *p)
{
	static UCHAR header[] = "¥xff¥xff¥xff¥x01¥x00¥x00¥x00" "OSASKCMP";
	int i;
	if (siz < 17)
		return -1;
	for (i = 0; i < 15; i++) {
		if (p[i + 1] != header[i])
			return -1;
	}
	if (p[0] != 0x89)
		return -2;
	p += 16;
	return tek_getnum_s7s(&p);
}

int tek_decode(int siz, UCHAR *p, UCHAR *q)
{
	UCHAR *p1 = p + siz;
	int dsiz, hed, bsiz, st = 0;
	p += 16;
	if ((dsiz = tek_getnum_s7s(&p)) > 0) {
		hed = tek_getnum_s7s(&p);
		if ((hed & 1) == 0)
			st = tek_lzrestore_tek5(p1 - p + 1, p - 1, dsiz, q);
		else {
			bsiz = 1 << (((hed >> 1) & 0x0f) + 8);
			if (hed & 0x20)
				return 1;
			if (bsiz == 256)
				st = tek_lzrestore_tek5(p1 - p, p, dsiz, q);
			else {
				if (dsiz > bsiz)
					return 1;
				if (hed & 0x40)
					tek_getnum_s7s(&p); /* オプション情報へのポインタを読み飛ばす */
				st = tek_lzrestore_tek5(p1 - p, p, dsiz, q);
			}
		}
	}
	return st;
}

static unsigned int tek_getnum_s7s(UCHAR **pp)
/* これは必ずbig-endian */
/* 下駄がないので中身をいじりやすい */
{
	unsigned int s = 0;
	UCHAR *p = *pp;
	do {
		s = s << 7 | *p++;
	} while ((s & 1) == 0);
	s >>= 1;
	*pp = p;
	return s;
}

static int tek_lzrestore_tek5(int srcsiz, UCHAR *src, int outsiz, UCHAR *outbuf)
{
	int wrksiz, lc, lp, pb, *work, prop0, fl;

	if ((fl = (prop0 = *src) & 0x0f) != 0x01) /* 0001 */
		return 1;
	src++;
	prop0 >>= 4;
	if (prop0 == 0)
		prop0 = *src++;
	else {
		static UCHAR prop0_table[] = { 0x5d, 0x00 };
		if (prop0 >= 3)
			return 1;
		prop0 = prop0_table[prop0 - 1];
	}
	pb = prop0 / (9 * 5);
	prop0 %= 9 * 5;
	lp = prop0 / 9;
	lc = prop0 % 9;
	wrksiz = (0x800 + (0x300 << (lc + lp))) * sizeof (tek_TPRB); /* 最低11KB, lc+lp=3なら、32KB */
	work = malloc(wrksiz);
	if (work == NULL)
		return -1;

	return tek_decmain5(work, src, outsiz, outbuf, lc, pb, lp);
}

struct tek_STR_RNGDEC {
	UCHAR *p;
	UINT32 range, code;
};

struct tek_STR_PRB {
	struct tek_STR_PRB_PB {
		struct tek_STR_PRB_PBST {
			tek_TPRB mch, rep0l1;
		} st[12];
		tek_TPRB lenlow[2][8], lenmid[2][8];
	} pb[16];
	struct tek_STR_PRB_ST {
		tek_TPRB rep, repg0, repg1, repg2;
	} st[12];
	tek_TPRB lensel[2][2], lenhigh[2][256], pslot[4][64], algn[16];
	tek_TPRB spdis[2][2+4+8+16+32];
	tek_TPRB lit[1];
};

struct tek_STR_WORK5 {
	struct tek_STR_RNGDEC rd;
	struct tek_STR_PRB prb;
};

static int tek_rdget0(struct tek_STR_RNGDEC *rd, int n, int i)
{
	do {
		if (rd->range < (UINT32) (1 << 24))
			goto shift;
shift1:
		rd->range >>= 1;
		i += i;
		if (rd->code >= rd->range) {
			rd->code -= rd->range;
			i |= 1;
		}
	} while (--n);
	return ‾i;
shift:
	do {
		rd->range <<= 8;
		rd->code = rd->code << 8 | *rd->p++;
	} while (rd->range < (UINT32) (1 << 24));
	goto shift1;
}

static int tek_rdget1(struct tek_STR_RNGDEC *rd, UINT32 *prob0, int n, int j)
{
	UINT32 i, *prob;
	prob0 -= j;
	do {
		prob = prob0 + j;
		if (rd->range < (UINT32) (1 << 24))
			goto shift;
shift1:
		j += j;
		i = (rd->range >> 11) * *prob;
		if (rd->code < i) {
			j |= 1;
			rd->range = i;
			*prob += (0x800 - *prob) >> 5;
		} else {
			rd->range -= i;
			rd->code -= i;
			*prob -= *prob >> 5;
		}
	} while (--n);
	return j;
shift:
	do {
		rd->range <<= 8;
		rd->code = rd->code << 8 | *rd->p++;
	} while (rd->range < (UINT32) (1 << 24));
	goto shift1;
}

static UINT32 tek