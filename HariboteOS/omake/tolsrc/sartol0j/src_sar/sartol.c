/* "sartol.c" */
/* copyright (C) 2004 H.Kawai & I.Tak. (KL-01) */

#define SAR_MODE_WIN32		1
/* Linuxなどでは↑をコメントアウトする */

//#define SAR_MODE_POSIX	1
/* Windowsなどでは↑をコメントアウトする */

#define SAR_MODE_SJIS		1

#define ERROUT		stdout

typedef unsigned char UCHAR;

/* フォーマットがsartol0e→0fであちこち変わった */

/* アラインが1なら前に14バイトを追加するだけで、KHBIOS対応にできる */
/* 16バイト追加でいいなら、1/2/4/8/16のアラインも問題ない */
/* だからこれらのアラインでは、-を指定する必然性は低いだろう */

struct sar_dirattr0 {
	int flags, unitnamelen, align;
	int baseattr_f, baseperm_f, baseattr_d, baseperm_d;
	int time_unit, subsec_mode;
	int time0[6];
	UCHAR *alignbase;
};

struct sar_attrtime {
	int attr, permission;
	int subsec;
	UCHAR sec, min, hour, day, mon, _dummy[3];
	unsigned int year, year_h;
};

struct sar_fileinfo0 {
	UCHAR *p, *p0, *s;
	int namelen;
	UCHAR *name;
	unsigned int size, size_h;
	UCHAR *content, *content1;
	struct sar_attrtime at;
#if 0
	int attr;
		/* attrの意味をsartol0e→0fで大幅に変更 */
		/* bit0-3:0000=normal, 0001=extend, 0010=vfile, 0011=vdir */
		/*		01xx=dir(bit0:0=inline, bit1:1=extend) */
			/* vfile, vdirはxfile, xdirを指す。xfile, xdirはデフォルトがhiddenで、リンクカウントがある */
			/* 原則としてxfile, xdirはルートディレクトリの末尾に置く */
			/* xfileやxdirはルートディレクトリ内の特別なディレクトリにおかれる */
			/* このディレクトリではファイルネームはなく全てID（namelen部分で記述）で管理される */
			/* extendディレクトリは長いヘッダがあってディレクトリフォーマットパラメータを変更できるディレクトリ */
		/* bit4:1=deleted */
		/* bit5:1=timeフィールドなし */
		/* bit6:1=パーミッションフィールドあり */
		/* bit7:1=read-only */
		/* bit8:1=hidden */
		/* bit9:1=system */
		/* bit10:archive-flag(DOS-compatible) */
	int permission;
		/* bit0-2:others */
		/* bit3-5:group */
		/* bit6-8:owner */
	int subsec;
	UCHAR sec, min, hour, day, mon, _dummy;
	unsigned int year, year_h;
#endif

	int ext_len;
	int *ext_p;
	struct sar_dirattr0 da0;
};

struct sar_archandle0 {
	int version;
	int lang;
	struct sar_dirattr0 da0;
	int ext_len, ext_id, ext_header_len;
	UCHAR *ext_header;
	UCHAR *p0, *p1;
};

struct sar_archandle1 {
	int version;
	int lang;
	struct sar_dirattr0 da0;
	int ext_len, ext_id, ext_header_len;
	UCHAR *ext_header;
	UCHAR *p, *p1, *s, *s0;
	UCHAR lastdir[4096];
	struct sar_archw_subdir {
		UCHAR *p0; /* とりあえずみんな4バイト確保し、あとでつめる */
		UCHAR *pl; /* lastdir内のポインタ */
		UCHAR *s0;
		UCHAR *ps; /* ディレクトリのsizeフィールドポインタ */
	} dir[16];
	int dirlev, reservelen;
};

#if 0

フォーマット情報

シグネチャ："sar¥0KHB0"  73 61 72 00 4b 48 42 30
	シグネチャがファイル先頭に見つからない場合、および後続のフォーマットバージョンがおかしい場合、
	ファイル先頭の前に理解できない14バイトがくっついているかもしれないと判定して
	オフセット0x0eからのシグネチャ判定をすること。
	このルールにより、KHBIOS用ディスクイメージに対応する。
	先頭512、先頭1k、先頭2k、先頭4k、・・・以下64kまで、でもよい。16から調べるか。

s7sでフォーマットバージョン(0)
s7sでフラグフィールド
	ルートのファイル相対開始位置省略（bit0を0に）。
	ファイル名余白は、後続のファイル属性s7sに0x00を連ねることでカバーできる。
	それが不満の場合は、bit1を1にして、ファイル名フィールド内に実ファイル名長を入れる（extend）。
	bit2が1だとファイル属性フィールドではs7sの変わりにt8sを使用。t8sは決め打ちするには扱いやすい。
	bit3が1だとベースアトリビュートあり
	bit4が1だとタイムゾーンフィールドあり
	bit5が1だとベース日時なし
	bit6が1だとベースアクセス権なし
	bit7が1だと拡張属性あり。
	bit8が1だと拡張ヘッダあり。
	bit9が1だと負の時刻を許す。
ランゲージコード(0:不明)。1だとIBM。
ベースアトリビュート、7bit+7bit。
日時単位・ベース単位
	日時単位が負の場合、秒よりも細かい単位を指定することになるが、その場合はLSBがさらに属性になり、
	LSBが0だと10進系、LSBが1だと2進系になる。
ベース日時・ベースアクセス権（これがXORされる）
	アクセス権フィールドはビットが逆順になっている（まず自分、グループ、その他、拡張）。
	しかも自分も、xwrの順（rが下位、xが上位）
アラインコード（これがなくても毎回位置を書けばアラインはできるよ）。
拡張属性長（0だと可変長）
拡張属性コード（0は未申請）
拡張ヘッダ長

s7sでヘッダ長

ファイルネーム長（s7s - アラインのため）
ファイルネーム
ファイル属性
ファイル日時（符号付s7s）
（パーミッション）
ファイル相対開始位置（符号付s7s）
ファイルサイズ（s7s）/スキップ長（バイト数：配下の個数は数えないことにした）/絶対開始位置

サブディレクトリでは、最初のファイルはヘッダ終端からの相対で開始位置を記録。

日時は、
秒に6bit、分に6bit、時に5bit、日に5bit、月に4bit、年にはいくらでも。
秒から月までで26ビット。
時刻は全て世界標準時で格納。時を31にすると、無効時刻。

sarは基本的にリードオンリーしか考えてない。リードオンリーでいかに短く書くかである。

#endif

#if 0

時刻も全部無効時刻。
属性も700
ベースパスと、ファイル名列挙で生成。eとd。
eの場合、argv[1]=="e"
	argv[2]==ベースパス("."など)
	argv[3]==格納ファイル名

ファイルネームは127まで（ファイルネーム長0で終端）
属性はやる気のない0。
ファイル日時はやる気なしの0。

リンクカウントはないが、並列ディレクトリは可能。
	→ 基本的にdeletedで消していって、たまに整理でディレクトリ構造全体を読み取ってデッドリンクを処分すればいいだろう。
		削除日を管理したい場合は、deletedディレクトリを作って、そこに好きな情報をためればいいかもしれない。
		こういう特別なディレクトリは、名前の最初を0x00にするとかで表現してもいいだろう
			（一般ファイルが0x00で始まるなら、0x00-0x00で表現させるとか）。

#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

UCHAR **ini2arg(int *pargc, UCHAR *ini);
int decode(int argc, UCHAR **argv);
int autorun(UCHAR *tmp, UCHAR *cmd, UCHAR *base);
void fixpath(UCHAR *src, UCHAR *dst);
void fixbasepath(UCHAR *base, UCHAR *arc);
int decode_main(int asiz, int bsiz, UCHAR *arc, UCHAR *tmp, UCHAR *basepath, int argc, UCHAR **argv);
int decode_sub(struct sar_fileinfo0 *dir, UCHAR *name, void *prm);
int access_all_r(struct sar_fileinfo0 *dir, UCHAR *name, UCHAR *name1, int argc, UCHAR **argv, int flags,
	int (*func)(struct sar_fileinfo0 *, UCHAR *, void *), void *prm);
int match(UCHAR *name, int argc, UCHAR **argv);
int list0(int argc, UCHAR **argv);
int list_main(int asiz, int bsiz, UCHAR *fbuf, UCHAR *tmp, int argc, UCHAR **argv, int bias, int flags);
int list_sub(struct sar_fileinfo0 *dir, UCHAR *name, void *prm);
int encode(int argc, UCHAR **argv);
int restore(UCHAR **argv);
int fmkdir(UCHAR *path, UCHAR *tmp);
FILE *ffopen(UCHAR *path, UCHAR *tmp);
void getattrtime(struct sar_attrtime *s, UCHAR *path);
void setattrtime(struct sar_attrtime *s, UCHAR *path);

#define SIZ_FILEBUF		32 * 1024 * 1024

void sar_openarchive_r(struct sar_archandle0 *handle, int siz, UCHAR *buf);
unsigned int sar_getnum_s7s(UCHAR **pp);
void sar_opendirectory_r(struct sar_fileinfo0 *dir, struct sar_dirattr0 *da0, UCHAR *p);
void sar_getfile_r(struct sar_fileinfo0 *dir);

UCHAR *sar_puts7s(UCHAR *p, unsigned int i);
void sar_openarchive_w(struct sar_archandle1 *handle, int siz, UCHAR *buf, int flags);
UCHAR *sar_putfile_w(struct sar_fileinfo0 *file, struct sar_archandle1 *arc, int flags);
UCHAR *sar_closedir_w(struct sar_archandle1 *arc, UCHAR *p);
int sar_closearchive_w(struct sar_archandle1 *arc);
int sar_permconv_u2s(int unx);
#define sar_permconv_s2u(x)	sar_permconv_u2s(x)
void sar_time2uc(struct sar_attrtime *at, UCHAR *uc24);
void sar_uc2time(UCHAR *uc24, struct sar_attrtime *at);
UCHAR *sar_puttime(UCHAR *p, struct sar_attrtime *at, struct sar_dirattr0 *da0);
UCHAR *sar_gettime(UCHAR *p, struct sar_attrtime *at, struct sar_dirattr0 *da0);
void sar_shifttime(struct sar_attrtime *at, int min, void *opt);

static struct sar_attrtime sar_atinv = {
	0, 0, -1,
	0, 0, 0, 0, 0x1f, { 0, 0, 0 },
	0, 0
};

extern int autodecomp(int siz0, UCHAR *p0, int siz);

int main(int argc, UCHAR **argv)
{
	int i = 1;
	UCHAR *p;

	if (argc >= 2 && argv[i][0] == '@') {
		argv = ini2arg(&argc, p = &argv[i][1]);
		if (argv == NULL) {
			fprintf(ERROUT, "can't open file : %s¥n", p);
			return 1;
		}
		i = 0;
	}

	if (argc - i >= 3 && argv[i][0] == 'd')
		return decode(argc - 1 - i, argv + 1 + i);
	if (argc - i >= 5 && argv[i][0] == 'e')
		return encode(argc - 1 - i, argv + 1 + i);
	if (argc - i >= 2 && argv[i][0] == 'l')
		return list0(argc - 1 - i, argv + 1 + i);
	if (argc - i == 3 && argv[i][0] == 'r')
		return restore(argv + 1 + i);

	fprintf(stdout,
		"usage>sartol e arcfile basepath align file1 file2 ...¥n"
		"usage>sartol d arcfile basepath[/] [autorun]¥n"
		"usage>sartol l [:bias] arcfile¥n"
		"usage>sartol r tek-file outfile¥n"
		"usage>sartol @params¥n");
	return 1;
}

UCHAR **ini2arg(int *pargc, UCHAR *ini)
{
	FILE *fp;
	int argc = 0, i;
	UCHAR *p = malloc(1024 * 1024), *q, *q0, **argv, mode = 0;

	fp = fopen(ini, "rb");
	if (fp == NULL)
		return NULL;
	i = fread(p, 1, 1024 * 1024 - 1, fp);
	fclose(fp);
	p[i] = '¥0';

	q = q0 = malloc(strlen((char *) p) + 1);
	do {
		do {
			if (*p == '¥0')
				break; /* コマンドラインバグ回避のため */
			if (*p == 0x22)
				mode ^= 1;
			else
				*q++ = *p;
			p++;
		} while (*p > ' ' || mode != 0);
		argc++;
		*q++ = '¥0';
		while ('¥0' < *p && *p <= ' ')
			p++;
	} while (*p);
	argv = malloc((argc + 1) * sizeof (char *));
	argv[0] = q = q0;
	i = 1;
	while (i < argc) {
		while (*q++);
		argv[i++] = q;
	}
	argv[i] = NULL;
	*pargc = argc;
	return argv;
}

int decode(int argc, UCHAR **argv)
{
	struct str_dec0_work {
		UCHAR fbuf[SIZ_FILEBUF], tmp[4096 * 4], basepath[4096], arcname[4096];
	} *work = malloc(sizeof (struct str_dec0_work));

	FILE *fp;
	int i;

	/* '¥'を'/'に直して以降の作業をやりやすくする */
	fixpath(argv[0], work->arcname);
	fixpath(argv[1], work->basepath);

	/* basepathの加工（/で終わっているときの処理など） */
	fixbasepath(work->basepath, work->arcname);

	/* アーカイブ読み込み */
	fp = fopen(argv[0], "rb");
	if (fp == NULL) {
		fprintf(ERROUT, "can't open file : %s¥n", argv[0]);
err:
		free(work);
		return 1;
	}
	i = fread(work->fbuf, 1, SIZ_FILEBUF, fp);
	fclose(fp);

	if (decode_main(i, SIZ_FILEBUF, work->fbuf, work->tmp, work->basepath, argc - 3, argv + 3))
		goto err;

	/* autorun */
	i &= 0;
	if (argc >= 3 && !(argv[2][0] == '.' && argv[2][1] == '¥0'))
		i = autorun(work->tmp, argv[2], work->basepath);
	return i;
}

struct str_dec_subwork {
	UCHAR *bp0, *tmp;
};

int decode_main(int asiz, int bsiz, UCHAR *arc, UCHAR *tmp, UCHAR *basepath, int argc, UCHAR **argv)
{
	struct str_dec_work {
		struct sar_fileinfo0 dir;
		struct sar_archandle0 arc;
		struct str_dec_subwork sub;
	} *work = malloc(sizeof (struct str_dec_work));

	UCHAR *bp1;
	int i;

	/* tek圧縮が掛かっていれば、これをとく */
	if (autodecomp(bsiz, arc, asiz) < 0)
		goto tekerr;

	/* 読み込み専用モードでアーカイブを開く */
	sar_openarchive_r(&work->arc, bsiz, arc);
	if (work->arc.p0 == NULL) {
tekerr:
		fprintf(ERROUT, "arcfile error¥n");
		free(work);
		return 1;
	}
	
	/* basepathの末尾を検出 */
	for (bp1 = basepath; *bp1 != 0; bp1++);
	work->sub.bp0 = basepath;
	work->sub.tmp = tmp;

	sar_opendirectory_r(&work->dir, &work->arc.da0, work->arc.p0);
	i = access_all_r(&work->dir, bp1, bp1, argc, argv, 0x02, decode_sub, &work->sub);
	free(work);
	*bp1 = '¥0';
	return i;
}

int decode_sub(struct sar_fileinfo0 *dir, UCHAR *name, void *prm)
{
	struct str_dec_subwork *work = prm;
	int i = dir->at.attr & 0x0f;
	FILE *fp;

	if (0x03 <= i && i <= 0x07) {
		/* ディレクトリ系 */
		i = fmkdir(work->bp0, work->tmp);
		if (i == 0)
			setattrtime(&dir->at, work->bp0);
		return i;
	}
	if (i) { /* 属性確認：まだ通常ファイル以外は扱えない */
		fprintf(ERROUT, "unsupported file type : %s¥n", name);
		return 1;
	}

	/* 一般ファイル */
	fp = ffopen(work->bp0, work->tmp);
	if (fp == NULL) {
		fprintf(ERROUT, "can't open file : %s¥n", work->bp0);
		return 1;
	}
	if (dir->size) {
		if (fwrite(dir->content, 1, dir->size, fp) != dir->size) {
			fclose(fp);
			fprintf(ERROUT, "output error : %s¥n", work->bp0);
			return 1;
		}
	}
	fclose(fp);
	setattrtime(&dir->at, work->bp0);
	return 0;
}

int access_all_r(struct sar_fileinfo0 *dir, UCHAR *name, UCHAR *name1, int argc, UCHAR **argv, int flags,
	int (*func)(struct sar_fileinfo0 *, UCHAR *, void *), void *prm)
/* 条件を満たすディレクトリ内の全てのファイル・ディレクトリに対してfuncを適用 */
{
	UCHAR *p, *q;
	int i, r = 0;
	do {
		sar_getfile_r(dir);
		if ((i = dir->namelen) == 0)
			break;

		if ((flags & 1) == 0 && (dir->at.attr & 0x10) != 0)
			continue; /* deletedはスキップ */

		/* ファイル名の連結 */
		p = dir->name;
		q = name1;
		do {
			*q++ = *p++;
		} while (--i);
		*q = 0;

		i = dir->at.attr & 0x0f;
		if (i == 0x00) {
			/* 通常ファイル */
			/* ファイル名を指定している場合、一致するものがあるかどうかをチェックする */
			if (match(name, argc, argv) >= 0)
				r = (*func)(dir, name, prm);
		} else if (i == 0x04) {
			/* インラインディレクトリ */
			q[0] = '/';
			q[1] = '¥0';
			/* ファイル名を指定している場合、一致するものがあるかどうかをチェックする */
			if (match(name, argc, argv) >= 0)
				r = (*func)(dir, name, prm);
			if (flags & 2) {
				struct sar_fileinfo0 sub;
				sub.da0 = dir->da0;
				sub.p = dir->content1;
				sub.s = dir->content;
				sar_getnum_s7s(&sub.p);
				r = access_all_r(&sub, name, q + 1, argc, argv, flags, func, prm);
			}
		}
	} while (r == 0);
	return r;
}

int autorun(UCHAR *tmp, UCHAR *cmd, UCHAR *base)
{
	UCHAR flag = 0, *p, *q, *r;

	*tmp++ = 0x22;
	p = cmd;
	for (q = tmp; *p != '¥0'; p++, q++) {
		*q = *p;
		if (*p <= ' ')
			flag = 1;
	}
	*q++ = ' ';
	if (flag) {
		q[-1] = 0x22;
		tmp--;
		*q++ = ' ';
	}
	r = q;
	*q++ = ' ';
	flag = 0;
	p = base;
	for (; *p != '¥0'; p++, q++) {
		*q = *p;
		#if (defined(SAR_MODE_WIN32))
			if (*q == '/')
				*q = '¥¥';
		#endif
		if (*q <= ' ')
			flag = 1;
	}
	*q++ = '¥0';
	if (flag) {
		*r = 0x22;
		q[-1] = 0x22;
		*q++ = '¥0';
	}

	return system(tmp);

	/* #_をスペース置換にすれば互換性は保てる */
	/* #Wも必要だ */
	/* #を検出したら自動#wはオフになる、と */
	/* というかそれなら#_はなくてもいいな・・・いや必要。
		""は使いたくないが、スペースを入れたい場合はありうる。 */
}

void fixpath(UCHAR *src, UCHAR *dst)
{
	UCHAR *p, *q;
	p = src;
	for (q = dst; *p != '¥0'; *q++ = *p++);
	*q = '¥0';

	#if (defined(SAR_MODE_WIN32))
		#if (!defined(SAR_MODE_SJIS))
			for (q = dst; *q != '¥0'; q++) {
				if (*q == '¥¥')
					*q = '/';
			}
		#else
			for (q = dst; *q != '¥0'; q++) {
				if (0x81 <= *q && *q <= 0x9f) {
					q++;
					continue;
				}
				if (0xe0 <= *q && *q <= 0xfc) {
					q++;
					continue;
				}
				if (*q == '¥¥')
					*q = '/';
			}
		#endif
	#endif
	return;
}

void fixbasepath(UCHAR *base, UCHAR *arc)
{
	UCHAR *p, *q, *r, flags = 0;
	int j;
	/* basepathの後ろの'/'の数を数える */
	for (q = base; *q != '¥0'; q++);
	while (base <= &q[-1] && q[-1] == '/') {
		*--q = '¥0';
		flags++;
	}

	/* basepathの末尾が"."であればこれを外す */
	/* "a:/." -> "a:/", "a:." -> "a:" */
	/* そうでなければ"/"をつける */
	if (base <= &q[-1] && q[-1] == '.' &&
		(base == &q[-1] || (base <= &q[-2] && (q[-2] == '/' || q[-2] == ':'))))
		q--;
	else
		*q++ = '/';
	*q = '¥0';

	/* "..@arcpath"のチェック */
	if (q - base == 11) {
		UCHAR c = 0;
		for (j = 0; j < 11; j++)
			c |= base[j] ^ "..@arcpath/"[j];
		if (c == 0) {
			p = arc;
			for (r = q = base; *p != '¥0'; p++, q++) {
				*q = *p;
				if (*q == '/')
					r = q + 1;
			}
			*r = '¥0';
			q = r;
		}
	}

	/* basepathにアーカイブ名を付け足す */
	if (flags >= 1) {
		for (r = p = arc; *p != '¥0'; p++) {
			if (*p == '/')
				r = p + 1;
		}
		for (p = NULL; *r != '¥0'; r++, q++) {
			*q = *r;
			if (*q == '.')
				p = q;
		}
		if (flags == 1) {
			if (p)
				q = p;
		}
		*q++ = '/';
		*q = '¥0';
	}

	return;
}

int match(UCHAR *name, int argc, UCHAR **argv)
/* 0:許可、-1:不許可, 1以上:代替表現が存在する */
{
	int i, j = 0, flags = 0, len = strlen(name);
	UCHAR *p;
	if (argc <= 0)
		