/* Multibyte Character Functions.
   Copyright (C) 1998 Free Software Foundation, Inc.

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

/* Note regarding cross compilation:

   In general, translation of multibyte characters to wide characters can
   only work in a native compiler since the translation function (mbtowc)
   needs to know about both the source and target character encoding.  However,
   this particular implementation for JIS, SJIS and EUCJP source characters
   will work for any compiler with a newlib target.  Other targets may also
   work provided that their wchar_t implementation is 2 bytes and the encoding
   leaves the source character values unchanged (except for removing the
   state shifting markers).  */

#include "config.h"
#ifdef MULTIBYTE_CHARS
#include "system.h"
#include "mbchar.h"
#include <locale.h>

typedef enum {ESCAPE, DOLLAR, BRACKET, AT, B, J, NUL, JIS_CHAR, OTHER,
	      JIS_C_NUM} JIS_CHAR_TYPE;

typedef enum {ASCII, A_ESC, A_ESC_DL, JIS, JIS_1, JIS_2, J_ESC, J_ESC_BR,
	     J2_ESC, J2_ESC_BR, INV, JIS_S_NUM} JIS_STATE; 

typedef enum {COPYA, COPYJ, COPYJ2, MAKE_A, MAKE_J, NOOP,
	      EMPTY, ERROR} JIS_ACTION;

/* State/action tables for processing JIS encoding:

   Where possible, switches to JIS are grouped with proceding JIS characters
   and switches to ASCII are grouped with preceding JIS characters.
   Thus, maximum returned length is:
     2 (switch to JIS) + 2 (JIS characters) + 2 (switch back to ASCII) = 6.  */

static JIS_STATE JIS_state_table[JIS_S_NUM][JIS_C_NUM] = {
/*            ESCAPE DOLLAR   BRACKET   AT     B      J     NUL JIS_CHAR OTH*/
/*ASCII*/   { A_ESC, ASCII,   ASCII,    ASCII, ASCII, ASCII, ASCII,ASCII,ASCII},
/*A_ESC*/   { ASCII, A_ESC_DL,ASCII,    ASCII, ASCII, ASCII, ASCII,ASCII,ASCII},
/*A_ESC_DL*/{ ASCII, ASCII,   ASCII,    JIS,   JIS,   ASCII, ASCII,ASCII,ASCII},
/*JIS*/     { J_ESC, JIS_1,   JIS_1,    JIS_1, JIS_1, JIS_1, INV,  JIS_1,INV },
/*JIS_1*/   { INV,   JIS_2,   JIS_2,    JIS_2, JIS_2, JIS_2, INV,  JIS_2,INV },
/*JIS_2*/   { J2_ESC,JIS,     JIS,      JIS,   JIS,   JIS,   INV,  JIS,  JIS },
/*J_ESC*/   { INV,   INV,     J_ESC_BR, INV,   INV,   INV,   INV,  INV,  INV },
/*J_ESC_BR*/{ INV,   INV,     INV,      INV,   ASCII, ASCII, INV,  INV,  INV },
/*J2_ESC*/  { INV,   INV,     J2_ESC_BR,INV,   INV,   INV,   INV,  INV,  INV },
/*J2_ESC_BR*/{INV,   INV,     INV,      INV,   ASCII, ASCII, INV,  INV,  INV },
};

static JIS_ACTION JIS_action_table[JIS_S_NUM][JIS_C_NUM] = {
/*            ESCAPE DOLLAR BRACKET AT     B       J      NUL  JIS_CHAR OTH */
/*ASCII */   {NOOP,  COPYA, COPYA, COPYA,  COPYA,  COPYA, EMPTY, COPYA, COPYA},
/*A_ESC */   {COPYA, NOOP,  COPYA, COPYA,  COPYA,  COPYA, COPYA, COPYA, COPYA},
/*A_ESC_DL */{COPYA, COPYA, COPYA, MAKE_J, MAKE_J, COPYA, COPYA, COPYA, COPYA},
/*JIS */     {NOOP,  NOOP,  NOOP,  NOOP,   NOOP,   NOOP,  ERROR, NOOP,  ERROR},
/*JIS_1 */   {ERROR, NOOP,  NOOP,  NOOP,   NOOP,   NOOP,  ERROR, NOOP,  ERROR},
/*JIS_2 */   {NOOP,  COPYJ2,COPYJ2,COPYJ2, COPYJ2, COPYJ2,ERROR, COPYJ2,COPYJ2},
/*J_ESC */   {ERROR, ERROR, NOOP,  ERROR,  ERROR,  ERROR, ERROR, ERROR, ERROR},
/*J_ESC_BR */{ERROR, ERROR, ERROR, ERROR,  NOOP,   NOOP,  ERROR, ERROR, ERROR},
/*J2_ESC */  {ERROR, ERROR, NOOP,  ERROR,  ERROR,  ERROR, ERROR, ERROR, ERROR},
/*J2_ESC_BR*/{ERROR, ERROR, ERROR, ERROR,  COPYJ,  COPYJ, ERROR, ERROR, ERROR},
};


con