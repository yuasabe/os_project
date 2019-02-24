#ifndef __GUIGUI00_H
#define __GUIGUI00_H

/* gg00lib+ ver.0.8 */
/* gg00lib9をベースにしている */

struct LIB_WORK {
	int data[256 / 4];
};

struct LIB_WINDOW {
	int data[128 / 4];
};

struct LIB_TEXTBOX {
	int data[64 / 4];
};

struct LIB_SIGHNDLREG {
	int ES, DS, FS, GS;
	int EDI, ESI, EBP, ESP;
	int EBX, EDX, ECX, EAX;
	int EIP, CS, EFLAGS;
};

struct LIB_GRAPHBOX {
	int reserve[64 / 4];
};

struct LIB_LINES1 {
	int x0, y0, dx, dy, length, color;
};

struct LIB_LINES0 {
	int x0, y0, x1, y1, dummy, color;
};

struct LIB_POINTS {
	int x, y, color;
};

//void lib_execcmd(void *EBX);

extern __inline__ void lib_execcmd(void *EBX)
{
	__asm__ (" .byte 154 ¥n¥t"
			 " .byte 0   ¥n¥t"
			 " .byte 0   ¥n¥t"
			 " .byte 0   ¥n¥t"
			 " .byte 0   ¥n¥t"
			 " .byte 199 ¥n¥t"
			 " .byte 0   ¥n¥t"
			 : : "b" (EBX) );
}

void lib_execcmd0(int cmd, ...);
#if 0
extern __inline__ void lib_execcmd0(int cmd, ...)
{
	__asm__ (" movl %%esp,%%ebx ¥n¥t"
			 " .byte 154 ¥n¥t"
			 " .byte 0   ¥n¥t"
			 " .byte 0   ¥n¥t"
			 " .byte 0   ¥n¥t"
			 " .byte 0   ¥n¥t"
			 " .byte 199 ¥n¥t"
			 " .byte 0   ¥n¥t"
			 : : : "%ebx");
}
#endif

int lib_execcmd1(int ret, int cmd, ...);
int lib_execcmd2(int ret, int cmd, ...);

void *malloc(const unsigned int nbytes);

#if 0
	/* 以下の関数はマクロで実現されている(高速化とコンパクト化のため) */
	/* 引数の型などが分かりやすいように、関数型宣言を註釈として残してある */

struct LIB_WORK *lib_init(struct LIB_WORK *work);
void lib_init_nm(struct LIB_WORK *work);
struct LIB_WORK *lib_init_am(struct LIB_WORK *work);
void lib_waitsignal(int opt, int signaldw, int nest);
struct LIB_WINDOW *lib_openwindow(struct LIB_WINDOW *window, int slot, int x_size, int y_size);
void lib_openwindow_nm(struct LIB_WINDOW *window, int slot, int x_size, int y_size);
void lib_openwindow_am(struct LIB_WINDOW *window, int slot, int x_size, int y_size);
struct LIB_TEXTBOX *lib_opentextbox(int opt, struct LIB_TEXTBOX *textbox, int backcolor,
	int x_size, int y_size, int x_pos, int y_pos, struct LIB_WINDOW *window, int charset,
	int init_char);
void lib_opentextbox_nm(int opt, struct LIB_TEXTBOX *textbox, int backcolor, int x_size,
	int y_size, int x_pos, int y_pos, struct LIB_WINDOW *window, int charset, int init_char);
void lib_opentextbox_am(int opt, struct LIB_TEXTBOX *textbox, int backcolor, int x_size,
	int y_size, int x_pos, int y_pos, struct LIB_WINDOW *window, int charset, int init_char);
void lib_waitsignaltime(int opt, int signaldw, int nest, unsigned int time0, unsigned int time1,
	unsigned int time2);
int *lib_opensignalbox(int bytes, int *signalbox, int eos, int rewind);
void lib_opensignalbox_nm(int bytes, int *signalbox, int eos, int rewind);
int *lib_opensignalbox_am(int bytes, int *signalbox, int eos, int rewind);
void lib_definesignal0p0(int opt, int default_assign0, int default_assign1, int default_assign2);
void lib_definesignal1p0(int opt, int default_assign0, int default_assign1,
	struct LIB_WINDOW *default_assign2, int signal);
void lib_opentimer(int slot);
void lib_closetimer(int slot);
void lib_settimertime(int opt, int slot, unsigned int time0, unsigned int time1,
	unsigned int time2);
void lib_settimer(int opt, int slot);
void lib_opensoundtrack(int slot);
void lib_controlfreq(int slot, int freq);
struct LIB_WINDOW *lib_openwindow1(struct LIB_WINDOW *window, int slot, int x_size, int y_size,
	int flags, int base);
void lib_openwindow1_nm(struct LIB_WINDOW *window, int slot, int x_size, int y_size, int flags,
	int base);
void lib_openwindow1_am(struct LIB_WINDOW *window, int slot, int x_size, int y_size, int flags,
	int base);
void lib_closewindow(int opt, struct LIB_WINDOW *window);
void lib_controlwindow(int opt, struct LIB_WINDOW *window);
void lib_close(int opt);
void lib_loadfontset(int opt, int slot, int len, void *font);
void lib_loadfontset0(int opt, int slot);
void lib_makecharset(int opt, int charset, int fontset, int len, int from, int base);
void lib_drawline(int opt, struct LIB_WINDOW *window, int color, int x0, int y0, int x1, int y1);
void lib_closetextbox(int opt, struct LIB_TEXTBOX *textbox);
void lib_mapmodule(int opt, int slot, int attr, int size, void *addr, int ofs);
void lib_unmapmodule(int opt, int size, void *addr);
void lib_initmodulehandle(int opt, int slot);
void lib_putblock1(struct LIB_WINDOW *win, int x, int y, int sx, int sy, int skip, void *p);
struct LIB_GRAPHBOX *lib_opengraphbox(int opt, struct LIB_GRAPHBOX *gbox, int mode, int mode_opt,
	int x_size, int y_size, int x_pos, int y_pos, struct LIB_WINDOW *window);
void lib_opengraphbox_nm(int opt, struct LIB_GRAPHBOX *gbox, int mode, int mode_opt,
	int x_size, int y_size, int x_pos, int y_pos, struct LIB_WINDOW *window);
struct LIB_GRAPHBOX *lib_opengraphbox_am(int opt, struct LIB_GRAPHBOX *gbox, int mode, int mode_opt,
	int x_size, int y_size, int x_pos, int y_pos, struct LIB_WINDOW *window);
void lib_flushgraphbox(int opt, struct LIB_WINDOW *win, int x, int y, int sx, int sy, int skip,
	void *p);
void lib_drawline0(int opt, struct LIB_GRAPHBOX *gbox, int color, int x0, int y0, int x1, int y1);
void lib_drawlines0(int opt, struct LIB_GRAPHBOX *gbox, int x0, int y0, int xsize, int ysize,
	int lines, const struct LIB_LINES1 *ofs);
void lib_convlines(int opt, int lines, struct LIB_LINES0 *src, struct LIB_LINES1 *dest);
void lib_initmodulehandle0(int opt, int slot);
void lib_putblock02001(struct LIB_GRAPHBOX *gbox, void *buf, int vx0, int vy0);
struct LIB_GRAPHBOX *lib_opengraphbox2(int opt, struct LIB_GRAPHBOX *gbox, int mode, int mode_opt,
	int x_bsize, int y_bsize, int x_vsize, int y_vsize, int x_pos, int y_pos,
	struct LIB_WINDOW *window);
void lib_opengraphbox2_nm(int opt, struct LIB_GRAPHBOX *gbox, int mode, int mode_opt,
	int x_bsize, int y_bsize, int x_vsize, int y_vsize, int x_pos, int y_pos,
	struct LIB_WINDOW *window);
struct LIB_GRAPHBOX *lib_opengraphbox2_am(int opt, struct LIB_GRAPHBOX *gbox, int mode, int mode_opt,
	int x_bsize, int y_bsize, int x_vsize, int y_vsize, int x_pos, int y_pos,
	struct LIB_WINDOW *window);
void lib_putblock03001(struct LIB_GRAPHBOX *gbox, void *buf, int vx0, int vy0, void *tbuf,
	int tbuf_skip, int tcol0);
void lib_drawpoints0(int opt, struct LIB_GRAPHBOX *gbox, int x0, int y0, int xsize, int ysize,
	int points, const struct LIB_POINTS *ofs);
void lib_wsjis2gg00jpn0(int len, const char *sjis, int *gg00jpn, int ankbase, int jpnbase);
void lib_loadfontset1(int opt, int slot, int sig);
void lib_drawlines1(int opt, struct LIB_WINDOW *win, int x0, int y0, int xsize, int ysize,
	int lines, const struct LIB_LINES1 *ofs);
void lib_drawpoints1(int opt, struct LIB_WINDOW *win, int x0, int y0, int xsize, int ysize,
	int points, const struct LIB_POINTS *ofs);
void lib_seuc2gg00(int len, const char *seuc, int *gg00, int ankbase, int rightbase);
void lib_resizemodule(int opt, int slot, int newsize, int sig);
void lib_drawpoint0(int opt, struct LIB_GRAPHBOX *gbox, int color, int x, int y);
const int lib_getrandseed();
void lib_putstring0(int opt, int x_pos, int y_pos, struct LIB_TEXTBOX *textbox,
	int color, int bcolor, int len, const int *str);
void lib_putstring1(int opt, int x_pos, int y_pos, struct LIB_TEXTBOX *tbox,
	int col, int bcol, int base, int len, const int *str);

#endif

void lib_putstring_ASCII(int opt, int x_pos, int y_pos, struct LIB_TEXTBOX *textbox, int color,
	int backcolor, const char *str);
void lib_definesignalhandler(void (*lib_signalhandler)(struct LIB_SIGHNDLREG *));
int lib_readCSb(int offset);
int lib_readCSd(int offset);
int lib_readmodulesize(int slot);
void lib_initmodulehandle1(int slot, int num, int sig);
void lib_steppath0(int opt, int slot, const char *name, int sig);
int lib_decodel2d3(int size, int src_ofs, int src_sel, int dest_ofs, int dest_sel);
void lib_putstring_SJIS0(int opt, int x_pos, int y_pos, struct LIB_TEXTBOX *textbox,
	int color, int backcolor, const char *str);
int lib_decodetek0(int size, int src_ofs, int src_sel, int dest_ofs, int dest_sel);
void lib_settimertime2(int opt, int slot0, in