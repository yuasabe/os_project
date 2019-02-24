//	"bim2bin"
//	Copyright(C) 2004 H.Kawai
//	usage : bim2bin [malloc:#] [mmarea:#] in:(path) out:(path)
//		[-bim | -exe512 | -bin0 | -data | -restore | -osacmp] [-simple | -l2d3 | -tek0 | -tek1 | -tek2]

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NOWARN			1
#define TEK1_NOWARN		1

#define	get32(ptr)		*((int *) (ptr))

#define	SIZEOFBUF		(16 * 1024 * 1024)
#define	SIZEOFOVERBUF	(16 * 1024 * 1024)
#define	SIZEOFSUBBUF	(SIZEOFOVERBUF * 4 / 4)

//#define DEBUGMSG		1

/* +0x10 : 総DS/SSサイズ */
/* +0x14 : file */
/* +0x18 : reserve */
/* +0x1c : reserve */
/* +0x20 : static start */
/* +0x24 : static bytes */

typedef unsigned char UCHAR;

#define TEK1_MAXLEN			31	/* 32+196=244 */

#define TEK1_BT_NODES0	4400 * 2
#define TEK1_BT_NODES1	65536 * 2
	/* maxdis:1MB用(10MBくらい必要になる) */

static unsigned char *putb_buf, *putb_overbuf;
static int putb_ptr;
static unsigned char putb_byte, putb_count = 8;
extern UCHAR *tek1_s7ptr;
static int complev = -1;
static FILE *hint = NULL;

int lzcompress_l2d3(unsigned char *buf, int k, int i, int outlimit, int maxdis);
int lzcompress_tek0(int prm, unsigned char *buf, int k, int i, int outlimit, int maxdis);
int lzcompress_tek1(int srcsiz, UCHAR *src, int outsiz, UCHAR *outbuf, int wrksiz, UCHAR *work, int bsiz, int flags, int opt, int prm, int maxdis, int submaxdis);
//int lzcompress_tek3(int srcsiz, UCHAR *src, int outsiz, UCHAR *outbuf, int wrksiz, UCHAR *work, int bsiz, int flags, int opt, int prm, int maxdis, int submaxdis);
int lzcompress_tek5(int srcsiz, UCHAR *src, int outsiz, UCHAR *outbuf, int wrksiz, UCHAR *work, UCHAR *argv0, UCHAR *eopt, int bsiz, int flags, int opt, int prm, int maxdis, int submaxdis);
int lzrestore_l2d3(unsigned char *buf, int k, int i, int outlimit);
int lzrestore_tek0(unsigned char *buf, int k, int i, int outlimit);
int tek1_lzrestore_tek1(int srcsiz, UCHAR *src, int outsiz, UCHAR *outbuf, int wrksiz, UCHAR *work, int flags);
void osarjc(int siz, UCHAR *p, int mode);

//int tek1_lzrestore_stk5(int srcsiz, UCHAR *src, int outsiz, UCHAR *outbuf);
int tek_lzrestore_tek5(int srcsiz, UCHAR *src, int outsiz, UCHAR *outbuf, UCHAR *mclp);

void tek1_puts7s(unsigned int i);
UCHAR *tek1_puts7sp(UCHAR *p, unsigned int i);
void tek1_puts7(unsigned int i);
unsigned int tek1_getnum_s7s(UCHAR **pp);

#define TEK1_BT_MAXLEV		10
#define TEK1_BT_NODESIZ		32	/* 必ず2の倍数 */

/* 登録したい数/16のノード1 */
/* 登録したい数/256+登録したい数/4096+登録したい数/65536+...のノード1 */
/* 1/256(16/15)=1/240 だから登録したい数/240のノード0 */
/* 1MBだとすると、ノード1は65536個、ノード0は4400個 */

struct STR_BT_NODE0 { /* 260バイト */
	int nodes, skiplen;
	void *pkey[TEK1_BT_NODESIZ];
	void *node[TEK1_BT_NODESIZ];
	 /* pkeyは各ノードの最低値（隣のノードの最大値よりも小さければ、最低値よりさらに小さくてもよい） */
};

struct STR_BT_NODE1 { /* 132バイト */
	int nodes, skiplen;
	void *pkey[TEK1_BT_NODESIZ];
};

struct STR_BT_HANDLE {
	int pos[TEK1_BT_MAXLEV + 1];
	void *node[TEK1_BT_MAXLEV];
	void *pkey;
};

struct STR_BTREE {
	int level;
	void *top;
	struct STR_BT_NODE0 *free0;
	struct STR_BT_NODE1 *free1;
};

void init_btree(struct STR_BTREE *btree);
void add_free0(struct STR_BTREE *btree, struct STR_BT_NODE0 *newnode);
void add_free1(struct STR_BTREE *btree, struct STR_BT_NODE1 *newnode);
int matchlen_bt(UCHAR *a, UCHAR *b, UCHAR *e);
void search0(struct STR_BTREE *btree, struct STR_BT_HANDLE *handle, UCHAR *s, UCHAR *s1);
int search_back(struct STR_BTREE *btree, struct STR_BT_HANDLE *handle);
int search_next(struct STR_BTREE *btree, struct STR_BT_HANDLE *handle);
int insert_val(struct STR_BTREE *btree, UCHAR *s, UCHAR *s1);
int delete_val(struct STR_BTREE *btree, UCHAR *s, UCHAR *s1);

UCHAR *getnum0(UCHAR *p, UCHAR *p1, int *pi)
{
	int i = 0, j, base = 10;
//	p = skipspace(p);
	if (p >= p1)
		goto fin;
	if (*p == '0') {
		p++;
		if (*p == 'X' || *p == 'x') {
			base = 16;
			p++;
		} else if (*p == 'O' || *p == 'o') {
			base = 8;
			p++;
		}
	}
	p--;
	for (;;) {
		p++;
		if (p >= p1)
			goto fin;
		if (*p == '_')
			continue;
		j = 99;
		if ('0' <= *p && *p <= '9')
			j = *p - '0';
		if ('A' <= *p && *p <= 'F')
			j = *p - 'A' + 10;
		if ('a' <= *p && *p <= 'f')
			j = *p - 'a' + 10;
		if (base <= j)
			break;
		i = i * base + j;
	}
	if (p >= p1)
		goto fin;
	if (*p == 'k' || *p == 'K') {
		i *= 1024;
	//	p++;
	} else if (*p == 'm' || *p == 'M') {
		i *= 1024 * 1024;
	//	p++;
	} else if (*p == 'g' || *p == 'G') {
		i *= 1024 * 1024 * 1024;
	//	p++;
	}
fin:
	*pi = i;
	return p;
}

int getnum(UCHAR *p)
{
	int i;
	getnum0(p, p + 100, &i);
	return i;
}

UCHAR *glb_str_eprm = NULL;

int main(int argc, char **argv)
{
	int mallocsize = 32 * 1024, reserve = 0, stacksize, datasize, worksize;
	int i, j, filesize, compress = -2, outtype = 0, prm0 = 12, maxdis = -1, bsiz = -1, submaxdis = 16 * 1024;
	FILE *fp; /* maxdis = 32 * 1024, bsiz = 32 * 1024 */
	UCHAR *filepath[2], c, *buf, *overbuf, *s7ptr, *eopt = NULL, *argv0 = argv[0];
	int code_end, data_begin, entry, opt = -1, rjc = -1, v48 = 0;
	static unsigned char signature[15] = "¥xff¥xff¥xff¥x01¥x00¥x00¥x00OSASKCMP";

	#if (defined(NOWARN))
		j = 0;
	#endif

	filepath[0] = filepath[1] = NULL;
	buf = malloc(SIZEOFBUF);
	overbuf = malloc(SIZEOFOVERBUF);

	if (argc <= 2) {
		fprintf(stdout,
			"¥"bim2bin¥" executable binary maker for GUIGUI00 (OSASK API)¥n"
			"¥tcopyright (C) 2004 H.Kawai¥n"
			"usage : ¥n"
			">bim2bin [malloc:#] [mmarea:#] in:(file) out:(file)¥n"
			" [-bim | -exe512 | -bin0 | -data | -restore | -osacmp]¥n"
			" [-simple | -l2d3 | -tek0 | -tek1 | -tek2 | -tek5]¥n"
		);
		return 1;
	}

	/* パラメーター解析 */
	for (argv++, i = 1; i < argc; argv++, i++) {
		UCHAR *s = *argv;
		if (strncmp(s, "malloc:", 7) == 0)
			mallocsize = getnum(s + 7);
		else if (strncmp(s, "file:", 5) == 0)
			reserve = getnum(s + 5) | 0x01;
		else if (strcmp(s, "-simple") == 0)
			compress = -1;
		else if (strncmp(s, "input:", 6) == 0)
			filepath[0] = s + 6;
		else if (strncmp(s, "output:", 7) == 0)
			filepath[1] = s + 7;
		else if (strcmp(s, "-l2d3") == 0)
			compress = 1;
		else if (strcmp(s, "-tek0") == 0)
			compress = 2;
		else if (strcmp(s, "-tek1") == 0)
			compress = 3;
		else if (strcmp(s, "-tek2") == 0)
			compress = 4;
	//	else if (strcmp(s, "-tek3") == 0)
	//		compress = 5;
	//	else if (strcmp(s, "-tek4") == 0)
	//		compress = 6;
		else if (strcmp(s, "-tek5") == 0)
			compress = 7;
		else if (strcmp(s, "-bim") == 0)
			outtype = 0;
		else if (strcmp(s, "-exe512") == 0)
			outtype = 1;
		else if (strcmp(s, "-data") == 0)
			outtype = 2;
		else if (strcmp(s, "-restore") == 0)
			outtype = 3;
		else if (strncmp(s, "prm0:", 5) == 0)
			prm0 = getnum(s + 5);
		else if (strncmp(s, "bsiz:", 5) == 0)
			bsiz = getnum(s + 5);
		else if (strncmp(s, "BS:", 3) == 0)
			bsiz = getnum(s + 3);
		else if (strncmp(s, "in:", 3) == 0)
			filepath[0] = s + 3;
		else if (strncmp(s, "out:", 4) == 0)
			filepath[1] = s + 4;
		else if (strncmp(s, "mmarea:", 7) == 0)
			reserve = getnum(s + 7) | 0x01;
		else if (strcmp(s, "-bin0") == 0)
			outtype = 4;
		else if (strcmp(s, "-osacmp") == 0)
			outtype = 5;
		else if (strncmp(s, "opt:", 4) == 0)
			opt = getnum(s + 4);
		else if (strncmp(s, "rjc:", 4) == 0)
			rjc = getnum(s + 4);
		else if (strncmp(s, "eopt:", 5) == 0)
			eopt = s + 5;
		else if (strncmp(s, "eprm:", 5) == 0)
			glb_str_eprm = s + 5;
		else if (strncmp(s, "maxdis:", 7) == 0)
			maxdis = getnum(s + 7);
		else if (strncmp(s, "MD:", 3) == 0)
			maxdis = getnum(s + 3);
		else if (strncmp(s, "submaxdis:", 10) == 0)
			submaxdis = getnum(s + 10);
		else if (strncmp(s, "SD:", 3) == 0)
			submaxdis = getnum(s + 3);
		else if (strncmp(s, "hint:", 5) == 0)
			hint = fopen(s + 5, "rb");
		else if (strncmp(s, "clv:", 4) == 0) {
		//	static UCHAR table_clv[10] = { }; /* 10段階 */
		//	5が十分、3が中間、1がデフォルト、0がもっとも弱い、9が最強(99)
		//	10-99 -> 00-88, 90
			complev = getnum(s + 4);
			if (complev > 9)
				complev = complev / 10 - 1;
		} else if (strcmp(s, "-v48") == 0)
			v48 |= 1;
		else if (strcmp(s, "-v48a") == 0)
			v48 |= 3;
		else
			fprintf(stderr, "Commnad line error : %s ... skiped¥n", s);
	}
	if (compress == -2) {
		compress = 7;
		if (v48)
			compress = -1;
	}
	if (complev == -1)
		complev = 4;
	if (maxdis == -1) {
		maxdis = 0;
		if (compress <= 2)
			maxdis = 32 * 1024;
	}
	if (bsiz == -1)
		bsiz = 0;

	if (maxdis > 2 * 1024 * 1024)
		maxdis = 2 * 1024 * 1024;
	if (maxdis == 0)
		maxdis = 2 * 1024 * 1024;
	if (bsiz > 8 * 1024 * 1024)
		bsiz = 8 * 1024 * 1024;
	if (bsiz == 0)
		bsiz = 8 * 1024 * 1024;
	if (bsiz < 512)
		bsiz = 512;

	fp = NULL;
	if (filepath[0])
		fp = fopen(filepath[0], "rb");
	if (fp == NULL) {
		fprintf(stderr, "Commnad line error : can't open input file¥n");
		return 1;
	}
	filesize = fread(buf, 1, SIZEOFBUF, fp);
	fclose(fp);
	if (rjc == -1 || rjc == 1)
		osarjc(filesize, buf, 1);
	if (outtype == 2 /* data */) {
		putb_overbuf = overbuf;
		putb_ptr = 0;
		if (compress < 3) {
			if (compress == 1)
				lzcompress_l2d3(buf, filesize, 0, SIZEOFOVERBUF - 8, maxdis);
			if (compress == 2)
				lzcompress_tek0(prm0, buf, filesize, 0, SIZEOFOVERBUF - 8, maxdis);
			for (i = 0; i < putb_ptr; i++)
				buf[i] = overbuf[i];
			filesize = putb_ptr;
		}
		goto write;
	}

	if (outtype == 3 /* restore */) {
		c = 0;
		for (i = 0; i < 15; i++)
			c |= buf[i + 1] ^ signature[i];
		if (c) {
			for (i = 0; i < filesize; i++)
				overbuf[i] = buf[i];
		} else {
			if (buf[0] == 0x83)
				compress = 3;
			if (buf[0] == 0x85)
				compress = 4;
			if (buf[0] == 0x86)
				compress = 5;
			if (buf[0] == 0x89)
				compress = 7;
			if (buf[0] == 0x82)
				compress = 2;
			if (buf[0] == 0x81)
				compress = 1;
			j = 20;
			if (compress >= 3) {
				s7ptr = &buf[16];
				putb_ptr = tek1_getnum_s7s(&s7ptr);
				j = s7ptr - buf;
			}
			filesize -= j;
			for (i = 0; i < filesize; i++)
				overbuf[i] = buf[i + j];
			j = putb_ptr;
		}
		putb_overbuf = overbuf;
		putb_ptr = filesize;
		if (compress == 1)
			filesize = lzrestore_l2d3(buf, filesize, 0, SIZEOFOVERBUF - 8);
		if (compress == 2)
			filesize = lzrestore_tek0(buf, filesize, 0, SIZEOFOVERBUF - 8);
		if (3 <= compress && compress <= 7) {
			UCHAR *work = malloc(512 * sizeof (int));
			if (compress == 3)
				tek1_lzrestore_tek1(filesize, overbuf, j, buf, 512 * sizeof (int), work, 0);
			if (compress == 4)
				tek1_lzrestore_tek1(filesize, overbuf, j, buf, 512 * sizeof (int), work, 1);
			if (compress == 5)
				tek1_lzrestore_tek1(filesize, overbuf, j, buf, 512 * sizeof (int), work, 2);
			if (compress == 6)
				tek1_lzrestore_tek1(filesize, overbuf, j, buf, 512 * sizeof (int), work, 3);
			if (compress == 7)
				tek_lzrestore_tek5(filesize, overbuf, j, buf, NULL);
			filesize = j;
			free(work);
		}
		goto write;
	}

	if (outtype == 5) { /* osacmp */
		for (i = filesize - 1; i >= 0; i--)
			buf[i + 20] = buf[i];
		for (i = 0; i < 15; i++)
			buf[i + 1] = signature[i];
		if (compress == 1)
			buf[0] = 0x81;
		if (compress == 2)
			buf[0] = 0x82;
		if (compress == 3)
			buf[0] = 0x83;
		if (compress == 4)
			buf[0] = 0x85;
		if (compress == 5)
			buf[0] = 0x86;
		if (compress == 6)
			buf[0] = 0x87;
		if (compress == 7)
			buf[0] = 0x89;
		if (compress < 3) {
			buf[16] = filesize         & 0xff;
			buf[17] = (filesize >>  8) & 0xff;
			buf[18] = (filesize >> 16) & 0xff;
			buf[19] = (filesize >> 24) & 0xff;
			buf[filesize + 20] = 0x14;
			buf[filesize + 21] = 0x00;
			buf[filesize + 22] = 0x00;
			buf[filesize + 23] = 0x00;
			filesize += 24;
		}
		if (3 <= compress && compress <= 7) {
			tek1_s7ptr = &buf[16];
			tek1_puts7s(filesize);
			j = &buf[20] - tek1_s7ptr;
			if (j != 0) {
				for (i = 0; i < filesize; i++)
					buf[i + 20 - j] = buf[i + 20];
			}
			buf[filesize + 20 - j] = 0x14 - j;
			buf[filesize + 21 - j] = 0x00;
			buf[filesize + 22 - j] = 0x00;
			buf[filesize + 23 - j] = 0x00;
			filesize += 24 - j;
		}
		outtype = 4; /* bin0 */
	}
	if (outtype == 1 || outtype == 4) { /* exe512 | bin0 */
		if (outtype == 1 /* exe512 */) {
			/* ヘッダー(512バイト)のカット */
			filesize -= 512;
			for (i = 0; i < filesize; i++)
				buf[i] = buf[i + 512];
		}
		if (compress == -1) /* 単なるヘッダカット */
			goto write;
		/* スタティックデーターイメージ圧縮 */
		data_begin = get32(&buf[filesize - 4]);
		datasize = filesize - 4 - data_begin;
		if (compress < 3) {
			putb_overbuf = overbuf;
			putb_ptr = 0;
			if (compress == 1)
				lzcompress_l2d3(buf + data_begin, datasize, 0, SIZEOFOVERBUF - 8, maxdis);
			if (compress == 2)
				lzcompress_tek0(prm0, buf + data_begin, datasize, 0, SIZEOFOVERBUF - 8, maxdis);
			for (i = 0; i < putb_ptr; i++)
				buf[data_begin + i] = overbuf[i];
			filesize = data_begin + putb_ptr;
		} else {
			UCHAR *work = malloc(i = 257 * 1024 + (bsiz + 272) * 8);
			if (compress == 3)
				j = lzcompress_tek1(datasize, buf + data_begin, SIZEOFOVERBUF - 8, overbuf, i, work, bsiz, 0, opt, prm0, maxdis, submaxdis);
			if (compress == 4)
				j = lzcompress_tek1(datasize, buf + data_begin, SIZEOFOVERBUF - 8, overbuf, i, work, bsiz, 1, opt, prm0, maxdis, submaxdis);
		//	if (compress == 5)
		//		j = lzcompress_tek3(datasize, buf + data_begin, SIZEOFOVERBUF - 8, overbuf, i, work, bsiz, 0, opt, prm0, maxdis, submaxdis);
		//	if (compress == 6)
		//		j = lzcompress_tek3(datasize, buf + data_begin, SIZEOFOVERBUF - 8, overbuf, i, work, bsiz, 1, opt, prm0, maxdis, submaxdis);
			if (compress == 7)
				j = lzcompress_tek5(datasize, buf + data_begin, SIZEOFOVERBUF - 8, overbuf, i, work, argv0, eopt, bsiz, 0, opt, prm0, maxdis, submaxdis);
			for (i = 0; i < j; i++)
				buf[data_begin + i] = overbuf[i];
			filesize = data_begin + j;
			free(work);
		}
		goto write;
	}

	if (reserve == 1)
		reserve = 0;
	code_end = get32(&buf[0x00]) + get32(&buf[0x04]);
	datasize = get32(&buf[0x0c]);
	data_begin = get32(&buf[0x10]);
	stacksize = get32(&buf[0x14]);
	entry = get32(&buf[0x18]);
	worksize = (stacksize + datasize + mallocsize + 0xfff) & ‾0xfff;
	if (compress >= 0) {
		static unsigned char header1[0x48] = {
			"¥x2e¥x8b¥x62¥x20"
			"¥x8b¥xfc"
			"¥xeb¥x20"
			"GUIGUI00"
			"¥0¥0¥0¥0¥0¥0¥0¥0"
			"¥0¥0¥0¥0¥0¥0¥0¥0"
			"¥0¥0¥0¥0¥0¥0¥0¥0"
			"¥x6a¥xff"
			"¥x0e"
			"¥x68¥0¥0¥0¥0"		/* +0x2c : data begin */
			"¥x2e¥xff¥x72¥x24"
			"¥x6a¥x81"
			"¥x6a¥x04"
			"¥x8b¥xdc"
			"¥x57"
			"¥x9a¥x00¥x00¥x00¥x00¥xc7¥x00"
			"¥x5c"
			"¥xe9¥0¥0¥0¥0"		/* +0x44 : entry */
		};
		for (i = 0; i < 0x48; i++)
			buf[i] = header1[i];
		get32(&buf[0x10]) = worksize;
		get32(&buf[0x14]) = reserve;
		get32(&buf[0x20]) = stacksize;
		get32(&buf[0x24]) = datasize;
		get32(&buf[0x2c]) = code_end;
		get32(&buf[0x44]) = entry - 0x48;
		if (compress < 3) {
			putb_overbuf = overbuf;
			putb_ptr = 0;
			if (compress == 1)
				lzcompress_l2d3(buf + data_begin, datasize, 0, SIZEOFOVERBUF - 8, maxdis);
			if (compress == 2) {
				buf[0x35] = 0x82; /* tek0圧縮データー展開 */
				lzcompress_tek0(prm0, buf + data_begin, datasize, 0, SIZEOFOVERBUF - 8, maxdis);
			}
			if (putb_ptr < datasize) {
				for (i = 0; i < putb_ptr; i++)
					buf[code_end + i] = overbuf[i];
				filesize = code_end + putb_ptr;
				goto write;
			}
		} else {
			UCHAR *work = malloc(i = 257 * 1024 + (bsiz + 272) * 8);
			if (compress == 3) {
				buf[0x35] = 0x83; /* tek1圧縮データー展開 */
				j = lzcompress_tek1(datasize, buf + data_begin, SIZEOFOVERBUF - 8, overbuf, i, work, bsiz, 0, opt, prm0, maxdis, submaxdis);
			}
			if (compress == 4) {
				buf[0x35] = 0x85; /* tek2圧縮データー展開 */
				j = lzcompress_tek1(datasize, buf + data_begin, SIZEOFOVERBUF - 8, overbuf, i, work, bsiz, 1, opt, prm0, maxdis, submaxdis);
			}
		//	if (compress == 5) {
		//		buf[0x35] = 0x86; /* tek3圧縮データー展開 */
		//		j = lzcompress_tek3(datasize, buf + data_begin, SIZEOFOVERBUF - 8, overbuf, i, work, bsiz, 0, opt, prm0, maxdis, submaxdis);
		//	}
		//	if (compress == 6) {
		//		buf[0x35] = 0x87; /* tek3圧縮データー展開 */
		//		j = lzcompress_tek3(datasize, buf + data_begin, SIZEOFOVERBUF - 8, overbuf, i, work, bsiz, 1, opt, prm0, maxdis, submaxdis);
		//	}
			if (compress == 7) {
				buf[0x35] = 0x89; /* tek5圧縮データー展開 */
				j = lzcompress_tek5(datasize, buf + data_begin, SIZEOFOVERBUF - 8, overbuf, i, work, argv0, eopt, bsiz, 1, opt, prm0, maxdis, submaxdis);
			}
			free(work);
			if (j < datasize) {
				for (i = 0; i < j; i++)
					buf[code_end + i] = overbuf[i];
				filesize = code_end + j;
				goto write;
			}
		}
	}
	if (v48 == 0) {
		static unsigned char header0[0x48] = {
			"¥x2e¥x8b¥x62¥x20"
			"¥x8b¥xfc"
			"¥xeb¥x20"
			"GUIGUI00"
			"¥0¥0¥0¥0¥0¥0¥0¥0"
			"¥0¥0¥0¥0¥0¥0¥0¥0"
			"¥0¥0¥0¥0¥0¥0¥0¥0"
			"¥x2e¥x8b¥x4a¥x24"
			"¥xbe¥0¥0¥0¥0"		/* +0x2d : data begin */
			"¥x2e¥x8a¥x06"
			"¥x46"
			"¥x88¥x07"
			"¥x47"
			"¥x49"
			"¥x75¥xf6"
			"¥xe9¥0¥0¥0¥0"		/* +0x3c : entry */
			"¥0¥0¥0¥0¥0¥0¥0¥0"
		};
		for (i = 0; i < 0x48; i++)
			buf[i] = header0[i];
		get32(&buf[0x10]) = worksize;
		get32(&buf[0x14]) = reserve;
		get32(&buf[0x20]) = stacksize;
		get32(&buf[0x24]) = datasize;
		buf[0x2d] = code_end         & 0xff;
		buf[0x2e] = (code_end >>  8) & 0xff;
		buf[0x2f] = (code_end >> 16) & 0xff;
		buf[0x30] = (code_end >> 24) & 0xff;
		get32(&buf[0x3c]) = entry - 0x40;
		for (i = 0; i < datasize; i++)
			buf[code_end + i] = buf[data_begin + i];
		filesize = code_end + datasize;
	} else {
		static unsigned char header0[0x48] = {
			"¥x2e¥x8b¥x62¥x20"
			"¥x6a¥xff"
			"¥xeb¥x28"
			"GUIGUI00"
			"¥0¥0¥0¥0¥0¥0¥0¥0"
			"¥0¥0¥0¥0¥0¥0¥0¥0"
			"¥0¥0¥0¥0¥0¥0¥0¥0"
			"¥0¥0¥0¥0¥0¥0¥0¥0"		/* +0x2c : data begin */
			"¥x6a¥x20"
			"¥x6a¥xf1"
			"¥x6a¥x04"
			"¥x89¥xe3"
			"¥x9a¥x00¥x00¥x00¥x00¥xc7¥x00"
			"¥x83¥xc4¥x10"
			"¥xe9¥0¥0¥0¥0"		/* +0x43 : entry */
			"¥0"
		};
		for (i = 0; i < 0x48; i++)
			buf[i] = header0[i];
		if (v48 & 2) {
			/* code_end, datasizeの16バイトアライン */
			for (i = datasize - 1; i >= 0; i--)
				buf[data_begin + i + 16] = buf[data_begin + i];
			data_begin += 16;
			while (code_end & 0x0f)
				buf[code_end++] = '¥0'; 
			while (datasize & 0x0f)
				buf[data_begin + datasize++] = '¥0'; 
		}
		get32(&buf[0x10]) = worksize;
		get32(&buf[0x14]) = reserve;
		get32(&buf[0x20]) = stacksize;
		get32(&buf[0x24]) = datasize;
		get32(&buf[0x2c]) = code_end;
		i = entry - 0x47;
		buf[0x43] =  i        & 0xff;
		buf[0x44] = (i >>  8) & 0xff;
		buf[0x45] = (i >> 16) & 0xff;
		buf[0x46] = (i >> 24) & 0xff;
		if (i == 1) {
			buf[0x42] = buf[0x43] = 0x90;
			get32(&buf[0x44]) = 0x90909090;
		}
		for (i = 0; i < datasize; i++)
			buf[code_end + i] = buf[data_begin + i];
		filesize = code_end + datasize;
	}

write:
	fp = NULL;
	if (filepath[1])
		fp = fopen(filepath[1], "wb");
	if (fp == NULL) {
		fprintf(stderr, "Commnad line error : can't open output file¥n");
		return 1;
	}
	if (rjc == -1 || rjc == 1)
		osarjc(filesize, buf, 0);
	fwrite(buf, 1, filesize, fp);
	fclose(fp);

	return 0;
}

const int search0a(int prm0, unsigned char *buf, unsigned char *buf0, const int max0, int *p, const int *table)
{
	int l, max = 0, d, ld, ld0 = 30;
	unsigned char *s;
	unsigned char c = buf[0];

	/* アクセスしてはいけないアドレス : buf + max0 */
	for (s = buf - 1; s >= buf0; s--) {
		if (*s == c) {
			for (l = max; l > 0; l--) { /* 高速化のため、後ろから比較 */
				if (buf[l] != s[l])
					goto nextloop;
			}
			/* 一致長を算出, ただしmax0になったら打ち止め */
			for (l = max; buf[l] == s[l]; ) {
				if (++l >= max0) {
					*p = s - buf;
					return l;
				}
			}

/* distanceの増加量がlの増加量のp倍を超えているようなら、損であるので採用しない */

			d = s - buf;
			if (d == -1)
				ld = 0;
			else
				for (ld = 30; d & (1 << ld); ld--);
			if ((max - l) * prm0 + ld - ld0 < 0) {
				if (l > 10) {
					max = l;
					*p = d;
					ld0 = ld;
				} else if (d >= table[l - 1]) {
					max = l;
					*p = d;
					ld0 = ld;
				}
			}
		}
nextloop:
		;
	}
	return max;
}

int search(unsigned char *buf, unsigned char *buf0, const int max0, int *p)
{
	static int table[10] = {
		-8, -512, -32 * 1024, -2 * 1024 * 1024, -0x7fffffff, 
		-0x7fffffff, -0x7fffffff, -0x7fffffff, -0x7fffffff, -0x7fffffff
	};
	return search0a(12, buf, buf0, max0, p, table);
}

int search0b_sub(struct STR_BTREE *btree, struct STR_BT_HANDLE *handle, int *lenhis, UCHAR *s, UCHAR *s1, int len)
/* 0:該当なし */
/* 一致長lenの中で一番近いものを返す */
/* handleは勝手に動かす */
{
	int max = -0x7fffffff;
	int i;
	for (i = 0; i < 2; i++) {
		while (lenhis[i] >= len) {
			if (max < (UCHAR *) handle[i].pkey - s)
				max = (UCHAR *) handle[i].pkey - s;
			if (i == 0) {
				if (search_back(btree, &handle[i]))
					break;
			} else {
				if (search_next(btree, &handle[i]))
					break;
			}
			lenhis[i] = matchlen_bt(s, handle[i].pkey, s1);
		}
	}
	if (max == -0x7fffffff)
		max = 0;
	return max;
}

int search0b_calcld(int d)
{
	int ld = 0;
	if (d < -1)
		for (ld = 30; d & (1 << ld); ld--);
	return ld;	
}

int search0c(int prm0, UCHAR *buf, UCHAR *buf0, int max0, int *p, const int *table, int trees, struct STR_BTREE *btree, UCHAR **l1table)
{
	int l, max = 0, d, ld, ld0 = 30, lenhis[2], l0, dd, tree;
	unsigned char *s1 = buf + max0, *t;
	struct STR_BT_HANDLE handle[2];

	t = l1table[*buf];
	if (t == NULL)
		 goto fin; /* 1文字一致すらもはやどこにも存在しない */
	if (t < buf0) {
		l1table[*buf] = NULL; /* 今後面倒にしないため */
		goto fin; /* 一番近くてもそれでもmaxdisより遠いらしいのでこれもおしまい */
	}
	dd = t - buf;
	if (dd >= table[0]) {
		max = 1;
		*p = dd;
	}
	for (tree = 0; tree < trees; tree++) {
		struct STR_BT_NODE1 *node1 = btree[tree].top;
		if (node1->nodes <= 0)
			continue;
		search0(&btree[tree], &handle[0], buf, s1);
		l = matchlen_bt(buf, handle[0].pkey, s1);
		handle[1] = handle[0];
		search_next(&btree[tree], &handle[1]); /* 隣も見る */
		d = matchlen_bt(buf, handle[1].pkey, s1);
		if (l >= d)
			handle[1] = handle[0];
		else {
			handle[0] = handle[1];
			l = d;
		}
		lenhis[1] = lenhis[0] = l;
		if (l <= 1)
			continue;
		d = search0b_sub(&btree[tree], handle, lenhis, buf, s1, l);
		ld = search0b_calcld(d) - search0b_calcld(dd); /* distanceによってどのくらいビット数が変わるか */
		l0 = l - (ld + prm0 - 1) / prm0;
		while ((l - l0) * prm0 - ld > 0)
			l0++;

		if (l0 < 2)
			l0 = 2; /* 最小長1は既に処理済み */
		ld = search0b_calcld(d);
		if ((max - l) * prm0 + ld - ld0 < 0) { /* maxより長くなれるか？ */
			if (l > 10) {
				max = l;
				*p = d;
				ld0 = ld;
			} else if (d >= table[l - 1]) {
				max = l;
				*p = d;
				ld0 = ld;
			}
		}
		for (l--; l >= l0; l--) {
			d = search0b_sub(&btree[tree], handle, lenhis, buf, s1, l);
			if (d >= 0)
				continue;
			ld = search0b_calcld(d);
			if ((max - l) * prm0 + ld - ld0 < 0) { /* maxより長くなれるか？ */
				if (l > 10) {
					max = l;
					*p = d;
					ld0 = ld;
				} else if (d >= table[l - 1]) {
					max = l;
					*p = d;
					ld0 = ld;
				}
			}
		}
	}
fin:

#if 0
	/* バグによる虚偽報告がないか確認 */
//	if (max > max0) { puts("len-max err!"); exit(1); } /* これはいつもOK */
//	for (l = 0; l < max; l++)
//		if (buf[*p+l] != buf[l]) { puts("string mismatch err!"); exit(1); } /* これもいつもOK */
	if (buf + *p < buf0 && max > 0) { printf("distance err!(%d:%d) ", *p, max); max = 0; }
#endif

	return max;

/* distanceの増加量がlの増加量のp倍を超えているようなら、損であるので採用しない */
/* これを増加量ではなく減少量から逆推定 */
/* distanceに基づき、lenはどこまで減らせるかを推定 */
/* distanceの最小値(l1のとき)は一瞬で出せる */

/* 1.まずdistanceの最小値を求める */
/* 2.次にdistanceの最大値を求める */
/* 3. */

}

void putbc(const int bits, int mask)
{
	do {
	//	putb((bits & mask) != 0);
		putb_byte = (putb_byte << 1) + ((bits & mask) != 0);
		if (--putb_count == 0) {
			putb_count = 8;
			if (putb_ptr < 0)
				putb_buf[putb_ptr] = putb_byte;
			else
				putb_overbuf[putb_ptr] = putb_byte;
			putb_ptr++;
		}
	} while (mask >>= 1);
	return;
}

void flushb()
{
	if (putb_count != 8) {
		putb_byte = putb_byte << 1 | 1; /* "1"を送る */
		if (--putb_count)
			putb_byte <<= putb_count;
		if (putb_ptr < 0)
			putb_buf[putb_ptr] = putb_byte;
		else
			putb_overbuf[putb_ptr] = putb_byte;
		putb_ptr++;
	}
	putb_count = 8;
	return;
}

void flushb0()
{
	if (putb_count != 8) {
		putb_byte = putb_byte << 1 | 0; /* "0"を送る */
		if (--putb_count)
			putb_byte <<= putb_count;
		if (putb_ptr < 0)
			putb_buf[putb_ptr] = putb_byte;
		else
			putb_overbuf[putb_ptr] = putb_byte;
		putb_ptr++;
	}
	putb_count = 8;
	return;
}

const int getbc(int bits)
{
	int ret = 0;
	do {
		if (putb_count == 8) {
			if (--putb_ptr < 0)
				return -1;
			putb_byte = *putb_overbuf++;
		}
		if (--putb_count == 0)
			putb_count = 8;
		ret <<= 1;
		if (putb_byte & 0x80)
			ret |= 0x01;
		putb_byte <<= 1;
	} while (--bits);
	return ret;
}

const int getbc0(int bits, int ret)
/* 初期値付き */
{
	do {
		if (putb_count == 8) {
			if (--putb_ptr < 0)
				return -1;
			putb_byte = *putb_overbuf++;
		}
		if (--putb_count == 0)
			putb_count = 8;
		ret <<= 1;
		if (putb_byte & 0x80)
			ret |= 0x01;
		putb_byte <<= 1;
	} while (--bits);
	return ret;
}

UCHAR *get_subbuf(UCHAR *subbuf, int *t, int *d, int *l)
{
	*t = *subbuf++;
	if (*t == 0x00) {
		*l = 0;
		*d = 0;
		return subbuf - 1;
	}
	if (*t == 0x01) {
		*l = 1;
		*d = 0;
		return subbuf;
	}
	*l = *subbuf++;
	if (*t & 0x20) {
		*l |= subbuf[0] <<  8;
		*l |= subbuf[1] << 16;
		*l |= subbuf[2] << 24;
		subbuf += 3;
	}
	*d = *subbuf++ | 0xffffff00;
	if (*t & 0x40) {
		*d = subbuf[-1];
		*d |= subbuf[0] <<  8;
		*d |= subbuf[1] << 16;
		*d |= subbuf[2] << 24;
		subbuf += 3;
	}
	return subbuf;
}

const int get_subbuflen(unsigned char *subbuf, int *pt, int j)
/* リピート長を検出 */
{
	int l, d, t, len = 0;

	get_subbuf(subbuf, &t, &d, &l);
	if (t == 0x00) {
		*pt = 0x00;
		return 0;
	}
	if (t == 0x01 || (t >= 0x02 && l < j + 1)) {
		*pt = 0x01;
		for (;;) {
			subbuf = get_subbuf(subbuf, &t, &d, &l);
			if (t == 0x00)
				break;
			if (t == 0x01)
				len++;
			else {
				if (l < j + 1)
					len += l;
				else
					break;
			}
		}
	} else {
		*pt = 0x02;
		for (;;) {
			subbuf = get_subbuf(subbuf, &t, &d, &l);
			if (t <= 0x01)
				break;
			if (l < j + 1)
				break;
			len++;
		}
	}
	return len;
}

void lzcmp_putnum1(int i)
/* 19 <= i <= 273 だと14bitコードになる */
/* 2-4-8-16形式で出力 */
{
	if (i <= 4 - 1)
		putbc(i - 1 + 1, 0x2); /* 2bit */
	else if (i <= 19 - 1)
		putbc(i - 4 + 1, 0x20); /* 6bit(4bit) */
	else if (i <= 274 - 1)
		putbc(i - 19 + 1, 0x2000); /* 14bit(8bit) */
	else if (i <= 65535)
		putbc(i, 0x20000000); /* 30bit(16bit) */
	else {
		unsigned int limit = 0xffff, mask = 0x4000, lenlen = 15;
		do {
			limit = limit * 2 + 1;
			mask <<= 1;
			lenlen++;
		} while (i > limit);
		putbc(lenlen, 0x20000000); /* 30bit */
		putbc(i, mask); /* nbit */
	}
	return;
}

void putnum_l1a(unsigned int i)
/* must i >= 1 */
/* sxsxsxsxs形式で出力 */
{
	int j;
	if (i == 1) {
		putbc(0x1, 0x1);
		return;
	}
	j = 31;
	while ((i & 0x80000000) == 0) {
		i <<= 1;
		j--;
	}
	do {
		i <<= 1; /* 最初の1は捨てる */
		if (i & 0x80000000)
			putbc(0x1, 0x2); /* sx */
		else
			putbc(0x0, 0x2); /* sx */
	} while (--j);
	putbc(0x1, 0x1); /* s("1") */
	return;
}

void putnum_l1b(unsigned int i)
/* must i >= 1 */
/* おそらく、l2aやl2bよりも優れている */
{
	if (i <= 2) {
		putbc(i + 1, 0x2); /* "10" or "11" */
		return;
	}
	putnum_l1a(i - 1); /* 3を2にする */
	return;
}

void putnum_df(int d, unsigned int s)
/* sのbitが1だと、そこまで出力したあとに、sビットを出力 */
{
	int len;
	unsigned int i = 1;
//	if (d == 0) { /* リピートマーク出力 */
//		for (i = 1; (i & s) == 0; i <<= 1)
//			putbc(1, 1);
//		putbc(0x2, 0x2); /* "10" */
//		return;
//	}

	i = 31;
	while (i > 0 && (d & (1 << i)) != 0)
		i--;
	/* i = dの0が見付かったビット位置(0〜31) */

	len = -1;
	do {
		do {
			len++;
		} while ((s & (1 << len)) == 0);
	} while (i > len);

	for (;;) {
		i = s & 1;
		s >>= 1;
		if (d & (1 << len))
			putbc(1, 1);
		else
			putbc(0, 1);
		len--;
		if (i == 0)
			continue;
		if (len < 0)
			break;
		putbc(0, 1); /* 継続bit */
	}
	if (s)
		putbc(1, 1); /* 非継続bit */
	return;
}

void putnum_s8(unsigned int s)
{
	int j = 4;
	while ((s & 0xff000000) == 0 && j > 1) {
		s <<= 8;
		j--;
	}
	for (;;) {
		putbc(s >> 24, 0x80);
		if (j == 1)
			break;
		putbc(0x0, 0x1);
		s <<= 8;
		j--;
	}
	putbc(0x1, 0x1);
	return;
}

void putnum_l0a(int i, int z)
{
	static int l[4] = { 0x7fffffff, 4, 8, 16 };
	int j;
	z = l[z];
	if (i <= z) {
		while (--i)
			putbc(0x0, 0x1); /* (i - 1)個の"0" */
		putbc(0x1, 0x1);
		return;
	}
	j = z;
	do {
		putbc(0x0, 0x1); /* z個の"0" */
	} while (--j);
	putnum_l1b(i - z);
	return;
}

int getnum_l1a()
{
	int i = 1, j;
	for (;;) {
		j = getbc(1);
		if (j < 0)
			return j;
		if (j)
			break;
		i = getbc0(1, i);
		if (i < 0)
			break;
	}
	return i;
}

int getnum_l1b()
{
	int i = getnum_l1a();
	if (i < 0)
		return i;
	if (i == 1) {
		i = getbc(1);
		if (i < 0)
			return i;
	}
	return i + 1;
}

int getnum_df(unsigned int s)
{
	int d = -1, t;
	for (;;) {
		do {
			d = getbc0(1, d);
			t = s & 1;
			s >>= 1;
		} while (t == 0);
		if (s == 0)
			break;
		if (getbc(1))
			break;
	//	if (d == -1)
	//		return 0;
	}
	return d;
}

int getnum_s8()
{
	int s;
	s = getbc(8);
	while (getbc(1) == 0)
		s = getbc0(8, s);
	return s;
}

const int getnum_l0a(int z)
{
	static int l[4] = { 0x7fffffff, 4, 8, 16 };
	int i = 1, j;
	z = l[z];
	while (i < z) {
		j = getbc(1);
		if (j < 0)
			return j;
		if (j)
			return i;
		i++;
	}
	j = getbc(1);
	if (j < 0)
		return j;
	if (j)
		return i;
	j = getnum_l1b();
	if (j < 0)
		return j;
	return j + i;
}

const int calclen_l1a(unsigned int i)
/* must i >= 1 */
/* sxsxsxsxs形式で出力 */
{
	int j, l = 0;
	if (i == 1)
		return 1;
	j = 31;
	while ((i & 0x80000000) == 0) {
		i <<= 1;
		j--;
	}
	do {
		i <<= 1; /* 最初の1は捨てる */
		l += 2;
	} while (--j);
	return l + 1;
}

const int calclen_l1b(unsigned int i)
/* must i >= 1 */
/* sxsxsxsxs形式で出力 */
{
	if (i <= 2)
		return 2;
	return calclen_l1a(i - 1); /* 3を2にする */
}

const int calclen_df(int d, unsigned int s)
/* sのbitが1だと、そこまで出力したあとに、sビットを出力 */
{
	int len, l = 0;
	unsigned int i = 1;
//	if (d == 0) { /* リピートマーク出力 */
//		for (i = 1; (i & s) == 0; i <<= 1)
//			l++;
//		l += 2;
//		return l;
//	}

	i = 31;
	while (i > 0 && (d & (1 << i)) != 0)
		i--;
	/* i = dの0が見付かったビット位置(0〜31) */

	len = -1;
	do {
		do {
			len++;
		} while ((s & (1 << len)) == 0);
	} while (i > len);

	for (;;) {
		i = s & 1;
		s >>= 1;
		l++;
		len--;
		if (i == 0)
			continue;
		if (len < 0)
			break;
		l++;/* 継続bit */
	}
	if (s)
		l++; /* 非継続bit */
	return l;
}

int calclen_l0a(unsigned int i, int z)
/* must i >= 1 */
{
	static int l[4] = { 0x7fffffff, 4, 8, 16 };
	z = l[z];
	if (i <= z)
		return i;
	return z + calclen_l1b(i - z);
}

/* l2d3エンコード */

int lzcompress_l2d3(unsigned char *buf, int k, int i, int outlimit, int maxdis)
{
	int len, maxlen, srchloglen = -1, srchlogdis = 0;
	int range, distance;
	int j, i0;
//	int ptr0 = putb_ptr;

	#if (defined(NOWARN))
		i0 = 0;
	#endif

	while (i < k) {
		if (outlimit >= putb_ptr + (putb_count != 8))
			i0 = i;
		else
			return i0;

		if (i == 0)
			len = 0;
		else {
			range = i - maxdis;
			if (range < 0)
				range = 0;
			maxlen = k - i;
			distance = srchlogdis;
			if ((len = srchloglen) < 0)
				len = search(buf + i,  buf + range, maxlen, &distance);
			srchloglen = -1;
			if (len >= 2) {
				range = i + 1 - maxdis;
				if (range < 0)
					range = 0;
				srchloglen = search(buf + i + 1,  buf + range, maxlen - 1, &srchlogdis);
				if (len < srchloglen)
					len = 0;
			}
		}

		if (len < 1) {
			putbc(0x100 | buf[i], 0x100); /* "1" + buf[i] */
			i++;
		} else {
			i += len;
			if (len >= 2)
				srchloglen = -1;
			putbc(0, 0x1);
			lzcmp_putnum1(len);

			/* 上位から出力せよ */

#if 1
#define	DLEN	3
			/* 2GB以上には対応していない */
			for (j = 31 / DLEN; j >= 1 && (distance >> (j * DLEN)) == -1; j--);
			if (j >= 0) {
			//	putbc(1, 0x1); /* 1bit */
				while (j) {
					putbc((distance >> (DLEN * j - 1)) | 1, 1 << DLEN); /* DLEN bit */
					j--;
				}
				putbc(distance << 1, 1 << DLEN); /* DLEN bit */
			} else
				putbc(0, 0x1); /* 1bit */

#endif

		}
	}

	flushb();

//	printf("%d -> %d (%f%%)¥n", k, putb_ptr - ptr0, (double) (putb_ptr - ptr0) * 100 / k);

	return k;
}

/* l2d3デコード */

int lzrestore_l2d3(unsigned char *buf, int k, int i, int outlimit)
{
	int len, distance, j;

	i = 0;
	for (;;) {
		j = getbc(1);
		if (j < 0)
			return i;
		if (j > 0) {
			j = getbc(8);
			if (j < 0)
				return i;
			buf[i++] = j;
			continue;
		}
		/* len */
		j = getbc(2);
//		if (j < 0)
//			return i;
		len = j;
		if (j == 0) {
			j = getbc(4);
			len = j + 3;
			if (j == 0) {
				j = getbc(8);
				len = j + 18;
				if (j == 0) {
					j = getbc(16);
					len = j;
					if (j <= 127)
						len = getbc0(j, 1); /* 最初のbitは1に決まっているから */
				}
			}
		}
		distance = -1;
		do {
			distance = getbc0(3, distance);
			j = getbc(1);
//			if (j < 0)
//				return i;
		} while (j);
		do {
			buf[i] = buf[i + distance];
			i++;
		} while (--len);
	}
}

/* tek0関係 */

struct STR_STATISTICS {
	unsigned int count, code;
};

#define STAT_TABLE_SIZE		1024 * 1024	* 2 /* 4MB(8MB) */


int setstatistics0(int siz, struct STR_STATISTICS *stat, unsigned int len, unsigned int *dat, int tablesize)
/* codeでソートされる */
{
	unsigned int *count0, i = 0, k;
	int j, l, min, max, middle;
	count0 = malloc(tablesize * sizeof (int));
	for (j = 0; j < tablesize; j++)
		count0[j] = 0;
	for (j = 0; j < len; j++) {
		if (dat[j] < tablesize)
			count0[dat[j]]++;
		else {
			k = 0;
			if (i == 0)
				goto find;
			k = i;
			if (stat[i - 1].code < dat[j])
				goto find;
			min = 0;
			max = i - 1;
			for (;;) {
				middle = (min + max) / 2;
				if (min == middle)
					break;
				if (stat[middle].code <= dat[j])
					min = middle;
				else
					max = middle;
			}
			k = min;

			for (; k < i && stat[k].code < dat[j]; k++);
find:
			if (k < i && stat[k].code == dat[j]) {
				stat[k].count++;
				goto skip;
			}
			if (i >= siz)
				goto err;
			for (l = i; l > k; l--) {
				stat[l].count = stat[l - 1].count;
				stat[l].code  = stat[l - 1].code;
			}
			stat[k].count = 1;
			stat[k].code  = dat[j];
			i++;
		}
skip:
		;
	}
	k = 0;
	for (j = 0; j < tablesize; j++) {
		if (count0[j])
			k++;
	}
	if (k + i >= siz) {
err:
		free(count0);
		return -1;
	}
	/* 押し下げて、さらに転送 */
	stat[k + i].count = 0; /* ターミネータ */
	for (j = i - 1; j >= 0; j--) {
		stat[k + j].count = stat[j].count;
		stat[k + j].code  = stat[j].code;
	}
	k = 0;
	for (j = 0; j < tablesize; j++) {
		if (count0[j]) {
			stat[k].count = count0[j];
			stat[k].code  = j;
			k++;
		}
	}
	free(count0);
	return k + i;
}

int calc_totalbits(const unsigned int *bit, const unsigned int stops)
{
	int i, t = 0;
	for (i = 0; i < 32; i++) {
		if (bit[i])
			t += bit[i] * calclen_df(-2 << i, stops);
	}
	return t;
}

#if 0

const int calc_stopbits0(const unsigned int *bit)
/* 下位bitから入れていく方法 */
{
	int l;
	unsigned int t, s, maxlen, t0;
	t = 0;
	for (l = 0; l < 32; l++) {
		t += bit[l];
	}
	if (t == 0)
		return 0;
	for (maxlen = 31; bit[maxlen] == 0; maxlen--);
	s = 1 << maxlen;
	t0 = calc_totalbits(bit, s);
	for (l = 0; l < maxlen; l++) { /* 下から入れている */
		t = calc_totalbits(bit, s | 1 << l);
		if (t0 > t) {
			s |= 1 << l;
			t0 = t;
		}
	}
	return s;
}

const int calc_stopbits1(const unsigned int *bit)
/* 上位bitから入れていく方法 */
{
	int l;
	unsigned int t, s, maxlen, t0;
	t = 0;
	for (l = 0; l < 32; l++) {
		t += bit[l];
	}
	if (t == 0)
		return 0;
	for (maxlen = 31; bit[maxlen] == 0; maxlen--);
	s = 1 << maxlen;
	t0 = calc_totalbits(bit, s);
	for (l = maxlen - 1; l >= 0; l--) { /* 上から入れている */
		t = calc_totalbits(bit, s | 1 << l);
		if (t0 > t) {
			s |= 1 << l;
			t0 = t;
		}
	}
	return s;
}

#endif

int calc_stopbits2(const unsigned int *bit)
/* 利益の多いところから入れていく方法 */
{
	int l, l0;
	unsigned int t, s, maxlen, t0, min;
	t = 0;
	#if (defined(NOWARN))
		l0 = 0;
	#endif
	for (l = 0; l < 32; l++) {
		t += bit[l];
	}
	if (t == 0)
		return 0;
	for (maxlen = 31; bit[maxlen] == 0; maxlen--);
	s = 1 << maxlen;
	min = t0 = calc_totalbits(bit, s);
	for (;;) {
		for (l = 0; l < maxlen; l++) {
			if ((s & (1 << l)) == 0) {
				t = calc_totalbits(bit, s | 1 << l);
				if (min > t) {
					min = t;
					l0 = l;
				}
			}
		}
		if (t0 <= min)
			break;
		t0 = min;
		s |= 1 << l0;
	}
	return s;
}

#define calc_stopbits	calc_stopbits2

#if 0

int calc_stopbits(const unsigned int *bit)
{
	unsigned int min, s0, method, t, s;
	s0 = calc_stopbits2(bit);
	min = calc_totalbits(bit, s0);
	method = 2;
#if 0
	s = calc_stopbits1(bit);
	t = calc_totalbits(bit, s);
	if (min > t) {
		min = t;
		s0 = s;
		method = 1;
	}
	s = calc_stopbits0(bit);
	t = calc_totalbits(bit, s);
	if (min > t) {
		min = t;
		s0 = s;
		method = 0;
	}
	printf("[%d] ", method);
#endif
	return s0;
}

#endif

int calcdis_s(UCHAR *subbuf, const int i)
/* disのエンコード方式の自動選択 */
{
	unsigned int *distbl = malloc(32 * sizeof (int)), dis_s;
	int j, l, d, t;
	for (j = 0; j < 32; j++)
		distbl[j] = 0;
	for (;;) {
		subbuf = get_subbuf(subbuf, &t, &d, &l);
		if (t == 0x00)
			break;
		if (t == 0x01)
			continue;
		if (l < i + 1)
			continue;
		if (d == -1)
			j = 0;
		else {
			j = 31;
			while (d & 0x80000000) {
				d <<= 1;
				j--;
			}
		}
		distbl[j]++;
	}
	dis_s = calc_stopbits(distbl);
	free(distbl);
	return dis_s;
}

int lzcompress_tek0(int prm, unsigned char *buf, int k, int i, int outlimit, int maxdis)
{
	unsigned char *subbuf, *subbuf0, *subbuf1;
	int len, maxlen, srchloglen = -1, srchlogdis = 0;
	int range, distance, dis_s;
	int i0, j, z, z0;
	static int table[10] = {
		-8, -2 * 1024, -128 * 1024, -16 * 1024 * 1024, -2 * 1024 * 1024 * 1024, 
		-0x7fffffff, -0x7fffffff, -0x7fffffff, -0x7fffffff, -0x7fffffff
	};
	int t, l, d, ii, ii0;

	#if (defined(NOWARN))
		z0 = ii0 = 0;
	#endif

	subbuf0 = malloc(SIZEOFSUBBUF);
	subbuf = subbuf0;

	while (i < k) {
	//	if (outlimit >= putb_ptr + (putb_count != 8))
	//		i0 = i;
	//	else
	//		return i0;

		if (i == 0)
			len = 0;
		else {
			range = i - maxdis;
			if (range < 0)
				range = 0;
			maxlen = k - i;
			len = srchloglen;
			distance = srchlogdis;
			if ((len = srchloglen) < 0)
				len = search0a(prm, buf + i,  buf + range, maxlen, &distance, table);
			srchloglen = -1;
			if (len >= 2) {
				range = i + 1 - maxdis;
				if (range < 0)
					range = 0;
				srchloglen = search0a(prm, buf + i + 1,  buf + range, maxlen - 1, &srchlogdis, table);
				if (len < srchloglen) {
					len = 0;
					if (i >= 8) {
						for (distance = -1; distance <= -8; distance--) {
							if (buf[i] == buf[i + distance]) {
								len = 1;
								break;
							}
						}
					}
				}
			}
		}

		if (len < 1) {
			subbuf[0] = 0x01;
			i++;
			subbuf++;
		} else {
			i += len;
			if (len >= 2)
				srchloglen = -1;
			subbuf[0] = 0x13;
			subbuf1 = subbuf;
			subbuf[1] = len & 0xff;
			subbuf += 2;
			if (len > 255) {
				subbuf[0] = (len >>  8) & 0xff;
				subbuf[1] = (len >> 16) & 0xff;
				subbuf[2] = (len >> 24) & 0xff;
				subbuf += 3;
				*subbuf1 = 0x36;
			}
			*subbuf++ = distance & 0xff;
			if (distance < -256) {
				subbuf[0] = (distance >>  8) & 0xff;
				subbuf[1] = (distance >> 16) & 0xff;
				subbuf[2] = (distance >> 24) & 0xff;
				subbuf += 3;
				*subbuf1 += 0x43;
			}

		}
	}
	*subbuf = 0x00;

	/* lenのエンコード方式の自動選択 */
	i0 = 0x7fffffff;
	for (z = 0; z < 16; z++) {
		for (ii = 0; ii < 2; ii++) {
			for (j = 0; j < 4; j++) {
				/* 最初にdis_sを算出し、l1bを正確に計算する */
				dis_s = calcdis_s(subbuf0, j);
				subbuf = subbuf0;
				len = 0;
				for (;;) {
					/* "0"-phase (非圧縮フェーズ) */
					srchloglen = get_subbuflen(subbuf, &t, j);
					if (srchloglen == 0)
						break;
					len += calclen_l0a(srchloglen, z & 0x03);
					do {
						subbuf = get_subbuf(subbuf, &t, &d, &l);
						if (t == 0x01) {
							len += 8;
							srchloglen--;
						} else {
							len += 8 * l;
							srchloglen -= l;
						}
					} while (srchloglen);

					/* "1"-phase (圧縮フェーズ) */
					srchloglen = get_subbuflen(subbuf, &t, j);
					if (srchloglen == 0)
						break;
					len += calclen_l0a(srchloglen, z >> 2);
					do {
						subbuf = get_subbuf(subbuf, &t, &d, &l);
						len += calclen_df(d, dis_s);
						if (ii == 0)
							len += calclen_l1a(l - j);
						else
							len += calclen_l1b(l - j);
					} while (--srchloglen);
				}
				if (i0 > len) {
					i0 = len;
					i = j;
					ii0 = ii;
					z0 = z;
				}
			}
		}
	}

	/* disのエンコード方式の自動選択 */
	dis_s = calcdis_s(subbuf0, i);
//	printf("method:l1%c(+%d) = %8d rep-mode:%x ", 'a' + ii0, i, i0, z0);
//	printf("dis_s = %08x¥n", dis_s);

	/* エンコード */
	putnum_s8(dis_s);
	putbc(i, 0x2);
	putbc(ii0, 0x1); /* l1a/l1b */
	putbc(z0, 0x2); /* "0"-phase */
	putbc(z0 >> 2, 0x2); /* "1"-phase */

	subbuf = subbuf0;
	j = 0;
	for (;;) {
		/* "0"-phase (非圧縮フェーズ) */
		srchloglen = get_subbuflen(subbuf, &t, i);
		if (srchloglen == 0)
			break;
		putnum_l0a(srchloglen, z0 & 0x03);
		do {
			subbuf = get_subbuf(subbuf, &t, &d, &l);
			if (t == 0x01)
				l = 1;
			do {
				putbc(buf[j], 0x80);
				j++;
				srchloglen--;
				l--;
			} while (l);
		} while (srchloglen);

		/* "1"-phase (圧縮フェーズ) */
		srchloglen = get_subbuflen(subbuf, &t, i);
		if (srchloglen == 0)
			break;
		putnum_l0a(srchloglen, z0 >> 2);
		do {
			subbuf = get_subbuf(subbuf, &t, &d, &l);
			putnum_df(d, dis_s);
			if (ii0 == 0)
				putnum_l1a(l - i);
			else
				putnum_l1b(l - i);
			j += l;
		} while (--srchloglen);
	}

	flushb0();
	free(subbuf0);

	return k;
}

int lzrestore_tek0(unsigned char *buf, int k, int i, int outlimit)
{
	int len, distance, j, z0, z1;
	unsigned int dis_s, l_ofs, method;

//int dbc_by = 0, dbc_lz = 0, dbc_cp = 0, dbc_ds = 0;
//int ddd = 100;

	/* ヘッダ読み込み */
	dis_s = getnum_s8();
	l_ofs = getbc(2);
	method = getbc(1); /* l1a/l1b */
	z0 = getbc(2);
	z1 = getbc(2);

	#if (defined(DEBUGMSG))
		printf("method:l1%x(+%d) dis_s = %08x z0 = %d z1 = %d¥n", 0xa + method, l_ofs, dis_s, z0, z1);
	#endif

	i = 0;
	for (;;) {
		/* "0"-phase (非圧縮フェーズ) */
		j = getnum_l0a(z0);
		if (j < 0)
			break;
//dbc_by += calclen_l0a(j, z0);
		do {
			len = getbc(8);
			if (len < 0)
				break;
			buf[i++] = len;
		} while (--j);

		/* "1"-phase (圧縮フェーズ) */
		j = getnum_l0a(z1);
		if (j < 0)
			break;
//dbc_lz += calclen_l0a(j, z1);
		do {
			distance = getnum_df(dis_s);
//dbc_ds += calclen_df(distance, dis_s);
			if (method == 0) {
				len = getnum_l1a();
//dbc_cp += calclen_l1a(len);
			} else {
				len = getnum_l1b();
//dbc_cp += calclen_l1b(len);
//printf("%d ", calclen_l1b(len));
//if (--ddd == 0) exit(1);
			}
			if (len < 0)
				break;
			len += l_ofs;
			do {
				buf[i] = buf[i + distance];
				i++;
			} while (--len);
		} while (--j);
	}
//printf("%d %d %d %d¥n", dbc_by, dbc_lz, dbc_cp, dbc_ds);
	return i;
}

int tek1_intlog2p(int i);

void tek1_puts7(unsigned int i)
{
	if (i < 0x80)
		goto len1;
	i -= 0x80;
	if (i < 0x4000)
		goto len2;
	i -= 0x4000;
	if (i < 0x200000)
		goto len3;
	i -= 0x200000;
	if (i < 0x10000000)
		goto len4;
	i -= 0x10000000;
	*tek1_s7ptr++ = ((i >> 28) & 0x7f) << 1;
len4:
	*tek1_s7ptr++ = ((i >> 21) & 0x7f) << 1;
len3:
	*tek1_s7ptr++ = ((i >> 14) & 0x7f) << 1;
len2:
	*tek1_s7ptr++ = ((i >>  7) & 0x7f) << 1;
len1:
	*tek1_s7ptr++ = (i & 0x7f) << 1 | 1;
	return;
}

UCHAR *tek1_puts7sp(UCHAR *p, unsigned int i)
{
	if (i < 0x80)
		goto len1;
	if (i < 0x4000)
		goto len2;
	if (i < 0x200000)
		goto len3;
	if (i < 0x10000000)
		goto len4;
	*p++ = ((i >> 28) & 0x7f) << 1;
len4:
	*p++ = ((i >> 21) & 0x7f) << 1;
len3:
	*p++ = ((i >> 14) & 0x7f) << 1;
len2:
	*p++ = ((i >>  7) & 0x7f) << 1;
len1:
	*p++ = (i & 0x7f) << 1 | 1;
	return p;
}

void tek1_puts7s(unsigned int i)
{
	tek1_s7ptr = tek1_puts7sp(tek1_s7ptr, i);
	return;
}

int lzcompress_tek3h(int srcsiz, int *src, int outsiz, UCHAR *outbuf, UCHAR *work, int modecode);
int lzcompress_tek3s(int srcsiz, UCHAR *src, int outsiz, UCHAR *outbuf, int wrksiz, UCHAR *work, int flags, int prm, int maxdis);

void debugtest();

int lzcompress_tek3(int srcsiz, UCHAR *src, int outsiz, UCHAR *outbuf, int wrksiz, UCHAR *work, int bsiz, int flags, int opt, int prm, int maxdis, int submaxdis)
/* workは8バイトアラインしておくこと、srcも8バイトアラインが望ましい */
/* レベル1までしかないので、これだと256KBまでしか圧縮できない */
/* レベル2も書けば16MB。これなら悪くはない */
/* レベル3も書けば1GB。ますますよろしい（bim2binとしてはこれが限界でもいいだろう） */
/* レベル1ヘッダは、66x5。レベル2ヘッダは65x5、レベル3ヘッダは65x5 */
/* (1 + 65 + 1 + 65 * 64 + 1 + 66 * 64 * 64 + 1) * 5 = 1341KB */
/* これはやってられないので、レベル2までのサポートに変更 */
/* (1 + 65 + 1 + 65 * 64 + 1) * 5 = 21KB */
/*
	64個というのはいいのか？
	tek1hで圧縮するべきではないのか。
	tek1hはレングス圧縮はなく、タイプやベースアドレスはs7で格納。次に継続ビットをs7で（limit=0)。4bit単位でlenbit総数。
	256個くらいはあったほうがいいだろう。これだと4KBブロックで、レベル1で1MB、レベル2で256MBになる。
	モード、ベースアドレス、継続ビットはs7、最初のサイズと差分サイズはuc
*/
/* workはlzcompress_tek1hが3KB使う */
{
	int i, j, k, l, m, *s256;
	UCHAR *p, *q, *r, *s, *t, *u;
	tek1_s7ptr = outbuf;
	if (bsiz > srcsiz) {
		while ((bsiz >> 1) >= srcsiz)
			bsiz >>= 1;
	}
	s256 = (int *) work;
	work += 256 * 257 * sizeof (int); /* 257KB */
	wrksiz -= 256 * 257 * sizeof (int);
//	if ((flags & 1) == 0)
//		tek1_puts7(srcsiz);
	for (i = 0, j = bsiz >> 8; j > 1; i++, j >>= 1);
	tek1_puts7s(i << 1 | 1); /* 上位3bitはバージョン(0はtek1sのみを意味する、さらにレベル0サイズ情報なし) */
	/* 下位1bitについて：0（スーパーショート） */
	/* 001:ノーマルロング、011:マルチモードロング、その他はリザーブ */
//	tek1_puts7(3); /* 3:tek1, 2:無圧縮 */
//	tek1_puts7(1); /* ターミネータ */
	p = tek1_s7ptr;
	r = q = &p[322 * 1024]; /* レベル0出力用 */
	i = (srcsiz + bsiz - 1) / bsiz;
	s = src;
	for (j = 0; j < i; j++, s += bsiz) {
		t = src;
		if (&src[srcsiz] >= &s[bsiz]) {
			for (k = 0; k < j; k++, t += bsiz) {
				for (l = 0; l < bsiz; l++) {
					if (s[l] != t[l])
						goto skip0;
				}
				/* 一致ブロック発見 */
				s256[j] = s256[k]; /* k番目のブロックの内容をそのまま使え */
				goto skip1;
	skip0:
				;
			}
		}
		k = &src[srcsiz] - s;
		if (k > bsiz)
			k = bsiz;
		s256[j] = q - r; /* ブロック開始位置 */
	//	k = lzcompress_tek3s(k, s, &outbuf[outsiz] - q, q, wrksiz, work,
	//		(flags & 1) | (i == 1 && (flags & 1) != 0) << 1 | opt << 2, prm, maxdis);
		q += k;
skip1:
		;
	}
	t = s = &p[(1 + 256) * 5];
	/* i:ブロック総数 */
	/* j:レベル1ブロック総数 */
	j = (i + 255) >> 8;
	for (k = 0; k < j; k++) {
		if (((k + 1) << 8) <= i) {
			for (l = 0; l < k; l++) {
				for (m = 0; m < 256; m++) {
					if (s256[(k << 8) + m] != s256[(l << 8) + m])
						goto skip2;
				}
				/* 一致ブロック発見 */
				s256[256 * 256 + k] = s256[256 * 256 + l];
				goto skip3;
	skip2:
				;
			}
		}
		l = i - (k << 8);
		if (l > 256)
			l = 256;
		s256[256 * 256 + k] = s - t; /* ブロック開始位置 */

		s += lzcompress_tek3h(l, &s256[k << 8], r - s, s, work, 0-1 /* mode */);
		/* フォーマットバージョン0ではモードフィールドはない */
skip3:
		;
	}
	if (j > 1) {
		/* レベル2出力 */
		u = &p[5];
		m = lzcompress_tek3h(j, &s256[256 * 256], t - u, u, work, -1 /* mode */);
		tek1_s7ptr = p;
		tek1_puts7s(m - 2); /* レベル2ゾーンサイズ */
		p = tek1_s7ptr;
		for (l = 0; l < m; l++)
			p[l] = u[l];
		p += m;
	}
	/* レベル1出力(s, t) */
	if (i > 1) {
		tek1_s7ptr = p;
		tek1_puts7s((m = s - t) - 2); /* レベル1ゾーンサイズ */
		p = tek1_s7ptr;
		for (l = 0; l < m; l++)
			p[l] = t[l];
		p += m;
	}
	/* レベル0出力(q, r) */
	if (i == 1 && (flags & 1) != 0)
		p--; /* tek2ショート */
	else {
		/* verによっては以下をやる */
	//	tek1_s7ptr = p;
	//	tek1_puts7((m = q - r) - 2); /* レベル0ゾーンサイズ */
	//	p = tek1_s7ptr;
	}
	m = q - r;
	for (l = 0; l < m; l++)
		p[l] = r[l];
	return &p[m] - outbuf;
}

struct STR_BITBUF {
	UCHAR *p0, *p2, *p, *p1;
	int bit, bit1;
};

void initbitbuf(struct STR_BITBUF *str, unsigned int len, UCHAR *p0)
{
	str->p0 = p0;
	str->p2 = p0 + len;
	str->p1 = p0;
	str->bit1 = 0;
//	rewindbitbuf(str);
	str->p = str->p0;
	str->bit = 0;
	return;
}

void putbitbuf(struct STR_BITBUF *str, unsigned int len, unsigned int dat)
/* 最大32bit出力 */
{
	do {
		if (dat & 1)
			*(str->p) |= 1 << (str->bit);
		else
			*(str->p) &= ‾(1 << (str->bit));
		if ((str->bit = (str->bit + 1) & 0x07) == 0)
			str->p++;
		dat >>= 1;
	} while (--len);
	if ((str->p1 < str->p) || (str->p1 == str->p && str->bit1 < str->bit)) {
		str->p1 = str->p;
		str->bit1 = str->bit;
	}
	return;
}

struct TEK1_STR_UC { /* ユニバーサルコード */
	UCHAR lentbl[TEK1_MAXLEN + 1], maxlen, dummy[3];
	int limit, base[TEK1_MAXLEN + 2];
};

#define TEK1_DEFINED_UC		1

#define STR_UC	TEK1_STR_UC

static UCHAR tek1_ucprm_s4[] = { 0x0c, 0x22, 0x22, 0x62 };
//	/* 0_0_011101 11011101 11011101 111_0_1011	24, 444444 */
	/* 0_1_100010 00100010 00100010 000_0_1100	24, 444444 */
//static UCHAR tek1_ucprm_l0a4[] = { 0x0b, 0x02, 0x00, 0x00, 0x00 };
//	/* 00000_0_00 00000000 00000000 00000010 000_0_1011	24, 1111111111111111111111110000 */
static UCHAR tek1_ucprm_l1a[] = { 0xac, 0xff, 0xff, 0xff };
//	/* 0_0000000 00000000 00000000 010_0_1011			24, 1111111111111111111111110 */
	/* 1_1111111 11111111 11111111 101_0_1100			24, 1111111111111111111111110 */
static UCHAR tek1_ucprm_l1b[] = { 0xdc, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0x6a };
//	/* 0_0_010101 01010101 01010101 01010101 01010101 01010101 001_1_1011	24, 1111111111111111111111101 */
	/* 0_1_101010 10101010 10101010 10101010 10101010 10101010 110_1_1100	24, 1111111111111111111111101 */
static UCHAR tek1_ucprm_l1c[] = { 0xcc, 0xff, 0xff, 0x7f };
//	/* 0_0_000000 00000000 00000000 001_0_1011			24, 111111111111111111111111 */
	/* 0_1_111111 11111111 11111111 110_0_1100			24, 111111111111111111111111 */
static UCHAR tek1_ucprm_ds1[] = { 0x2c, 0x44, 0x52, 0xd5 };
//	/* 0_0101010 10101010 10111011 110_0_1011 			24, 222222 22440 */
	/* 1_1010101 01010101 01010100 001_0_1100 			24, 222222 22440 */
	/* 1_1010101 01010010 01000100 001_0_1100 			24, 222222 33440 */
//static UCHAR tek2_ucprm_stbyds[] = { 0x82, 0x77, 0x00 };
//	/* 0000000_0 _01110111 10000_0_10	8, 440000 */
static UCHAR tek1_ucprm_s41[] = { 0x2c, 0x44, 0x44, 0xc4 };
//	/* 0_0111011 10111011 10111011 110_0_1011			24, 4444440 */
	/* 1_1000100 01000100 01000100 001_0_1100			24, 4444440 */

#if 0

static UCHAR tek2_ucprm_tr8[] = { 0x03, 0x18 };
//	/* 000_0_0111 11111_0_10							8, 8 */
	/* 000_1_1000 00000_0_11							8, 8 */
static UCHAR tek1_ucprm_ds0[] = { 0x0c, 0x22, 0xa9, 0x6a };
//	/* 0_1_101010 10101010 10101010 000_0_1100 			24, 222222 2244 */
	/* 0_1_101010 10101001 00100010 000_0_1100 			24, 222222 3344 */

#endif

//staic UCHAR pad[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

//void tek1_inituc0(struct TEK1_STR_BTBUF *btbuf, struct TEK1_STR_UC *ucstr);
void tek1_inituc1(struct TEK1_STR_UC *ucstr, UCHAR *prm);
unsigned int calclen_uc(struct STR_UC *ucstr, struct STR_STATISTICS *stat);
void putnum_uc0(struct STR_BITBUF *bitbuf, struct STR_UC *ucstr, unsigned int i);

void initucstr(struct TEK1_STR_UC *ucstr, UCHAR flag, UCHAR *param0)
{
//	if (flag)
//		tek1_inituc0(ucstr);
//	else
		tek1_inituc1(ucstr, param0);
	return;
}

UCHAR calclen_ucstr_sub(struct STR_UC *ucstr, int i0)
/* bit3-7:max(4, 8, 12, 16, 20, 24), bit0-2:mode(0, 1, 7) */
{
	UCHAR m = 0, max;
	int i;
	i = ucstr->maxlen;
	max = ucstr->lentbl[i];
	for (i--; i >= i0; i--) {
		if (max < ucstr->lentbl[i])
			max = ucstr->lentbl[i];
		if (ucstr->lentbl[i] > ucstr->lentbl[i + 1])
			m = 7;
	}
	max = (max + 3) & ‾3;
	if (max == 0)
		max = 4;
	if (m < 7) {
		i = ucstr->maxlen;
		if (i > 0 && ucstr->lentbl[i - 1] >= max)
			max += 4; /* maxが2度以上出現するのを回避するため */
		for (i = i0; i < ucstr->maxlen && ucstr->lentbl[i] == 0; i++);
		for (; i < ucstr->maxlen; i++) {
			if (ucstr->lentbl[i] >= ucstr->lentbl[i + 1]) {
				m = 1;
				break;
			}
		}
	}
	return m | max << 3;
}

int calclen_uc0str(struct STR_UC *ucstr, int i, int n)
{
	UCHAR m, l = 2 + 1 + 1, max;
	int d;
	max = m = calclen_ucstr_sub(ucstr, i);
	m &= 0x07;
	max = (max >> 3) & 0x1f;
	if (m == 7)
		l = (3 + 2 + 2) + ucstr->maxlen * 2; /* 符号bit出力回数をあらかじめ足しておく(maxlen + 1)個 */
	if (m == 1)
		l = (2 + 1 + 1) + ucstr->maxlen; /* m == 0のときに比べて1長いことを足しておく */
	for (i = 0; i <= ucstr->maxlen; i++) {
		d = ucstr->lentbl[i] - n;
		if (d < 0)
			d ^= -1;
		l += d;
		if (m == 0 && d == 0)
			l++;
		n = ucstr->lentbl[i];
	}
	if (m < 7) {
		if (n == max)
			goto fin;
		l += max - n + m; /* (max + m - n)個の1を出す */
	} else {
		if (n > max - n)
			l += max - n + 2; /* プラス側への追い出し */
		else {
			l += n + 2; /* マイナス側への追い出し */
			if (n == 0)
				l--; /* 符号bitだけでいいので */
		}
	}
fin:
	if (max >= 12)
		l++;
	if (max >= 20)
		l++;
	if (m < 7 && n > 0 && ucstr->maxlen > 0)
		l++;
	return l;
}

int calclen_ucstr(struct STR_UC *ucstr)
{
	return calclen_uc0str(ucstr, 0, 0);
}

void output_uc0str(struct STR_BITBUF *bitbuf, struct STR_UC *ucstr, int i, int n)
/* 最後だけ小さいというやつにはうまく対応できず、m=7でやってしまう */
{
	UCHAR m, max;
	int d;
	max = m = calclen_ucstr_sub(ucstr, i);
	m &= 0x07;
	max = (max >> 3) & 0x1f;
	if (m == 7)
		putbitbuf(bitbuf, 3, 0); /* "000" */
	if (max <= 8)
		putbitbuf(bitbuf, 1, 1); /* "1" */
	else if (max <= 16)
		putbitbuf(bitbuf, 2, 2); /* "10" */
	else
		putbitbuf(bitbuf, 3, 4); /* "100" */
	putbitbuf(bitbuf, 1, (max >> 2) - 1);
	if (m < 7)
		putbitbuf(bitbuf, 1, m);
//	n = 0;
	for ( /* i = 0 */; i <= ucstr->maxlen; i++) {
		d = ucstr->lentbl[i] - n;
		if (m == 7) {
			if (d >= 0)
				putbitbuf(bitbuf, 1, 0);
			else {
				putbitbuf(bitbuf, 1, 1);
				d ^= -1;
			}
		}
		if (m == 0 && n > 0)
			d--;
		if (d)
			putbitbuf(bitbuf, d, 0);
		putbitbuf(bitbuf, 1, 1);
		n = ucstr->lentbl[i];
	}
	if (m < 7) {
		if (n == max)
			goto fin;
		d = max - n + 1; /* (max + 1 - n)個の"0"を出す */
		if (m == 0 && n > 0)
			d--;
	} else {
		if (n >= max - n) {
			putbitbuf(bitbuf, 1, 0);
			d = max - n + 1; /* プラス側への追い出し */
		} else {
			putbitbuf(bitbuf, 1, 1);
			if (n == 0)
				goto fin;
			d = n + 1; /* マイナス側への追い出し */
		}
	}
	putbitbuf(bitbuf, d, 0);
fin:
	if (m < 7 && n > 0 && ucstr->maxlen > 0)
		putbitbuf(bitbuf, 1, 1); /* 最終パラメータ補正値 */
	return;
}

void output_ucstr(struct STR_BITBUF *bitbuf, struct STR_UC *ucstr)
{
	output_uc0str(bitbuf, ucstr, 0, 0);
	return;
}


void putnum_uc0(struct STR_BITBUF *bitbuf, struct STR_UC *ucstr, unsigned int i)
{
	int j;
	for (j = 0; j <= TEK1_MAXLEN; j++) {
		if (ucstr->base[j] <= i && i < ucstr->base[j + 1]) {
			if (j > 0)
				putbitbuf(bitbuf, j, 0);
			if (j < ucstr->maxlen)
				putbitbuf(bitbuf, 1, 1);
			if (ucstr->lentbl[j])
				putbitbuf(bitbuf, ucstr->lentbl[j], i - ucstr->base[j]);
			break;
		}
	}
	return;
}


unsigned int calclen_uc(struct STR_UC *ucstr, struct STR_STATISTICS *stat)
{
	unsigned int i, j, k;
	k = 0;
	j = 0;
	for (i = 0; stat[i].count != 0; i++) {
		for (;;) {
			if (stat[i].code < ucstr->base[j + 1]) {
				k += (j + ucstr->lentbl[j]) * stat[i].count;
				if (j < ucstr->maxlen)
					k += stat[i].count;
				break;
			}
			j++;
		}
	}
	return k;
}

void optsub_inituc(struct STR_UC *ucstr, UCHAR *prm)
{
	int i;
	ucstr->maxlen = prm[TEK1_MAXLEN + 1];
	ucstr->base[0] = 0;
	for (i = 0; i <= ucstr->maxlen; i++) {
		ucstr->lentbl[i] = prm[i];
		ucstr->base[i + 1] = ucstr->base[i] + (1 << prm[i]);
	}
	return;
}

void optsub_limcbit(struct STR_UC *ucstr, int lim, unsigned int cbits)
/* maxlenもいじるので注意 */
{
	int i, j;
	for (i = 0; i < lim; i++)
		ucstr->lentbl[i] = 0;
	j = 0;
	do {
		for (;;) {
			j++;
			if ((cbits & 1) == 0)
				break;
			cbits >>= 1;
		}
		cbits >>= 1;
		ucstr->lentbl[i] = j;
		i++;
	} while (j < 24);
	ucstr->base[0] = 0;
	ucstr->maxlen = i - 1;
	for (j = 0; j < i; j++)
		ucstr->base[j + 1] = ucstr->base[j] + (1 << ucstr->lentbl[j]);
	return;
}

void optsub_saveprm(UCHAR *prm, struct STR_UC *ucstr)
{
	int i;
	prm[TEK1_MAXLEN + 1] = ucstr->maxlen;
	for (i = 0; i <= TEK1_MAXLEN; i++)
		prm[i] = ucstr->lentbl[i];
	return;
}

#if 1

static struct STR_UC *optsub_ucstr;
static int optsub_min, optsub_max;
static UCHAR *optsub_minprm;
static int (*optsub_func)(struct STR_UC *);

void optsub_fullopt1(int i, int rest, struct STR_STATISTICS *stat0, int total0)
{
	int base0 = optsub_ucstr->base[i], j, tmp, total1, base1;
	struct STR_STATISTICS *stat;
	optsub_ucstr->lentbl[i] = j = tek1_intlog2p(optsub_max - base0);
	optsub_ucstr->maxlen = i;
	tmp = total0 + (j + i) * rest;
	if (optsub_min > tmp) {
		tmp += (*optsub_func)(optsub_ucstr);
		if (optsub_min > tmp) {
			optsub_min = tmp;
			optsub_saveprm(optsub_minprm, optsub_ucstr);
		}
	}
	if (i == TEK1_MAXLEN)
		return;
	i++;
	for (j--; j >= 0; j--) {
		if (i > 1 && optsub_ucstr->lentbl[i - 2] > j)
			continue; /* 時間がかかるのでやらない */
		base1 = base0 + (1 << j);
		tmp = 0;
		for (stat = stat0; stat->code < base1; stat++)
			tmp += stat->count;
		total1 = total0 + (i + j) * tmp;
		optsub_ucstr->maxlen = i - 1; /* ここでいったん打ち切ってみる */
		optsub_ucstr->base[i] = base1;
		optsub_ucstr->lentbl[i - 1] = j;
		if (total1 + i * (rest - tmp) + (*optsub_func)(optsub_ucstr) < optsub_min)
			optsub_fullopt1(i, rest - tmp, stat, total1);
	}
	return;
}

void optsub_fullopt2(int i, int rest, struct STR_STATISTICS *stat0, int total0)
{
	int base0 = optsub_ucstr->base[i], j, tmp, total1, base1;
	struct STR_STATISTICS *stat;
	optsub_ucstr->lentbl[i] = j = tek1_intlog2p(optsub_max - base0);
	optsub_ucstr->maxlen = i;
	tmp = total0 + (j + i) * rest;
	if (optsub_min > tmp) {
		tmp += (*optsub_func)(optsub_ucstr);
		if (optsub_min > tmp) {
			optsub_min = tmp;
			optsub_saveprm(optsub_minprm, optsub_ucstr);
		}
	}
	if (i == TEK1_MAXLEN)
		return;
	i++;
	for (j--; j >= 0; j--) {
		base1 = base0 + (1 << j);
		tmp = 0;
		for (stat = stat0; stat->code < base1; stat++)
			tmp += stat->count;
		total1 = total0 + (i + j) * tmp;
		optsub_ucstr->maxlen = i - 1; /* ここでいったん打ち切ってみる */
		optsub_ucstr->base[i] = base1;
		optsub_ucstr->lentbl[i - 1] = j;
		if (total1 + i * (rest - tmp) + (*optsub_func)(optsub_ucstr) < optsub_min)
			optsub_fullopt2(i, rest - tmp, stat, total1);
	}
	return;
}

int optsub_evaluate1(int rest, struct STR_STATISTICS *st, int base0, int *pj, int k0, int mode);

int optsub_evaluate0(int rest, struct STR_STATISTICS *st, int base0, int *pj)
/* 結構それっぽい評価関数 */
/* ゼロ無しモードだけど、現在0行進中用 */
{
	int base1, k, eval = 0, tmp, j = 0;
	do {
		k = -1;
		tmp = 0;
		do {
			k++;
			base1 = base0 + (1 << k);
			while (st->count != 0 && st->code < base1) {
				tmp += st->count;
				st++;
			}
		} while (rest > tmp * 3);
		eval += tmp * k;
		base0 = base1;
		if (rest > tmp)
			eval += rest;
		j++;
		rest -= tmp;
	} while (rest > 0 && k == 0);
	if (rest > 0) {
		eval += optsub_evaluate1(rest, st, base0, &tmp, k, 0);
		j += tmp;
	}
	*pj = j;
	return eval;
}

int optsub_evaluate1(int rest, struct STR_STATISTICS *st, int base0, int *pj, int k0, int mode)
/* 結構それっぽい評価関数 */
/* ゼロ無しモードで、現在非0行進中用(mode=0) */
/* もしくはゼロありモード(mode=-1) */
{
	int base1, k, eval = 0, tmp, j = 0;
	do {
		k = k0 + mode;
		tmp = 0;
		do {
			k++;
			base1 = base0 + (1 << k);
			while (st->count != 0 && st->code < base1) {
				tmp += st->count;
				st++;
			}
		} while (rest > tmp * 3);
		eval += tmp * k;
		base0 = base1;
		if (rest > tmp)
			eval += rest;
		j++;
		rest -= tmp;
	} while (rest > 0);
	*pj = j;
	return eval;
}

int optsub_evaluate2(int rest, struct STR_STATISTICS *st, int base0, int *pj)
/* 結構それっぽい評価関数 */
/* マイナスもありモード */
{
	int base1, k, eval = 0, tmp, j = 0;
	do {
		k = -1;
		tmp = 0;
		do {
			k++;
			base1 = base0 + (1 << k);
			while (st->count != 0 && st->code < base1) {
				tmp += st->count;
				st++;
			}
		} while (rest > tmp * 3);
		eval += tmp * k;
		base0 = base1;
		if (rest > tmp)
			eval += rest;
		j++;
		rest -= tmp;
	} while (rest > 0);
	*pj = j;
	return eval;
}

int optimize_uc(struct STR_UC *ucstr, struct STR_STATISTICS *stat, int (*func)(struct STR_UC *), UCHAR *ucprm0, UCHAR *ucprm1, UCHAR *ucprm2)
/* 評価関数利用型の前進法＋単純二分法 */
{
	UCHAR minprm[TEK1_MAXLEN + 2];
	int min, i, j, k, tmp, rc = 0, rest, rest0, base0, base1, max, dummy;
	int count, tmpmin, min_k, min_c;
	struct STR_STATISTICS *st, *min_s;
	#if (defined(NOWARN))
		min_k = min_c = 0;
		min_s = stat;
	#endif
	if (ucprm0) {
		initucstr(ucstr, 0, ucprm0);
		if (stat->count == 0)
			goto fin;
		min = calclen_uc(ucstr, stat);
		optsub_saveprm(minprm, ucstr);
		if (ucprm2)
			min += 2;
	} else {
		if (ucprm1) {
			min = 0x7fffffff;
			goto skip;
		}
		optsub_limcbit(ucstr, 0, 0);
		if (stat->count == 0)
			goto fin;
		optsub_saveprm(minprm, ucstr);
		min = calclen_uc(ucstr, stat);
		for (j = 1; j < 8; j++) {
			optsub_limcbit(ucstr, j, 0);
			tmp = calclen_uc(ucstr, stat);
			if (min > tmp) {
				min = tmp;
				optsub_saveprm(minprm, ucstr);
				rc = j;
			}
		}
		min += 3;
	}
	if (ucprm1) {
		initucstr(ucstr, 0, ucprm1);
		tmp = calclen_uc(ucstr, stat);
		if (ucprm2)
			tmp += 2;
		if (min > tmp) {
			min = tmp;
			optsub_saveprm(minprm, ucstr);
			rc = 1;
		}
	}
	if (ucprm2) {
		initucstr(ucstr, 0, ucprm2);
		tmp = calclen_uc(ucstr, stat) + 2;
		if (min > tmp) {
			min = tmp;
			optsub_saveprm(minprm, ucstr);
			rc = 2;
		}
	}
skip:
	rest0 = 0;
	for (i = 0; stat[i].count != 0; i++)
		rest0 += stat[i].count;
	max = stat[i - 1].code + 1;

	for (i = 0; i < 3; i++) {
		ucstr->base[0] = base0 = 0;
		st = stat;
		rest = rest0;
		j = 0;
		do {
			k = 0;
			if (j > 0) {
				if (i < 2)
					k = ucstr->lentbl[j - 1];
				if (i == 0 && k > 0)
					k++;
			}
			count = 0;
			tmpmin = 0x7fffffff;
			do {
				base1 = base0 + (1 << k);
				while (st->count != 0 && st->code < base1) {
					count += st->count;
					st++;
				}
				tmp = k * count;
				if (rest > count) {
					tmp += rest;
					if (i == 0) {
						if (k == 0)
							tmp += optsub_evaluate0(rest - count, st, base1, &dummy);
						else
							tmp += optsub_evaluate1(rest - count, st, base1, &dummy, k, 0);
					}
					if (i == 1)
						tmp += optsub_evaluate1(rest - count, st, base1, &dummy, k, -1);
					if (i == 2)
						tmp += optsub_evaluate2(rest - count, st, base1, &dummy);
				}
				if (tmpmin > tmp) {
					tmpmin = tmp;
					min_k = k;
					min_c = count;
					min_s = st;
				}
				k++;
			} while (rest > count);
			base0 += 1 << min_k;
			ucstr->lentbl[j] = min_k;
			ucstr->base[j + 1] = base0;
			rest -= min_c;
			st = min_s;
			j++;
		} while (rest > 0 && j < TEK1_MAXLEN);
		ucstr->maxlen = j - 1;
		if (rest > 0) {
			k = 0;
			if (i < 2)
				k = ucstr->lentbl[TEK1_MAXLEN - 1];
			if (i == 0 && k > 0)
				k++;
			count = 0;
			do {
				base1 = base0 + (1 << k);
				while (st->count != 0 && st->code < base1) {
					count += st->count;
					st++;
				}
				k++;
			} while (st->count != 0);
			k--;
			ucstr->lentbl[TEK1_MAXLEN] = k;
			ucstr->base[TEK1_MAXLEN + 1] = base0 + (1 << k);
			ucstr->maxlen = TEK1_MAXLEN;
		}
		tmp = calclen_uc(ucstr, stat) + (*func)(ucstr);
		if (min > tmp) {
			min = tmp;
			optsub_saveprm(minprm, ucstr);
			rc = -1;
		}

		/* 単純二分法 */
		st = stat;
		rest = rest0;
		j = 0;
		base0 = 0;
		do {
			k = 0;
			if (j > 0) {
				if (i < 2)
					k = ucstr->lentbl[j - 1];
				if (i == 0 && k > 0)
					k++;
			}
			tmp = 0;
			k--;
			do {
				k++;
				base1 = base0 + (1 << k);
				while (st->count != 0 && st->code < base1) {
					tmp += st->count;
					st++;
				}
			} while (tmp * 3 < rest);
			ucstr->lentbl[j] = k;
			ucstr->base[j + 1] = base0 = base1;
			rest -= tmp;
			j++;
		} while (rest > 0 && j < TEK1_MAXLEN);
		ucstr->maxlen = j - 1;
		if (rest > 0) {
			k = 0;
			if (i < 2)
				k = ucstr->lentbl[TEK1_MAXLEN - 1];
			if (i == 0 && k > 0)
				k++;
			tmp = 0;
			k--;
			do {
				k++;
				base1 = base0 + (1 << k);
				while (st->count != 0 && st->code < base1) {
					tmp += st->count;
					st++;
				}
			} while (rest > tmp);
			ucstr->lentbl[TEK1_MAXLEN] = k;
			ucstr->base[TEK1_MAXLEN + 1] = base1;
			ucstr->maxlen = TEK1_MAXLEN;
		}
		tmp = calclen_uc(ucstr, stat) + (*func)(ucstr);
		if (min > tmp) {
			min = tmp;
			optsub_saveprm(minprm, ucstr);
			rc = -1;
		}
	}

	/* 再帰を使う完全走査型 */
	if (complev >= 6) {
		optsub_ucstr = ucstr;
		optsub_max = max;
		optsub_min = min;
		optsub_minprm = minprm;
		optsub_func = func;
		if (complev == 6)
			optsub_fullopt1(0, rest0, stat, 0);
		else
			optsub_fullopt2(0, rest0, stat, 0);
		if (min > optsub_min) {
		//	min = optsub_min;
			rc = -1;
		}
	}

	optsub_inituc(ucstr, minprm);
fin:
	return rc;
}

#endif

static UCHAR tek2_invtrt[256];

int setstatistics1(struct STR_STATISTICS *stat, UCHAR *trt, unsigned int len, UCHAR *dat)
/* 当然sizは257以上とみなされる */
/* countでソートもされる */
{
	unsigned int count0[256], i, j, k, l, max, max_j;
	for (j = 0; j < 256; j++)
		count0[j] = 0;
	for (j = 0; j < len; j++)
		count0[dat[j]]++;
	k = 0;
	for (j = 0; j < 256; j++) {
		if (count0[j]) {
			stat[k].count = count0[j];
			stat[k].code  = j;
			k++;
		}
	}
	l = k;
	for (j = 0; j < 256; j++) { /* refresh用 */
		if (count0[j] == 0) {
			stat[l].count &= 0;
			stat[l].code  = j;
			trt[j] = l;
			l++;
		}
	}
	stat[256].count &= 0;
	for (i = k; i < 257; i++)
		stat[i].count &= 0; /* 存在判定に使うのでゼロクリア */
//	stat[k].count = 0; /* ターミネータ */
	/* 頭の悪いソート */
	/* とりあえずノンゼロだけでソートしているので少しは速い */
	for (i = 0; i < k; i++) {
		max = stat[i].count;
		max_j = i;
		for (j = i + 1; j < k; j++) {
			if (max < stat[j].count) {
				max = stat[j].count;
				max_j = j;
			}
		}
		stat[max_j].count = stat[i].count;
		stat[i].count = max;
		j = stat[max_j].code;
		stat[max_j].code = stat[i].code;
		stat[i].code = j;
		trt[j] = i;
	}
	for (i = 0; i < 256; i++) {
		tek2_invtrt[i] = stat[i].code;
		stat[i].code = i;
	}
	return k;
}

int setstatistics2(struct STR_STATISTICS *stat, int *trt, int *invtrt, unsigned int len, int *dat)
/* 当然sizは288以上とみなされる */
/* countでソートもされる */
{
	unsigned int count0[288], i, j, k, l, max, max_j;
	for (j = 0; j < 288; j++)
		count0[j] = 0;
	for (j = 0; j < len; j++)
		count0[dat[j]]++;
	k = 0;
	for (j = 0; j < 288; j++) {
		if (count0[j]) {
			stat[k].count = count0[j];
			stat[k].code  = j;
			k++;
		}
	}
	l = k;
	for (j = 0; j < 288; j++) { /* refresh用 */
		if (count0[j] == 0) {
			stat[l].count &= 0;
			stat[l].code  = j;
			trt[j] = l;
			l++;
		}
	}
	stat[288].count &= 0;
	for (i = k; i < 288 + 1; i++)
		stat[i].count &= 0; /* 存在判定に使うのでゼロクリア */
//	stat[k].count = 0; /* ターミネータ */
	for (i = 0; i < 288; i++) {
		trt[i] |= -1; /* まず、全て未使用に */
		invtrt[i] |= -1;
	}
	/* 頭の悪いソート */
	/* とりあえずノンゼロだけでソートしているので少しは速い */
	for (i = 0; i < k; i++) {
		max = stat[i].count;
		max_j = i;
		for (j = i + 1; j < k; j++) {
			if (max < stat[j].count) {
				max = stat[j].count;
				max_j = j;
			}
		}
		stat[max_j].count = stat[i].count;
		stat[i].count = max;
		j = stat[max_j].code;
		stat[max_j].code = stat[i].code;
		stat[i].code = j;
		trt[j] = i;	/* 外部コードを内部コードに変換するテーブル */
	}
	for (i = 0; i < 288; i++) {
		invtrt[i] = stat[i].code; /* 内部コードを外部コードに変換するテーブル */
		stat[i].code = i;
	}
	return k;
}

int setstatistics3(struct STR_STATISTICS *stat, unsigned int len, UCHAR *dat)
/* 当然sizは256以上とみなされる */
/* countでソートもされる */
{
	unsigned int count0[256], j, k;
	for (j = 0; j < 256; j++)
		count0[j] = 0;
	for (j = 0; j < len; j++)
		count0[dat[j]]++;
	k = 0;
	for (j = 0; j < 256; j++) {
		if (count0[j]) {
			stat[k].count = count0[j];
			stat[k].code  = j;
			k++;
		}
	}
	stat[k].count &= 0;
	return k;
}

static struct STR_STATISTICS *tek2_stat_calclen_ucstr_by;
extern UCHAR tek2_table_tr0[256], tek2_table_tr1[256];

int tek2_output_ucstr_by_sub(struct STR_UC *ucstr, UCHAR *lvt, int *pfix)
{
	int i, j, k, max = 0;
	for (i = 0; i < 256; i++)
		lvt[i] = 0x7f; /* 未使用マーク */
refresh:
	for (i = 0; tek2_stat_calclen_ucstr_by[i].count > 0; i++) {
		j = tek2_invtrt[i];
		for (k = 0; k < 32; k++) {
			if (ucstr->base[k] <= i && i < ucstr->base[k + 1])
				break;
		}
		lvt[j] = k;
		if (max < k)
			max = k;
	}
	*pfix = 0;
	if (ucstr->base[max + 1] >= 256) {
		for (i = 0; i < 256; i++) {
			if (lvt[i] == 0x7f) {
				lvt[i] = max;
				*pfix = 2;
			}
		}
		if (*pfix == 2) {
			for (i = 0; tek2_stat_calclen_ucstr_by[i].count > 0; i++);
			do {
				tek2_stat_calclen_ucstr_by[i].count = 1;
				i++;
			} while (i < 256);
			tek2_stat_calclen_ucstr_by[256].count &= 0;
			goto refresh;
		}
	} else {
		for (i = 0; i < 256; i++) {
			if (lvt[i] == 0x7f) {
				*pfix = 1;
				max++;
				for (; i < 256; i++) {
					if (lvt[i] == 0x7f)
						lvt[i] = max;
				}
			}
		}
	}
	return max;
}

int tek2_calclen_ucstr_by(struct STR_UC *ucstr)
{
	int max, fix;
	UCHAR lvt[256];
//	struct STR_STATISTICS *stat;
//	UCHAR table[256], outtmp[512], work[(512 + 272) * 34];
	max = tek2_output_ucstr_by_sub(ucstr, lvt, &fix);

/* 超頭悪い方法で出力させよう、とりあえず */
	return 1 + 4 + 4 + 8 + (tek1_intlog2p(max + 1) << 8); /* fix以降 */
//	return 3 + 5 + 3 + 2 + 2 + 8 + (intlog2p(max + 1) << 8);
		/* 利用レベルが14までに抑えられたら、これは131バイトになる */
		/* 利用レベルが6までに抑えられたら、これは99バイトになる */
/*
255を出力せよ

0:0(1bit)
1:2(2bit)
2:6(3bit)
3:14(4bit)
4:30(5bit)
5:62(6bit)
6:126(7bit)
7:254(1bit)
*/
	/* 一致などを抽出 */
//	return lzcompress_tek1s(256, table, 512, outtmp, sizeof work, work, 0x02 | intlog2m(max + 1) << 8, 12, 256);
	/* このときはbit数で返される、実際の出力はされない */
	/* このことで分かるように、符号類ではbytableが最初に出力される。 */
}

void tek2_output_ucstr_by(struct STR_BITBUF *bitbuf, struct STR_UC *ucstr)
{
	int i, max, fix;
	UCHAR lvt[256];
	putbitbuf(bitbuf, 3, max = tek1_intlog2p(tek2_output_ucstr_by_sub(ucstr, lvt, &fix) + 1));
	#if (defined(DEBUGMSG))
		printf("bitlen = log2p(max + 1) = %d¥n", max);
		for (i = 0; i < 256; i++) printf("%x", lvt[i]);
		fputc('¥n', stdout);
	#endif
	putbitbuf(bitbuf, 1, fix);
	putbitbuf(bitbuf, 4 + 4, 0); /* 000_0, 000_0 */
//	putbitbuf(bitbuf, 4 + 3 + 2 + 2, 0);
	putbitbuf(bitbuf, 8 - 1, 0); /* (1)0000000 */
	putbitbuf(bitbuf, 1, 1); /* 255 */
	for (i = 0; i < 256; i++)
		putbitbuf(bitbuf, max, lvt[i]);
	return;
}

void tek2_initucstr(UCHAR *table_lv, struct STR_UC *ucstr, int fix)
{
	int i, j, k, pop[TEK1_MAXLEN + 1];
	for (i = 0; i <= TEK1_MAXLEN; i++)
		pop[i] = 0;
	for (i = 0; i < 256; i++)
		pop[table_lv[i]]++;
	ucstr->base[0] = 0;
	for (i = 0; pop[i] > 0; i++) {
		ucstr->lentbl[i] = tek1_intlog2p(pop[i]);
		ucstr->base[i + 1] = ucstr->base[i] + (1 << ucstr->lentbl[i]);
	}
	ucstr->maxlen = i - 1 - fix; /* 制御ビット（旧len）の最高長 */

	j = 0;
	for (i = 0; pop[i] > 0; i++) {
		k = j;
		j += pop[i];
		pop[i] = k;
	}
	for (i = 0; i < 256; i++)
		tek2_table_tr0[pop[table_lv[i]]++] = i;
	return;
}

void tek2_fix_trt(struct STR_UC *ucstr, UCHAR *trt)
/* tek2_table_t[]を破壊 */
{
	int i, j, fix;
	UCHAR lvt[256], tmp[256];
	tek2_output_ucstr_by_sub(ucstr, lvt, &fix);
	tek2_initucstr(lvt, ucstr, fix);
//	for (i = 0; i < 256 && tek2_stat_calclen_ucstr_by[i].count > 0; i++) {
	for (i = 0; tek2_stat_calclen_ucstr_by[i].count > 0; i++) {
		j = tek2_table_tr0[i]; /* 内部コードiは、文字jに変換される */
		tmp[j] = i;
	}
	for (i = 0; i < 256; i++)
		trt[i] = tmp[i];
	return;
}

int output_lifetime(struct STR_BITBUF *btout, int lt0, int lt1, int len, int def0)
{
	if (lt1 < 0) {
		/* 最初 */
		if (lt0 < len && len <= (lt0 * 3) / 2)
			lt0 *= 2;
		if (lt0 >= len) {
			if (len < def0)
				lt0 = def0; /* 小さすぎをx1に引き上げる */
			if (def0 < len && len <= def0 * 2)
				lt0 = def0 * 2; /* x2で済むならx2にする */
		}
		if (lt0 == def0)
			putbitbuf(btout, 1, 0); /* lifetime x1 0 */
		else {
			if (lt0 >= len && lt0 != def0 * 2)
				putbitbuf(btout, 4, 0x0f); /* inf 111_1 */
			else if (lt0 == def0 / 32)
				putbitbuf(btout, 7, 0x7d /* 1_1_11_10_1 */);
			else if (lt0 == def0 / 16)
				putbitbuf(btout, 7, 0x75 /* 1_1_10_10_1 */);
			else if (lt0 == def0 / 8)
				putbitbuf(btout, 7, 0x6d /* 1_1_01_10_1 */);
			else if (lt0 == def0 / 4)
				putbitbuf(btout, 7, 0x65 /* 1_1_00_10_1 */);
			else if (lt0 == def0 / 2)
				putbitbuf(btout, 3, 0x03 /* 01_1 */);
			else if (lt0 == def0 * 2)
				putbitbuf(btout, 3, 0x01 /* 00_1 */);
			else if (lt0 == def0 * 4)
				putbitbuf(btout, 7, 0x45 /* 1_0_00_10_1 */);
			else if (lt0 == def0 * 8)
				putbitbuf(btout, 7, 0x4d /* 1_0_01_10_1 */);
			else if (lt0 == def0 * 16)
				putbitbuf(btout, 7, 0x55 /* 1_0_10_10_1 */);
			else if (lt0 == def0 * 32)
				putbitbuf(btout, 7, 0x5d /* 1_0_11_10_1 */);
			else
				exit(1); /* エラー */
		}
	} else {
		/* 2度目以降 */
		if (lt0 < len && len <= (lt0 * 3) / 2) {
			/* 2倍モード */
			lt0 *= 2;
			putbitbuf(btout, 3, 1); /* lifetime x2 00_1 */
		} else
			putbitbuf(btout, 1, 0); /* lifetime x1 0 */
	}
	return lt0;
}

int calclen_lifetime(int *plt0, int lt1, int len, int def0)
{
	UCHAR l;
	int lt0 = *plt0;
	if (lt1 < 0) {
		/* 最初 */
		if (lt0 < len && len <= (lt0 * 3) / 2)
			lt0 *= 2;
		if (lt0 >= len) {
			if (len < def0)
				lt0 = def0; /* 小さすぎをx1に引き上げる */
			if (def0 < len && len <= def0 * 2)
				lt0 = def0 * 2; /* x2で済むならx2にする */
		}
		if (lt0 == def0)
			l = 1;
		else {
			if (lt0 >= len && lt0 != def0 * 2)
				l = 4; /* inf 111_1 */
			else if (lt0 == def0 / 32)
				l = 7; /* 1_1_11_10_1 */
			else if (lt0 == def0 / 16)
				l = 7; /* 1_1_10_10_1 */
			else if (lt0 == def0 / 8)
				l = 7; /* 1_1_01_10_1 */
			else if (lt0 == def0 / 4)
				l = 7; /* 1_1_00_10_1 */
			else if (lt0 == def0 / 2)
				l = 3; /* 01_1 */
			else if (lt0 == def0 * 2)
				l = 3; /* 00_1 */
			else if (lt0 == def0 * 4)
				l = 7; /* 1_0_00_10_1 */
			else if (lt0 == def0 * 8)
				l = 7; /* 1_0_01_10_1 */
			else if (lt0 == def0 * 16)
				l = 7; /* 1_0_10_10_1 */
			else if (lt0 == def0 * 32)
				l = 7; /* 1_0_11_10_1 */
			else
				exit(1); /* エラー */
		}
	} else {
		/* 2度目以降 */
		if (lt0 < len && len <= (lt0 * 3) / 2) {
			/* 2倍モード */
			lt0 *= 2;
			l = 3; /* lifetime x2 00_1 */
		} else
			l = 1; /* lifetime x1 0 */
	}
	*plt0 = lt0;
	return (int) l;
}

//extern struct STR_UC tek2_ucstby[2];

int lzcompress_tek3h(int srcsiz, int *src, int outsiz, UCHAR *outbuf, UCHAR *work, int modecode)
/* workは3KB必要(intが32bitだとして) */
{
	int *s256 = (int *) work, i, j;
	UCHAR *p;
	struct STR_BITBUF btbu