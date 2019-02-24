#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned char UCHAR;

int aksa(UCHAR *p, UCHAR *p1, UCHAR *s, int eip, int dd);

int main(int argc, UCHAR **argv)
{
	FILE *fp;
	UCHAR *fbuf0, *sbuf, *fbuf1, *p;
	int i, ofs, eip, dd, lin;
	sbuf = malloc(256);
	fbuf0 = malloc(8 * 1024 * 1024);
	if (argc != 6) {
		printf("usage>aksa binfile ofs0 eip0 dd lines¥n");
		return 1;
	}
	fp = fopen(argv[1], "rb");
	if (fp == NULL) {
		printf("binfile open error!¥n");
		return 2;
	}
	i = fread(fbuf0, 1, 8 * 1024 * 1024, fp);
	if (i >= 8 * 1024 * 1024) {
		printf("binfile too large error!¥n");
		return 3;
	}
	fbuf1 = fbuf0 + i; 
	p = argv[3];
	if (p[0] == '.' && (p[1] == '+' || p[1] == '-'))
		p += 2;
	ofs = strtol(argv[2], NULL, 0);
	eip = strtol(p, NULL, 0);
	dd  = strtol(argv[4], NULL, 0);
	lin = strtol(argv[5], NULL, 0);
	if (argv[3][0] == '.') {
		if (argv[3][1] == '+') {
			eip += ofs;
		}
		if (argv[3][1] == '-') {
			eip = ofs - eip;
		}
	}
	p = fbuf0 + ofs;
	if (ofs < 0 || p >= fbuf1) {
		printf("ofs out of range error!¥n");
		return 4;
	}
	printf("file-ofs   EIP¥n");
	do {
		int j;
		i = aksa(p, fbuf1, sbuf, eip, dd);
		if (i <= 0)
			break;
		printf("%08X %08X ", ofs, eip);
		for (j = 0; j < 6; j++) {
			if (j < i)
				printf("%02X ", p[j]);
			else
				printf("   ");
		}
		printf("%s;¥n", sbuf);
		ofs += i;
		eip += i;
		p += i;
	} while (--lin > 0);
	return 0;
}

int aksa_ea(UCHAR *p, UCHAR *s, UCHAR len, UCHAR adrsiz, UCHAR segovr)
{
	static UCHAR *basestr[] = {
		"EAX", "ECX", "EDX", "EBX", "ESP", "EBP", "ESI", "EDI",
		"BX + SI", "BX + DI",
		"BP + SI", "BP + DI",
		"SI", "DI", "BP", "BX"
	};
	int i;
	UCHAR c = *p & 0xc7, displen, idx, scl = 0, base;
	UCHAR *dp = &p[1];
	if ((c & 0xc0) == 0xc0) {
		/* reg8/16/32 */
		if (len == 1) {
			s[0] = "ACDB"[*p & 0x03];
			s[1] = (c & 0x04) ? 'H' : 'L';
			s[2] = '¥0';
			return 1;
		}
		if (len == 4)
			*s++ = 'E';
		s[0] = "ACDBSBSD"[c & 0x07];
		s[1] = "XXXXPPII"[c & 0x07];
		s[2] = '¥0';
		return 1;
	}
	*s++ = '(';
	if (len == 1) {
		strcpy(s, "char");
		s += 4;
	}
	if (len == 2) {
		strcpy(s, "short");
		s += 5;
	}
	if (len == 4) {
		strcpy(s, "int");
		s += 3;
	}
	s[0] = ')';
	s[1] = ' ';
	s[2] = '[';
	s += 3;
	displen = c >> 6;
	base = c & 0x07;
	if (adrsiz) {
		if (displen == 2)
			displen = 4;
		if (base == 4) {
			dp++;
			base = p[1] & 0x07;
			idx = (p[1] >> 3) & 0x07;
			scl = '0' | 1 << ((p[1] >> 6) & 0x03);
			if (idx == 4)
				scl = 0;
		}
		if (displen == 0 && base == 5) {
			base = 16;
			displen = 4;
		}

	} else {
		base |= 8;
		if (c == 0x06) {
			base = 16;
			displen = 2;
		}
	}
	if ((s[0] = segovr) == 0)
		s[0] = "DDDDSSDDDDSSDDSDD"[base];
	s[1] = 'S';
	s[2] = ':';
	s += 3;
	c = 0;
	if (base != 16) {
		strcpy(s, basestr[base]);
		do {
			s++;
		} while (*s);
		c = 1;
	}
	if (scl) {
		if (c) {
			s[0] = ' ';
			s[1] = '+';
			s[2] = ' ';
			s += 3;
		}
		s[0] = 'E';
		s[1] = basestr[idx][1];
		s[2] = basestr[idx][2];
		s[3] = ' ';
		s[4] = '*';
		s[5] = ' ';
		s[6] = scl;
		s += 7;
		c = 1;
	}
	if (displen) {
		i = dp[0] | dp[1] << 8 | dp[2] << 16 | dp[3] << 24;
		if (displen == 1)
			i = (signed char) i;
		if (displen == 2)
			i = (signed short) i;
		if (c) {
			s[0] = ' ';
			s[1] = '+';
			s[2] = ' ';
			if (i < 0) {
				s[1] = '-';
				i = - i;
			}
			s += 3;
		}
		sprintf(s, (displen == 1) ? "0x%02X" : "0x%08X", i);
		if (displen == 2)
			sprintf(s, "0x%04X", i & 0xffff); /* &するのは、c == 0のときのため */
		do {
			s++;
		} while (*s);
	}
	s[0] = ']';
	s[1] = '¥0';
	return dp - p + displen;
}

int aksa(UCHAR *p, UCHAR *p1, UCHAR *s, int eip, int dd)
{
	UCHAR bytes[16], tmp[32], dd_flag = dd & 0x01, dlen, *q, adrsiz = dd_flag;
	UCHAR segovr = 0, c, pre = 0, clen, tmp2[32];
	static UCHAR *reg16[] = { "AX", "CX", "DX", "BX", "SP", "BP", "SI", "DI" };
	static UCHAR *reg8[] = { "AL", "CL", "DL", "BL", "AH", "CH", "DH", "BH" };
	static