/* "edimg.c":ディスクイメージエディタ */
/*	copyright(C) 2004 川合秀実, I.Tak. (KL-01) */

#include <stdio.h>
#include <stdlib.h>

/* x86以外で動かすことは全く考えていない。endian依存部がたくさんある。
	依存をなくすことはそう難しくないが、依存をなくせばその分サイズが増えるか
	条件コンパイルが多数必要になり、非x86上でこのedimgが必要になる状況の
	少なさを勘案すると、endian依存をなくすことはむしろ改悪であると考えた。 */

#define SAR_MODE_WIN32		1
/* Linuxなどでは↑をコメントアウトする */

//#define SAR_MODE_POSIX	1
/* Windowsなどでは↑をコメントアウトする */

typedef unsigned char UCHAR;

#define SIZ_IMGBUF		8 * 1024 * 1024 + 1024
#define SIZ_FILEBUF		2 * 1024 * 1024 + 1024
#define SIZ_SCRIPT		64 * 1024

UCHAR *filebuf0;
UCHAR *imgbuf0;
unsigned int len_filebuf, len_imgbuf = 0, len_sec, len_clu, limit_fat, alloc0;
unsigned short *fat;
UCHAR *dir0, *BPB, *KHBIOS, *fat0, *fat1, *clu0002;
UCHAR flag = 0;
	/* bit0:FAT16, bit1:FAT12, bit2:SF16 */
	/* bit4:bininモード */
const UCHAR *_path = "";
static int bias = 0;

void errend(int i);
char cmdmatch(const char *script, const char *cmd);
char optmatch(const char *script, const char *opt);
const UCHAR *cmd_copy(const char *cmd);
const UCHAR *cmd_ovrcopy(const char *cmd);
const UCHAR *cmd_create(const char *cmd);
const UCHAR *cmd_ovrcreate(const char *cmd);
const UCHAR *cmd_delete(const char *cmd);
const UCHAR *cmd_setattr(const char *cmd);
const UCHAR *cmd_wbinimg(const char *cmd);
const UCHAR *cmd_release(const char *cmd);
const UCHAR *cmd_writedata(const char *cmd);
const UCHAR *cmd_list(const char *cmd);
const UCHAR *cmd_copyall(const char *cmd);
const UCHAR *cmd_exe2bin(const char *cmd);
const UCHAR *opt_imgin(const char *cmd);
const UCHAR *opt_vsiz(const char *cmd);
const UCHAR *opt_imgout(const char *cmd);
const UCHAR *opt_binin(const char *cmd);
const UCHAR *opt_binout(const char *cmd);
const UCHAR *opt_bpath(const char *cmd);
const UCHAR *opt_bias(const char *cmd);
void readfile(const UCHAR *path, int flags);
void writefile(const UCHAR *path, int alloc0, int asiz, int flags);
void ovrwritefile(const UCHAR *path, int alloc0, int asiz, int flags);
void deletefile(const UCHAR *path);
void setattrfile(const UCHAR *path, UCHAR attr);
void exe2bin(const int seg0);
//	void readbin(int bytes, int offset_img, int offset_file);
//	void writebin(int bytes, int offset_img, int offset_file);
const UCHAR *dir_search(const UCHAR *name, UCHAR attr_mask, UCHAR attr_comp);
UCHAR *test_BPB(UCHAR *p);
void bpbfix_sub();
const UCHAR *test_KHBIOS(const UCHAR *p);
void decode_l2d3(int k, const UCHAR *src, UCHAR *dest);
void decode_tek0(int k, const UCHAR *src, UCHAR *dest);

struct sar_attrtime {
	int attr, permission;
	int subsec;
	UCHAR sec, min, hour, day, mon, _dummy[3];
	unsigned int year, year_h;
};
void getattrtime(struct sar_attrtime *s, const UCHAR *path);
void setattrtime(struct sar_attrtime *s, const UCHAR *path, const int flags);
void sar_shifttime(struct sar_attrtime *at, int min, void *opt);

int main(int argc, UCHAR **argv)
{
	UCHAR *script0 = malloc(SIZ_SCRIPT);
//	fat_flags = malloc(65536);
	fat = malloc(65536 * 2);
	filebuf0 = malloc(SIZ_FILEBUF);
	imgbuf0 = malloc(SIZ_IMGBUF);
	UCHAR *p, *q, *r, *script1;
	int i;
	FILE *fp;
	if (imgbuf0 == NULL)
		errend(1); /* out of memory */

	/* スクリプト準備 */
	q = script0;
	while ((p = *++argv) != NULL) {
		if (*p != '@') {
			r = ++q;
			while ((*q++ = *p++) != '¥0') {
				if (q - script0 >= SIZ_SCRIPT - 4)
					errend(2); /* script too long */
			}
			if (q - r > 255 + 1)
				errend(3); /* script too long */
			*(r - 1) = (q - r) - 1;
			continue;
		}
		p++;
		fp = fopen(p, "rb");
		if (fp == NULL)
			errend(4); /* script file open error */
		i = fread(filebuf0, 1, SIZ_FILEBUF, fp);
		if (i >= SIZ_FILEBUF)
			errend(5); /* script file open error */
		script1 = filebuf0 + i;
		p = filebuf0;
		for (;;) {
			while (p < script1 && *p <= ' ')
				p++;
			if (p >= script1)
				break;
			r = ++q;
			while (p < script1 && *p > ' ') {
				if (q - script0 >= SIZ_SCRIPT - 4)
					errend(6); /* script too long */
				*q++ = *p++;
			}
			if (q - r > 255)
				errend(7); /* script too long */
			*(r - 1) = q - r;
			*q++ = '¥0';
		}
	}
	*q++ = '¥0';
	script1 = q;

	/* スクリプト解釈 */
	p = script0;
	for (;;) {
		if (*p == '¥0')
			break;
		if (cmdmatch(p, "copy")) {
			p = (UCHAR *) cmd_copy(p);
			continue;
		}
		if (cmdmatch(p, "ovrcopy")) {
			p = (UCHAR *) cmd_ovrcopy(p);
			continue;
		}
		if (cmdmatch(p, "create")) {
			p = (UCHAR *) cmd_create(p);
			continue;
		}
		if (cmdmatch(p, "ovrcreate")) {
			p = (UCHAR *) cmd_ovrcreate(p);
			continue;
		}
		if (cmdmatch(p, "delete")) {
			p = (UCHAR *) cmd_delete(p);
			continue;
		}
		if (cmdmatch(p, "setattr")) {
			p = (UCHAR *) cmd_setattr(p);
			continue;
		}
		if (cmdmatch(p, "wbinimg")) {
			p = (UCHAR *) cmd_wbinimg(p);
			continue;
		}
		if (cmdmatch(p, "release")) {
			p = (UCHAR *) cmd_release(p);
			continue;
		}
		if (cmdmatch(p, "writedata")) {
			p = (UCHAR *) cmd_writedata(p);
			continue;
		}
		if (cmdmatch(p, "list")) {
			p = (UCHAR *) cmd_list(p);
			continue;
		}
		if (cmdmatch(p, "copyall")) {
			p = (UCHAR *) cmd_copyall(p);
			continue;
		}
		if (cmdmatch(p, "exe2bin")) {
			p = (UCHAR *) cmd_exe2bin(p);
			continue;
		}
		if (cmdmatch(p, "opt")) {
			p += *p + 2;
			continue;
		}
		if (optmatch(p, "imgin")) {
			p = (UCHAR *) opt_imgin(p);
			continue;
		}
		if (optmatch(p, "vsiz")) {
			p = (UCHAR *) opt_vsiz(p);
			continue;
		}
		if (optmatch(p, "imgout")) {
			p = (UCHAR *) opt_imgout(p);
			continue;
		}
		if (optmatch(p, "binin")) {
			p = (UCHAR *) opt_binin(p);
			continue;
		}
		if (optmatch(p, "binout")) {
			p = (UCHAR *) opt_binout(p);
			continue;
		}
		if (optmatch(p, "_path")) {
			p = (UCHAR *) opt_bpath(p);
			continue;
		}
		if (optmatch(p, "bias")) {
			p = (UCHAR *) opt_bias(p);
			continue;
		}
		if (cmdmatch(p, "/*")) {
			i = 1;
			do {
				p += *p + 2;
				if (*p == '¥0')
					errend(9); /* comment nesting error */
				if (cmdmatch(p, "/*"))
					i++;
				if (cmdmatch(p, "*/"))
					i--;
			} while (i > 0);
			p += *p + 2;
			continue;
		}
		fputs("script syntax error. : ", stderr);
		fputs(p + 1, stderr);
		errend(8); /* script syntax error */
	}
	return 0;
}

void errend(int i)
{
	static const UCHAR *msg[] = {
		"out of memory.¥n",
		"script too long.¥n",
		"script too long.¥n",
		"script file open error.¥n",
		"script file open error.¥n",
		"script too long.¥n",
		"script too long.¥n",
		"¥n",
		"comment nesting error.¥n",
		"copy command error.¥n",
		"ovrcopy command error.¥n",
		"¥n",
		"¥n",
		"¥n",
		"¥n",
		"¥n",
		"opt-imgin format error.¥n",
		"disk-image full.¥n",
		"create command error.¥n",
		"ovrcreate command error.¥n",
		"delete command error.¥n",
		"setattr command error.¥n",
		"opt-vsiz error.¥n", /* 23 */
		"¥n",
		"wbinimg command error.¥n",
		"release command error.¥n",
		"bin-file too large.¥n",
		"¥n",
		"¥n",
		"writedata command error.¥n", /* 30 */
		"list command error.¥n",
		"not SF16 error.¥n",
		"efat error.¥n",
		"nofrag error.¥n",
		"copyall command error.¥n",
		"exe2bin command error.¥n",
		"imgout BPB data error.¥n"
	};
	fputs(msg[i - 1], stderr);
	exit(i);
}

char cmdmatch(const char *script, const char *cmd)
{
	script++;
	for (;;) {
		if (*cmd != *script)
			return 0;
		if (*cmd == '¥0')
			return 1;
		cmd++;
		script++;
	}
}

char optmatch(const char *script, const char *opt)
{
	script++;
	for (;;) {
		if (*opt == '¥0') {
			if (*script == ':')
				return 1;
			break;
		}
		if (*opt != *script)
			break;
		opt++;
		script++;
	}
	return 0;
}

const UCHAR *pathfix(const UCHAR *path, char flg)
{
	static UCHAR pathbuf[1024], pathbuf2[1024];
	static const UCHAR *defname;
	UCHAR *p;
	const UCHAR *q, *r;

	if (path[0] == '_' && path[1] == ':') {
		p = pathbuf2;
		r = _path;
		while ((*p = *r) != '¥0') {
			p++;
			r++;
			if ((unsigned int) (p - pathbuf2) >= (sizeof pathbuf) - 1)
				goto toolong;
		}
		r = path + 2;
		while ((*p = *r) != '¥0') {
			p++;
			r++;
			if ((unsigned int) (p - pathbuf2) >= (sizeof pathbuf) - 1)
				goto toolong;
		}
		path = pathbuf2;
	}
	if (flg == 1) {
		p = (UCHAR *) path;
		while (*p != '¥0')
			p++;
		if (p - path >= 1) {
			if (p[-1] == '/')
				goto fix;
			if (p[-1] == '¥¥')
				goto fix;
			if (p[-1] == ':') {
	fix:
				if ((unsigned int) (p - path) >= sizeof pathbuf) {
toolong:
					fputs("too long path. : ", stderr);
					fputs(path, stderr);
					errend(12);
				}
				p = pathbuf;
				do {
					*p++ = *path++;
				} while (*path != '¥0');
				path = defname;
				while (*path != '¥0') {
					if ((unsigned int) (p - pathbuf) >= (sizeof pathbuf) - 1) {
						fputs("too long path. : ", stderr);
						fputs(pathbuf, stderr);
						errend(12);
					}
					*p++ = *path++;
				}
				*p = '¥0';
				path = pathbuf;
			}
		}
	}
	if (flg == 0) {
		q = path;
		r = path;
		if (*q != '¥0') {
			do {
				if (*q == ':')
					r = q + 1;
				if (*q == '/')
					r = q + 1;
				if (*q == '¥¥')
					r = q + 1;
				q++;
			} while (*q != '¥0');
		}
		defname = r;
	}
	return path;
}

unsigned int tek1_getnum_s7s(UCHAR **pp)
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

int tek1_decode1(int siz, UCHAR *p, UCHAR *q);
int tek1_decode2(int siz, UCHAR *p, UCHAR *q);
int tek1_decode5(int siz, UCHAR *p, UCHAR *q);

int autodecomp(int siz0, UCHAR *p0, int siz);

int autodecomp2(int siz0, UCHAR *p0, int siz)
{
	unsigned char *b = p0, *c;
	int s, i;
	if ((*(int *) &b[0x08] == 0x5341534f) && (*(int *) &b[0x0c] == 0x504d434b)) {
		if (*(int *) &b[0x04] == 0x00000001) {
			unsigned int t = *(int *) &b[0x00];
			if (0xffffff81 <= t && t <= 0xffffff82) {
				s = *(int *) &b[0x10];
				if (s + siz - 0x14 <= siz0) {
					c = b + siz0 - siz;
					for (i = siz - 1; i >= 0x10; i--)
						c[i] = b[i];
					c += 0x14;
					if (t == 0xffffff81)
						decode_l2d3(s, c, b);
					if (t == 0xffffff82)
						decode_tek0(s, c, b);
					siz = s;
				}
			} else if (0xffffff83 <= t && t <= 0xffffff89) {
				s = autodecomp(siz0, b, siz);
				if (s >= 0)
					siz = s;
			}
		}
	}
	return siz;
}

const UCHAR *cmd_copy(const char *cmd)
{
	char nocmp = 0;
	cmd += *cmd + 2;
	if (optmatch(cmd, "nocmp")) {
		cmd += *cmd + 2;
		nocmp = 1;
	}
	if (optmatch(cmd, "from") == 0)
		errend(10); /* copy command error */
	readfile(pathfix(cmd + (4 + 2), 0), 0x0f);
	cmd += *cmd + 2;
	if (nocmp)
		len_filebuf = autodecomp2(SIZ_FILEBUF, filebuf0, len_filebuf);
	if (optmatch(cmd, "to") == 0)
		errend(10); /* copy command error */
	writefile(pathfix(cmd + (2 + 2), 1), 2, -1, 0x07);
	return cmd + (*cmd + 2);
}

const UCHAR *cmd_ovrcopy(const char *cmd)
{
	char nocmp = 0;
	cmd += *cmd + 2;
	if (optmatch(cmd, "nocmp")) {
		cmd += *cmd + 2;
		nocmp = 1;
	}
	if (optmatch(cmd, "from") == 0)
		errend(11); /* ovrcopy command error */
	readfile(pathfix(cmd + (4 + 2), 0), 0x0f);
	cmd += *cmd + 2;
	if (nocmp)
		len_filebuf = autodecomp2(SIZ_FILEBUF, filebuf0, len_filebuf);
	if (optmatch(cmd, "to") == 0)
		errend(11); /* ovrcopy command error */
	ovrwritefile(pathfix(cmd + (2 + 2), 1), 2, -1, 0x04); /* 更新日のみ更新 */
	return cmd + (*cmd + 2);
}

int getnum(const UCHAR *p)
{
	int i = 0, base = 10, sign = 1;
	UCHAR c;
	if (*p == '-') {
		p++;
		sign = -1;
	}
	if (*p == '0') {
		p++;
		base = 8;
		c = *p;
		if (c >= 'a')
			c -= 'a' - 'A';
		if (c == 'X') {
			p++;
			base = 16;
		}
		if (c == 'O') {
			p++;
			base = 8;
		}
		if (c == 'B') {
			p++;
			base = 2;
		}
	}
	for (;;) {
		c = *p++;
		if ('0' <= c && c <= '9')
			c -= '0'; 
		else if ('A' <= c && c <= 'F')
			c -= 'A' - 10;
		else if ('a' <= c && c <= 'f')
			c -= 'a' - 10;
		else
	