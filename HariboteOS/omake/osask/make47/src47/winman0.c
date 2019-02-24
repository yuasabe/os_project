/* "winman0.c":ぐいぐい仕様ウィンドウマネージャー ver.3.9
		copyright(C) 2004 川合秀実, I.Tak., 小柳雅明, KIYOTO, nikq
    stack:64k malloc:4272k file:4160k */

/* 2004.08.13 スクリーンショットをBMP化
   8,15,16,32bppで正常な画像が取れる……はず by I.Tak. */

/* 2004.09.10 画面関係のglobal変数を構造体にするぞ
 * 2004.09.11 デコーダをDLL使用にするぞ……ぐっpokonが死ぬぞtss=4000 EIP=2f10で
 *            page faultだよ……mmarea分が不足ぐはあ
 *            PICTURE0.BINがSCRNSHOT.BMP (8bpp) をデコードできない(T_T
 */
#define int3 asm(".byte 0xcc")
#include <guigui00.h>
#include <sysgg00.h>
/* sysggは、一般のアプリが利用してはいけないライブラリ
   仕様もかなり流動的 */
#include <stdlib.h>

/* プリプロセッサのオプションで、-DPCATか-DTOWNSを指定すること */

/* ハードコード。gas2naskバージョンアップ待ち */
#define lib_readCSd(ofs)  ({ int _ret;¥
 __asm__(".byte 0x2e¥n movl %1, %0" : "=r" (_ret) : "m" (*(int*)(ofs)));¥
 _ret;})

static inline void call_dll0207_48(int *env, int *cmd)
{
  __asm__ 
    ("pushl %1¥n"
     "pushl %0¥n"
     ".byte 0x9a¥n"
     ".long 0x48¥n"
     ".word 0x207¥n"		/* "lcall $0x0207,$0x00000048¥n" */
     "addl $8, %%esp"		/* これを最適化できないのが残念 */
     : : "g" (env), "g" (cmd), "m" (*cmd) :"%ecx","%edx","memory");
}

static void call_dll0207_48i(void *env, int cmd, ...)
{
  __asm__
    ("pushl %1¥n"
     "pushl %0¥n"
     ".byte 0x9a¥n"
     ".long 0x48¥n"
     ".word 0x207¥n"		/* "lcall $0x0207,$0x00000048¥n" */
     "addl $8, %%esp"
     : : "g" (env), "g" (&cmd), "m" (cmd) :"%ecx","%edx","memory");
}

#if (defined(TOWNS))
	/* マウス存在flag (FMR<<4)|(R<<2)|L, 0:none,1:mouse,2:pad,3:6pad */
	static char townsmouse = 0x04;/* right mouse only */
	#if (defined(VMODE) || defined(CLGD543X))
	  #undef TWVSW        /* 1024でないとインターレースできない */
	  #define TWVSW 1024  /* インターレースしないなら問題ないが
                               * CLGDには1024のパラメータしか作ってない */
	#endif
	#define FMRMOUSE 1    /* FMRMOUSEに対応 2004.04.12 by I.Tak. */
	#define KROM 1        /* font file が無いときにROMで代用する 2004.04.12 by I.Tak. */
	#if (!defined(TWVSW))
		#define	TWVSW		1024
	#endif
	#define DEFAULTCOLDEP 1
#else
	#define DEFAULTCOLDEP 0
#endif

/* NEC PC-98 では変化しない変数にconstをつけ, 最適化を期待する 
 * 変数そのものは無くならないが (ポインタ渡しがありうるため),
 * 参照回数は減るはずだ……こんなことする意味あるのか */
#if (defined(PCAT)) || (defined(TOWNS))
	#define CONST98
#elif (defined(NEC98))
	#define CONST98 const
#endif

typedef struct str_screen {
  CONST98 unsigned char vbecoldep, driver;
  unsigned char wallpaper_name[13];
  unsigned char wallpaper_exist, *wallpaper;
  CONST98 int x2, y2;
  int backcolors[5];		/* default color array for bpp 4,8,16,24,32 */
  CONST98 int backcolor;	/* in use */
  int moveunits[5];
  CONST98 int moveunit;
} SCREEN;

static SCREEN screen = {
  DEFAULTCOLDEP, 0, "OSASK   .BMP", 0, (char*)0,
  640,400,
#if (defined(WIN31))
  {8, 8, 0xc618, 0x00c0c0c0, 0x00c0c0c0}, 8,
#else
  {6, 6, 0x0410, 0x00008080, 0x00008080}, 6,
#endif
  {8, 4, 2, 4, 1}, 8
};

#if (defined(WIN9X))
	#define	RESERVELINE0		   0
	#define	RESERVELINE1		  28
	#if (defined(PCAT) || defined(TOWNS))
		#define TIMEX				-192	/* 8の倍数 */
		#define TIMEY				 -20
		#define TIMEC				   0
		#define TIMEBC				   8
		#define ERRMSGX				  80
		#define ERRMSGY				 -20
		#define ERRMSGC				   0
		#define ERRMSGBC			   8
		#define ERRMSGCC			   8
	#endif
#elif (defined(TMENU))
	#define	RESERVELINE0		  20
	#define	RESERVELINE1		  28
	#if (defined(PCAT)) || (defined(TOWNS))
		#define TIMEX				-240
		#define TIMEY				   2
		#define TIMEC				   0
		#define TIMEBC				   7
		#define ERRMSGX				 104
		#define ERRMSGY				 -20
		#define ERRMSGC				   0
		#define ERRMSGBC			   7
		#define ERRMSGCC			   7
	#endif
#elif (defined(CHO_OSASK))
	#define	RESERVELINE0		   0
	#define	RESERVELINE1		  20
	#if (defined(PCAT)) || (defined(TOWNS))
		#define TIMEX				-192	/* 8の倍数 */
		#define TIMEY				 -16
		#define TIMEC				  15
		#define TIMEBC				   7
		#define ERRMSGX				   8
		#define ERRMSGY				 -16
		#define ERRMSGC				   0
		#define ERRMSGBC			   7
		#define ERRMSGCC			   7
	#endif
#elif (defined(NEWSTYLE))
	#define	RESERVELINE0		   0
	#define	RESERVELINE1		   0
#elif (defined(WIN31))
	#define	RESERVELINE0		   0
	#define	RESERVELINE1		   0
#endif

#define WALLPAPERMAXSIZE	(4 * 1024 * 1024)
#define	SCRNSHOTMAXSIZ		2048 * 1024
#define MAXWINDEF			16

//static int MALLOC_ADDR;
#define MALLOC_ADDR			j
#define malloc(bytes)		(void *) (MALLOC_ADDR -= ((bytes) + 7) & ‾7)
#define free(addr)			for (;;); /* freeがあっては困るので永久ループ */

#define	AUTO_MALLOC			   0
#define NULL				   0
#define	MAX_WINDOWS		 	  80		// 8.1KB
#define JOBLIST_SIZE		 256		// 1KB
#define	MAX_SOUNDTRACK		  16		// 0.5KB
#define	DEFSIGBUFSIZ		2048
#define	MOSWINSIGS			 128		/* 4KB */

#define WINFLG_MUSTREDRAW		0x80000000	/* bit31 */
#define WINFLG_MUSTREDRAWDIF	0x40000000	/* bit30 */
#define WINFLG_OVERRIDEDISABLED	0x01000000	/* bit24 */
#define WINFLG_NOWINDOW			0x00000400	/* bit10 */
#define	WINFLG_WAITREDRAW		0x00000200	/* bit 9 */
#define	WINFLG_WAITDISABLE		0x00000100	/* bit 8 */

//	#define	DEBUG	1

#define	sgg_debug00(opt, bytes, reserve, src_ofs, src_sel, dest_ofs, dest_sel) ¥
	sgg_execcmd0(0x8010, (int) (opt), (int) (bytes), (int) (reserve), ¥
	(int) (src_ofs), (int) (src_sel), (int) (dest_ofs), (int) (dest_sel), ¥
	0x0000)

struct DEFINESIGNAL { // 32bytes
	int win, opt, dev, cod, len, sig[3];
};

struct WM0_WINDOW {	// total 108bytes
//	struct DEFINESIGNAL defsig[29]; // 928bytes
	struct SGG_WINDOW sgg; // 68bytes
//	struct DEFINESIGNAL *ds1;
	int condition, x0, y0, x1, y1, job_flag0, job_flag1;
	int tx0, ty0, tx1, ty1; /* ウィンドウ移動のためのタブ */
	int flags;
	struct WM0_WINDOW *up, *down;
};

struct SOUNDTRACK {
	int sigbox, sigbase, slot, reserve[5];
};

struct MOSWINSIG { /* 32bytes */
	int flags, sig[6];
	struct WM0_WINDOW *win;
	/* flagsの下位4bitはlen */
	/* sig[4], sig[5]は場合によってはx0, y0 */
};

static struct STR_JOB {
	int now, movewin4_ready, fontflag;
	int *list, free, *rp, *wp;
	int count, int0;
	int movewin_x, movewin_y, movewin_x0, movewin_y0; /* 移動先と移動元 */
	int readCSd10;
	void (*func)(int, int);
	struct WM0_WINDOW *win;
	int fonttss, sig;
} job = { 0 /* now */, 0 /* movewin4_ready */, /* fontflag */ };

#if (defined(PCAT))
	struct STR_VBEMODE { /* 16bytes */
		unsigned int linear, linebytes;
		unsigned short x_res, y_res, mode;
		unsigned char flag, dummy;
	};
	static char flag_vbe2 = 0;
	static struct STR_VBEMODE vbelist[128];
	static int f3mode, f4mode;
	static unsigned int vbeoverride[3];
#elif (defined(TOWNS)) && (defined(CLGD543X))
	static int pf13mode = -1; /* (^^; */
#endif

struct WM0_WINDOW *window, *top = NULL, *unuse = NULL, *iactive = NULL, *pokon0 = NULL;
int mx = 0x80000000, my = 1, mbutton = 0, mxx = 1;
int fromboot = 0, winmanerr_time = 0;
struct {
	int x, y;
} windef[MAXWINDEF];
int mouseaccel = 2;	/* これより大きいと加速度倍増 */
int mousescale = 3; /* 加速スケールにしよう */
struct SOUNDTRACK *sndtrk_buf, *sndtrk_active = NULL;
struct DEFINESIGNAL *defsigbuf;
struct MOSWINSIG *moswinsig;
int mws_sensitivecount = 0;
struct WM0_WINDOW *mws_mousewin = NULL;

void init_screen(const int x, const int y);
struct WM0_WINDOW *handle2window(const int handle);
void chain_unuse(struct WM0_WINDOW *win);
struct WM0_WINDOW *get_unuse();
void mousesignal(const unsigned int header, int dx, int dy);
void mousesignal_sub0(int _mx, int _my);
void mousesignal_sub1(int _mx, int _my);

int writejob_n(int n, int p0,...);
void runjobnext();
void job_openwin0(struct WM0_WINDOW *win);
void redirect_input(struct WM0_WINDOW *win);
void job_activewin0(struct WM0_WINDOW *win);
void job_movewin0(struct WM0_WINDOW *win);
void job_movewin1(const int cmd, const int handle);
void job_movewin2();
void job_movewin3();
void job_movewin4(int sig);
void job_movewin4m(int x, int y);
int job_movewin5();
void job_movewin6(const int cmd, const int handle);
void job_closewin0(struct WM0_WINDOW *win0);
void job_general1();
void job_general2(const int cmd, const int handle);
void job_openvgadriver(const int drv);
void job_setvgamode0(const int mode);
void job_setvgamode1(const int cmd, const int handle);
void job_setvgamode2();
void job_setvgamode3(const int sig, const int result);
void job_loadfont0(int fonttype, int tss, int sig);
void job_loadfont1(int flag, int dmy);
void job_loadfont2();
void job_loadfont3(int flag, int dmy);
void moswinsig_flagset();
struct WM0_WINDOW *searchwin(int x, int y);
int lock_v86();
void unlock_v86();

#if (defined(PCAT) || defined(TOWNS))
	void job_savevram0(void);
	void job_savevram1(int flag, int dmy);
	void job_savevram2(int flag, int dmy);
#endif
void job_openwallpaper(void);
void job_loadwallpaper(int flag, int dmy);
void putwallpaper(int x0, int y0, int x1, int y1);

#if (defined(PCAT))
	void job_vesacheck0();
	void job_vesacheck1(int sig, int result);
	void job_vesacheck2();
#endif

//void free_sndtrk(struct SOUNDTRACK *sndtrk);

struct SOUNDTRACK *alloc_sndtrk();
void send_signal2dw(const int sigbox, const int data0, const int data1);
void send_signal3dw(const int sigbox, const int data0, const int data1, const int data2);
void send_signal4dw(const int sigbox, const int data0, const int data1, const int data2, int data3);

void lib_drawletters_ASCII(const int opt, const int win, const int charset, const int x0, const int y0,
	const int color, const int backcolor, const char *str);
void debug_bin2hex(unsigned int i, unsigned char *s);

void sgg_wm0_definesignal3(const int opt, const int device, int keycode,
	const int sig0, const int sig1, const int sig2);
void sgg_wm0_definesignal3sub(const int keycode);
void sgg_wm0_definesignal3sub2(const int rawcode, const int shiftmap);
void sgg_wm0_definesignal3sub3(int rawcode, const int shiftmap);
// void sgg_wm0_definesignal3com();

void write_time();
void winmanerr(const unsigned char *s);
void winmanerr_clr();

/* キー操作：
      F9:一番下のウィンドウへ
      F10:上から２番目のウィンドウを選択
      F11:ウィンドウの移動
      F12:ウィンドウクローズ */

//int allclose = 0;

static int tapisigvec[] = {
	0x006c, 6 * 4, 0x011c /* cmd fot tapi */, 0, 0, 0x0000, 0x0000
};

#define SYSTEM_TIMER		0x01c0
#define SIG_WRITE_TIME		0x0060

#define	NOSHIFT		0	/* 0x0000c070 */
#define	SHIFT		1	/* 0x0010c070 */
#define	IGSHIFT		2	/* 0x0000c060 */
#define	CAPLKON		3	/* 0x0004c074, 0x0010c074 */
#define	CAPLKOF		4	/* 0x0000c074, 0x0014c074 */
#define	NUMLKON		5	/* 0x0002c072, 0x0010c072 */
#define	NUMLKOF		6	/* 0x0000c072, 0x0012c072 */
#define ALT			7   /* 0x0040c070 */

/* 入力方法テーブル(2通りまでサポート) */
static struct KEYTABLE {
	unsigned char rawcode0, shifttype0;
	unsigned char rawcode1, shifttype1;
} keybindtable[] = {
	#if (defined(PCAT))
		{ 0x39, IGSHIFT, 0xff, 0xff    } /* ' ' */,
		{ 0x02, SHIFT,   0xff, 0xff    } /* '!' */,
		{ 0x03, SHIFT,   0xff, 0xff    } /* '¥x22' */,
		{ 0x04, SHIFT,   0xff, 0xff    } /* '#' */,
		{ 0x05, SHIFT,   0xff, 0xff    } /* '%' */,
		{ 0x06, SHIFT,   0xff, 0xff    } /* '$' */,
		{ 0x07, SHIFT,   0xff, 0xff    } /* '&' */,
		{ 0x08, SHIFT,   0xff, 0xff    } /* '¥x27' */,
		{ 0x09, SHIFT,   0xff, 0xff    } /* '(' */,
		{ 0x0a, SHIFT,   0xff, 0xff    } /* ')' */,
		{ 0x28, SHIFT,   0x37, IGSHIFT } /* '*' */,
		{ 0x27, SHIFT,   0x4e, IGSHIFT } /* '+' */,
		{ 0x33, NOSHIFT, 0xff, 0xff    } /* ',' */,
		{ 0x0c, NOSHIFT, 0x4a, IGSHIFT } /* '-' */,
		{ 0x34, NOSHIFT, 0x53, NUMLKON } /* '.' */,
		{ 0x35, NOSHIFT, 0xb5, IGSHIFT } /* '/' */,
		{ 0x0b, NOSHIFT, 0x52, NUMLKON } /* '0' */,
		{ 0x02, NOSHIFT, 0x4f, NUMLKON } /* '1' */,
		{ 0x03, NOSHIFT, 0x50, NUMLKON } /* '2' */,
		{ 0x04, NOSHIFT, 0x51, NUMLKON } /* '3' */,
		{ 0x05, NOSHIFT, 0x4b, NUMLKON } /* '4' */,
		{ 0x06, NOSHIFT, 0x4c, NUMLKON } /* '5' */,
		{ 0x07, NOSHIFT, 0x4d, NUMLKON } /* '6' */,
		{ 0x08, NOSHIFT, 0x47, NUMLKON } /* '7' */,
		{ 0x09, NOSHIFT, 0x48, NUMLKON } /* '8' */,
		{ 0x0a, NOSHIFT, 0x49, NUMLKON } /* '9' */,
		{ 0x28, NOSHIFT, 0xff, 0xff    } /* ':' */,
		{ 0x27, NOSHIFT, 0xff, 0xff    } /* ';' */,
		{ 0x33, SHIFT,   0xff, 0xff    } /* '<' */,
		{ 0x0c, SHIFT,   0xff, 0xff    } /* '=' */,
		{ 0x34, SHIFT,   0xff, 0xff    } /* '>' */,
		{ 0x35, SHIFT,   0xff, 0xff    } /* '?' */,
		{ 0x1a, NOSHIFT, 0xff, 0xff    } /* '@' */,
		{ 0x1e, CAPLKON, 0xff, 0xff    } /* 'A' */,
		{ 0x30, CAPLKON, 0xff, 0xff    } /* 'B' */,
		{ 0x2e, CAPLKON, 0xff, 0xff    } /* 'C' */,
		{ 0x20, CAPLKON, 0xff, 0xff    } /* 'D' */,
		{ 0x12, CAPLKON, 0xff, 0xff    } /* 'E' */,
		{ 0x21, CAPLKON, 0xff, 0xff    } /* 'F' */,
		{ 0x22, CAPLKON, 0xff, 0xff    } /* 'G' */,
		{ 0x23, CAPLKON, 0xff, 0xff    } /* 'H' */,
		{ 0x17, CAPLKON, 0xff, 0xff    } /* 'I' */,
		{ 0x24, CAPLKON, 0xff, 0xff    } /* 'J' */,
		{ 0x25, CAPLKON, 0xff, 0xff    } /* 'K' */,
		{ 0x26, CAPLKON, 0xff, 0xff    } /* 'L' */,
		{ 0x32, CAPLKON, 0xff, 0xff    } /* 'M' */,
		{ 0x31, CAPLKON, 0xff, 0xff    } /* 'N' */,
		{ 0x18, CAPLKON, 0xff, 0xff    } /* 'O' */,
		{ 0x19, CAPLKON, 0xff, 0xff    } /* 'P' */,
		{ 0x10, CAPLKON, 0xff, 0xff    } /* 'Q' */,
		{ 0x13, CAPLKON, 0xff, 0xff    } /* 'R' */,
		{ 0x1f, CAPLKON, 0xff, 0xff    } /* 'S' */,
		{ 0x14, CAPLKON, 0xff, 0xff    } /* 'T' */,
		{ 0x16, CAPLKON, 0xff, 0xff    } /* 'U' */,
		{ 0x2f, CAPLKON, 0xff, 0xff    } /* 'V' */,
		{ 0x11, CAPLKON, 0xff, 0xff    } /* 'W' */,
		{ 0x2d, CAPLKON, 0xff, 0xff    } /* 'X' */,
		{ 0x15, CAPLKON, 0xff, 0xff    } /* 'Y' */,
		{ 0x2c, CAPLKON, 0xff, 0xff    } /* 'Z' */,
		{ 0x1b, NOSHIFT, 0xff, 0xff    } /* '[' */,
		{ 0x7d, NOSHIFT, 0x73, NOSHIFT } /* '¥' */,
		{ 0x2b, NOSHIFT, 0xff, 0xff    } /* ']' */,
		{ 0x0d, NOSHIFT, 0xff, 0xff    } /* '^' */,
		{ 0x73, SHIFT,   0xff, 0xff    } /* '_' */,
		{ 0x1a, SHIFT,   0xff, 0xff    } /* '`' */,
		{ 0x1e, CAPLKOF, 0xff, 0xff    } /* 'a' */,
		{ 0x30, CAPLKOF, 0xff, 0xff    } /* 'b' */,
		{ 0x2e, CAPLKOF, 0xff, 0xff    } /* 'c' */,
		{ 0x20, CAPLKOF, 0xff, 0xff    } /* 'd' */,
		{ 0x12, CAPLKOF, 0xff, 0xff    } /* 'e' */,
		{ 0x21, CAPLKOF, 0xff, 0xff    } /* 'f' */,
		{ 0x22, CAPLKOF, 0xff, 0xff    } /* 'g' */,
		{ 0x23, CAPLKOF, 0xff, 0xff    } /* 'h' */,
		{ 0x17, CAPLKOF, 0xff, 0xff    } /* 'i' */,
		{ 0x24, CAPLKOF, 0xff, 0xff    } /* 'j' */,
		{ 0x25, CAPLKOF, 0xff, 0xff    } /* 'k' */,
		{ 0x26, CAPLKOF, 0xff, 0xff    } /* 'l' */,
		{ 0x32, CAPLKOF, 0xff, 0xff    } /* 'm' */,
		{ 0x31, CAPLKOF, 0xff, 0xff    } /* 'n' */,
		{ 0x18, CAPLKOF, 0xff, 0xff    } /* 'o' */,
		{ 0x19, CAPLKOF, 0xff, 0xff    } /* 'p' */,
		{ 0x10, CAPLKOF, 0xff, 0xff    } /* 'q' */,
		{ 0x13, CAPLKOF, 0xff, 0xff    } /* 'r' */,
		{ 0x1f, CAPLKOF, 0xff, 0xff    } /* 's' */,
		{ 0x14, CAPLKOF, 0xff, 0xff    } /* 't' */,
		{ 0x16, CAPLKOF, 0xff, 0xff    } /* 'u' */,
		{ 0x2f, CAPLKOF, 0xff, 0xff    } /* 'v' */,
		{ 0x11, CAPLKOF, 0xff, 0xff    } /* 'w' */,
		{ 0x2d, CAPLKOF, 0xff, 0xff    } /* 'x' */,
		{ 0x15, CAPLKOF, 0xff, 0xff    } /* 'y' */,
		{ 0x2c, CAPLKOF, 0xff, 0xff    } /* 'z' */,
		{ 0x1b, SHIFT,   0xff, 0xff    } /* '{' */,
		{ 0x7d, SHIFT,   0xff, 0xff    } /* '|' */,
		{ 0x2b, SHIFT,   0xff, 0xff    } /* '}' */,
		{ 0x0d, SHIFT,   0x0b, SHIFT   } /* '‾' */,
		{ 0xff, 0xff,    0xff, 0xff    } /* '¥x7f' */,
		{ 0x01, NOSHIFT, 0xff, 0xff    } /* Esc */,
		{ 0x3b, NOSHIFT, 0xff, 0xff    } /* F1 */,
		{ 0x3c, NOSHIFT, 0xff, 0xff    } /* F2 */,
		{ 0x3d, NOSHIFT, 0xff, 0xff    } /* F3 */,
		{ 0x3e, NOSHIFT, 0xff, 0xff    } /* F4 */,
		{ 0x3f, NOSHIFT, 0xff, 0xff    } /* F5 */,
		{ 0x40, NOSHIFT, 0xff, 0xff    } /* F6 */,
		{ 0x41, NOSHIFT, 0xff, 0xff    } /* F7 */,
		{ 0x42, NOSHIFT, 0xff, 0xff    } /* F8 */,
		{ 0x43, NOSHIFT, 0xff, 0xff    } /* F9 */,
		{ 0x44, NOSHIFT, 0xff, 0xff    } /* F10 */,
		{ 0x57, NOSHIFT, 0xff, 0xff    } /* F11 */,
		{ 0x58, NOSHIFT, 0xff, 0xff    } /* F12 */,
		{ 0xff, 0xff,    0xff, 0xff    } /* F13 */,
		{ 0xff, 0xff,    0xff, 0xff    } /* F14 */,
		{ 0xff, 0xff,    0xff, 0xff    } /* F15 */,
		{ 0xff, 0xff,    0xff, 0xff    } /* F16 */,
		{ 0xff, 0xff,    0xff, 0xff    } /* F17 */,
		{ 0xff, 0xff,    0xff, 0xff    } /* F18 */,
		{ 0xff, 0xff,    0xff, 0xff    } /* F19 */,
		{ 0xff, 0xff,    0xff, 0xff    } /* F20 */,
		{ 0xff, 0xff,    0xff, 0xff    } /* '¥x95' */,
		{ 0xff, 0xff,    0xff, 0xff    } /* '¥x96' */,
		{ 0xff, 0xff,    0xff, 0xff    } /* '¥x97' */,
		{ 0xff, 0xff,    0xff, 0xff    } /* '¥x98' */,
		{ 0xff, 0xff,    0xff, 0xff    } /* '¥x99' */,
		{ 0xff, 0xff,    0xff, 0xff    } /* '¥x9a' */,
		{ 0xff, 0xff,    0xff, 0xff    } /* '¥x9b' */,
		{ 0xff, 0xff,    0xff, 0xff    } /* '¥x9c' */,
		{ 0xff, 0xff,    0xff, 0xff    } /* '¥x9d' */,
		{ 0xff, 0xff,    0xff, 0xff    } /* '¥x9e' */,
		{ 0xff, 0xff,    0xff, 0xff    } /* '¥x9f' */,
		{ 0x1c, IGSHIFT, 0x9c, IGSHIFT } /* Enter */,
		{ 0x0e, IGSHIFT, 0xff, 0xff    } /* BackSpace */,
		{ 0x0f, NOSHIFT, 0xff, 0xff    } /* Tab */,
		{ 0xff, 0xff,    0xff, 0xff    } /* '¥xa3' */,
		{ 0xd2, NOSHIFT, 0x52, NUMLKOF } /* Insert */,
		{ 0xd3, NOSHIFT, 0x53, NUMLKOF } /* Delete */,
		{ 0xc7, NOSHIFT, 0x47, NUMLKOF } /* Home */,
		{ 0xcf, NOSHIFT, 0x4f, NUMLKOF } /* End */,
		{ 0xc9, NOSHIFT, 0x49, NUMLKOF } /* PageUp */,
		{ 0xd1, NOSHIFT, 0x51, NUMLKOF } /* PageDown */,
		{ 0xff, 0xff,    0xff, 0xff    } /* '¥xaa' */,
		{ 0xff, 0xff,    0xff, 0xff    } /* '¥xab' */,
		{ 0xcb, NOSHIFT, 0x4b, NUMLKOF } /* Left */,
		{ 0xcd, NOSHIFT, 0x4d, NUMLKOF } /* Right */,
		{ 0xc8, NOSHIFT, 0x48, NUMLKOF } /* Up */,
		{ 0xd0, NOSHIFT, 0x50, NUMLKOF } /* Down */,
		{ 0x46, NOSHIFT, 0xff, 0xff    } /* ScrollLock */,
		{ 0x45, NOSHIFT, 0xff, 0xff    } /* NumLock */,
		{ 0x3a, SHIFT,   0xff, 0xff    } /* CapsLock */,
		{ 0xff, 0xff,    0xff, 0xff    } /* '¥xb3' */,
		{ 0x2a, 0xfe,    0x36, 0xfe    } /* Shift */,
		{ 0x1d, 0xfe,    0x9d, 0xfe    } /* Ctrl */,
		{ 0x38, 0xfe,    0xb8, 0xfe    } /* Alt */,
		{ 0xff, 0xff,    0xff, 0xff    } /* '¥xb7' */,
		{ 0xb7, NOSHIFT, 0xff, 0xff    } /* PrintScreen */,
		{ 0xff, NOSHIFT, 0xff, 0xff    } /* Pause */,
		{ 0xc6, NOSHIFT, 0xff, 0xff    } /* Break(ALT?) */,
		{ 0x54, NOSHIFT, 0xff, 0xff    } /* SysRq(ALT?) */,
		{ 0xff, 0xff,    0xff, 0xff    } /* '¥xbc' */,
		{ 0xff, 0xff,    0xff, 0xff    } /* '¥xbd' */,
		{ 0xff, 0xff,    0xff, 0xff    } /* '¥xbe' */,
		{ 0xff, 0xff,    0xff, 0xff    } /* '¥xbf' */,
		{ 0xdb, NOSHIFT, 0xdc, NOSHIFT } /* Windows */,
		{ 0xdd, NOSHIFT, 0xff, 0xff    } /* Menu */,
		{ 0xff, 0xff,    0xff, 0xff    } /* Power */,
		{ 0xff, 0xff,    0xff, 0xff    } /* Sleep */,
		{ 0xff, 0xff,    0xff, 0xff    } /* Wake */,
		{ 0xff, 0xff,    0xff, 0xff    } /* '¥xc5' */,
		{ 0xff, 0xff,    0xff, 0xff    } /* '¥xc6' */,
		{ 0xff, 0xff,    0xff, 0xff    } /* '¥xc7' */,
		{ 0xff, 0xff,    0xff, 0xff    } /* '¥xc8' */,
		{ 0xff, 0xff,    0xff, 0xff    } /* '¥xc9' */,
		{ 0xff, 0xff,    0xff, 0xff    } /* '¥xca' */,
		{ 0xff, 0xff,    0xff, 0xff    } /* '¥xcb' */,
		{ 0xff, 0xff,    0xff, 0xff    } /* '¥xcc' */,
		{ 0xff, 0xff,    0xff, 0xff    } /* '¥xcd' */,
		{ 0xff, 0xff,    0xff, 0xff    } /* '¥xce' */,
		{ 0xff, 0xff,    0xff, 0xff    } /* '¥xcf' */,
		{ 0x29, NOSHIFT, 0xff, 0xff    } /* Zenkaku */,
		{ 0x7b, NOSHIFT, 0xff, 0xff    } /* Muhenkan */,
		{ 0x79, NOSHIFT, 0xff, 0xff    } /* Henkan */,
		{ 0x70, NOSHIFT, 0xff, 0xff    } /* Hiragana */,
		{ 0x70, SHIFT,   0xff, 0xff    } /* Katakana */,
		{ 0xff, 0xff,    0xff, 0xff    } /* '¥xd5' */,
		{ 0xff, 0xff,    0xff, 0xff    } /* '¥xd6' */,
		{ 0xff, 0xff,    0xff, 0xff    } /* '¥xd7' */,
		{ 0xff, 0xff,    0xff, 0xff    } /* '¥xd8' */,
		{ 0xff, 0xff,    0xff, 0xff    } /* '¥xd9' */,
		{ 0xff, 0xff,    0xff, 0xff    } /* '¥xda' */,
		{ 0xff, 0xff,    0xff, 0xff    } /* '¥xdb' */,
		{ 0xff, 0xff,    0xff, 0xff    } /* '¥xdc' */,
		{ 0xff, 0xff,    0xff, 0xff    } /* '¥xdd' */,
		{ 0xff, 0xff,    0xff, 0xff    } /* '¥xde' */,
		{ 0xff, 0xff,    0xff, 0xff    } /* '¥xdf' */,
		{ 0xff, 0xff,    0xff, 0xff    } /* '¥xe0' */,