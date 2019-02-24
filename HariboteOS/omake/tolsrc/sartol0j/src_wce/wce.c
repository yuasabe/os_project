/* "wce.c":ワイルドカード展開 */

/* +:on -:off #:esc */
/* defaultはon */

/* prompt>wce.exe e arcfile basepath align #b=basepath * > $esar.ini */

/* *:一般ファイルとディレクトリにマッチ */
/* **:一般ファイルのみにマッチ（＝スラッシュを含まない） */

/* #f=* と#f内の#wのサポートを、あと##も */
/*	"#f=copy from:* to:@:" と、 "#f=#wpre *#w" の違い */
/*	ちょっとまて、自動"付加機能との兼ね合いは？ */
/*	じゃあこうすればいいのか */
/*	"#f=copy #wfrom:*#w to:@:" */
/*	#wは自動判別。#Wは強制 */
/*	デフォルトは#f=#w*#wということになる */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned char UCHAR;

UCHAR *outnames(UCHAR *outb0, UCHAR *outb1, UCHAR *outb3, UCHAR flags);
UCHAR *setnames(UCHAR *names0, UCHAR *base0, UCHAR *base1, UCHAR *tmp0, UCHAR *tmp1, UCHAR *ltmp0, UCHAR *ltmp1);
UCHAR *fixbase(UCHAR *base0, UCHAR *base1);
int pathcmp(UCHAR *p0, UCHAR *p1);

int main(int argc, UCHAR **argv)
{
	int i, j, len0, len1;
	UCHAR m = 1, mc = 0, mw, mo = 0, ms = 1, mp = 0;
	UCHAR *p, *base0 = malloc(4096), *base1 = base0 + 8, *p0, *p1, *n;
	UCHAR *names0 = malloc(8 * 1024 * 1024), *names1 = NULL;
	UCHAR *outb0 = malloc(4 * 1024 * 1024), *outb1 = outb0;
	UCHAR *outb2 = malloc(4 * 1024 * 1024), *outb3 = outb2;
	UCHAR *ltmp = malloc(1024 * 1024), *tmp = malloc(4096);

	for (i = 0; i < 8; i++)
		base0[i] = "dir    ¥x22"[i];

	for (i = 1; i < argc; i++) {
		p = argv[i];
		if (p[0] == '(' && p[1] == '¥0') {
			mo++;
			continue;
		}
		if (p[0] == ')' && p[1] == '¥0' && mo > 0) {
			mo--;
			goto check_mo;
		}
		if (p[0] == '+' && p[1] == '¥0') {
			m = 1;
			continue;
		}
		if (p[0] == '-' && p[1] == '¥0') {
			m = 0;
			continue;
		}
		if (p[0] == '#' && p[1] != '¥0') {
			if (p[1] == 'b' && p[2] == '=') { /* basepath指定 */
			//	if (p[3])
					p += 3;
			//	else {
			//		i++;
			//		if (i >= argc)
			//			break;
			//		p = argv[i];
			//	}
				base1 = base0 + 8;
				while (*p)
					*base1++ = *p++;
				names1 = NULL;
				continue;
			}
			if (p[1] == 'C' && p[2] == '=') { /* CAP指定 */
				/* よく考えると本来はこんなオプションはあるべきではない */
			//	if (p[3] == '¥0')
			//		mc ^= 1;
				if (p[3] == '+' || p[3] == '1')
					mc = 1;
				if (p[3] == '-' || p[3] == '0')
					mc = 0;
				continue;
			}
			if (p[1] == 's' && p[2] == '=') { /* sort指定 */
			//	if (p[3] == '¥0')
			//		ms ^= 1;
				if (p[3] == '+' || p[3] == '1')
					ms = 1;
				if (p[3] == '-' || p[3] == '0')
					ms = 0;
				continue;
			}
			if (p[1] == '!' && p[2] == '=') { /* 排除指定 */
				/* 将来的には排除指定でもワイルドカードを使えるようにする */
				p0 = outb0;
			//	if (p[3])
					p += 3;
			//	else {
			//		i++;
			//		if (i >= argc)
			//			break;
			//		p = argv[i];
			//	}
				len1 = strlen(p) + 2;
				while (p0 < outb1) {
					len0 = strlen(p0 + 1) + 2;
					if (len0 == len1 && strcmp(p, p0 + 1) == 0) {
						outb1 -= len0;
						p1 = p0;
						while (p1 < outb1) {
							*p1 = *(p1 + len0);
							p1++;
						}
						continue;
					}
					p0 += len0;
				}
				continue;
			}
#if 0
			if (p[1] == 'a' && p[2] == '=') { /* 文字列付加（要バッファリング） */
				if (p[3])
					p += 3;
				else {
					i++;
					if (i >= argc)
						break;
					p = argv[i];
				}
				if (outb0 >= outb1) {
					*outb1++ = 0x00;
					outb1++;
				}
				outb1--;
				while (*p)
					*outb1++ = *p++;
				*outb1++ = '¥0';
				continue;
			}
#endif
			if (p[1] == 'p' && p[2] == '=') { /* print指定 */
			//	if (p[3] == '¥0')
			//		mp ^= 1;
				if (p[3] == '+' || p[3] == '1')
					mp = 1;
				if (p[3] == '-' || p[3] == '0')
					mp = 0;
				continue;
			}
			p++; /* #+, #-, ## */
		}
		if (m == 0) {
through:
			*outb1++ = 0;
			while (*p)
				*outb1++ = *p++;
			*outb1++ = '¥0';
			goto check_mo;
		}
	//	if (m == 1) {
			p0 = p;
			while (*p) {
				if (*p++ == '*')
					goto wild;
			}
			p = p0;
			goto through;
	//	}
wild:
		/* ワイルドカードを分割 */
		/* 前方一致部分、後方一致部分、