void io_hlt(void);
void io_cli(void);
void io_out8(int port, int data); // send "data" to specified "port"
int io_load_eflags(void);
void io_store_eflags(int eflags);

void init_palette(void);
void set_palette(int start, int end, unsigned char *rgb);

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

void HariMain(void) {
	int i;
	char *p;

	init_palette();

	// 0xa0000 - 0xbffff ビデオアクセス用アドレス空間
	p = (char *) 0xa0000;

	// for (i = 0; i <= 0xffff; i++) {
	// 	p[i] = i & 0x0f;
	// 	// AND: 特定のビットを0にする
	// 	// OR: 特定のビットを1にする
	// }

	boxfill8(p, 320, COL8_FF0000, 20, 20, 120, 120);
	boxfill8(p, 320, COL8_00FF00, 70, 50, 170, 150);
	boxfill8(p, 320, COL8_0000FF, 120, 80, 220, 180);

	for (;;) {
		io_hlt();
	}
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






