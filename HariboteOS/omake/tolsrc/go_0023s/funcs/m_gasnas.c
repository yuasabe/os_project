/* それぞれのgas2naskにインクルードされる */
/*		Copyright(C) 2004 H.Kawai   (KL-01) */

static UCHAR *checkparam(UCHAR *p);
static void convparam(UCHAR *p, int i);

static UCHAR *skipspace(UCHAR *p)
{
	while (*p <= ' ' && *p != '¥n')
		p++;
	return p;
}

static UCHAR *getparam(UCHAR *p)
{
	if (*p != '¥n') {
		do {
			p++;
		} while (*p != ',' && *p > ' ' && *p != ';');
	}
	return p;
}

static UCHAR *seek_token_end(UCHAR *s)
{
	while (*s != ':' && *s != '+' && *s > ' ' && *s != '(' && *s != '#' && *s != '*' && *s != '$'
			&& *s != ')' && *s != 0x22 && *s != ',' && *s != ';' && *s != '-')
		s++;
	return s;
}

static UCHAR *opcmp(UCHAR *q, UCHAR *s)
{
	int i;
	for (i = 0; i < 8; i++) {
		if (q[i] == ' ' && (s[i] <= ' ' || s[i] == ';'))
			goto match;
		if (q[i] != s[i])
			goto mismatch;
	}
	if (s[i] <= ' ' || s[i] == ';')
		goto match;
mismatch:
	return NULL;
match:
	return skipspace(&s[i]);
}

static char my_strcmp(const char *s, const char *t)
{
	while (*s == *t) {
		s++;
		t++;
		if (*t == '¥0' && (*s <= ' ' || *s == ';'))
			return 1;
	}
	return 0;
}

static UCHAR *convmain(UCHAR *src0, UCHAR *src1, UCHAR *dest0, UCHAR *dest1, struct STR_FLAGS flags)
{
	UCHAR *p, *q, *r, *s, *t;
	UCHAR intext = 0, c, flag, flag1, flag2;
	static UCHAR no_params[][16] = {
		"cld     CLD",
		"cltd    CDQ",
		"cmpsb   CMPSB",
		"cwtd    CWD",
		"cwtl    CWDE",
		"cbtw    CBW",
		"fabs    FABS",
		"fchs    FCHS",
		"fcos    FCOS",
		"fld1    FLD1",
		"fldz    FLDZ",
		"fsin    FSIN",
		"fsqrt   FSQRT",
		"fucom   FUCOM",
		"fucomp  FUCOMP",
		"fucompp FUCOMPP",
		"leave   LEAVE",
		"movsb   MOVSB",
		"movsl   MOVSD",
		"rep     REP",
		"repe    REPE",
		"repne   REPNE",
		"ret     RET",
		"sahf    SAHF",
		"scasb   SCASB",
		"stosb   STOSB",
		"stosl   STOSD",
		"¥0"
	};
	static UCHAR one_param[][16] = {
		"0call    CALL",
		"1decb    DEC",
		"4decl    DEC",
		"2decw    DEC",
		"1divb    DIV",
		"4divl    DIV",
		"2divw    DIV",
		"8faddl   FADD",
		"4fadds   FADD",
		"8fdivl   FDIV",
		"4fdivs   FDIV",
		"8fdivrl  FDIVR",
		"4fdivrs  FDIVR",
		"4fidivl  FIDIV",
		"4fidivrl FIDIVR",
		"2filds   FILD",
		"4fildl   FILD",
		"8fildq   FILD",
		"4fimull  FIMUL",
		"2fistps  FISTP",
		"4fistpl  FISTP",
		"8fistpq  FISTP",
		"2fists   FIST",
		"4fistl   FIST",
		"3fld     FLD",
		"2fldcw   FLDCW",
		"8fldl    FLD",
		"4flds    FLD",
		"8fmull   FMUL",
		"4fmuls   FMUL",
		"2fnstcw  FNSTCW",
		"2fnstsw  FNSTSW",
		"8fstl    FST",
		"3fstp    FSTP",
		"8fstpl   FSTP",
		"4fstps   FSTP",
		"4fsts    FST",
		"8fsubl   FSUB",
		"8fsubrl  FSUBR",
		"4fsubrs  FSUBR",
		"3fucom   FUCOM",
		"3fucomp  FUCOMP",
		"3fxch    FXCH",
		"1idivb   IDIV",
		"4idivl   IDIV",
		"2idivw   IDIV",
		"1imulb   IMUL",
		"4imull   IMUL",
		"2imulw   IMUL",
		"1incb    INC",
		"4incl    INC",
		"2incw    INC",
		"0ja      JA",
		"0jae     JAE",
		"0jb      JB",
		"0jbe     JBE",
		"0je      JE",
		"0jg      JG",
		"0jge     JGE",
		"0jl      JL",
		"0jle     JLE",
		"0jmp     JMP",
		"0jne     JNE",
		"0jnp     JNP",
		"0jns     JNS",
		"0jp      JP",
		"0js      JS",
		"0loop    LOOP",
		"1mulb    MUL",
		"4mull    MUL",
		"2mulw    MUL",
		"1negb    NEG",
		"4negl    NEG",
		"2negw    NEG",
		"1notb    NOT",
		"4notl    NOT",
		"2notw    NOT",
		"4popl    POP",
		"4pushl   PUSH",
		"4ret     RET",
		"1seta    SETA",
		"1setae   SETAE",
		"1setb    SETB",
		"1setbe   SETBE",
		"1sete    SETE",
		"1setg    SETG",
		"1setge   SETGE",
		"1setl    SETL",
		"1setle   SETLE",
		"1setne   SETNE",
		"1setnp   SETNP",
		"1setns   SETNS",
		"1sets    SETS",
		"1setp    SETP",
		"¥0"
	};
	static UCHAR one_shifts[][16] = {
		"1salb    SAL",
		"4sall    SAL",
		"2salw    SAL",
		"1sarb    SAR",
		"4sarl    SAR",
		"2sarw    SAR",
		"1shrb    SHR",
		"4shrl    SHR",
		"2shrw    SHR",
		"¥0"
	};
	static UCHAR two_params[][16] = {
		"1addb    ADD",
		"4addl    ADD",
		"2addw    ADD",