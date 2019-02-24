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

void lib_execcmd(void *EBX);
void lib_execcmd0(int cmd, ...);
int lib_execcmd1(int ret, int cmd, ...);

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
void l