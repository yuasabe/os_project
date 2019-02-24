/* Generate information regarding function declarations and definitions based
   on information stored in GCC's tree structure.  This code implements the
   -aux-info option.
   Copyright (C) 1989, 1991, 1994, 1995, 1997, 1998,
   1999, 2000 Free Software Foundation, Inc.
   Contributed by Ron Guilmette (rfg@segfault.us.com).

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
#include "toplev.h"
#include "flags.h"
#include "tree.h"
#include "c-tree.h"

enum formals_style_enum {
  ansi,
  k_and_r_names,
  k_and_r_decls
};
typedef enum formals_style_enum formals_style;


static const char *data_type;

static char *affix_data_type		PARAMS ((const char *)) ATTRIBUTE_MALLOC;
static const char *gen_formal_list_for_type PARAMS ((tree, formals_style));
static int   deserves_ellipsis		PARAMS ((tree));
static const char *gen_formal_list_for_func_def PARAMS ((tree, formals_style));
static const char *gen_type		PARAMS ((const char *, tree, formals_style));
static const char *gen_decl		PARAMS ((tree, int, formals_style));

/* Given a string representing an entire type or an entire declaration
   which only lacks the actual "data-type" specifier (at its left end),
   affix the data-type specifier to the left end of the given type
   specification or object declaration.

   Because of C language weirdness, the data-type specifier (which normally
   goes in at the very left end) may have to be slipped in just to the
   right of any leading "const" or "volatile" qualifiers (there may be more
   than one).  Actually this may not be strictly necessary because it seems
   that GCC (at least) accepts `<data-type> const foo;' and treats it the
   same as `const <data-type> foo;' but people are accustomed to seeing
   `const char *foo;' and *not* `char const *foo;' so we try to create types
   that look as expected.  */

static char *
affix_data_type (param)
     const char *param;
{
  char *const type_or_decl = ASTRDUP (param);
  char *p = type_or_decl;
  char *qualifiers_then_data_type;
  char saved;

  /* Skip as many leading const's or volatile's as there are.  */

  for (;;)
    {
      if (!strncmp (p, "volatile ", 9))
        {
          p += 9;
          continue;
        }
      if (!strncmp (p, "const ", 6))
        {
          p += 6;
          continue;
        }
      break;
    }

  /* p now points to the place where we can insert the data type.  We have to
     add a blank after the data-type of course.  */

  if (p == type_or_decl)
    return concat (data_type, " ", type_or_decl, NULL);

  saved = *p;
  *p = '¥0';
  qualifiers_then_data_type = concat (type_or_decl, data_type, NULL);
  *p = saved;
  return reconcat (qualifiers_then_data_type,
		   qualifiers_then_data_type, " ", p, NULL);
}

/* Given a tree node which represents some "function type", generate the
   source code version of a formal parameter list (of some given style) for
   this function type.  Return the whole formal parameter list (including
   a pair of surrounding parens) as a string.   Note that if the style
   we are currently aiming for is non-ansi, then we just return a pair
   of empty parens here.  */

static const char *
gen_formal_list_for_type (fntype, style)
     tree fntype;
     formals_style style;
{
  const char *formal_list = "";
  tree formal_type;

  if (style != an