/*
 * JPEG decoding engine for DCT-baseline
 *
 *      copyrights 2003 by nikq | nikq::club.
 *
 * history::
 * 2003/04/28 | added OSASK-GUI ( by H.Kawai )
 * 2003/05/12 | optimized DCT ( 20-bits fixed point, etc...) -> line 407-464 ( by I.Tak. )
 * 2003/09/27 | PICTURE0.BIN(DLL)用に改造 ( by くーみん )
 * 2003/09/28 | 各種バグフィクス＆多少の最適化 ( by H.Kawai )
 *
 */


typedef unsigned char UCHAR;

struct DLL_STRPICENV { int work[16384]; };

typedef struct
{
    int elem; //要素数
    unsigned short code[256];
    unsigned char  size[256];
    unsigned char  value[256];
}HUFF;

typedef struct
{
    // SOF
    int width;
    int height;
    // MCU
    int mcu_width;
    int mcu_height;

    int max_h,max_v;
    int compo_count;
    int compo_id[3];
    int compo_sample[3];
    int compo_h[3];
    int compo_v[3];
    int compo_qt[3];

    // SOS
    int scan_count;
    int scan_id[3];
    int scan_ac[3];
    int scan_dc[3];
    int scan_h[3];  // サンプリング要素数
    int scan_v[3];  // サンプリング要素数
    int scan_qt[3]; // 量子化テーブルインデクス
    
    // DRI
    int interval;

    int mcu_buf[32*32*4]; //バッファ
    int *mcu_yuv[4];
    int mcu_preDC[3];
    
    // DQT
    int dqt[3][64];
    int n_dqt;
    
    // DHT
    HUFF huff[2][3];
    
    
    // FILE i/o
	unsigned char *fp, *fp1;
    unsigned long bit_buff;
    int bit_remain;
    int width_buf;

	int base_img[64][64]; // 基底画像 ( [横周波数uπ][縦周波数vπ][横位相(M/8)][縦位相(N/8)]

    /* for dll 
    
    JPEG *jpeg = (JPEG *)malloc(sizeof(JPEG) + 256);
    */
    int dummy[64];
    
}JPEG;

/* for 16bit */
#ifndef PIXEL16
#define PIXEL16(r, g, b)	((r) << 11 | (g) << 5 | (b))
	/* 0 <= r <= 31, 0 <= g <= 63, 0 <= b <= 31 */
#endif

int info_JPEG(struct DLL_STRPICENV *env, int *info, int size, UCHAR *fp);
int decode0_JPEG(struct DLL_STRPICENV *env, int size, UCHAR *fp, int b_type, UCHAR *buf, int skip);

void jpeg_idct_init(int base_img[64][64]);
int jpeg_init(JPEG *jpeg);
// int jpeg_header(JPEG *jpge);
void jpeg_decode(JPEG *jpeg, unsigned char *rgb,int b_type);

/* ----------------- start main section ----------------- */

int info_JPEG(struct DLL_STRPICENV *env,int *info, int size, UCHAR *fp0)
{
	JPEG *jpeg = (JPEG *) (((int *) env) + 128);
	jpeg->fp = fp0;
	jpeg->fp1 = fp0 + size;

//	if (512 + sizeof (JPEG) > 64 * 1024)
//		return 0;

	if (jpeg_init(jpeg))
		return 0;
//	jpeg_header(jpeg);

	if (jpeg->width == 0)
		return 0;
	
	info[0] = 0x0002;
	info[1] = 0x0000;
	info[2] = jpeg->width;
	info[3] = jpeg->height;

	/* OK */
	return 1;
}

int decode0_JPEG(struct DLL_STRPICENV *env,int size, UCHAR *fp0, int b_type, UCHAR *buf, int skip)
{
	JPEG *jpeg = (JPEG *) (((int *) env) + 128);
	jpeg->fp = fp0;
	jpeg->fp1 = fp0 + size;

	jpeg_idct_init(jpeg->base_img);
	jpeg_init(jpeg);
//	jpeg_header(jpeg);

//	if (jpeg->width == 0)
//		return 8;
	/* decode0ではinfoしてから呼ばれるので、これはない */

	jpeg->width_buf = skip / (b_type & 0x7f) + jpeg->width;
    jpeg_decode(jpeg, buf, b_type);

	/* OK */
	return 0;
}

// -------------------------- I/O ----------------------------

unsigned short get_bits(JPEG *jpeg, int bit)
{
	unsigned char  c, c2;
	unsigned short ret;
	unsigned long  buff;
	int remain;

	buff   = jpeg->bit_buff;
	remain = jpeg->bit_remain;

	while (remain <= 16) {
		if (jpeg->fp >= jpeg->fp1) {
			ret = 0;
			goto fin;
		}
		c = *jpeg->fp++;
		if (c == 0xff) { // マーカエラーを防ぐため、FF -> FF 00 にエスケープされてる
			if (jpeg->fp >= jpeg->fp1) {
				ret = 0;
				goto fin;
			}
			jpeg->fp++; /* 00をskip */
		}
		buff = (buff << 8) | c;
		remain += 8;
	}
	ret = (buff >> (remain - bit)) & ((1 << bit) - 1);
	remain -= bit;

	jpeg->bit_remain = remain;
	jpeg->bit_buff   = buff;
fin:
	return ret;
}

// ------------------------ JPEG セグメント実装 -----------------

// start of frame
int jpeg_sof(JPEG *jpeg)
{
	unsigned char c;
	int i, h, v, n;

	if (jpeg->fp + 8 > jpeg->fp1)
		goto err;
	/* fp[2] は bpp */
	jpeg->height = jpeg->fp[3] << 8 | jpeg->fp[4];
	jpeg->width  = jpeg->fp[5] << 8 | jpeg->fp[6];
	n = jpeg->compo_count = jpeg->fp[7]; // Num of compo, nf
	jpeg->fp += 8;
	if (jpeg->fp + n * 3 > jpeg->fp1)
		goto err;

	for (i = 0; i < n; i++) {
		jpeg->compo_id[i] = jpeg->fp[0];

		jpeg->compo_sample[i] = c = jpeg->fp[1];
		h = jpeg->compo_h[i] = (c >> 4) & 0x0f;
		v = jpeg->compo_v[i] = c & 0x0f;

		if (jpeg->max_h < h)
			jpeg->max_h = h;
		if (jpeg->max_v < v)
			jpeg->max_v = v;

        jpeg->compo_qt[i] = jpeg->fp[2];
		jpeg->fp += 3;
    }
    return 0;
err:
	jpeg->fp = jpeg->fp1;
	return 1;
}

// define quantize table
int jpeg_dqt(JPEG *jpeg)
{
	unsigned char c;
	int i, j, v, size;

	if (jpeg->fp + 2 > jpeg->fp1)
		goto err;
	size = (jpeg->fp[0] << 8 | jpeg->fp[1]) - 2;
	jpeg->fp += 2;
	if (jpeg->fp + size > jpeg->fp1)
		goto err;

	while (size > 0) {
		c = *jpeg->fp++;
		size--;
		j = c & 7;
		if (j > jpeg->n_dqt)
			jpeg->n_dqt = j;

 		if (c & 0xf8) {
			// 16 bit DQT
			for (i = 0; i < 64; i++) {
				jpeg->dqt[j][i] = jpeg->fp[0]; /* 下位は読み捨てている */
				jpeg->fp += 2;
			}
			size += -64 * 2;
		} else {
			//  8 bit DQT
			for (i = 0; i < 64; i++)
				jpeg->dqt[j][i] = *jpeg->fp++;
			size -= 64;
		}
	}
	return 0;
err:
	jpeg->fp = jpeg->fp1;
	return 1;
}

// define huffman table
int jpeg_dht(JPEG *jpeg)
{
	unsigned tc, th;
	unsigned code = 0;
	unsigned char val;
	int i, j, k, num, Li[17];
	int len, max_val;
	HUFF *table;

	if (jpeg->fp + 2 > jpeg->fp1)
		goto err;
	len = (jpeg->fp[0] << 8 | jpeg->fp[1]) - 2;
	jpeg->fp += 2;

	while (len > 0) {
		if (jpeg->fp + 17 > jpeg->fp1)
			goto err;
		val = jpeg->fp[0];

		tc = (val >> 4) & 0x0f; // テーブルクラス(DC/AC成分セレクタ)
		th =  val       & 0x0f; // テーブルヘッダ(何枚目のプレーンか)
		table = &(jpeg->huff[tc][th]);

		num = 0;
		k = 0;
		for (i = 1; i <= 16; i++) {
			Li[i] = jpeg->fp[i];
			num += Li[i];
            for (j = 0; j < Li[i]; j++)
                table->size[k++] = i;
		}
		table->elem = num;
		jpeg->fp += 17;

		k=0;
		code=0;
		i = table->size[0];
		while (k < num) {
			while (table->size[k] == i)
				table->code[k++] = code++;
			if (k >= num)
				break;
			do {
				code <<= 1;
				i++;
			} while (table->size[k] != i);
		}

		if (jpeg->fp + num > jpeg->fp1)
			goto err;
		for (k = 0; k < num; k++)
			table->value[k] = jpeg->fp[k];
		jpeg->fp += num;

        len -= 18 + num;
    }
    return 0;
err:
	jpeg->fp = jpeg->fp1;
	return 1;
}

int jpeg_init(JPEG *jpeg)
{
	unsigned char c;
	int r = 0, i;
	jpeg->width = 0;
	jpeg->mcu_preDC[0] = 0;
	jpeg->mcu_preDC[1] = 0;
	jpeg->mcu_preDC[2] = 0;
	jpeg->n_dqt = 0;
	jpeg->max_h = 0;
	jpeg->max_v = 0;
	jpeg->bit_remain = 0;
	jpeg->bit_buff   = 0;
	// DRIリセット無し
	jpeg->interval = 0;
//	return;
//}
//
//int jpeg_header(JPEG *jpeg)
//{
//	unsigned char c;
//	int r = 0, i;

	for (;;) {
		if (jpeg->fp + 2 > jpeg->fp1)
			goto err;
		if (jpeg->fp[0] != 0xff)
            goto err0;
		c = jpeg->fp[1];
		jpeg->fp += 2;
		if (c == 0xd8)
			continue; /* SOI */
		if (c == 0xd9)
			goto err; /* EOI */

		if (c == 0xc0)
            jpeg_sof(jpeg);
		else if (c == 0xc4)
            jpeg_dht(jpeg);
		else if (c == 0xdb)
            jpeg_dqt(jpeg);
		else if (c == 0xdd) {
			if (jpeg->fp + 4 > jpeg->fp1)
				goto err;
			jpeg->interval = jpeg->fp[2] << 8 | jpeg->fp[3];
			jpeg->fp += 4;
		} else if (c == 0xda) {
			if (jpeg->fp + 3 > jpeg->fp1)
				goto err;
			jpeg->scan_count = jpeg->fp[2];
			jpeg->fp += 3;
			if (jpeg->fp + jpeg->scan_count * 2 > jpeg->fp1)
				goto err;
			for (i = 0; i < jpeg->scan_count; i++) {
				jpeg->scan_id[i] = jpeg->fp[0];
				jpeg->scan_dc[i] = jpeg->fp[1] >> 4;   // DC Huffman Table
				jpeg->scan_ac[i] = jpeg->fp[1] & 0x0F; // AC Huffman Table
				jpeg->fp += 2;
			}
			jpeg->fp += 3; /* 3bytes skip */
            goto 