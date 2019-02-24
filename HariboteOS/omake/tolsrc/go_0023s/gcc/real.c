/* real.c - implementation of REAL_ARITHMETIC, REAL_VALUE_ATOF,
   and support for XFmode IEEE extended real floating point arithmetic.
   Copyright (C) 1993, 1994, 1995, 1996, 1997, 1998,
   1999, 2000, 2002 Free Software Foundation, Inc.
   Contributed by Stephen L. Moshier (moshier@world.std.com).

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 2, or (at your option) any later
version.

GCC is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with GCC; see the file COPYING.  If not, write to the Free
Software Foundation, 59 Temple Place - Suite 330, Boston, MA
02111-1307, USA.  */

#include "config.h"
#include "system.h"
#include "tree.h"
#include "toplev.h"
#include "tm_p.h"

/* To enable support of XFmode extended real floating point, define
LONG_DOUBLE_TYPE_SIZE 96 in the tm.h file (m68k.h or i386.h).

To support cross compilation between IEEE, VAX and IBM floating
point formats, define REAL_ARITHMETIC in the tm.h file.

In either case the machine files (tm.h) must not contain any code
that tries to use host floating point arithmetic to convert
REAL_VALUE_TYPEs from `double' to `float', pass them to fprintf,
etc.  In cross-compile situations a REAL_VALUE_TYPE may not
be intelligible to the host computer's native arithmetic.

The emulator defaults to the host's floating point format so that
its decimal conversion functions can be used if desired (see
real.h).

The first part of this file interfaces gcc to a floating point
arithmetic suite that was not written with gcc in mind.  Avoid
changing the low-level arithmetic routines unless you have suitable
test programs available.  A special version of the PARANOIA floating
point arithmetic tester, modified for this purpose, can be found on
usc.edu: /pub/C-numanal/ieeetest.zoo.  Other tests, and libraries of
XFmode and TFmode transcendental functions, can be obtained by ftp from
netlib.att.com: netlib/cephes.  */

/* Type of computer arithmetic.
   Only one of DEC, IBM, IEEE, C4X, or UNK should get defined.

   `IEEE', when REAL_WORDS_BIG_ENDIAN is non-zero, refers generically
   to big-endian IEEE floating-point data structure.  This definition
   should work in SFmode `float' type and DFmode `double' type on
   virtually all big-endian IEEE machines.  If LONG_DOUBLE_TYPE_SIZE
   has been defined to be 96, then IEEE also invokes the particular
   XFmode (`long double' type) data structure used by the Motorola
   680x0 series processors.

   `IEEE', when REAL_WORDS_BIG_ENDIAN is zero, refers generally to
   little-endian IEEE machines. In this case, if LONG_DOUBLE_TYPE_SIZE
   has been defined to be 96, then IEEE also invokes the particular
   XFmode `long double' data structure used by the Intel 80x86 series
   processors.

   `DEC' refers specifically to the Digital Equipment Corp PDP-11
   and VAX floating point data structure.  This model currently
   supports no type wider than DFmode.

   `IBM' refers specifically to the IBM System/370 and compatible
   floating point data structure.  This model currently supports
   no type wider than DFmode.  The IBM conversions were contributed by
   frank@atom.ansto.gov.au (Frank Crawford).

   `C4X' refers specifically to the floating point format used on
   Texas Instruments TMS320C3x and TMS320C4x digital signal
   processors.  This supports QFmode (32-bit float, double) and HFmode
   (40-bit long double) where BITS_PER_BYTE is 32. Unlike IEEE
   floats, C4x floats are not rounded to be even. The C4x conversions
   were contributed by m.hayes@elec.canterbury.ac.nz (Michael Hayes) and
   Haj.Ten.Brugge@net.HCC.nl (Herman ten Brugge).

   If LONG_DOUBLE_TYPE_SIZE = 64 (the default, unless tm.h defines it)
   then `long double' and `double' are both implemented, but they
   both mean DFmode.  In this case, the software floating-point
   support available here is activated by writing
      #define REAL_ARITHMETIC
   in tm.h.

   The case LONG_DOUBLE_TYPE_SIZE = 128 activates TFmode support
   and may deactivate XFmode since `long double' is used to refer
   to both modes.  Defining INTEL_EXTENDED_IEEE_FORMAT to non-zero
   at the same time enables 80387-style 80-bit floats in a 128-bit
   padded image, as seen on IA-64.

   The macros FLOAT_WORDS_BIG_ENDIAN, HOST_FLOAT_WORDS_BIG_ENDIAN,
   contributed by Richard Earnshaw <Richard.Earnshaw@cl.cam.ac.uk>,
   separate the floating point unit's endian-ness from that of
   the integer addressing.  This permits one to define a big-endian
   FPU on a little-endian machine (e.g., ARM).  An extension to
   BYTES_BIG_ENDIAN may be required for some machines in the future.
   These optional macros may be defined in tm.h.  In real.h, they
   default to WORDS_BIG_ENDIAN, etc., so there is no need to define
   them for any normal host or target machine on which the floats
   and the integers have the same endian-ness.  */


/* The following converts gcc macros into the ones used by this file.  */

/* REAL_ARITHMETIC defined means that macros in real.h are
   defined to call emulator functions.  */
#ifdef REAL_ARITHMETIC

#if TARGET_FLOAT_FORMAT == VAX_FLOAT_FORMAT
/* PDP-11, Pro350, VAX: */
#define DEC 1
#else /* it's not VAX */
#if TARGET_FLOAT_FORMAT == IBM_FLOAT_FORMAT
/* IBM System/370 style */
#define IBM 1
#else /* it's also not an IBM */
#if TARGET_FLOAT_FORMAT == C4X_FLOAT_FORMAT
/* TMS320C3x/C4x style */
#define C4X 1
#else /* it's also not a C4X */
#if TARGET_FLOAT_FORMAT == IEEE_FLOAT_FORMAT
#define IEEE
#else /* it's not IEEE either */
/* UNKnown arithmetic.  We don't support this and can't go on.  */
unknown arithmetic type
#define UNK 1
#endif /* not IEEE */
#endif /* not C4X */
#endif /* not IBM */
#endif /* not VAX */

#define REAL_WORDS_BIG_ENDIAN FLOAT_WORDS_BIG_ENDIAN

#else
/* REAL_ARITHMETIC not defined means that the *host's* data
   structure will be used.  It may differ by endian-ness from the
   target machine's structure and will get its ends swapped
   accordingly (but not here).  Probably only the decimal <-> binary
   functions in this file will actually be used in this case.  */

#if HOST_FLOAT_FORMAT == VAX_FLOAT_FORMAT
#define DEC 1
#else /* it's not VAX */
#if HOST_FLOAT_FORMAT == IBM_FLOAT_FORMAT
/* IBM System/370 style */
#define IBM 1
#else /* it's also not an IBM */
#if HOST_FLOAT_FORMAT == IEEE_FLOAT_FORMAT
#define IEEE
#else /* it's not IEEE either */
unknown arithmetic type
#define UNK 1
#endif /* not IEEE */
#endif /* not IBM */
#endif /* not VAX */

#define REAL_WORDS_BIG_ENDIAN HOST_FLOAT_WORDS_BIG_ENDIAN

#endif /* REAL_ARITHMETIC not defined */

/* Define INFINITY for support of infinity.
   Define NANS for support of Not-a-Number's (NaN's).  */
#if !defined(DEC) && !defined(IBM) && !defined(C4X)
#define INFINITY
#define NANS
#endif

/* Support of NaNs requires support of infinity.  */
#ifdef NANS
#ifndef INFINITY
#define INFINITY
#endif
#endif

/* Find a host integer type that is at least 16 bits wide,
   and another type at least twice whatever that size is.  */

#if HOST_BITS_PER_CHAR >= 16
#define EMUSHORT char
#define EMUSHORT_SIZE HOST_BITS_PER_CHAR
#define EMULONG_SIZE (2 * HOST_BITS_PER_CHAR)
#else
#if HOST_BITS_PER_SHORT >= 16
#define EMUSHORT short
#define EMUSHORT_SIZE HOST_BITS_PER_SHORT
#define EMULONG_SIZE (2 * HOST_BITS_PER_SHORT)
#else
#if HOST_BITS_PER_INT >= 16
#define EMUSHORT int
#define EMUSHORT_SIZE HOST_BITS_PER_INT
#define EMULONG_SIZE (2 * HOST_BITS_PER_INT)
#else
#if HOST_BITS_PER_LONG >= 16
#define EMUSHORT long
#define EMUSHORT_SIZE HOST_BITS_PER_LONG
#define EMULONG_SIZE (2 * HOST_BITS_PER_LONG)
#else
/*  You will have to modify this program to have a smaller unit size.  */
#define EMU_NON_COMPILE
#endif
#endif
#endif
#endif

/* If no 16-bit type has been found and the compiler is GCC, try HImode.  */
#if defined(__GNUC__) && EMUSHORT_SIZE != 16
typedef int HItype __attribute__ ((mode (HI)));
typedef unsigned int UHItype __attribute__ ((mode (HI)));
#undef EMUSHORT
#undef EMUSHORT_SIZE
#undef EMULONG_SIZE
#define EMUSHORT HItype
#define UEMUSHORT UHItype
#define EMUSHORT_SIZE 16
#define EMULONG_SIZE 32
#else
#define UEMUSHORT unsigned EMUSHORT
#endif

#if HOST_BITS_PER_SHORT >= EMULONG_SIZE
#define EMULONG short
#else
#if HOST_BITS_PER_INT >= EMULONG_SIZE
#define EMULONG int
#else
#if HOST_BITS_PER_LONG >= EMULONG_SIZE
#define EMULONG long
#else
#if HOST_BITS_PER_LONGLONG >= EMULONG_SIZE
#define EMULONG long long int
#else
/*  You will have to modify this program to have a smaller unit size.  */
#define EMU_NON_COMPILE
#endif
#endif
#endif
#endif


/* The host interface doesn't work if no 16-bit size exists.  */
#if EMUSHORT_SIZE != 16
#define EMU_NON_COMPILE
#endif

/* OK to continue compilation.  */
#ifndef EMU_NON_COMPILE

/* Construct macros to translate between REAL_VALUE_TYPE and e type.
   In GET_REAL and PUT_REAL, r and e are pointers.
   A REAL_VALUE_TYPE is guaranteed to occupy contiguous locations
   in memory, with no holes.  */

#if MAX_LONG_DOUBLE_TYPE_SIZE == 96 || ¥
    ((INTEL_EXTENDED_IEEE_FORMAT != 0) && MAX_LONG_DOUBLE_TYPE_SIZE == 128)
/* Number of 16 bit words in external e type format */
# define NE 6
# define MAXDECEXP 4932
# define MINDECEXP -4956
# define GET_REAL(r,e)  memcpy ((e), (r), 2*NE)
# define PUT_REAL(e,r)						¥
	do {							¥
	  memcpy ((r), (e), 2*NE);				¥
	  if (2*NE < sizeof (*r))				¥
	    memset ((char *) (r) + 2*NE, 0, sizeof (*r) - 2*NE);	¥
	} while (0)
# else /* no XFmode */
#  if MAX_LONG_DOUBLE_TYPE_SIZE == 128
#   define NE 10
#   define MAXDECEXP 4932
#   define MINDECEXP -4977
#   define GET_REAL(r,e) memcpy ((e), (r), 2*NE)
#   define PUT_REAL(e,r)					¥
	do {							¥
	  memcpy ((r), (e), 2*NE);				¥
	  if (2*NE < sizeof (*r))				¥
	    memset ((char *) (r) + 2*NE, 0, sizeof (*r) - 2*NE);	¥
	} while (0)
#else
#define NE 6
#define MAXDECEXP 4932
#define MINDECEXP -4956
#ifdef REAL_ARITHMETIC
/* Emulator uses target format internally
   but host stores it in host endian-ness.  */

#define GET_REAL(r,e)							¥
do {									¥
     if (HOST_FLOAT_WORDS_BIG_ENDIAN == REAL_WORDS_BIG_ENDIAN)		¥
       e53toe ((const UEMUSHORT *) (r), (e));				¥
     else								¥
       {								¥
	 UEMUSHORT w[4];					¥
         memcpy (&w[3], ((const EMUSHORT *) r), sizeof (EMUSHORT));	¥
         memcpy (&w[2], ((const EMUSHORT *) r) + 1, sizeof (EMUSHORT));	¥
         memcpy (&w[1], ((const EMUSHORT *) r) + 2, sizeof (EMUSHORT));	¥
         memcpy (&w[0], ((const EMUSHORT *) r) + 3, sizeof (EMUSHORT));	¥
	 e53toe (w, (e));						¥
       }								¥
   } while (0)

#define PUT_REAL(e,r)							¥
do {									¥
     if (HOST_FLOAT_WORDS_BIG_ENDIAN == REAL_WORDS_BIG_ENDIAN)		¥
       etoe53 ((e), (UEMUSHORT *) (r));				¥
     else								¥
       {								¥
	 UEMUSHORT w[4];					¥
	 etoe53 ((e), w);						¥
         memcpy (((EMUSHORT *) r), &w[3], sizeof (EMUSHORT));		¥
         memcpy (((EMUSHORT *) r) + 1, &w[2], sizeof (EMUSHORT));	¥
         memcpy (((EMUSHORT *) r) + 2, &w[1], sizeof (EMUSHORT));	¥
         memcpy (((EMUSHORT *) r) + 3, &w[0], sizeof (EMUSHORT));	¥
       }								¥
   } while (0)

#else /* not REAL_ARITHMETIC */

/* emulator uses host format */
#define GET_REAL(r,e) e53toe ((const UEMUSHORT *) (r), (e))
#define PUT_REAL(e,r) etoe53 ((e), (UEMUSHORT *) (r))

#endif /* not REAL_ARITHMETIC */
#endif /* not TFmode */
#endif /* not XFmode */


/* Number of 16 bit words in internal format */
#define NI (NE+3)

/* Array offset to exponent */
#define E 1

/* Array offset to high guard word */
#define M 2

/* Number of bits of precision */
#define NBITS ((NI-4)*16)

/* Maximum number of decimal digits in ASCII conversion
 * = NBITS*log10(2)
 */
#define NDEC (NBITS*8/27)

/* The exponent of 1.0 */
#define EXONE (0x3fff)

#if defined(HOST_EBCDIC)
/* bit 8 is significant in EBCDIC */
#define CHARMASK 0xff
#else
#define CHARMASK 0x7f
#endif

extern int extra_warnings;
extern const UEMUSHORT ezero[NE], ehalf[NE], eone[NE], etwo[NE];
extern const UEMUSHORT elog2[NE], esqrt2[NE];

static void endian	PARAMS ((const UEMUSHORT *, long *,
			       enum machine_mode));
static void eclear	PARAMS ((UEMUSHORT *));
static void emov	PARAMS ((const UEMUSHORT *, UEMUSHORT *));
#if 0
static void eabs	PARAMS ((UEMUSHORT *));
#endif
static void eneg	PARAMS ((UEMUSHORT *));
static int eisneg	PARAMS ((const UEMUSHORT *));
static int eisinf	PARAMS ((const UEMUSHORT *));
static int eisnan	PARAMS ((const UEMUSHORT *));
static void einfin	PARAMS ((UEMUSHORT *));
#ifdef NANS
static void enan	PARAMS ((UEMUSHORT *, int));
static void einan	PARAMS ((UEMUSHORT *));
static int eiisnan	PARAMS ((const UEMUSHORT *));
static int eiisneg	PARAMS ((const UEMUSHORT *));
static void make_nan	PARAMS ((UEMUSHORT *, int, enum machine_mode));
#endif
static void emovi	PARAMS ((const UEMUSHORT *, UEMUSHORT *));
static void emovo	PARAMS ((const UEMUSHORT *, UEMUSHORT *));
static void ecleaz	PARAMS ((UEMUSHORT *));
static void ecleazs	PARAMS ((UEMUSHORT *));
static void emovz	PARAMS ((const UEMUSHORT *, UEMUSHORT *));
#if 0
static void eiinfin	PARAMS ((UEMUSHORT *));
#endif
#ifdef INFINITY
static int eiisinf	PARAMS ((const UEMUSHORT *));
#endif
static int ecmpm	PARAMS ((const UEMUSHORT *, const UEMUSHORT *));
static void eshdn1	PARAMS ((UEMUSHORT *));
static void eshup1	PARAMS ((UEMUSHORT *));
static void eshdn8	PARAMS ((UEMUSHORT *));
static void eshup8	PARAMS ((UEMUSHORT *));
static void eshup6	PARAMS ((UEMUSHORT *));
static void eshdn6	PARAMS ((UEMUSHORT *));
static void eaddm	PARAMS ((const UEMUSHORT *, UEMUSHORT *));
static void esubm	PARAMS ((const UEMUSHORT *, UEMUSHORT *));
static void m16m	PARAMS ((unsigned int, const UEMUSHORT *, UEMUSHORT *));
static int edivm	PARAMS ((const UEMUSHORT *, UEMUSHORT *));
static int emulm	PARAMS ((const UEMUSHORT *, UEMUSHORT *));
static void emdnorm	PARAMS ((UEMUSHORT *, int, int, EMULONG, int));
static void esub	PARAMS ((const UEMUSHORT *, const UEMUSHORT *,
				 UEMUSHORT *));
static void eadd	PARAMS ((const UEMUSHORT *, const UEMUSHORT *,
				 UEMUSHORT *));
static void eadd1	PARAMS ((const UEMUSHORT *, const UEMUSHORT *,
				 UEMUSHORT *));
static void ediv	PARAMS ((const UEMUSHORT *, const UEMUSHORT *,
				 UEMUSHORT *));
static void emul	PARAMS ((const UEMUSHORT *, const UEMUSHORT *,
				 UEMUSHORT *));
static void e53toe	PARAMS ((const UEMUSHORT *, UEMUSHORT *));
static void e64toe	PARAMS ((const UEMUSHORT *, UEMUSHORT *));
#if (INTEL_EXTENDED_IEEE_FORMAT == 0)
static void e113toe	PARAMS ((const UEMUSHORT *, UEMUSHORT *));
#endif
static void e24toe	PARAMS ((const UEMUSHORT *, UEMUSHORT *));
#if (INTEL_EXTENDED_IEEE_FORMAT == 0)
static void etoe113	PARAMS ((const UEMUSHORT *, UEMUSHORT *));
static void toe113	PARAMS ((UEMUSHORT *, UEMUSHORT *));
#endif
static void etoe64	PARAMS ((const UEMUSHORT *, UEMUSHORT *));
static void toe64	PARAMS ((UEMUSHORT *, UEMUSHORT *));
static void etoe53	PARAMS ((const UEMUSHORT *, UEMUSHORT *));
static void toe53	PARAMS ((UEMUSHORT *, UEMUSHORT *));
static void etoe24	PARAMS ((const UEMUSHORT *, UEMUSHORT *));
static void toe24	PARAMS ((UEMUSHORT *, UEMUSHORT *));
static int ecmp		PARAMS ((const UEMUSHORT *, const UEMUSHORT *));
#if 0
static void eround	PARAMS ((const UEMUSHORT *, UEMUSHORT *));
#endif
static void ltoe	PARAMS ((const HOST_WIDE_INT *, UEMUSHORT *));
static void ultoe	PARAMS ((const unsigned HOST_WIDE_INT *, UEMUSHORT *));
static void eifrac	PARAMS ((const UEMUSHORT *, HOST_WIDE_INT *,
				 UEMUSHORT *));
static void euifrac	PARAMS ((const UEMUSHORT *, unsigned HOST_WIDE_INT *,
				 UEMUSHORT *));
static int eshift	PARAMS ((UEMUSHORT *, int));
static int enormlz	PARAMS ((UEMUSHORT *));
#if 0
static void e24toasc	PARAMS ((const UEMUSHORT *, char *, int));
static void e53toasc	PARAMS ((const UEMUSHORT *, char *, int));
static void e64toasc	PARAMS ((const UEMUSHORT *, char *, int));
static void e113toasc	PARAMS ((const UEMUSHORT *, char *, int));
#endif /* 0 */
static void etoasc	PARAMS ((const UEMUSHORT *, char *, int));
static void asctoe24	PARAMS ((const char *, UEMUSHORT *));
static void asctoe53	PARAMS ((const char *, UEMUSHORT *));
static void asctoe64	PARAMS ((const char *, UEMUSHORT *));
#if (INTEL_EXTENDED_IEEE_FORMAT == 0)
static void asctoe113	PARAMS ((const char *, UEMUSHORT *));
#endif
static void asctoe	PARAMS ((const char *, UEMUSHORT *));
static void asctoeg	PARAMS ((const char *, UEMUSHORT *, int));
static void efloor	PARAMS ((const UEMUSHORT *, UEMUSHORT *));
#if 0
static void efrexp	PARAMS ((const UEMUSHORT *, int *,
				 UEMUSHORT *));
#endif
static void eldexp	PARAMS ((const UEMUSHORT *, int, UEMUSHORT *));
#if 0
static void eremain	PARAMS ((const UEMUSHORT *, const UEMUSHORT *,
				 UEMUSHORT *));
#endif
static void eiremain	PARAMS ((UEMUSHORT *, UEMUSHORT *));
static void mtherr	PARAMS ((const char *, int));
#ifdef DEC
static void dectoe	PARAMS ((const UEMUSHORT *, UEMUSHORT *));
static void etodec	PARAMS ((const UEMUSHORT *, UEMUSHORT *));
static void todec	PARAMS ((UEMUSHORT *, UEMUSHORT *));
#endif
#ifdef IBM
static void ibmtoe	PARAMS ((const UEMUSHORT *, UEMUSHORT *,
				 enum machine_mode));
static void etoibm	PARAMS ((const UEMUSHORT *, UEMUSHORT *,
				 enum machine_mode));
static void toibm	PARAMS ((UEMUSHORT *, UEMUSHORT *,
				 enum machine_mode));
#endif
#ifdef C4X
static void c4xtoe	PARAMS ((const UEMUSHORT *, UEMUSHORT *,
				 enum machine_mode));
static void etoc4x	PARAMS ((const UEMUSHORT *, UEMUSHORT *,
				 enum machine_mode));
static void toc4x	PARAMS ((UEMUSHORT *, UEMUSHORT *,
				 enum machine_mode));
#endif
#if 0
static void uditoe	PARAMS ((const UEMUSHORT *, UEMUSHORT *));
static void ditoe	PARAMS ((const UEMUSHORT *, UEMUSHORT *));
static void etoudi	PARAMS ((const UEMUSHORT *, UEMUSHORT *));
static void etodi	PARAMS ((const UEMUSHORT *, UEMUSHORT *));
static void esqrt	PARAMS ((const UEMUSHORT *, UEMUSHORT *));
#endif

/* Copy 32-bit numbers obtained from array containing 16-bit numbers,
   swapping ends if required, into output array of longs.  The
   result is normally passed to fprintf by the ASM_OUTPUT_ macros.  */

static void
endian (e, x, mode)
     const UEMUSHORT e[];
     long x[];
     enum machine_mode mode;
{
  unsigned long th, t;

  if (REAL_WORDS_BIG_ENDIAN)
    {
      switch (mode)
	{
	case TFmode:
#if (INTEL_EXTENDED_IEEE_FORMAT == 0)
	  /* Swap halfwords in the fourth long.  */
	  th = (unsigned long) e[6] & 0xffff;
	  t = (unsigned long) e[7] & 0xffff;
	  t |= th << 16;
	  x[3] = (long) t;
#else
	  x[3] = 0;
#endif
	  /* FALLTHRU */

	case XFmode:
	  /* Swap halfwords in the third long.  */
	  th = (unsigned long) e[4] & 0xffff;
	  t = (unsigned long) e[5] & 0xffff;
	  t |= th << 16;
	  x[2] = (long) t;
	  /* FALLTHRU */

	case DFmode:
	  /* Swap halfwords in the second word.  */
	  th = (unsigned long) e[2] & 0xffff;
	  t = (unsigned long) e[3] & 0xffff;
	  t |= th << 16;
	  x[1] = (long) t;
	  /* FALLTHRU */

	case SFmode:
	case HFmode:
	  /* Swap halfwords in the first word.  */
	  th = (unsigned long) e[0] & 0xffff;
	  t = (unsigned long) e[1] & 0xffff;
	  t |= th << 16;
	  x[0] = (long) t;
	  break;

	default:
	  abort ();
	}
    }
  else
    {
      /* Pack the output array without swapping.  */

      switch (mode)
	{
	case TFmode:
#if (INTEL_EXTENDED_IEEE_FORMAT == 0)
	  /* Pack the fourth long.  */
	  th = (unsigned long) e[7] & 0xffff;
	  t = (unsigned long) e[6] & 0xffff;
	  t |= th << 16;
	  x[3] = (long) t;
#else
	  x[3] = 0;
#endif
	  /* FALLTHRU */

	case XFmode:
	  /* Pack the third long.
	     Each element of the input REAL_VALUE_TYPE array has 16 useful bits
	     in it.  */
	  th = (unsigned long) e[5] & 0xffff;
	  t = (unsigned long) e[4] & 0xffff;
	  t |= th << 16;
	  x[2] = (long) t;
	  /* FALLTHRU */

	case DFmode:
	  /* Pack the second long */
	  th = (unsigned long) e[3] & 0xffff;
	  t = (unsigned long) e[2] & 0xffff;
	  t |= th << 16;
	  x[1] = (long) t;
	  /* FALLTHRU */

	case SFmode:
	case HFmode:
	  /* Pack the first long */
	  th = (unsigned long) e[1] & 0xffff;
	  t = (unsigned long) e[0] & 0xffff;
	  t |= th << 16;
	  x[0] = (long) t;
	  break;

	default:
	  abort ();
	}
    }
}


/* This is the implementation of the REAL_ARITHMETIC macro.  */

void
earith (value, icode, r1, r2)
     REAL_VALUE_TYPE *value;
     int icode;
     REAL_VALUE_TYPE *r1;
     REAL_VALUE_TYPE *r2;
{
  UEMUSHORT d1[NE], d2[NE], v[NE];
  enum tree_code code;

  GET_REAL (r1, d1);
  GET_REAL (r2, d2);
#ifdef NANS
/*  Return NaN input back to the caller.  */
  if (eisnan (d1))
    {
      PUT_REAL (d1, value);
      return;
    }
  if (eisnan (d2))
    {
      PUT_REAL (d2, value);
      return;
    }
#endif
  code = (enum tree_code) icode;
  switch (code)
    {
    case PLUS_EXPR:
      eadd (d2, d1, v);
      break;

    case MINUS_EXPR:
      esub (d2, d1, v);		/* d1 - d2 */
      break;

    case MULT_EXPR:
      emul (d2, d1, v);
      break;

    case RDIV_EXPR:
#ifndef REAL_INFINITY
      if (ecmp (d2, ezero) == 0)
	{
#ifdef NANS
	enan (v, eisneg (d1) ^ eisneg (d2));
	break;
#else
	abort ();
#endif
	}
#endif
      ediv (d2, d1, v);	/* d1/d2 */
      break;

    case MIN_EXPR:		/* min (d1,d2) */
      if (ecmp (d1, d2) < 0)
	emov (d1, v);
      else
	emov (d2, v);
      break;

    case MAX_EXPR:		/* max (d1,d2) */
      if (ecmp (d1, d2) > 0)
	emov (d1, v);
      else
	emov (d2, v);
      break;
    default:
      emov (ezero, v);
      break;
    }
PUT_REAL (v, value);
}


/* Truncate REAL_VALUE_TYPE toward zero to signed HOST_WIDE_INT.
   implements REAL_VALUE_RNDZINT (x) (etrunci (x)).  */

REAL_VALUE_TYPE
etrunci (x)
     REAL_VALUE_TYPE x;
{
  UEMUSHORT f[NE], g[NE];
  REAL_VALUE_TYPE r;
  HOST_WIDE_INT l;

  GET_REAL (&x, g);
#ifdef NANS
  if (eisnan (g))
    return (x);
#endif
  eifrac (g, &l, f);
  ltoe (&l, g);
  PUT_REAL (g, &r);
  return (r);
}


/* Truncate REAL_VALUE_TYPE toward zero to unsigned HOST_WIDE_INT;
   implements REAL_VALUE_UNSIGNED_RNDZINT (x) (etruncui (x)).  */

REAL_VALUE_TYPE
etruncui (x)
     REAL_VALUE_TYPE x;
{
  UEMUSHORT f[NE], g[NE];
  REAL_VALUE_TYPE r;
  unsigned HOST_WIDE_INT l;

  GET_REAL (&x, g);
#ifdef NANS
  if (eisnan (g))
    return (x);
#endif
  euifrac (g, &l, f);
  ultoe (&l, g);
  PUT_REAL (g, &r);
  return (r);
}


/* This is the REAL_VALUE_ATOF function.  It converts a decimal or hexadecimal
   string to binary, rounding off as indicated by the machine_mode argument.
   Then it promotes the rounded value to REAL_VALUE_TYPE.  */

REAL_VALUE_TYPE
ereal_atof (s, t)
     const char *s;
     enum machine_mode t;
{
  UEMUSHORT tem[NE], e[NE];
  REAL_VALUE_TYPE r;

  switch (t)
    {
#ifdef C4X
    case QFmode:
    case HFmode:
      asctoe53 (s, tem);
      e53toe (tem, e);
      break;
#else
    case HFmode:
#endif

    case SFmode:
      asctoe24 (s, tem);
      e24toe (tem, e);
      break;

    case DFmode:
      asctoe53 (s, tem);
      e53toe (tem, e);
      break;

    case TFmode:
#if (INTEL_EXTENDED_IEEE_FORMAT == 0)
      asctoe113 (s, tem);
      e113toe (tem, e);
      break;
#endif
      /* FALLTHRU */

    case XFmode:
      asctoe64 (s, tem);
      e64toe (tem, e);
      break;

    default:
      asctoe (s, e);
    }
  PUT_REAL (e, &r);
  return (r);
}


/* Expansion of REAL_NEGATE.  */

REAL_VALUE_TYPE
ereal_negate (x)
     REAL_VALUE_TYPE x;
{
  UEMUSHORT e[NE];
  REAL_VALUE_TYPE r;

  GET_REAL (&x, e);
  eneg (e);
  PUT_REAL (e, &r);
  return (r);
}


/* Round real toward zero to HOST_WIDE_INT;
   implements REAL_VALUE_FIX (x).  */

HOST_WIDE_INT
efixi (x)
     REAL_VALUE_TYPE x;
{
  UEMUSHORT f[NE], g[NE];
  HOST_WIDE_INT l;

  GET_REAL (&x, f);
#ifdef NANS
  if (eisnan (f))
    {
      warning ("conversion from NaN to int");
      return (-1);
    }
#endif
  eifrac (f, &l, g);
  return l;
}

/* Round real toward zero to unsigned HOST_WIDE_INT
   implements  REAL_VALUE_UNSIGNED_FIX (x).
   Negative input returns zero.  */

unsigned HOST_WIDE_INT
efixui (x)
     REAL_VALUE_TYPE x;
{
  UEMUSHORT f[NE], g[NE];
  unsigned HOST_WIDE_INT l;

  GET_REAL (&x, f);
#ifdef NANS
  if (eisnan (f))
    {
      warning ("conversion from NaN to unsigned int");
      return (-1);
    }
#endif
  euifrac (f, &l, g);
  return l;
}


/* REAL_VALUE_FROM_INT macro.  */

void
ereal_from_int (d, i, j, mode)
     REAL_VALUE_TYPE *d;
     HOST_WIDE_INT i, j;
     enum machine_mode mode;
{
  UEMUSHORT df[NE], dg[NE];
  HOST_WIDE_INT low, high;
  int sign;

  if (GET_MODE_CLASS (mode) != MODE_FLOAT)
    abort ();
  sign = 0;
  low = i;
  if ((high = j) < 0)
    {
      sign = 1;
      /* complement and add 1 */
      high = ‾high;
      if (low)
	low = -low;
      else
	high += 1;
    }
  eldexp (eone, HOST_BITS_PER_WIDE_INT, df);
  ultoe ((unsigned HOST_WIDE_INT *) &high, dg);
  emul (dg, df, dg);
  ultoe ((unsigned HOST_WIDE_INT *) &low, df);
  eadd (df, dg, dg);
  if (sign)
    eneg (dg);

  /* A REAL_VALUE_TYPE may not be wide enough to hold the two HOST_WIDE_INTS.
     Avoid double-rounding errors later by rounding off now from the
     extra-wide internal format to the requested precision.  */
  switch (GET_MODE_BITSIZE (mode))
    {
    case 32:
      etoe24 (dg, df);
      e24toe (df, dg);
      break;

    case 64:
      etoe53 (dg, df);
      e53toe (df, dg);
      break;

    case 96:
      etoe64 (dg, df);
      e64toe (df, dg);
      break;

    case 128:
#if (INTEL_EXTENDED_IEEE_FORMAT == 0)
      etoe113 (dg, df);
      e113toe (df, dg);
#else
      etoe64 (dg, df);
      e64toe (df, dg);
#endif
      break;

    default:
      abort ();
  }

  PUT_REAL (dg, d);
}


/* REAL_VALUE_FROM_UNSIGNED_INT macro.  */

void
ereal_from_uint (d, i, j, mode)
     REAL_VALUE_TYPE *d;
     unsigned HOST_WIDE_INT i, j;
     enum machine_mode mode;
{
  UEMUSHORT df[NE], dg[NE];
  unsigned HOST_WIDE_INT low, high;

  if (GET_MODE_CLASS (mode) != MODE_FLOAT)
    abort ();
  low = i;
  high = j;
  eldexp (eone, HOST_BITS_PER_WIDE_INT, df);
  ultoe (&high, dg);
  emul (dg, df, dg);
  ultoe (&low, df);
  eadd (df, dg, dg);

  /* A REAL_VALUE_TYPE may not be wide enough to hold the two HOST_WIDE_INTS.
     Avoid double-rounding errors later by rounding off now from the
     extra-wide internal format to the requested precision.  */
  switch (GET_MODE_BITSIZE (mode))
    {
    case 32:
      etoe24 (dg, df);
      e24toe (df, dg);
      break;

    case 64:
      etoe53 (dg, df);
      e53toe (df, dg);
      break;

    case 96:
      etoe64 (dg, df);
      e64toe (df, dg);
      break;

    case 128:
#if (INTEL_EXTENDED_IEEE_FORMAT == 0)
      etoe113 (dg, df);
      e113toe (df, dg);
#else
      etoe64 (dg, df);
      e64toe (df, dg);
#endif
      break;

    default:
      abort ();
  }

  PUT_REAL (dg, d);
}


/* REAL_VALUE_TO_INT macro.  */

void
ereal_to_int (low, high, rr)
     HOST_WIDE_INT *low, *high;
     REAL_VALUE_TYPE rr;
{
  UEMUSHORT d[NE], df[NE], dg[NE], dh[NE];
  int s;

  GET_REAL (&rr, d);
#ifdef NANS
  if (eisnan (d))
    {
      warning ("conversion from NaN to int");
      *low = -1;
      *high = -1;
      return;
    }
#endif
  /* convert positive value */
  s = 0;
  if (eisneg (d))
    {
      eneg (d);
      s = 1;
    }
  eldexp (eone, HOST_BITS_PER_WIDE_INT, df);
  ediv (df, d, dg);		/* dg = d / 2^32 is the high word */
  euifrac (dg, (unsigned HOST_WIDE_INT *) high, dh);
  emul (df, dh, dg);		/* fractional part is the low word */
  euifrac (dg, (unsigned HOST_WIDE_INT *) low, dh);
  if (s)
    {
      /* complement and add 1 */
      *high = ‾(*high);
      if (*low)
	*low = -(*low);
      else
	*high += 1;
    }
}


/* REAL_VALUE_LDEXP macro.  */

REAL_VALUE_TYPE
ereal_ldexp (x, n)
     REAL_VALUE_TYPE x;
     int n;
{
  UEMUSHORT e[NE], y[NE];
  REAL_VALUE_TYPE r;

  GET_REAL (&x, e);
#ifdef NANS
  if (eisnan (e))
    return (x);
#endif
  eldexp (e, n, y);
  PUT_REAL (y, &r);
  return (r);
}

/* These routines are conditionally compiled because functions
   of the same names may be defined in fold-const.c.  */

#ifdef REAL_ARITHMETIC

/* Check for infinity in a REAL_VALUE_TYPE.  */

int
target_isinf (x)
     REAL_VALUE_TYPE x ATTRIBUTE_UNUSED;
{
#ifdef INFINITY
  UEMUSHORT e[NE];

  GET_REAL (&x, e);
  return (eisinf (e));
#else
  return 0;
#endif
}

/* Check whether a REAL_VALUE_TYPE item is a NaN.  */

int
target_isnan (x)
     REAL_VALUE_TYPE x ATTRIBUTE_UNUSED;
{
#ifdef NANS
  UEMUSHORT e[NE];

  GET_REAL (&x, e);
  return (eisnan (e));
#else
  return (0);
#endif
}


/* Check for a negative REAL_VALUE_TYPE number.
   This just checks the sign bit, so that -0 counts as negative.  */

int
target_negative (x)
     REAL_VALUE_TYPE x;
{
  return ereal_isneg (x);
}

/* Expansion of REAL_VALUE_TRUNCATE.
   The result is in floating point, rounded to nearest or even.  */

REAL_VALUE_TYPE
real_value_truncate (mode, arg)
     enum machine_mode mode;
     REAL_VALUE_TYPE arg;
{
  UEMUSHORT e[NE], t[NE];
  REAL_VALUE_TYPE r;

  GET_REAL (&arg, e);
#ifdef NANS
  if (eisnan (e))
    return (arg);
#endif
  eclear (t);
  switch (mode)
    {
    case TFmode:
#if (INTEL_EXTENDED_IEEE_FORMAT == 0)
      etoe113 (e, t);
      e113toe (t, t);
      break;
#endif
      /* FALLTHRU */

    case XFmode:
      etoe64 (e, t);
      e64toe (t, t);
      break;

    case DFmode:
      etoe53 (e, t);
      e53toe (t, t);
      break;

    case SFmode:
#ifndef C4X
    case HFmode:
#endif
      etoe24 (e, t);
      e24toe (t, t);
      break;

#ifdef C4X
    case HFmode:
    case QFmode:
      etoe53 (e, t);
      e53toe (t, t);
      break;
#endif

    case SImode:
      r = etrunci (arg);
      return (r);

    /* If an unsupported type was requested, presume that
       the machine files know something useful to do with
       the unmodified value.  */

    default:
      return (arg);
    }
  PUT_REAL (t, &r);
  return (r);
}

/* Try to change R into its exact multiplicative inverse in machine mode
   MODE.  Return nonzero function value if successful.  */

int
exact_real_inverse (mode, r)
     enum machine_mode mode;
     REAL_VALUE_TYPE *r;
{
  UEMUSHORT e[NE], einv[NE];
  REAL_VALUE_TYPE rinv;
  int i;

  GET_REAL (r, e);

  /* Test for input in range.  Don't transform IEEE special values.  */
  if (eisinf (e) || eisnan (e) || (ecmp (e, ezero) == 0))
    return 0;

  /* Test for a power of 2: all significand bits zero except the MSB.
     We are assuming the target has binary (or hex) arithmetic.  */
  if (e[NE - 2] != 0x8000)
    return 0;

  for (i = 0; i < NE - 2; i++)
    {
      if (e[i] != 0)
	return 0;
    }

  /* Compute the inverse and truncate it to the required mode.  */
  ediv (e, eone, einv);
  PUT_REAL (einv, &rinv);
  rinv = real_value_truncate (mode, rinv);

#ifdef CHECK_FLOAT_VALUE
  /* This check is not redundant.  It may, for example, flush
     a supposedly IEEE denormal value to zero.  */
  i = 0;
  if (CHECK_FLOAT_VALUE (mode, rinv, i))
    return 0;
#endif
  GET_REAL (&rinv, einv);

  /* Check the bits again, because the truncation might have
     generated an arbitrary saturation value on overflow.  */
  if (einv[NE - 2] != 0x8000)
    return 0;

  for (i = 0; i < NE - 2; i++)
    {
      if (einv[i] != 0)
	return 0;
    }

  /* Fail if the computed inverse is out of range.  */
  if (eisinf (einv) || eisnan (einv) || (ecmp (einv, ezero) == 0))
    return 0;

  /* Output the reciprocal and return success flag.  */
  PUT_REAL (einv, r);
  return 1;
}
#endif /* REAL_ARITHMETIC defined */

/* Used for debugging--print the value of R in human-readable format
   on stderr.  */

void
debug_real (r)
     REAL_VALUE_TYPE r;
{
  char dstr[30];

  REAL_VALUE_TO_DECIMAL (r, "%.20g", dstr);
  fprintf (stderr, "%s", dstr);
}


/* The following routines convert REAL_VALUE_TYPE to the various floating
   point formats that are meaningful to supported computers.

   The results are returned in 32-bit pieces, each piece stored in a `long'.
   This is so they can be printed by statements like

      fprintf (file, "%lx, %lx", L[0],  L[1]);

   that will work on both narrow- and wide-word host computers.  */

/* Convert R to a 128-bit long double precision value.  The output array L
   contains four 32-bit pieces of the result, in the order they would appear
   in memory.  */

void
etartdouble (r, l)
     REAL_VALUE_TYPE r;
     long l[];
{
  UEMUSHORT e[NE];

  GET_REAL (&r, e);
#if INTEL_EXTENDED_IEEE_FORMAT == 0
  etoe113 (e, e);
#else
  etoe64 (e, e);
#endif
  endian (e, l, TFmode);
}

/* Convert R to a double extended precision value.  The output array L
   contains three 32-bit pieces of the result, in the order they would
   appear in memory.  */

void
etarldouble (r, l)
     REAL_VALUE_TYPE r;
     long l[];
{
  UEMUSHORT e[NE];

  GET_REAL (&r, e);
  etoe64 (e, e);
  endian (e, l, XFmode);
}

/* Convert R to a double precision value.  The output array L contains two
   32-bit pieces of the result, in the order they would appear in memory.  */

void
etardouble (r, l)
     REAL_VALUE_TYPE r;
     long l[];
{
  UEMUSHORT e[NE];

  GET_REAL (&r, e);
  etoe53 (e, e);
  endian (e, l, DFmode);
}

/* Convert R to a single precision float value stored in the least-significant
   bits of a `long'.  */

long
etarsingle (r)
     REAL_VALUE_TYPE r;
{
  UEMUSHORT e[NE];
  long l;

  GET_REAL (&r, e);
  etoe24 (e, e);
  endian (e, &l, SFmode);
  return ((long) l);
}

/* Convert X to a decimal ASCII string S for output to an assembly
   language file.  Note, there is no standard way to spell infinity or
   a NaN, so these values may require special treatment in the tm.h
   macros.  */

void
ereal_to_decimal (x, s)
     REAL_VALUE_TYPE x;
     char *s;
{
  UEMUSHORT e[NE];

  GET_REAL (&x, e);
  etoasc (e, s, 20);
}

/* Compare X and Y.  Return 1 if X > Y, 0 if X == Y, -1 if X < Y,
   or -2 if either is a NaN.  */

int
ereal_cmp (x, y)
     REAL_VALUE_TYPE x, y;
{
  UEMUSHORT ex[NE], ey[NE];

  GET_REAL (&x, ex);
  GET_REAL (&y, ey);
  return (ecmp (ex, ey));
}

/*  Return 1 if the sign bit of X is set, else return 0.  */

int
ereal_isneg (x)
     REAL_VALUE_TYPE x;
{
  UEMUSHORT ex[NE];

  GET_REAL (&x, ex);
  return (eisneg (ex));
}

/* End of REAL_ARITHMETIC interface */

/*
  Extended precision IEEE binary floating point arithmetic routines

  Numbers are stored in C language as arrays of 16-bit unsigned
  short integers.  The arguments of the routines are pointers to
  the arrays.

  External e type data structure, similar to Intel 8087 chip
  temporary real format but possibly with a larger significand:

	NE-1 significand words	(least significant word first,
				 most significant bit is normally set)
	exponent		(value = EXONE for 1.0,
				top bit is the sign)


  Internal exploded e-type data structure of a number (a "word" is 16 bits):

  ei[0]	sign word	(0 for positive, 0xffff for negative)
  ei[1]	biased exponent	(value = EXONE for the number 1.0)
  ei[2]	high guard word	(always zero after normalization)
  ei[3]
  to ei[NI-2]	significand	(NI-4 significand words,
 				 most significant word first,
 				 most significant bit is set)
  ei[NI-1]	low guard word	(0x8000 bit is rounding place)



 		Routines for external format e-type numbers

 	asctoe (string, e)	ASCII string to extended double e type
 	asctoe64 (string, &d)	ASCII string to long double
 	asctoe53 (string, &d)	ASCII string to double
 	asctoe24 (string, &f)	ASCII string to single
 	asctoeg (string, e, prec) ASCII string to specified precision
 	e24toe (&f, e)		IEEE single precision to e type
 	e53toe (&d, e)		IEEE double precision to e type
 	e64toe (&d, e)		IEEE long double precision to e type
 	e113toe (&d, e)		128-bit long double precision to e type
#if 0
 	eabs (e)			absolute value
#endif
 	eadd (a, b, c)		c = b + a
 	eclear (e)		e = 0
 	ecmp (a, b)		Returns 1 if a > b, 0 if a == b,
 				-1 if a < b, -2 if either a or b is a NaN.
 	ediv (a, b, c)		c = b / a
 	efloor (a, b)		truncate to integer, toward -infinity
 	efrexp (a, exp, s)	extract exponent and significand
 	eifrac (e, &l, frac)    e to HOST_WIDE_INT and e type fraction
 	euifrac (e, &l, frac)   e to unsigned HOST_WIDE_INT and e type fraction
 	einfin (e)		set e to infinity, leaving its sign alone
 	eldexp (a, n, b)	multiply by 2**n
 	emov (a, b)		b = a
 	emul (a, b, c)		c = b * a
 	eneg (e)			e = -e
#if 0
 	eround (a, b)		b = nearest integer value to a
#endif
 	esub (a, b, c)		c = b - a
#if 0
 	e24toasc (&f, str, n)	single to ASCII string, n digits after decimal
 	e53toasc (&d, str, n)	double to ASCII string, n digits after decimal
 	e64toasc (&d, str, n)	80-bit long double to ASCII string
 	e113toasc (&d, str, n)	128-bit long double to ASCII string
#endif
 	etoasc (e, str, n)	e to ASCII string, n digits after decimal
 	etoe24 (e, &f)		convert e type to IEEE single precision
 	etoe53 (e, &d)		convert e type to IEEE double precision
 	etoe64 (e, &d)		convert e type to IEEE long double precision
 	ltoe (&l, e)		HOST_WIDE_INT to e type
 	ultoe (&l, e)		unsigned HOST_WIDE_INT to e type
	eisneg (e)              1 if sign bit of e != 0, else 0
	eisinf (e)              1 if e has maximum exponent (non-IEEE)
 				or is infinite (IEEE)
        eisnan (e)              1 if e is a NaN


 		Routines for internal format exploded e-type numbers

 	eaddm (ai, bi)		add significands, bi = bi + ai
 	ecleaz (ei)		ei = 0
 	ecleazs (ei)		set ei = 0 but leave its sign alone
 	ecmpm (ai, bi)		compare significands, return 1, 0, or -1
 	edivm (ai, bi)		divide  significands, bi = bi / ai
 	emdnorm (ai,l,s,exp)	normalize and round off
 	emovi (a, ai)		convert external a to internal ai
 	emovo (ai, a)		convert internal ai to external a
 	emovz (ai, bi)		bi = ai, low guard word of bi = 0
 	emulm (ai, bi)		multiply significands, bi = bi * ai
 	enormlz (ei)		left-justify the significand
 	eshdn1 (ai)		shift significand and guards down 1 bit
 	eshdn8 (ai)		shift down 8 bits
 	eshdn6 (ai)		shift down 16 bits
 	eshift (ai, n)		shift ai n bits up (or down if n < 0)
 	eshup1 (ai)		shift significand and guards up 1 bit
 	eshup8 (ai)		shift up 8 bits
 	eshup6 (ai)		shift up 16 bits
 	esubm (ai, bi)		subtract significands, bi = bi - ai
        eiisinf (ai)            1 if infinite
        eiisnan (ai)            1 if a NaN
 	eiisneg (ai)		1 if sign bit of ai != 0, else 0
        einan (ai)              set ai = NaN
#if 0
        eiinfin (ai)            set ai = infinity
#endif

  The result is always normalized and rounded to NI-4 word precision
  after each arithmetic operation.

  Exception flags are NOT fully supported.

  Signaling NaN's are NOT supported; they are treated the same
  as quiet NaN's.

  Define INFINITY for support of infinity; otherwise a
  saturation arithmetic is implemented.

  Define NANS for support of Not-a-Number items; otherwise the
  arithmetic will never produce a NaN output, and might be confused
  by a NaN input.
  If NaN's are supported, the output of `ecmp (a,b)' is -2 if
  either a or b is a NaN. This means asking `if (ecmp (a,b) < 0)'
  may not be legitimate. Use `if (ecmp (a,b) == -1)' for `less than'
  if in doubt.

  Denormals are always supported here where appropriate (e.g., not
  for conversion to DEC numbers).  */

/* Definitions for error codes that are passed to the common er