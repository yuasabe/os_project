#include <stdio.h>		/* NULL */
#include <stdlib.h>		/* malloc, free */
#include <setjmp.h>

typedef unsigned char UCHAR;
typedef unsigned int UINT32;
typedef UINT32 tek_TPRB;

static int tek_decode1(int siz, UCHAR *p, UCHAR *q);
static int tek_decode2(int siz, UCHAR *p, UCHAR *q);
static int tek_decode5(int siz, UCHAR *p, UCHAR *q);

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

int autodecomp(int siz0, UCHAR *p0, int siz)
{
	unsigned char *b = p0, *c, *c0;
	int s, i, e = 0;
	if ((*(int *) &b[0x08] == 0x5341534f) && (*(int *) &b[0x0c] == 0x504d434b)) {
		if (*(int *) &b[0x04] == 0x00000001) {
			unsigned int t = *(int *) &b[0x00];
			e |= 1;
			if (0xffffff83 <= t && t <= 0xffffff89) {
				c = &b[0x10];
				s = tek_getnum_s7s(&c);
				if (s + siz - 0x10 <= siz0) {
					c0 = c = b + siz0 - siz;
					for (i = siz - 1; i >= 0x10; i--)
						c[i] = b[i];
					c += 0x10;
					tek_getnum_s7s(&c);
					if (t == 0xffffff83)
						e = tek_decode1(siz, c0, b);
					if (t == 0xffffff85)
						e = tek_decode2(siz, c0, b);
					if (t == 0xffffff89)
						e = tek_decode5(siz, c0, b);
					siz = s;
				}
			}
		}
	}
	if (e)
		siz |= -1;
	return siz;
}

static int tek_lzrestore_stk1(int srcsiz, UCHAR *src, int outsiz, UCHAR *q)
{
	int by, lz, cp, ds;
	UCHAR *q1 = q + outsiz, *s7ptr = src, *q0 = q;
	do {
		if ((by = (lz = *s7ptr++) & 0x0f) == 0)
			by = tek_getnum_s7s(&s7ptr);
		if ((lz >>= 4) == 0)
			lz = tek_getnum_s7s(&s7ptr);
		do {
			*q++ = *s7ptr++;
		} while (--by);
		if (q >= q1)
			break;
		do {
			ds = (cp = *s7ptr++) & 0x0f;
			if ((ds & 1) == 0) {
				do {
					ds = ds << 7 | *s7ptr++;
				} while ((ds & 1) == 0);
			}
			ds = ‾(ds >> 1);
			if ((cp >>= 4) == 0) {
				do {
					cp = cp << 7 | *s7ptr++;
				} while ((cp & 1) == 0);
				cp >>= 1;
			} /* 0がこないことをあてにする */
			cp++;
			if (q + ds < q0)
				goto err;
			if (q + cp > q1)
				cp = q1 - q;
			do {
				*q = *(q + ds);
				q++;
			} while (--cp);
		} while (--lz);
	} while (q < q1);
	return 0;
err:
	return 1;
}

static int tek_decode1(int siz, UCHAR *p, UCHAR *q)
{
	int dsiz, hed, bsiz;
	UCHAR *p1 = p + siz;
	p += 16;
	if ((dsiz = tek_getnum_s7s(&p)) > 0) {
		hed = tek_getnum_s7s(&p);
		bsiz = 1 << (((hed >> 1) & 0x0f) + 8);
		if (dsiz > bsiz || (hed & 0x21) != 0x01)
			return 1;
		if (hed & 0x40)
			tek_getnum_s7s(&p); /* オプション情報へのポインタを読み飛ばす */
		if (tek_getnum_s7s(&p) != 0)
			return 1; /* 補助バッファ使用 */
		return tek_lzrestore_stk1(p1 - p, p, dsiz, q);
	}
	return 0;
}

static unsigned int tek_getnum_s7(UCHAR **pp)
/* これは必ずbig-endian */
{
	unsigned int s = 0, b = 0, a = 1;
	UCHAR *p = *pp;
	for (;;) {
		s = s << 7 | *p++;
		if (s & 1)
			break;
		a <<= 7;
		b += a;
	}
	s >>= 1;
	*pp = p;
	return s + b;
}

static int tek_lzrestore_stk2(int srcsiz, UCHAR *src, int outsiz, UCHAR *q)
{
	int cp, ds, repdis[4], i, j;
	UCHAR *q1 = q + outsiz, *s7ptr = src, *q0 = q, bylz, cbylz;
	for (j = 0; j < 4; j++)
		repdis[j] = -1 - j;
	bylz = cbylz = 0;
	if (outsiz) {
		if (tek_getnum_s7s(&s7ptr))
			return 1;
		do {
			/* byフェーズ */
			j = 0;
			do {
				j++;
				if (j >= 17) {
					j += tek_getnum_s7s(&s7ptr);
					break;
				}
				if (cbylz == 0) {
					cbylz = 8;
					bylz = *s7ptr++;
				}
				cbylz--;
				i = bylz & 1;
				bylz >>= 1;
			} while (i == 0);
			do {
				*q++ = *s7ptr++;
			} while (--j);
			if (q >= q1)
				break;

			/* lzフェーズ */
			j = 0;
			do {
				j++;
				if (j >= 17) {
					j += tek_getnum_s7s(&s7ptr);
					break;
				}
				if (cbylz == 0) {
					cbylz = 8;
					bylz = *s7ptr++;
				}
				cbylz--;
				i = bylz & 1;
				bylz >>= 1;
			} while (i == 0);
			do {
				i = *s7ptr++;
				cp = i >> 4;
				i &= 0x0f;
				if ((i & 1) == 0)
					i |= (tek_getnum_s7(&s7ptr) + 1) << 4;
				i >>= 1;
				ds = ‾(i - 6);
				if (i < 4)
					ds = repdis[i];
				if (i == 4)
					ds = repdis[0] - tek_getnum_s7(&s7ptr) - 1;
				if (i == 5)
					ds = repdis[0] + tek_getnum_s7(&s7ptr) + 1;
				if (cp == 0)
					cp = tek_getnum_s7(&s7ptr) + 16;
				cp++;
				if (i > 0) {
					if (i > 1) {
						if (i > 2)
							repdis[3] = repdis[2];
						repdis[2] = repdis[1];
					}
					repdis[1] = repdis[0];
					repdis[0] = ds;
				}
				if (q + ds < q0)
					goto err;
				if (q + cp > q1)
					cp = q1 - q;
				do {
					*q = *(q + ds);
					q++;
				} while (--cp);
			} while (--j);
		} while (q < q1);
	}
	return 0;
err:
	return 1;
}

static int tek_decode2(int siz, UCHAR *p, UCHAR *q)
{
	UCHAR *p1 = p + siz;
	int dsiz, hed, bsiz, st = 0;
	p += 16;
	if ((dsiz = tek_getnum_s7s(&p)) > 0) {
		hed = tek_getnum_s7s(&p);
		bsiz = 1 << (((hed >> 1) & 0x0f) + 8);
		if (dsiz > bsiz || (hed & 0x21) != 0x01)
			return 1;
		if (hed & 0x40)
			tek_getnum_s7s(&p); /* オプション情報へのポインタを読み飛ばす */
		st = tek_lzrestore_stk2(p1 - p, p, dsiz, q);
	}
	return st;
}

static int tek_decmain5(int *work, UCHAR *src, int osiz, UCHAR *q, int lc, int pb, int lp, int flags);

static int tek_lzrestore_tek5(int srcsiz, UCHAR *src, int outsiz, UCHAR *outbuf)
{
	int wrksiz, lc, lp, pb, flags, *work, prop0, fl;

	if ((fl = (prop0 = *src) & 0x0f) == 0x01) /* 0001 */
		flags |= -1;
	else if (fl == 0x05)
		flags = -2;
	else if (fl == 0x09)
		flags &= 0;
	else
		return 1;
	src++;
	prop0 >>= 4;
	if (prop0 == 0)
		prop0 = *src++;
	else {
		static UCHAR prop0_table[] = { 0x5d, 0x00 }, prop1_table[] = { 0x00 };
		if (flags == -1) {
			if (prop0 >= 3)
				return 1;
			prop0 = prop0_table[prop0 - 1];
		} else {
			if (prop0 >= 2)
				return 1;
			prop0 = prop1_table[prop0 - 1];
		}
	}
	lp = prop0 / (9 * 5);
	prop0 %= 9 * 5;
	pb = prop0 / 9;
	lc = prop0 % 9;
	if (flags == 0) /* tek5:z2 */
		flags = *src++;
	if (flags == -1) { /* stk5 */
		wrksiz = lp;
		lp = pb;
		pb = wrksiz;
	}
	wrksiz = 0x180 * sizeof (UINT32) + (0x840 + (0x300 << (lc + lp))) * sizeof (tek_TPRB); /* 最低15KB, lc+lp=3なら、36KB */
	work = malloc(wrksiz);
	if (work == NULL)
		return -1;
	flags = tek_decmain5(work, src, outsiz, outbuf, lc, pb, lp, flags);
	free(work);
	return flags;
}

struct tek_STR_BITMODEL {
	UCHAR t, m, s, dmy;
	UINT32 prb0, prb1, tmsk, ntm, lt, lt0, dmy4;
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
	tek_TPRB lensel[2][2], lenhigh[2][256], pslot[4][64], algn[64];
	tek_TPRB spdis[2][2+4+8+16+32], lenext[2+4+8+16+32];
	tek_TPRB repg3, fchgprm[2 * 32], tbmt[16], tbmm[16], fchglt;
	tek_TPRB lit[1];
};

struct tek_STR_RNGDEC {
	UCHAR *p;
	UINT32 range, code, rmsk;
	jmp_buf errjmp;
	struct tek_STR_BITMODEL bm[32], *ptbm[16];
	struct tek_STR_PRB probs;
};

static void tek_setbm5(struct tek_STR_BITMODEL *bm, int t, int m)
{
	bm->t = t;
	bm->m = m;
	bm->prb1 = -1 << (m + t);
	bm->prb0 = ‾bm->prb1;
	bm->prb1 |= 1 << t;
	bm->tmsk = (-1 << t) & 0xffff;
	bm->prb0 &= bm->tmsk;
	bm->prb1 &= bm->tmsk;
	bm->ntm = ‾bm->tmsk;
	return;
}

static int tek_rdget0(struct tek_STR_RNGDEC *rd, int n, int i)
{
	do {
		while (rd->range < (UINT32) (1 << 24)) {
			rd->range <<= 8;
			rd->code = rd->code << 8 | *rd->p++;
		}
		rd->range >>= 1;
		i += i;
		if (rd->code >= rd->range) {
			rd->code -= rd->range;
			i |= 1;
		}
	} while (--n);
	return ‾i;
}

static int tek_rdget1(struct tek_STR_RNGDEC *rd, tek_TPRB *prob0, int n, int j, struct tek_STR_BITMODEL *bm)
{
	UINT32 p, i, *prob, nm = n >> 4;
	n &= 0x0f;
	prob0 -= j;
	do {
		p = *(prob = prob0 + j);
		if (bm->lt > 0) {
			if (--bm->lt == 0) {
				/* 寿命切れ */
				if (tek_rdget1(rd, &rd->probs.fchglt, 0x71, 0, &rd->bm[3]) == 0) {
					/* 