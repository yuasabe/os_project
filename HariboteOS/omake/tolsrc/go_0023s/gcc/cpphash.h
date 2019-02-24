/* Part of CPP library.
   Copyright (C) 1997, 1998, 1999, 2000, 2001, 2002
   Free Software Foundation, Inc.

This program is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 2, or (at your option) any
later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

/* This header defines all the internal data structures and functions
   that need to be visible across files.  It's called cpphash.h for
   historical reasons.  */

#ifndef GCC_CPPHASH_H
#define GCC_CPPHASH_H

#include "hashtable.h"

struct directive;		/* Deliberately incomplete.  */

/* Test if a sign is valid within a preprocessing number.  */
#define VALID_SIGN(c, prevc) ¥
  (((c) == '+' || (c) == '-') && ¥
   ((prevc) == 'e' || (prevc) == 'E' ¥
    || (((prevc) == 'p' || (prevc) == 'P') ¥
        && CPP_OPTION (pfile, extended_numbers))))

#define CPP_OPTION(PFILE, OPTION) ((PFILE)->opts.OPTION)
#define CPP_BUFFER(PFILE) ((PFILE)->buffer)
#define CPP_BUF_COLUMN(BUF, CUR) ((CUR) - (BUF)->line_base + (BUF)->col_adjust)
#define CPP_BUF_COL(BUF) CPP_BUF_COLUMN(BUF, (BUF)->cur)

/* Maximum nesting of cpp_buffers.  We use a static limit, partly for
   efficiency, and partly to limit runaway recursion.  */
#define CPP_STACK_MAX 200

/* A generic memory buffer, and operations on it.  */
typedef struct _cpp_buff _cpp_buff;
struct _cpp_buff
{
  struct _cpp_buff *next;
  unsigned char *base, *cur, *limit;
};

extern _cpp_buff *_cpp_get_buff PARAMS ((cpp_reader *, size_t));
extern void _cpp_release_buff PARAMS ((cpp_reader *, _cpp_buff *));
extern void _cpp_extend_buff PARAMS ((cpp_reader *, _cpp_buff **, size_t));
extern _cpp_buff *_cpp_append_extend_buff PARAMS ((cpp_reader *, _cpp_buff *,
						   size_t));
extern void _cpp_free_buff PARAMS ((_cpp_buff *));
extern unsigned char *_cpp_aligned_alloc PARAMS ((cpp_reader *, size_t));
extern unsigned char *_cpp_unaligned_alloc PARAMS ((cpp_reader *, size_t));

#define BUFF_ROOM(BUFF) (size_t) ((BUFF)->limit - (BUFF)->cur)
#define BUFF_FRONT(BUFF) ((BUFF)->cur)
#define BUFF_LIMIT(BUFF) ((BUFF)->limit)

/* ino_tやdev_tは、sys/types.hが必要 */
typedef int ino_t;
typedef int dev_t;

/* List of directories to look for include files in.  */
struct search_path
{
  struct search_path *next;

  /* NOTE: NAME may not be null terminated for the case of the current
     file's directory!  */
  const char *name;
  unsigned int len;
  /* We use these to tell if the directory mentioned here is a duplicate
     of an earlier directory on the search path.  */
  ino_t ino;
  dev_t dev;
  /* Non-zero if it is a system include directory.  */
  int sysp;
  /* Mapping of file names for this directory.  Only used on MS-DOS
     and related platforms.  */
  struct file_name_map *name_map;
};

/* #include types.  */
enum include_type {IT_INCLUDE, IT_INCLUDE_NEXT, IT_IMPORT, IT_CMDLINE};

union utoken
{
  const cpp_token *token;
  const cpp_token **ptoken;
};

/* A "run" of tokens; part of a chain of runs.  */
typedef struct tokenrun tokenrun;
struct tokenrun
{
  tokenrun *next, *prev;
  cpp_token *base, *limit;
};

typedef struct cpp_context cpp_context;
struct cpp_context
{
  /* Doubly-linked list.  */
  cpp_context *next, *prev;

  /* Contexts other than the base context are contiguous tokens.
     e.g. macro expansions, expanded argument tokens.  */
  union utoken first;
  union utoken last;

  /* If non-NULL, a buffer used for storage related to this context.
     When the context is popped, the buffer is released.  */
  _cpp