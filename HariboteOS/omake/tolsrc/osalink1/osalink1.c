/* "osalink1.c":OSASK LINKプログラム version 0.1.
	copyright(C) 2003 H.Kawai (川合秀実)

	最初がBASE.EXE、その後は各種.BIN(.TEK)を想定している */

#include <stdio.h>
#include <ctype.h>
#include <string.h>

#define	OPTIONFILE	"OSALINK1.OPT"
#define	OUTPUTFILE	"OSASK.EXE"
#define	BUFSIZE		2 * 1024 * 1024

void wordstore(char *p, const int w)
{
	p[0] =  w       & 0xff;
	p[1] = (w >> 8) & 0xff;
	return;
}

void dwordstore(char *p, const int d)
{
	p[0] =  d        & 0xff;
	p[1] = (d >>  8) & 0xff;
	p[2] = (d >> 16) & 0xff;
	p[3] = (d >> 24) & 0xff;
	return;
}

const int wordload(const unsigned char *p)
{
	return p[0] | p[1] << 8;
}

const int dwordload(const unsigned char *p)
{
	return p[0] | p[1] << 8 | p[2] << 16 | p[3] << 24;
}

const int script(char *opt, char *inp, char *out);

static unsigned char buf0[BUFSIZE];

const int main(int argc, char **argv)
{
	FILE *fp0, *fp1;
	int i, j, k, size, totalsize = 0;
	unsigned char fname[32], name[8], c;
	unsigned char *buf = buf0, *buf1, *buf2, *buf3;
	unsigned char *optfile = OPTIONFILE, *outfile = OUTPUTFILE;

	if (argc == 4)
		return script(argv[1], argv[2], argv[3]);
	if (argc >= 2)
		optfile = argv[1];
	if (argc >= 3)
		outfile = argv[2];

	// OPTIONFILEの読み込みと各ファイルのサイズ取得
	fp0 = fopen(optfile, "r");
	if (fp0 == NULL) {
		fprintf(stderr, "Can't open ¥"%s¥".¥n", optfile);
		return 1;
	}
	for (i = 0; fscanf(fp0, " %s", fname) == 1; i++) {
		fp1 = fopen(fname, "rb");
		if (fp1 == NULL) {
err1:
			fclose(fp0);
			fprintf(stderr, "Can't open ¥"%s¥".¥n", fname);
			return 1;
		}
		buf1 = buf + totalsize;
		size = fread(buf + totalsize, 1, BUFSIZE - totalsize, fp1);
		fclose(fp1);
		if (size <= 0)
			goto err1;
		if (size >= BUFSIZE - totalsize)
			goto err1;
		if (i == 0) {
			buf2 = buf + (wordload(buf + 0x204) << 4) - 16 + 0x220;
			goto skip; /* BASE.EXEは加工しない */
		}

		for (j = 0; j < 8; j++)
			name[j] = ' ';
		for (j = 0; j < 8 && fname[j] != '.'; j++)
			name[j] = tolower(fname[j]);
		for (buf3 = buf2; *buf3; buf3 += 16) {
			c = 0;
			for (j = 0; j < 8; j++)
				c |= name[j] ^ buf3[j];
			if (c)
				continue;
			j = size;
			if (size > 20) {
			//	c = 0;
				for (k = 1; k < 16; k++)
					c |= buf1[k] ^ "¥x82¥xff¥xff¥xff¥x01¥x00¥x00¥x00OSASKCMP"[k];
				if (c == 0) {
					size -= 20;
					j = dwordload(buf1 + 16); /* 展開後のサイズ */
					for (k = 0; k < size; k++)
						buf1[k] = buf1[k + 20];
					buf3[0x0f] = 0x80; /* 圧縮フラグを立てる */
				}
			}
			dwordstore(buf3 + 0x08, j);
			wordstore(buf3 + 0x0c, (totalsize - 0x200) >> 4);
			break;
		}
skip:
		totalsize += size;
		while (totalsize & 0x0f)
			*(buf + totalsize++) = '¥0'; /* パラグラフ単位のアライン */
	}
	fclose(fp0);

	/* ヘッダ調整 */
	wordstore(buf + 0x02, totalsize & 0x01ff); // 最終ページサイズ
	wordstore(buf + 0x04, (totalsize + 0x1ff) >> 9); // ファイルページ数
	wordstore(buf + 0x0e, (totalsize - 0x200) >> 4); // 初期SS

	if (buf[0x0208] == 0x10 && buf[0x0209] == 0x89 && buf[0x020a] == 0x00) {
		/* OSASKのKHBIOS用スクリプトを発見 */
		size = (totalsize + (0x1ff - 0x200)) >> 9;
		buf[0x020b] =  size       & 0xff;
		buf[0x020c] = (size >> 8) & 0xff;
	}

	/* 出力 */
	fp1 = fopen(outfile, "wb");
	fwrite(buf, 1, totalsize, fp1);
	fclose(fp1);
	return 0;
}

char *skipspace(char *s, char *s1)
{
retry:
	while (s < s1 && *s <= ' ')
		s++;
	if (s + 1 < s1 && *s == '/' && *(s + 1) == '*') {
		s += 2;
		do {
			while (s < s1 && *s != '*')
				s++;
			if (s + 1 >= s1)
				return s1;
			s += 2;
		} while (*(s - 1) != '/');
		goto retry;
	}
	return s;
}

char *checktoken(char *s, char *s1)
/* これを呼ぶ前にskipspaceしておくこと */
{
	char *s0 = s;
	if (s < s1) {
		do {
			char c = *s;
			if (c <= ' ')
				break;
			if (c == ',')
				break;
			if (c == '(')
				break;
			if (c == ')')
				break;
			if (c == '{')
				break;
			if (c == '}')
				break;
			if (c == ';')
				break;
			if (c == '¥x22')
				break;
			s++;
		} whi