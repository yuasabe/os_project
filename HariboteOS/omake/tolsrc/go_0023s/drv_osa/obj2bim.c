/* "obj2bim" */
//	Copyright(C) 2003 H.Kawai
//	usage : obj2bim @(rule file) out:(file) [map:(file)] [stack:#] [(.obj/.lib file) ...]

#include "../include/stdio.h"
#include "../include/stdlib.h"
#include "../include/string.h"
#include "../include/setjmp.h"

/* OBJBUFSIZは、全ファイル合計の制限サイズ */

#define FILEBUFSIZ		(4 * 1024)	/* rulefileの最大サイズ */
#define	OBJBUFSIZ		(512 * 1024)	/* 512KB */
#define	LABELSTRSIZ		 		(8000)	/* 総ラベル数 (8000*140Bytes) */
#define	OBJFILESTRSIZ			(512)	/* 最大オブジェクトファイル数(512*260bytes)
// #define LINKSTRSIZ		(LABELSTRSIZ * 1)	/* 8000*12Bytes */
#define	MAXSECTION				  8	/* 1つの.objファイルあたりの最大セクション数 */

/* LINKSTRSIZがなぜか効かない。cpp0はバグもちか？ */
#define LINKSTRSIZ		LABELSTRSIZ

/* 計1833.5KB？ */

struct STR_OBJ2BIM {
	UCHAR *cmdlin; /* '¥0'で終わる */
	UCHAR *outname; /* '¥0'で終わる, workのどこかへのポインタ */
	UCHAR *mapname; /* '¥0'で終わる, workのどこかへのポインタ */
	UCHAR *dest0, *dest1; /* 出力ファイル(dest0は書き換えられる) */
	UCHAR *map0, *map1; /* 出力ファイル(map0は書き換えられる) */
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

#define SIZ_SYSWRK			(4 * 1024)

static int main0(const int argc, const char **argv, struct STR_OBJ2BIM *params);

int obj2bim_main(struct STR_OBJ2BIM *params)
{
//	static char execflag = 0;
	int argc;
	UCHAR **argv, *tmp0;
	UCHAR **argv1, **p;
	GO_stdout.p0 = GO_stdout.p = params->map0;
	GO_stdout.p1 = params->map1; /* stdoutはmap */
	GO_stdout.dummy = ‾0;
	GO_stderr.p0 = GO_stderr.p = params->err0;
	GO_stderr.p1 = params->err1;
	GO_stderr.dummy = ‾0;

	/* 多重実行阻止 (staticを再初期化すればできるが) */
//	if (execflag)
//		return 7;
//	execflag = 1;

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
	params->map0 = GO_stdout.p;

skip:
	/* バッファを出力 */
	GOL_sysabort(0);
}


const int get32b(unsigned char *p)
{
	return (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
}

const int get32l(unsigned char *p)
{
	return p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
}

struct LABELSTR {
	unsigned char type, sec, flags, align;
	/* type  0xff:未使用 */
	/* type  0x01:global/local label */
	/* type  0x02:constant */
	/* flags bit0 : used */
	/* flags bit1 : linked */
	unsigned int name[128 / 4 - 4];
	struct OBJFILESTR *name_obj; /* ローカル.objへのポインタ。publicならNULL */
	struct OBJFILESTR *def_obj; /* 所属オブジェクトファイル */
	unsigned int offset;
};

struct OBJFILESTR {
	struct {
		unsigned char *ptr;
		int links, sh_paddr, sectype;
		unsigned int size, addr;
		struct LINKSTR *linkptr;
		signed char align;
		unsigned char flags; /* bit0 : pure-COFF(0)/MS-COFF(1) */
	} section[MAXSECTION];
	unsigned int flags;
	/* flags  0xff : terminator */
	/* flags  bit0 : link */
};

struct LINKSTR {
	unsigned char type, dummy[3];
	int offset;
	struct LABELSTR *label;
	/* type  0x06:absolute */
	/* type  0x14:relative */
};

static unsigned char *skipspace(unsigned char *p);
static void loadlib(unsigned char *p);
static void loadobj(unsigned char *p);
static const int getnum(unsigned char **pp);
static struct LABELSTR *symbolconv0(unsigned char *s, struct OBJFILESTR *obj);
static struct LABELSTR *symbolconv(unsigned char *p, unsigned char *s, stru