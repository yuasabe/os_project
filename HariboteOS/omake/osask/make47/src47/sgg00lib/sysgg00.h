#ifndef __SYSGG00_H
#define __SYSGG00_H

struct SGG_WORK {
	int data[256 / 4];
};

struct SGG_FILELIST {
	int id;
	unsigned char name[11], type;
	int size;
};

struct SGG_WINDOW {
	int image[32 / 4];
	int handle;
};

#if 1

#define	WINSTR_STATUS		 0 / 4
#define	WINSTR_USERID		 4 / 4
#define	WINSTR_SIGNALBASE	 8 / 4
#define	WINSTR_SIGNALEBOX	12 / 4
#define	WINSTR_XSIZE		16 / 4
#define	WINSTR_YSIZE		20 / 4
#define	WINSTR_X0			24 / 4
#define	WINSTR_Y0			28 / 4

#endif

void sgg_execcmd0(const int cmd, ...);
const int sgg_execcmd1(const int ret, const int cmd, ...);
void sgg_execcmd(void *EBX);

#define	sgg_init(work)		(work)

#if 0

#define	sgg_init(work) ¥
	(struct SGG_WORK *) sgg_execcmd1(1 * 4 + 12, 0x0010, ¥
	(work) ? (void *) (work) : malloc(sizeof (struct SGG_WORK)), 0x0000)

#endif

#if 1

#define sgg_wm0s_movewindow(window, x, y) ¥
	sgg_execcmd0(0x0020, 0x80000000 + 5, ¥
	((struct SGG_WINDOW *) (window))->image[WINSTR_SIGNALEBOX] | 4, ¥
	((struct SGG_WINDOW *) (window))->image[WINSTR_SIGNALBASE] + 0, ¥
	((struct SGG_WINDOW *) (window))->image[WINSTR_USERID], ¥
	((struct SGG_WINDOW *) (window))->image[WINSTR_X0] = (int) (x), ¥
	((struct SGG_WINDOW *) (window))->image[WINSTR_Y0] = (int) (y), ¥
	0x002c, 0, ((struct SGG_WINDOW *) (window))->handle, ¥
	((struct SGG_WINDOW *) (window))->image, 0x0c, 0x0000)

#define sgg_wm0s_setstatus(window, status) ¥
	sgg_execcmd0(0x0020, 0x80000000 + 4, ¥
	((struct SGG_WINDOW *) (window))->image[WINSTR_SIGNALEBOX] | 3, ¥
	((struct SGG_WINDOW *) (window))->image[WINSTR_SIGNALBASE] + 1, ¥
	((struct SGG_WINDOW *) (window))->image[WINSTR_USERID], ¥
	(int) (status), 0x0000)

#define sgg_wm0s_accessenable(window) ¥
	sgg_execcmd0(0x0020, 0x80000000 + 3, ¥
	((struct SGG_WINDOW *) (window))->image[WINSTR_SIGNALEBOX] | 2, ¥
	((struct SGG_WINDOW *) (window))->image[WINSTR_SIGNALBASE] + 2, ¥
	((struct SGG_WINDOW *) (window))->image[WINSTR_USERID], 0x0000)

#define sgg_wm0s_accessdisable(window) ¥
	sgg_execcmd0(0x0020, 0x80000000 + 3, ¥
	((struct SGG_WINDOW *) (window))->image[WINSTR_SIGNALEBOX] | 2, ¥
	((struct SGG_WINDOW *) (window))->image[WINSTR_SIGNALBASE] + 4, ¥
	((struct SGG_WINDOW *) (window))->image[WINSTR_USERID], 0x0000)

#define sgg_wm0s_redraw(window) ¥
	sgg_execcmd0(0x0020, 0x80000000 + 3, ¥
	((struct SGG_WINDOW *) (window))->image[WINSTR_SIGNALEBOX] | 2, ¥
	((struct SGG_WINDOW *) (window))->image[WINSTR_SIGNALBASE] + 8, ¥
	((struct SGG_WINDOW *) (window))->image[WINSTR_USERID], 0x0000)

#define sgg_wm0s_redrawdif(window) ¥
	sgg_execcmd0(0x0020, 0x80000000 + 3, ¥
	((struct SGG_WINDOW *) (window))->image[WINSTR_SIGNALEBOX] | 2, ¥
	((struct SGG_WINDOW *) (window))->image[WINSTR_SIGNALBASE] + 9, ¥
	((struct SGG_WINDOW *) (window))->image[WINSTR_USERID], 0x0000)

#define sgg_wm0_win2sbox(window) ¥
	(((struct SGG_WINDOW *) (window))->image[WINSTR_SIGNALEBOX])

#define sgg_wm0_winsizex(window) ¥
	(((struct SGG_WINDOW *) (window))->image[WINSTR_XSIZE])

#define sgg_wm0_winsizey(window) ¥
	(((struct SGG_WINDOW *) (window))->image[WINSTR_YSIZE])

#define sgg_wm0s_close(window) ¥
	sgg_execcmd0(0x0020, 0x80000000 + 3, ¥
	((struct SGG_WINDOW *) (window))->image[WINSTR_SIGNALEBOX] | 2, ¥
	((struct SGG_WINDOW *) (window))->image[WINSTR_SIGNALBASE] + 3, ¥
	((struct SGG_WINDOW *) (window))->image[WINSTR_USERID], 0x0000)

#define sgg_wm0s_windowclosed(window) ¥
	sgg_execcmd0(0x0020, 0x80000000 + 3, ¥
	((struct SGG_WINDOW *) (window))->image[WINSTR_SIGNALEBOX] | 2, ¥
	((struct SGG_WINDOW *) (window))->image[WINSTR_SIGNALBASE] + 5, ¥
	((struct SGG_WINDOW *) (window))->image[WINSTR_USERID], 0x0000)

#define sgg_wm0_setvideomode(mode, signal) ¥
	sgg_execcmd0(0x0054, 0, (int) (mode), 0x3240 + 3, 0x7f000002, ¥
	(int) (signal), 0x0000)

#define sgg_wm0_gapicmd_0010_0000() ¥
	sgg_execcmd0(0x0050, 6 * 4, 0x0010, 0, 0x0000, 0x0000, 0x0000)

#define	sgg_wm0_gapicmd_001c_0004() ¥
	sgg_execcmd0(0x0050, 6 * 4, 0x001c, 0, 0x0004, 0x0000, 0x0000)

#define	sgg_wm0_gapicmd_001c_0020() ¥
	sgg_execcmd0(