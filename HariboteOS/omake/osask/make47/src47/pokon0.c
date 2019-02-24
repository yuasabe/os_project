/* "pokon0.c":アプリケーションラウンチャー  ver.4.7
     copyright(C) 2004 I.Tak., 小柳雅明, 川合秀実
     stack:1m malloc:90k file:4096k */

/* scrollbar & mouse by I.Tak. */
/* シグナル受信通知を*sbp==0のときに一回だけする……ほとんど意味無し
   描画処理を抑えるための第一歩です */
#define PROCESS_ALL_POOLED_SIGNAL	1
#if defined(CHO_OSASK)
	#define LISTX0 16
	#define LISTY0 32
	#define LISTW (8*16)
	#define LISTH (16*8)
	#define SBARX0 (16+16*8)
	#define SBARY0 30
	#define SBARW 16
	#define SBARH (16*8+4)
	#define SBARC 7
	#define SBARBC 8
#elif defined(WIN9X) || defined(WIN31) || defined(NEWSTYLE)
	#define LISTX0 8
	#define LISTY0 32
	#define LISTW (8*16)
	#define LISTH (16*8)
	#define SBARX0 (8+16*8)
	#define SBARY0 48
	#define SBARW 17
	#define SBARH (16*8+2-17*2)
	#define SBARC 8
	#define SBARBC 8
#elif defined(TMENU)
	#define LISTX0 16
	#define LISTY0 32
	#define LISTW (8*16)
	#define LISTH (16*8)
	#define SBARX0 (16+16*8+1)
	#define SBARY0 46
	#define SBARW 15
	#define SBARH (16*8-15*2+1)
	#define SBARC 15
	#define SBARBC 14
#endif

#define LISTX1 (LISTW -1 + LISTX0)
#define LISTY1 (LISTH -1 + LISTY0)
#define SBARX1 (SBARX0+SBARW -1)
#define SBARY1 (SBARY0+SBARH -1)

#include <guigui00.h>
#include <sysgg00.h>
/* sysggは、一般のアプリが利用してはいけないライブラリ
   仕様もかなり流動的 */
#include <stdlib.h>

#include "../pokon0.h"

#define POKON_VERSION "pokon47"

#define POKO_VERSION "Heppoko-shell ¥"poko¥" version 2.7¥n    Copyright (C) 2004 OSASK Project¥n"
#define POKO_PROMPT "¥npoko>"

#define	FILEAREA		(4 * 1024 * 1024)

//static int MALLOC_ADDR;
#define MALLOC_ADDR			j
#define malloc(bytes)		(void *) (MALLOC_ADDR -= ((bytes) + 7) & ‾7)
#define free(addr)			for (;;); /* freeがあっては困るので永久ループ */

/* pokon console error message */
enum {
	ERR_CODE_START = 1,
	ERR_BAD_COMMAND = ERR_CODE_START,
	ERR_ILLEGAL_PARAMETERS,
};

const static char *pokon_error_message[] = {
	"¥nBad command.¥n",
	"¥nIllegal parameter(s).¥n",
};

#define MAX_VIEWERS				30

static struct STR_JOBLIST job;
static struct STR_VIEWER viewers[MAX_VIEWERS] = {
	{ "***", "BEDITC00BIN" },
	{ "TXT", "TEDITC02BIN", 2, 0x7f000001, 42 },
	{ "BMP", "PICTURE0BIN" },
	{ "JPG", "PICTURE0BIN" },
	{ ".B ", "BEDITC00BIN" },
	{ "..S", "RESIZER0BIN" },
	{ ".T ", "TEDITC02BIN", 2, 0x7f000001, 42 },
	{ ".P ", "CMPTEK0 BIN" },
	{ ".U ", "MCOPYC1 BIN" },
	{ "HEL", "HELO    BIN" },
	{ "MML", "MMLPLAY BIN" },
	{ "WRP", "WABA    BIN" },
	{ "SH1", "SHIBAI1 BIN" }
};

typedef unsigned char UCHAR;

struct STR_BANK *banklist;
struct SGG_FILELIST *file;
struct FILEBUF *filebuf;
static int defaultIL = 2;
static struct FILESELWIN *selwin0;
static unsigned char *pselwincount;
static struct VIRTUAL_MODULE_REFERENCE *vmref0;
static int need_wb, readCSd10;
static int sort_mode = DEFAULT_SORT_MODE;
static signed char *pfmode;
static signed char auto_decomp = 1, auto_dearc = 1;
static struct STR_ARCBUF *arcbuf;

//static struct STR_CONSOLE console = { -1, 0, 0, NULL, NULL, NULL, NULL, 0, 0, 0 };
static struct STR_CONSOLE *console;

struct FILEBUF *searchfbuf(struct SGG_FILELIST *fp);
struct FILEBUF *check_wb_cache(struct FILEBUF *fbuf);
struct STR_BANK *run_viewer(struct STR_VIEWER *viewer, struct SGG_FILELIST *fp2);
void runfile(struct SGG_FILELIST *fp, char *name);
struct SGG_FILELIST *searchfid1(const unsigned char *s);
struct FILEBUF *searchfrefbuf();
struct STR_ARCBUF *arcsub_getfree();
struct STR_ARCBUF *arcsub_srchdslt(int dirslot);
void arcsub_unlink(struct STR_ARCBUF *arc);
int arcsub_gethex(unsigned char *p);
unsigned char *arcsub_srchname0(struct STR_ARCBUF *arc, unsigned char *p, unsigned char *name, unsigned int *siz, unsigned int *ofs);
unsigned char *arcsub_srchname1(struct STR_ARCBUF *arc, unsigned char *p, unsigned char *name);
unsigned char *arcsub_srchnum(struct STR_ARCBUF *arc, unsigned char *p, int num);
void arcsub_map(struct STR_ARCBUF *arc, unsigned char *fp0);
void arcsub_unmap(struct STR_ARCBUF *arc, unsigned char 