/* Extended support for using errno values.
   Written by Fred Fish.  fnf@cygnus.com
   This file is in the public domain.  --Per Bothner.  */

/* !kawai! */
#include "../include/ansidecl.h"
#include "../include/libiberty.h"
/* end of !kawai! */



#include "config.h"

#undef HAVE_SYS_ERRLIST

#ifdef HAVE_SYS_ERRLIST
/* Note that errno.h (not sure what OS) or stdio.h (BSD 4.4, at least)
   might declare sys_errlist in a way that the compiler might consider
   incompatible with our later declaration, perhaps by using const
   attributes.  So we hide the declaration in errno.h (if any) using a
   macro. */
#define sys_nerr sys_nerr__
#define sys_errlist sys_errlist__
#endif

#include "../include/stdio.h"
#include "../include/errno.h"

#ifdef HAVE_SYS_ERRLIST
#undef sys_nerr
#undef sys_errlist
#endif


/*  Routines imported from standard C runtime libraries. */

#ifdef HAVE_STDLIB_H
#include "../include/stdlib.h"
#endif

#ifdef HAVE_STRING_H
#include "../include/string.h"
#endif

#ifndef MAX
#  define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif

static void init_error_tables PARAMS ((void));

/* Translation table for errno values.  See intro(2) in most UNIX systems
   Programmers Reference Manuals.

   Note that this table is generally only accessed when it is used at runtime
   to initialize errno name and message tables that are indexed by errno
   value.

   Not all of these errnos will exist on all systems.  This table is the only
   thing that should have to be updated as new error numbers are introduced.
   It's sort of ugly, but at least its portable. */

struct error_info
{
  const int value;		/* The numeric value from <errno.h> */
  const char *const name;	/* The equivalent symbolic value */
#ifndef HAVE_SYS_ERRLIST
  const char *const msg;	/* Short message about this value */
#endif
};

#ifndef HAVE_SYS_ERRLIST
#   define ENTRY(value, name, msg)	{value, name, msg}
#else
#   define ENTRY(value, name, msg)	{value, name}
#endif

static const struct error_info error_table[] =
{
#if defined (EPERM)
  ENTRY(EPERM, "EPERM", "Not owner"),
#endif
#if defined (ENOENT)
  ENTRY(ENOENT, "ENOENT", "No such file or directory"),
#endif
#if defined (ESRCH)
  ENTRY(ESRCH, "ESRCH", "No such process"),
#endif
#if defined (EINTR)
  ENTRY(EINTR, "EINTR", "Interrupted system call"),
#endif
#if defined (EIO)
  ENTRY(EIO, "EIO", "I/O error"),
#endif
#if defined (ENXIO)
  ENTRY(ENXIO, "ENXIO", "No such device or address"),
#endif
#if defined (E2BIG)
  ENTRY(E2BIG, "E2BIG", "Arg list too long"),
#endif
#if defined (ENOEXEC)
  ENTRY(ENOEXEC, "ENOEXEC", "Exec format error"),
#endif
#if defined (EBADF)
  ENTRY(EBADF, "EBADF", "Bad file number"),
#endif
#if defined (ECHILD)
  ENTRY(ECHILD, "ECHILD", "No child processes"),
#endif
#if defined (EWOULDBLOCK)	/* Put before EAGAIN, sometimes aliased */
  ENTRY(EWOULDBLOCK, "EWOULDBLOCK", "Operation would block"),
#endif
#if defined (EAGAIN)
  ENTRY(EAGAIN, "EAGAIN", "No more processes"),
#endif
#if defined (ENOMEM)
  ENTRY(ENOMEM, "ENOMEM", "Not enough space"),
#endif
#if defined (EACCES)
  ENTRY(EACCES, "EACCES", "Permission denied"),
#endif
#if defined (EFAULT)
  ENTRY(EFAULT, "EFAULT", "Bad address"),
#endif
#if defined (ENOTBLK)
  ENTRY(ENOTBLK, "ENOTBLK", "Block device required"),
#endif
#if defined (EBUSY)
  ENTRY(EBUSY, "EBUSY", "Device busy"),
#endif
#if defined (EEXIST)
  ENTRY(EEXIST, "EEXIST", "File exists"),
#endif
#if defined (EXDEV)
  ENTRY(EXDEV, "EXDEV", "Cross-device link"),
#endif
#if defined (ENODEV)
  ENTRY(ENODEV, "ENODEV", "No such device"),
#endif
#if defined (ENOTDIR)
  ENTRY(ENOTDIR, "ENOTDIR", "Not a directory"),
#endif
#if defined (EISDIR)
  ENTRY(EISDIR, "EISDIR", "Is a directory"),
#endif
#if defined (EINVAL)
  ENTRY(EINVAL, "EINVAL", "Invalid argument"),
#endif
#if defined (ENFILE)
  ENTRY(ENFILE, "ENFILE", "File table overflow"),
#endif
#if defined (EMFILE)
  ENTRY(EMFILE, "EMFILE", "Too many open files"),
#endif
#if define