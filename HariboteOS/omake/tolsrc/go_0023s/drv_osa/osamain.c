
#include "../include/setjmp.h"
#include "../include/string.h"
#include "stdlib.h"
#include "guigui00.h"

int GOL_abortcode;
jmp_buf setjmp_env;

#define SIZ_MEMTEMP		(128 * 1024)

static UCHAR *memtemp0, *memtemp1;
static int mt_size0 = 0, mt_size1 = 0;

struct STR_CC1MAIN {
	UCHAR *cmdlin; /* '¥0'で終わる */
	UCHAR *outname; /* '¥0'で終わる, workのどこかへのポインタ */
	UCHAR *dest0, *dest1; /* 出力ファイル(dest0は書き換えられる) */
	UCHAR *err0, *err1; /* コンソールメッセージ(err0は書き換えられる) */
	UCHAR *work0, *work1;
	int errcode;
};

struct STR_GAS2NASK {
	UCHAR *cmdlin; /* '¥0'で終わる */
	UCHAR *outname; /* '¥0'で終わる, workのどこかへのポインタ */
	UCHAR *dest0, *dest1; /* 出力ファイル(dest0は書き換えられる) */
	UCHAR *err0, *err1; /* コンソールメッセージ(err0は書き換えられる) */
	UCHAR *work0, *work1;
	int errcode;
};

struct STR_NASKMAIN {
	UCHAR *cmdlin; /* '¥0'で終わる */
	UCHAR *outname; /* '¥0'で終わる, workのどこかへのポインタ */
	UCHAR *listname; /* '¥0'で終わる, workのどこかへのポインタ */
	UCHAR *dest0, *dest1; /* 出力ファイル(dest0は書き換えられる) */
	UCHAR *list0, *list1; /* 出力ファイル(list0は書き換えられる) */
	UCHAR *err0, *err1; /* コンソールメッセージ(err0は書き換えられる) */
	UCHAR *work0, *work1;
	int errcode;
};

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

struct STR_BIM2BIN {
	UCHAR *cmdlin; /* '¥0'で終わる */
	UCHAR *outname; /* '¥0'で終わる, workのどこかへのポインタ */
	UCHAR *dest0, *dest1; /* 出力ファイル(dest0は書き換えられる) */
	UCHAR *err0, *err1; /* コンソールメッセージ(err0は書き換えられる) */
	UCHAR *work0, *work1;
	int errcode;
};

struct STR_SJISCONV { /* STR_GAS2NASKとコンパチ */
	UCHAR *cmdlin; /* '¥0'で終わる */
	UCHAR *outname; /* '¥0'で終わる, workのどこかへのポインタ */
	UCHAR *dest0, *dest1; /* 出力ファイル(dest0は書き換えられる) */
	UCHAR *err0, *err1; /* コンソールメッセージ(err0は書き換えられる) */
	UCHAR *work0, *work1;
	int errcode;
};

struct CONSOLE {
	int x_size, y_size, x_cur, y_cur, color;
	int ibuf_rptr, ibuf_wptr, ebuf_rptr, ebuf_wptr, curflag;
	unsigned char *cons_buf, *input_buf, *echo_buf;
	struct LIB_TEXTBOX *textbox;
};

static int getsignalw();

struct CONSOLE *copen(const int x_size, const int y_size,
	struct LIB_WINDOW *window, const int x0, const int y0, const int color, const int backcolor);
void cputc(int c, struct CONSOLE *cons);
void cputs(const unsigned char *str, struct CONSOLE *cons);
const int cgetc(struct CONSOLE *cons);
void cgets(unsigned char *str, int n, struct CONSOLE *cons);
const int cungetc(const int c, struct CONSOLE *cons);
void cons_cursoron(struct CONSOLE *cons);
void cons_cursoroff(struct CONSOLE *cons);
const int cons_keyin(struct CONSOLE *cons, int c);
const int cons_readycgetc(struct CONSOLE *cons);
const int cons_readycgets(struct CONSOLE *cons);

#define	cons_readyinput(cons)	cons_readycgets(cons)

void waitready(struct CONSOLE *cons);
int execute(UCHAR *p, struct CONSOLE *cons);
int osamap(UCHAR *f, int slot, int limit, UCHAR *fp, int opt, int rw);
int cc1main(struct STR_CC1MAIN *str_cc1main);
int gas2nask_main(struct STR_GAS2NASK *params);
int naskmain(struct STR_NASKMAIN *params);
int obj2bim_main(struct STR_OBJ2BIM *params);
int bim2bin_main(struct STR_BIM2BIN *params);
int sjisconv_main(struct STR_SJISCONV *params);

#define AUTO_MALLOC		0
#define REWIND_CODE		1

static int *sig_ptr, *signalbox0;
static UCHAR cmdlinbuf0[1024];
static struct CONSOLE *stdin;
static UCHAR osaname[] = "        .   ";

void OsaskMain()
{
	struct LIB_WINDOW *window;
	struct LIB_TEXTBOX *wintitle;
	struct CONSOLE *stdout;

	/* ライブラリの初期化(必ず最初にやらなければならない) */
	lib_init(AUTO_MALLOC);

	/* シグナルボックス初期化 */
    sig_ptr = signalbox0 = lib_opensignalbox(256, AUTO_MALLOC, 0, REWIND_CODE);

	/* ウィンドウのオープン */
	window = lib_openwindow(AUTO_MALLOC, 0x0200, 480, 240);
	wintitle = lib_opentextbox(0x1000, AUTO_MALLOC, 0, 7, 1, 0, 0, window, 0x00c0, 0);

	/* コンソールのオープン */
	stdin = stdout = copen(60, 15, window, 0,  0, 0, 15);

	/* ウィンドウタイトルなどを表示 */
	lib_putstring_ASCII(0x0000, 0, 0, wintitle,  0, 0, "osaskgo");

	/* シグナル定義 */
	lib_definesignal1p0(0x7e - 0x20, 0x0100, 0x20, window, 128 + 0x20); /* ASCIIキャラクター入力 */
	lib_definesignal1p0(0, 0x0100, 0xa0, window, 128 + '¥n'); /* Enter入力 */
	lib_definesignal1p0(0, 0x0100, 0xa1, window, 128 + '¥b'); /* Backspace入力 */
	lib_definesignal0p0(0, 0, 0, 0);

	memtemp0 = malloc(SIZ_MEMTEMP);
	memtemp1 = malloc(SIZ_MEMTEMP);

	/* メインループ */
	for (;;) {
		int i, j;
		UCHAR *p, *fp0, c;
		fp0 = (UCHAR *) lib_readCSd(0x0010);
		lib_unmapmodule(0, 520 * 1024, fp0);
		cputc('>', stdout);
		if (!cons_readyinput(stdin))
			waitready(stdin);
		cgets(cmdlinbuf0, sizeof cmdlinbuf0, stdin);
		if (i = strlen(cmdlinbuf0) > sizeof cmdlinbuf0 - 3) {
			cputs("[ERROR : too long command-line.]¥n", stdout);
			continue;
		}
		for (p = cmdlinbuf0; *p != '¥0' && *p <= ' '; p++);
		for (i = 0; p[i] != '¥0'; i++) {
			if (p[i] == '¥r')
				p[i] = '¥0';
			if (p[i] == '¥n')
				p[i] = '¥0';
		}
		if (*p == '¥0')
			continue;
		if ((i = execute(p, stdout)) > 0) {
			cputs("[ERROR : abnormal termination.]¥n", stdout);
			continue;
		}
		if (i == -1) {
			/* コマンドが見付からない */
			/* pをファイル名に変換してバッチファイルとしてオープン */
			/* そして一行ずつexecuteする */
			i = osamap(p, 0x210, 4096, fp0 + 516 * 1024, 0, 5 /* ReadOnly */);
			if (i == -1) {
				cputs("[ERROR : fail to open batch.]¥n", stdout);
				continue;
			}
			p = fp0 + 516 * 1024;

			/* i:ファイルサイズ, p:ファイルポインタ */
			while (i) {
				if (*p <= ' ') {
					i--;
					cputc(*p++, stdout);
					continue;
				}
				j = 0;
				for (;;) {
					if (i == 0)
						break;
					c = *p++;
					i--;
					cputc(c, stdout);
					if (c == '¥r')
						continue;
					if (c == '¥n')
						break;
					cmdlinbuf0[j++] = c;
					if (j > sizeof cmdlinbuf0 - 3) {
						/* コマンドラインが長すぎる */
						do {
							if (i == 0)
								break;
							c = *p++;
							i--;
							cputc(c, stdout);
						} while (c != '¥n');
						cputs("[ERROR : too long command-line.]¥n", stdout);
						goto batch_error_skip;
					}
				}
				cmdlinbuf0[j] = '¥0';
				if (execute(cmdlinbuf0, stdout)) {
					cputs("[ERROR : abnormal termination.]¥n", stdout);
					goto batch_error_skip;
				}
			}
batch_error_skip:
			;
		}
	}
}

int osaout(UCHAR *f, int size, UCHAR *buf);
int osaoutb(UCHAR *f, int size, UCHAR *buf);

void refresh_static_sub();

extern int _gg00_malloc_base, _gg00_malloc_addr0;

static void refresh_static()
{
	int *sigp = sig_ptr, *sigp0 = signalbox0;
	struct CONSOLE *cons = stdin;
	UCHAR *mt0 = memtemp0, *mt1 = memtemp1;
	int mt0s = mt_size0, mt1s = mt_size1;
	int mallocptr = _gg00_malloc_base;
	int malloc0 = _gg00_malloc_addr0;
	refresh_static_sub();
	sig_ptr = sigp;
	signalbox0 = sigp0;
	stdin = cons;
	memtemp0 = mt0;
	memtemp1 = mt1;
	mt_size0 = mt0s;
	mt_size1 = mt1s;
	_gg00_malloc_base = mallocptr;
	_gg00_malloc_addr0 = malloc0;
	return;
}

int execute(UCHAR *p, struct CONSOLE *cons)
{
	struct STR_CC1MAIN str_cc1main;
	struct STR_GAS2NASK str_gas2nask;
	struct STR_NASKMAIN str_naskmain;
	struct STR_OBJ2BIM str_obj2bim;
	struct STR_BIM2BIN str_bim2bin;
	struct STR_SJISCONV str_sjisconv;
	UCHAR *q, *r, *s, *t, err;
	int i;

	if (p[0] == 'c' && p[1] == 'c' && p[2] == '1' && p[3] <= ' ') {
		/* malloc_size : 5632k (5440) */
		static UCHAR *msg_term[] = {
			"[TERM_WORKOVER]¥n",
			"[TERM_OUTOVER]¥n",
			"[TERM_ERROVER]¥n",
			"[TERM_BUGTRAP]¥n",
			"[TERM_SYSRESOVER]¥n",