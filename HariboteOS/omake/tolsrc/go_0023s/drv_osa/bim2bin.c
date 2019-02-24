//	"bim2bin"
//	Copyright(C) 2003 H.Kawai
//	usage : bim2bin [malloc:#] [mmarea:#] in:(path) out:(path)
//		[-bim | -exe512 | -bin0 | -data | -restore | -osacmp] [-simple | -l2d3 | -tek0]

#include "../include/stdio.h"
#include "../include/stdlib.h"
#include "../include/string.h"
#include "../include/setjmp.h"

#define	get32(ptr)		*((int *) (ptr))

#define	SIZEOFBUF		(256 * 1024)
#define	SIZEOFOVERBUF	(256 * 1024)
#define	SIZEOFSUBBUF	(SIZEOFOVERBUF * 4)

/* 1536KB */

#define SIZ_SYSWRK			(4 * 1024)

/* +0x10 : 総DS/SSサイズ */
/* +0x14 : file */
/* +0x18 : reserve */
/* +0x1c : reserve */
/* +0x20 : static start */
/* +0x24 : static bytes */

static unsigned char *putb_buf, *putb_overbuf;
static int putb_ptr;
static unsigned char putb_byte, putb_count;

static const int lzcompress_l2d3(unsigned char *buf, int k, int i, int outlimit, int maxdis);
static const int lzcompress_tek0(int prm, unsigned char *buf, int k, int i, int outlimit, int maxdis);
static const int lzrestore_l2d3(unsigned char *buf, int k, int i, int outlimit);
static const int lzrestore_tek0(unsigned char *buf, int k, int i, int outlimit);

struct STR_BIM2BIN {
	UCHAR *cmdlin; /* '¥0'で終わる */
	UCHAR *outname; /* '¥0'で終わる, workのどこかへのポインタ */
	UCHAR *dest0, *dest1; /* 出力ファイル(dest0は書き換えられる) */
	UCHAR *err0, *err1; /* コンソールメッセージ(err0は書き換えられる) */
	UCHAR *work0, *work1;
	int errcode;
};

extern GO_FILE GO_stdin, GO_stdout, GO_stderr;
extern struct GOL_STR_MEMMAN GOL_memman, GOL_sysman;
extern int GOL_abortcode;
extern jmp_buf setjmp_env;
void *GOL_memmaninit(struct GOL_STR_MEMMAN *man, size_t size, void *p);
void *GOL_sysmalloc(size_t size);
UCHAR **ConvCmdLine1(int *pargc, UCHAR *p);

static int main0(int argc, char **argv, struct STR_BIM2BIN *params);

int bim2bin_main(struct STR_BIM2BIN *params)
{
	int argc;
	UCHAR **argv, *tmp0;
	UCHAR **argv1, **p;
	GO_stdout.p0 = GO_stdout.p = params->err0;
	GO_stdout.p1 = params->err0; /* stdoutはなし */
	GO_stdout.dummy = ‾0;
	GO_stderr.p0 = GO_stderr.p = params->err0;
	GO_stderr.p1 = params->err1;
	GO_stderr.dummy = ‾0;

	putb_count = 8;

	if (setjmp(setjmp_env)) {
		params->err0 = GO_stderr.p;
		return GOL_abortcode;
	}

	if (params->work1 - params->work0 < SIZ_SYSWRK + 16 * 1024)
		return GO_TERM_WORKOVER;
	GOL_memmaninit(&GOL_sysman, SIZ_SYSWRK, params->work0);
	GOL_memmaninit(&GOL_memman, params->work1 - params->work0 - SIZ_SYSWRK, params->work0 + SIZ_SYSWRK);
	argv = ConvCmdLine1(&argc, params->cmdlin);

	params->errcode = main0(argc, argv, params);

skip:
	/* バッファを出力 */
	GOL_sysabort(0);
}

static const int getnum(unsigned char *p)
{
	int i = 0, j, base = 10;
//	p = skipspace(p);
	if (*p == '0') {
		p++;
		if (*p == 'X' || *p == 'x') {
			base = 16;
			p++;
		} else if (*p == 'O' || *p == 'o') {
			base = 8;
			p++;
		}
	}
	p--;
	for (;;) {
		p++;
		if (*p == '_')
			continue;
		j = 99;
		if ('0' <= *p && *p <= '9')
			j = *p - '0';
		if ('A' <= *p && *p <= 'F')
			j = *p - 'A' + 10;
		if ('a' <= *p && *p <= 'f')
			j = *p - 'a' + 10;
		if (base <= j)
			break;
		i = i * base + j;
	}
	if (*p == 'k' || *p == 'K') {
		i *= 1024;
	//	p++;
	} else if (*p == 'm' || *p == 'M') {
		i *= 1024 * 1024;
	//	p++;
	} else if (*p == 'g' || *p == 'G') {
		i *= 1024 * 1024 * 1024;
	//	p++;
	}
	return i;
}

static int main0(int argc, char **argv, struct STR_BIM2BIN *params)
{
	int mallocsize = 32 * 1024, reserve = 0, stacksize, datasize, worksize;
	int i, filesize, compress = 2, outtype = 0, prm0 = 12, maxdis = 32 * 1024;
//	FILE *fp;
	unsigned char *filepath[2], *s, *t, c;
	unsigned char *buf, *overbuf;
	int oversize, j, k, code_end, data_begin, entry;
	static unsigned char signature[15] = "¥xff¥xff¥xff¥x01¥x00¥x00¥x00OSASKCMP";

	filepath[0] = filepath[1] = NULL;
	buf = malloc(SIZEOFBUF);
	overbuf = malloc(SIZEOFOVERBUF);

	if (argc <= 2) {
		fprintf(stderr,
			"¥"bim2bin¥" executable binary maker for GUIGUI00 (OSASK API)¥n"
		