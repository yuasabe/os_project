/* Part of CPP library.  (include file handling)
   Copyright (C) 1986, 1987, 1989, 1992, 1993, 1994, 1995, 1998,
   1999, 2000, 2001, 2002 Free Software Foundation, Inc.
   Written by Per Bothner, 1994.
   Based on CCCP program by Paul Rubin, June 1986
   Adapted to ANSI C, Richard Stallman, Jan 1987
   Split out of cpplib.c, Zack Weinberg, Oct 1998

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

/* !kawai! */
/* inc->fd -> inc->fp */

#include "config.h"
#include "system.h"
#include "cpplib.h"
#include "cpphash.h"
#include "intl.h"
#include "mkdeps.h"
#include "../include/splay-tree.h"

#ifdef HAVE_MMAP_FILE
/* # include <sys/mman.h> */
/* end of !kawai! */

# ifndef MMAP_THRESHOLD
#  define MMAP_THRESHOLD 3 /* Minimum page count to mmap the file.  */
# endif
/* !kawai! */
# if MMAP_THRESHOLD & 0
/* end of !kawai! */
#  define TEST_THRESHOLD(size, pagesize) ¥
     (size / pagesize >= MMAP_THRESHOLD && (size % pagesize) != 0)
   /* Use mmap if the file is big enough to be worth it (controlled
      by MMAP_THRESHOLD) and if we can safely count on there being
      at least one readable NUL byte after the end of the file's
      contents.  This is true for all tested operating systems when
      the file size is not an exact multiple of the page size.  */
#  ifndef __CYGWIN__
#   define SHOULD_MMAP(size, pagesize) TEST_THRESHOLD (size, pagesize)
#  else
#   define WIN32_LEAN_AND_MEAN
#   include <windows.h>
    /* Cygwin can't correctly emulate mmap under Windows 9x style systems so
       disallow use of mmap on those systems.  Windows 9x does not zero fill
       memory at EOF and beyond, as required.  */
#   define SHOULD_MMAP(size, pagesize) ((GetVersion() & 0x80000000) ¥
    					? 0 : TEST_THRESHOLD (size, pagesize))
#  endif
# endif

#else  /* No MMAP_FILE */
#  undef MMAP_THRESHOLD
#  define MMAP_THRESHOLD 0
#endif

#ifndef O_BINARY
# define O_BINARY 0
#endif

/* If errno is inspected immediately after a system call fails, it will be
   nonzero, and no error number will ever be zero.  */
#ifndef ENOENT
# define ENOENT 0
#endif
#ifndef ENOTDIR
# define ENOTDIR 0
#endif

/* Suppress warning about function macros used w/o arguments in traditional
   C.  It is unlikely that glibc's strcmp macro helps this file at all.  */
/* #undef strcmp */

/* This structure is used for the table of all includes.  */
struct include_file
{
  const char *name;		/* actual path name of file */
  const cpp_hashnode *cmacro;	/* macro, if any, preventing reinclusion.  */
  const struct search_path *foundhere;
				/* location in search path where file was
				   found, for #include_next and sysp.  */
  const unsigned char *buffer;	/* pointer to cached file contents */
	unsigned int size;
#if 0
  struct stat st;		/* copy of stat(2) data for file */
#endif
  FILE *fp;			/* fd open on file (short term storage only) */
  int err_no;			/* errno obtained if opening a file failed */
  unsigned short include_count;	/* number of times file has been read */
  unsigned short refcnt;	/* number of stacked buffers using this file */
  unsigned char mapped;		/* file buffer is mmapped */
};

/* Variable length record files on VMS will have a stat size that includes
   record control characters that won't be included in the read size.  */
#ifdef VMS
# define FAB_C_VAR 2 /* variable length records (see Starlet fabdef.h) */
# define STAT_SIZE_TOO_BIG(ST) ((ST).st_fab_rfm == FAB_C_VAR)
#else
# define STAT_SIZE_TOO_BIG(ST) 0
#endif

/* The cmacro works like this: If it's NULL, the file is to be
   included again.  If it's NEVER_REREAD, the file is never to be
   included again.  Otherwise it is a macro hashnode, and the file is
   to be included again if the macro is defined.  */
#define NEVER_REREAD ((const cpp_hashnode *)-1)
#define DO_NOT_REREAD(inc) ¥
((inc)->cmacro && ((inc)->cmacro == NEVER_REREAD ¥
		   || (inc)->cmacro->type == NT_MACRO))
#define NO_INCLUDE_PATH ((struct include_file *) -1)

static struct file_name_map *read_name_map
				PARAMS ((cpp_reader *, const char *));
static char *read_filename_string PARAMS ((int, FILE *));
static char *remap_filename 	PARAMS ((cpp_reader *, char *,
					 struct search_path *));
static struct search_path *search_from PARAMS ((cpp_reader *,
						enum include_type));
static struct include_file *
	find_include_file PARAMS ((cpp_reader *, const cpp_token *,
				   enum include_type));
static struct include_file *open_file PARAMS ((cpp_reader *, const char *));
static int read_include_file	PARAMS ((cpp_reader *, struct include_file *));
static bool stack_include_file	PARAMS ((cpp_reader *, struct include_file *));
static void purge_cache 	PARAMS ((struct include_file *));
static void destroy_node	PARAMS ((splay_tree_value));
static int report_missing_guard		PARAMS ((splay_tree_node, void *));
static splay_tree_node find_or_create_entry PARAMS ((cpp_reader *,
						     const char *));
static void handle_missing_header PARAMS ((cpp_reader *, const char *, int));
static int remove_component_p	PARAMS ((const char *));

/* Set up the splay tree we use to store information about all the
   file names seen in this compilation.  We also have entries for each
   file we tried to open but failed; this saves system calls since we
   don't try to open it again in future.

   The key of each node is the file name, after processing by
   _cpp_simplify_pathname.  The path name may or may not be absolute.
   The path string has been malloced, as is automatically freed by
   registering free () as the splay tree key deletion function.

   A node's value is a pointer to a struct include_file, and is never
   NULL.  */
void
_cpp_init_includes (pfile)
     cpp_reader *pfile;
{
  pfile->all_include_files
    = splay_tree_new ((splay_tree_compare_fn) strcmp,
		      (splay_tree_delete_key_fn) free,
		      destroy_node);
}

/* Tear down the splay tree.  */
void
_cpp_cleanup_includes (pfile)
     cpp_reader *pfile;
{
  splay_tree_delete (pfile->all_include_files);
}

/* Free a node.  The path string is automatically freed.  */
static void
destroy_node (v)
     splay_tree_value v;
{
  struct include_file *f = (struct include_file *)v;

  if (f)
    {
      purge_cache (f);
      free (f);
    }
}

/* Mark a file to not be reread (e.g. #import, read failure).  */
void
_cpp_never_reread (file)
     struct include_file *file;
{
  file->cmacro = NEVER_REREAD;
}

/* Lookup a filename, which is simplified after making a copy, and
   create an entry if none exists.  errno is nonzero iff a (reported)
   stat() error occurred during simplification.  */
static splay_tree_node
find_or_create_entry (pfile, fname)
     cpp_reader *pfile;
     const char *fname;
{
  splay_tree_node node;
  struct include_file *file;
  char *name = xstrdup (fname);

  _cpp_simplify_pathname (name);
  node = splay_tree_lookup (pfile->all_include_files, (splay_tree_key) name);
  if (node)
    free (name);
  else
    {
      file = xcnew (struct include_file);
      file->name = name;
      file->err_no = errno;
      node = splay_tree_insert (pfile->all_include_files,
				(splay_tree_key) file->name,
				(splay_tree_value) file);
    }

  return node;
}

/* Enter a file name in the splay tree, for the sake of cpp_included.  */
void
_cpp_fake_include (pfile, fname)
     cpp_reader *pfile;
     const char *fname;
{
  find_or_create_entry (pfile, fname);
}

/* Given a file name, look it up in the cache; if there is no entry,
   create one with a non-NULL value (regardless of success in opening
   the file).  If the file doesn't exist or is inaccessible, this
   entry is flagged so we don't attempt to open it again in the
   future.  If the file isn't open, open it.  The empty string is
   interpreted as stdin.

   Returns an include_file structure with an open file descriptor on
   success, or NULL on failure.  */
static struct include_file *
open_file (pfile, filename)
     cpp_reader *pfile;
     const char *filename;
{
  splay_tree_node nd = find_or_create_entry (pfile, filename);
  struct include_file *file = (struct include_file *) nd->value;

  if (file->err_no)
    {
      /* Ugh.  handle_missing_header () needs errno to be set.  */
      errno = file->err_no;
      return 0;
    }

  /* Don't reopen an idempotent file.  */
  if (DO_NOT_REREAD (file))
    return file;
      
  /* Don't reopen one which is already loaded.  */
  if (file->buffer != NULL)
    return file;

  /* We used to open files in nonblocking mode, but that caused more
     problems than it solved.  Do take care not to acquire a
     controlling terminal by mistake (this can't happen on sane
     systems, but paranoia is a virtue).

     Use the three-argument form of open even though we aren't
     specifying O_CREAT, to defend against broken system headers.

     O_BINARY tells some runtime libraries (notably DJGPP) not to do
     newline translation; we can handle DOS line breaks just fine
     ourselves.

     Special case: the empty string is translated to stdin.  */

	/* special-case is disabled. */

#if 0
  if (filename[0] == '¥0')
    file->fp = stdin;
  else
#endif
    file->fp = fopen (file->name, "rb");

	if (file->fp != NULL) {
		fseek(file->fp, 0, SEEK_END);
		file->size = ftell(file->fp);
		fseek(file->fp, 0, SEEK_SET);

		return file;
	}

  file->err_no = errno;
  return NULL;
}

/* Place the file referenced by INC into a new buffer on the buffer
   stack, unless there are errors, or the file is not re-included
   because of e.g. multiple-include guards.  Returns true if a buffer
   is stacked.  */
static bool
stack_include_file (pfile, inc)
     cpp_reader *pfile;
     struct include_file *inc;
{
  cpp_buffer *fp;
  int sysp;
  const char *filename;

  if (DO_NOT_REREAD (inc))
    return false;

  sysp = MAX ((pfile->map ? pfile->map->sysp : 0),
	      (inc->foundhere ? inc->foundhere->sysp : 0));

  /* For -M, add the file to the dependencies on its first inclusion.  */
  if (CPP_OPTION (pfile, print_deps) > sysp && !inc->include_count)
    deps_add_dep (pfile->deps, inc->name);

  /* Not in cache?  */
  if (! inc->buffer)
    {
      if (read_include_file (pfile, inc))
	{
	  /* If an error occurs, do not try to read this file again.  */
	  _cpp_never_reread (inc);
	  return false;
	}
      /* Mark a regular, zero-length file never-reread.  We read it,
	 NUL-terminate it, and stack it once, so preprocessing a main
	 file of zero length does not raise an error.  */
      if (/* S_ISREG (inc->st.st_mode) && */ inc->size == 0)
	_cpp_never_reread (inc);
      fclose (inc->fp);
      inc->fp = NULL;
    }

  if (pfile->buffer)
    /* We don't want MI guard advice for the main file.  */
    inc->include_count++;

  /* Push a buffer.  */
  fp = cpp_push_buffer (pfile, inc->buffer, inc->size,
			/* from_stage3 */ CPP_OPTION (pfile, preprocessed), 0);
  fp->inc = inc;
  fp->inc->refcnt++;

  /* Initialise controlling macro state.  */
  pfile->mi_valid = true;
  pfile->mi_cmacro = 0;

  /* Generate the call back.  */
  filename = inc->name;
  if (*filename == '¥0')
    filename = "<stdin>";
  _cpp_do_file_change (pfile, LC_ENTER, filename, 1, sysp);

  return true;
}

/* Read the file referenced by INC into the file cache.

   If fd points to a plain file, we might be able to mmap it; we can
   definitely allocate the buffer all at once.  If fd is a pipe or