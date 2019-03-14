void io_hlt(void);
void io_cli(void);
void io_out8(int port, int data); // send "data" to specified "port"
int io_load_eflags(void);
void io_store_eflags(int eflags);

void init_palette(void);
void set_palette(int start, int end, unsigned char *rgb);
void init_screen(char *vram, int x, int y);
void boxfill8(unsigned char *vram, int xsize, unsigned char c, int x0, int y0, int x1, int y1);
void putfont8(char *vram, int xsize, int x, int y, char c, char *font);

// 画素座標(x,y)に対応するVRAMの番地は、
// 0xa0000 + x + y * 320

#define COL8_000000	0
#define COL8_FF0000	1
#define COL8_00FF00	2
#define COL8_FFFF00	3
#define COL8_0000FF	4
#define COL8_FF00FF	5
#define COL8_00FFFF	6
#define COL8_FFFFFF	7
#define COL8_C6C6C6	8
#define COL8_840000	9
#define COL8_008400	10
#define COL8_848400	11
#define COL8_000084	12
#define COL8_840084	13
#define COL8_008484	14
#define COL8_848484	15

struct BOOTINFO {
	char cyls, leds, vmode, reserve;
	short scrnx, scrny;
	char *vram;
};

void HariMain(void) {
	struct BOOTINFO *binfo = (struct BOOTINFO *) 0x0ff0;
	static char font_A[16] = {
		0x00, 0x18, 0x18, 0x18, 0x18, 0x24, 0x24, 0x24,
		0x24, 0x7e, 0x42, 0x42, 0x42, 0xe7, 0x00, 0x00
	};
	init_palette();
	init_screen(binfo->vram, binfo->scrnx, binfo->scrny);

	putfont8(binfo->vram, binfo->scrnx, 10, 10, COL8_FFFFFF, font_A);
	// 0xa0000 - 0xbffff ビデオアクセス用アドレス空間
	// p = (char *) 0xa0000;

	// for (i = 0; i <= 0xffff; i++) {
	// 	p[i] = i & 0x0f;
	// 	// AND: 特定のビットを0にする
	// 	// OR: 特定のビットを1にする
	// }

	for (;;) {
		io_hlt();
	}
}

void putfont8(char *vram, int xsize, int x, int y, char c, char *font) {
	char d, *p;
	int i;
	for (i = 0; i < 16; i++) {
		d = font[i];
		p = vram + (y + i) * xsize + x;
		if ((d & 0x80) != 0) { p[0] = c; }
		if ((d & 0x40) != 0) { p[1] = c; }
		if ((d & 0x20) != 0) { p[2] = c; }
		if ((d & 0x10) != 0) { p[3] = c; }
		if ((d & 0x08) != 0) { p[4] = c; }
		if ((d & 0x04) != 0) { p[5] = c; }
		if ((d & 0x02) != 0) { p[6] = c; }
		if ((d & 0x01) != 0) { p[7] = c; }
	}
	return;
}

void init_screen(char *vram, int x, int y) {
	boxfill8(vram, x, COL8_008484, 0, 0, 		x - 1, y - 29);
	boxfill8(vram, x, COL8_C6C6C6, 0, y - 28, 	x - 1, y - 28);
	boxfill8(vram, x, COL8_FFFFFF, 0, y - 27, 	x - 1, y - 27);
	boxfill8(vram, x, COL8_C6C6C6, 0, y - 26, 	x - 1, y - 1);

	boxfill8(vram, x, COL8_FFFFFF, 3, 	y - 24, 59, y - 24);
	boxfill8(vram, x, COL8_FFFFFF, 2, 	y - 24, 2, 	y - 4);
	boxfill8(vram, x, COL8_848484, 3, 	y - 4, 	59, y - 4);
	boxfill8(vram, x, COL8_848484, 59, 	y - 23, 59, y - 5);
	boxfill8(vram, x, COL8_000000, 2, 	y - 3, 	59, y - 3);
	boxfill8(vram, x, COL8_000000, 60, 	y - 24, 60, y - 3);

	boxfill8(vram, x, COL8_848484, x - 47, 	y - 24, x - 4, 	y - 24);
	boxfill8(vram, x, COL8_848484, x - 47, 	y - 23, x - 47, y - 4);
	boxfill8(vram, x, COL8_FFFFFF, x - 47, 	y - 3, 	x - 4, 	y - 3);
	boxfill8(vram, x, COL8_FFFFFF, x - 3, 	y - 24, x - 3, 	y - 3);

	return;
}

void boxfill8(unsigned char *vram, int xsize, unsigned char c, int x0, int y0, int x1, int y1) {
	int x, y;
	for (y = y0; y <= y1; y++) {
		for (x = x0; x <= x1; x++) {
			vram[y * xsize + x] = c;
		}
	}
	return;
}

void init_palette(void) {
	// By using "static", DB is used instead of RESB
	static unsigned char table_rgb[16 * 3] = {
		0x00, 0x00, 0x00, // black
		0xff, 0x00, 0x00, // red
		0x00, 0xff, 0x00, // green
		0xff, 0xff, 0x00, // yellow
		0x00, 0x00, 0xff, // blue
		0xff, 0x00, 0xff, // purple
		0x00, 0xff, 0xff, // light blue
		0xff, 0xff, 0xff, // white
		0xc6, 0xc6, 0xc6, // light gray
		0x84, 0x00, 0x00, // dark red
		0x00, 0x84, 0x00, // dark green
		0x84, 0x84, 0x00, // dark yellow
		0x00, 0x00, 0x84, // dark blue
		0x84, 0x00, 0x84, // dark purple
		0x00, 0x84, 0x84, // dark water color
		0x84, 0x84, 0x84  // dark gray
	};
	set_palette(0, 15, table_rgb);
	return;
}

void set_palette(int start, int end, unsigned char *rgb) {
	int i, eflags;
	eflags = io_load_eflags(); // save eflags to variable
	io_cli(); // clear interrupt flag (CLI) vs. set interrupt flag (STI)
	io_out8(0x3c8, start);
	for (i = start; i <= end; i++) {
		io_out8(0x3c9, rgb[0] / 4);
		io_out8(0x3c9, rgb[1] / 4);
		io_out8(0x3c9, rgb[2] / 4);
		rgb += 3;
	}
	io_store_eflags(eflags); // restore eflags to variable "eflags"
	return;

	// 割り込み処理
}






