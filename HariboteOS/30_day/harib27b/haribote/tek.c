#include "bootpack.h"
#include <setjmp.h>
#include <string.h>
#define NULL		0

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

int tek_getsize(unsigned char *p)
{
	static char header[15] = {
		0xff, 0xff, 0xff, 0x01, 0x00, 0x00, 0x00, 0x4f, 0x53, 0x41, 0x53, 0x4b, 0x43, 0x4d, 0x50
	};
	int size = -1;
	if (memcmp(p + 1, header, 15) == 0 && (*p == 0x83 || *p == 0x85 || *p == 0x89)) {
		p += 16;
		size = tek_getnum_s7s(&p);
	}
	return size;
}	  /* （註）memcmpはstrncmpの仲間で、文字列中に0があっても指定された15文字まで比較する関数 */

int tek_decomp(unsigned char *p, char *q, int size)
{
	int err = -1;
	if (*p == 0x83) {
		err = tek_decode1(size, p, q);
	} else if (*p == 0x85) {
		err = tek_decode2(size, p, q);
	} else if (*p == 0x89) {
		err = tek_decode5(size, p, q);
	}
	if (err != 0) {
		return -1;	/* 失敗 */
	}
	return 0;	/* 成功 */
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
				ds = ‾(