/* Implementation of the internal dcigettext function.
   Copyright (C) 1995-1999, 2000, 2001 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU Library General Public License as published
   by the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
   USA.  */

/* Tell glibc's <string.h> to provide a prototype for mempcpy().
   This must come before <config.h> because <config.h> may include
   <features.h>, and once <features.h> has been included, it's too late.  */
#ifndef _GNU_SOURCE
# define _GNU_SOURCE	1
#endif

#define LOCALEDIR	""

/* !kawai! */
#ifdef HAVE_CONFIG_H
# include "../gcc/config.h"
#endif
/* end of !kawai! */

# define alloca __builtin_alloca
# define HAVE_ALLOCA 1

#include "../include/errno.h"
#ifndef __set_errno
# define __set_errno(val) errno = (val)
#endif

#include "../include/stddef.h"
#include "../include/stdlib.h"

#include "../include/string.h"


/* #include <locale.h> */

#include "gettextP.h"
#ifdef _LIBC
# include <libintl.h>
#else
# include "libgnuintl.h"
#endif
#include "hash-string.h"

/* Thread safetyness.  */
#ifdef _LIBC
# include <bits/libc-lock.h>
#else
/* Provide dummy implementation if this is outside glibc.  */
# define __libc_lock_define_initialized(CLASS, NAME)
# define __libc_lock_lock(NAME)
# define __libc_lock_unlock(NAME)
# define __libc_rwlock_define_initialized(CLASS, NAME)
# define __libc_rwlock_rdlock(NAME)
# define __libc_rwlock_unlock(NAME)
#endif

/* Alignment of types.  */
#if defined __GNUC__ && __GNUC__ >= 2
# define alignof(TYPE) __alignof__ (TYPE)
#else
# define alignof(TYPE) ¥
    ((int) &((struct { char dummy1; TYPE dummy2; } *) 0)->dummy2)
#endif

/* The internal variables in the standalone libintl.a must have different
   names than the internal variables in GNU libc, otherwise programs
   using libintl.a cannot be linked statically.  */
#if !defined _LIBC
# define _nl_default_default_domain _nl_default_default_domain__
# define _nl_current_default_domain _nl_current_default_domain__
# define _nl_default_dirname _nl_default_dirname__
# define _nl_domain_bindings _nl_domain_bindings__
#endif

/* Some compilers, like SunOS4 cc, don't have offsetof in <stddef.h>.  */
#ifndef offsetof
# define offsetof(type,ident) ((size_t)&(((type*)0)->ident))
#endif

/* @@ end of prolog @@ */

#ifdef _LIBC
/* Rename the non ANSI C functions.  This is required by the standard
   because some ANSI C functions will require linking with this object
   file and the name space must not be polluted.
   GCC LOCAL: Don't use #elif.  */
# define getcwd __getcwd
# ifndef stpcpy
#  define stpcpy __stpcpy
# endif
# define tfind __tfind
#else
# if !defined HAVE_GETCWD
char *getwd ();
#  define getcwd(buf, max) getwd (buf)
# else
#  if !defined HAVE_DECL_GETCWD
char *getcwd ();
#  endif
# endif
# ifndef HAVE_STPCPY
static char *stpcpy PARAMS ((char *dest, const char *src));
# endif
# ifndef HAVE_MEMPCPY
static void *mempcpy PARAMS ((void *dest, const void *src, size_t n));
# endif
#endif

/* Amount to increase buffer size by in each try.  */
#define PATH_INCR 32

/* The following is from pathmax.h.  */
/* Non-POSIX BSD systems might have gcc's limits.h, which doesn't define
   PATH_MAX but might cause redefinition warnings when sys/param.h is
   later included (as on MORE/BSD 4.3).  */
#if defined _POSIX_VERSION || (defined HAVE_LIMITS_H && !defined __GNUC__)
# include "../include/limits.h"
#endif

#ifndef _POSIX_PATH_MAX
# define _POSIX_PATH_MAX 255
#endif

#if !defined PATH_MAX && defined _PC_PATH_MAX
# define PATH_MAX (pathconf ("/", _PC_PATH_MAX) < 1 ? 1024 : pathconf ("/", _PC_PATH_MAX))
#endif

#if !defined PATH_MAX && defined MAXPATHLEN
# define PATH_MAX MAXPATHLEN
#endif

#ifndef PATH_MAX
# define PATH_MAX _POSIX_PATH_MAX
#endif

/* Pathname support.
   ISSLASH(C)           tests whether C is a directory separator character.
   IS_ABSOLUTE_PATH(P)  tests whether P is an absolute path.  If it is not,
                        it may be concatenated to a directory pathname.
   IS_PATH_WITH_DIR(P)  tests whether P contains a directory specification.
 */
#if defined _WIN32 || defined __WIN32__ || defined __EMX__ || defined __DJGPP__
  /* Win32, OS/2, DOS */
# define ISSLASH(C) ((C) == '/' || (C) == '¥¥')
# define HAS_DEVICE(P) ¥
    ((((P)[0] >= 'A' && (P)[0] <= 'Z') || ((P)[0] >= 'a' && (P)[0] <= 'z')) ¥
     && (P)[1] == ':')
# define IS_ABSOLUTE_PATH(P) (ISSLASH ((P)[0]) || HAS_DEVICE (P))
# define IS_PATH_WITH_DIR(P) ¥
    (strchr (P, '/') != NULL || strchr (P, '¥¥') != NULL || HAS_DEVICE (P))
#else
  /* Unix */
# define ISSLASH(C) ((C) == '/')
# define IS_ABSOLUTE_PATH(P) ISSLASH ((P)[0])
# define IS_PATH_WITH_DIR(P) (strchr (P, '/') != NULL)
#endif

/* XPG3 defines the result of `setlocale (category, NULL)' as:
   ``Directs `setlocale()' to query `category' and return the current
     setting of `local'.''
   However it does not specify the exact format.  Neither do SUSV2 and
   ISO C 99.  So we can use this feature only on selected systems (e.g.
   those using GNU C Library).  */
#if defined _LIBC || (defined __GNU_LIBRARY__ && __GNU_LIBRARY__ >= 2)
# define HAVE_LOCALE_NULL
#endif

/* This is the type used for the search tree where known translations
   are stored.  */
struct known_translation_t
{
  /* Domain in which to search.  */
  char *domainname;

  /* The category.  */
  int category;

  /* State of the catalog counter at the point the string was found.  */
  int counter;

  /* Catalog where the string was found.  */
  struct loaded_l10nfile *domain;

  /* And finally the translation.  */
  const char *translation;
  size_t translation_length;

  /* Pointer to the string in question.  */
  char msgid[ZERO];
};

/* Root of the search tree with known translations.  We can use this
   only if the system provides the `tsearch' function family.  */
#if defined HAVE_TSEARCH || defined _LIBC
# include <search.h>

static void *root;

# ifdef _LIBC
#  define tsearch __tsearch
# endif

/* Function to compare two entries in the table of known translations.  */
static int transcmp PARAMS ((const void *p1, const void *p2));
static int
transcmp (p1, p2)
     const void *p1;
     const void *p2;
{
  const struct known_translation_t *s1;
  const struct known_translation_t *s2;
  int result;

  s1 = (const struct known_translation_t *) p1;
  s2 = (const struct known_translation_t *) p2;

  result = strcmp (s1->msgid, s2->msgid);
  if (result == 0)
    {
      result = strcmp (s1->domainname, s2->domainname);
      if (result == 0)
	/* We compare the category last (though this is the cheapest
	   operation) since it is hopefully always the same (namely
	   LC_MESSAGES).  */
	result = s1->category - s2->category;
    }

  return result;
}
#endif

/* Name of the default domain used for gettext(3) prior any call to
   textdomain(3).  The default value for this is "messages".  */
const char _nl_default_default_domain[] = "messages";

/* Value used as the default domain for gettext(3).  */
const char *_nl_current_default_domain = _nl_default_default_domain;

/* Contains the default location of the message catalogs.  */
const char _nl_default_dirname[] = LOCALEDIR;

/* List with bindings of specific domains created by bindtextdomain()
   calls.  */
struct binding *_nl_domain_bindings;

/* Prototypes for local functions.  */
static char *plural_lookup PARAMS ((struct loaded_l10nfile *domain,
				    unsigned long int n,
				    const char *translation,
				    size_t translation_len))
     internal_function;
static unsigned long int plural_eval PARAMS ((struct expression *pexp,
					      unsigned long int n))
     internal_function;
static const char *category_to_name PARAMS ((int category)) internal_function;
static const char *guess_category_value PARAMS ((int category,
						 const char *categoryname))
     internal_function;


/* For those loosing systems which don't have `alloca' we have to add
   some additional code emulating it.  */
#ifdef HAVE_ALLOCA
/* Nothing has to be done.  */
# define ADD_BLOCK(list, address) /* nothing */
# define FREE_BLOCKS(list) /* nothing */
#else
struct block_list
{
  void *address;
  struct block_list *next;
};
# define ADD_BLOCK(list, addr)						      ¥
  do {									      ¥
    struct block_list *newp = (struct block_list *) malloc (sizeof (*newp));  ¥
    /* If we cannot get a free block we cannot add the new element to	      ¥
       the list.  */							      ¥
    if (newp != NULL) {							      ¥
      newp->address = (addr);						      ¥
      newp->next = (list);						      ¥
      (list) = newp;							      ¥
    }									      ¥
  } while (0)
# define FREE_BLOCKS(list)						      ¥
  do {									      ¥
    while (list != NULL) {						      ¥
      struct block_list *old = list;					      ¥
      list = list->next;						      ¥
      free (old);							      ¥
    }									      ¥
  } while (0)
# undef alloca
# define alloca(size) (malloc (size))
#endif	/* have alloca */


#ifdef _LIBC
/* List of blocks allocated for translations.  */
typedef struct transmem_list
{
  struct transmem_list *next;
  char data[ZERO];
} transmem_block_t;
static struct transmem_list *transmem_list;
#else
typedef unsigned char transmem_block_t;
#endif


/* Names for the libintl functions are a problem.  They must not clash
   with existing names and they should follow ANSI C.  But this source
   code is also used in GNU C Library where the names have a __
   prefix.  So we have to make a difference here.  */
#ifdef _LIBC
# define DCIGETTEXT __dcigettext
#else
# define DCIGETTEXT dcigettext__
#endif

/* Lock variable to protect the global data in the gettext implementation.  */
#ifdef _LIBC
__libc_rwlock_define_initialized (, _nl_state_lock)
#endif

/* Checking whether the binaries runs SUID must be done and glibc provides
   easier methods therefore we make a difference here.  */
#ifdef _LIBC
# define ENABLE_SECURE __libc_enable_secure
# define DETERMINE_SECURE
#else
# ifndef HAVE_GETUID
#  define getuid() 0
# endif
# ifndef HAVE_GETGID
#  define getgid() 0
# endif
# ifndef HAVE_GETEUID
#  define geteuid() getuid()
# endif
# ifndef HAVE_GETEGID
#  define getegid() getgid()
# endif
static int enable_secure;
# define ENABLE_SECURE (enable_secure == 1)
# define DETERMINE_SECURE ¥
  if (enable_secure == 0)						      ¥
    {									      ¥
/* !kawai! */ ¥
      if (1 /* getuid () != geteuid () || getgid () != getegid () */)		      ¥
/* end of !kawai! */ ¥
	enable_secure = 1;						      ¥
      else								      ¥
	enable_secure = -1;						      ¥
    }
#endif

/* Look up MSGID in the DOMAINNAME message catalog for the current
   CATEGORY locale and, if PLURAL is nonzero, search over string
   depending on the plural form determined by N.  */
char *
DCIGETTEXT (domainname, msgid1, msgid2, plural, n, category)
     const char *domainname;
     const char *msgid1;
     const char *msgid2;
     int plural;
     unsigned long int n;
     int category;
{
#ifndef HAVE_ALLOCA
  struct block_list *block_list = NULL;
#endif
  struct loaded_l10nfile *domain;
  struct binding *binding;
  const char *categoryname;
  const char *categoryvalue;
  char *dirname, *xdomainname;
  char *single_locale;
  char *retval;
  size_t retlen;
  int saved_errno;
#if defined HAVE_TSEARCH || defined _LIBC
  struct known_translation_t *search;
  struct known_translation_t **foundp = NULL;
  size_t msgid_len;
#endif
  size_t domainname_len;

  /* If no real MSGID is given return NULL.  */
  if (msgid1 == NULL)
    return NULL;

  __libc_rwlock_rdlock (_nl_state_lock);

  /* If DOMAINNAME is NULL, we are interested in the default domain.  If
     CATEGORY is not LC_MESSAGES this might not make much sense but the
     definition left this undefined.  */
  if (domainname == NULL)
    domainname = _nl_current_default_domain;

#if defined HAVE_TSEARCH || defined _LIBC
  msgid_len = strlen (msgid1) + 1;

  /* Try to find the translation among those which we found at
     some time.  */
  search = (struct known_translation_t *)
	   alloca (offsetof (struct known_translation_t, msgid) + msgid_len);
  memcpy (search->msgid, msgid1, msgid_len);
  search->domainname = (char *) domainname;
  search->category = category;

  foundp = (struct known_translation_t **) tfind (search, &root, transcmp);
  if (foundp != NULL && (*foundp)->counter == _nl_msg_cat_cntr)
    {
      /* Now deal with plural.  */
      if (plural)
	retval = plural_lookup ((*foundp)->domain, n, (*foundp)->translation,
				(*foundp)->translation_length);
      else
	retval = (char *) (*foundp)->translation;

      __libc_rwlock_unlock (_nl_state_lock);
      return retval;
    }
#endif

  /* Preserve the `errno' value.  */
  saved_errno = errno;

  /* See whether this is a SUID binary or not.  */
  DETERMINE_SECURE;

  /* First find matching binding.  */
  for (binding = _nl_domain_bindings; binding != NULL; binding = binding->next)
    {
      int compare = strcmp (domainname, binding->domainname);
      if (compare == 0)
	/* We found it!  */
	break;
      if (compare < 0)
	{
	  /* It is not in the list.  */
	  binding = NULL;
	  break;
	}
    }

  if (binding == NULL)
    dirname = (char *) _nl_default_dirname;
  else if (IS_ABSOLUTE_PATH (binding->dirname))
    dirname = binding->dirname;
  else
    {
      /* We have a relative path.  Make it absolute now.  */
      size_t dirname_len = strlen (binding->dirname) + 1;
      size_t path_max;
      char *ret;

      path_max = (unsigned int) PATH_MAX;
      path_max += 2;		/* The getcwd docs say to do this.  */

      for (;;)
	{
	  dirname = (char *) alloca (path_max + dirname_len);
	  ADD_BLOCK (block_list, dirname);

	  __set_errno (0);
	  ret = getcwd (dirname, path_max);
	  if (ret != NULL || errno != ERANGE)
	    break;

	  path_max += path_max / 2;
	  path_max += PATH_INCR;
	}

      if (ret == NULL)
	{
	  /* We cannot get the current working directory.  Don't signal an
	     error but simply return the default string.  */
	  FREE_BLOCKS (block_list);
	  __libc_rwlock_unlock (_nl_state_lock);
	  __set_errno (saved_errno);
	  return (plural == 0
		  ? (char *) msgid1
		  /* Use the Germanic plural rule.  */
		  : n == 1 ? (char *) msgid1 : (char *) msgid2);
	}

      stpcpy (stpcpy (strchr (dirname, '¥0'), "/"), binding->dirname);
    }

  /* Now determine the symbolic name of CATEGORY and its value.  */
  categoryname = category_to_name (category);
  categoryvalue = guess_category_value (category, categoryname);

  domainname_len = strlen (domainname);
  xdomainname = (char *) alloca (strlen (categoryname)
				 + domainname_len + 5);
  ADD_BLOCK (block_list, xdomainname);

  stpcpy (mempcpy (stpcpy (stpcpy (xdomainname, categoryname), "/"),
		  domainname, domainname_len),
	  ".mo");

  /* Creating working area.  */
  single_locale = (char *) alloca (strlen (categoryvalue) + 1);
  ADD_BLOCK (block_list, single_locale);


  /* Search for the given string.  This is a loop because we perhaps
     got an ordered list of languages to consider for the translation.  */
  while (1)
    {
      /* Make CATEGORYVALUE point to the next element of the list.  */
      while (categoryvalue[0] != '¥0' && categoryvalue[0] == ':')
	++categoryvalue;
      if (categoryvalue[0] == '¥0')
	{
	  /* The whole contents of CATEGORYVALUE has been searched but
	     no valid entry has been found.  We solve this situation
	     by implicitly appending a "C" entry, i.e. no translation
	     will take place.  */
	  single_locale[0] = 'C';
	  single_locale[1] = '¥0';
	}
      else
	{
	  char *cp = single_locale;
	  while (categoryvalue[0] != '¥0' && categoryvalue[0] != ':')
	    *cp++ = *categoryvalue++;
	  *cp = '¥0';

	  /* When this is a SUID binary we must not allow accessing files
	     outside the dedicated directories.  */
	  if (ENABLE_SECURE && IS_PATH_WITH_DIR (single_locale))
	    /* Ingore this entry.  */
	    continue;
	}

      /* If the current locale value is C (or POSIX) we don't load a
	 domain.  Return the MSGID.  */
      if (strcmp (single_locale, "C") == 0
	  || strcmp (single_locale, "POSIX") == 0)
	{
	  FREE_BLOCKS (block_list);
	  __libc_rwlock_unlock (_nl_state_lock);
	  __set_errno (saved_errno);
	  return (plural == 0
		  ? (char *) msgid1
		  /* Use the Germanic plural rule.  */
		  : n == 1 ? (char *) msgid1 : (char *) msgid2);
	}


      /* Find structure describing the message catalog matching the
	 DOMAINNAME and CATEGORY.  */
      domain = _nl_find_domain (dirname, single_locale, xdomainname, binding);

      if (domain != NULL)
	{
	  retval = _nl_find_msg (domain, binding, msgid1, &retlen);

	  if (retval == NULL)
	    {
	      int cnt;

	      for (cnt = 0; domain->successor[cnt] != NULL; ++cnt)
		{
		  retval = _nl_find_msg (domain->successor[cnt], binding,
					 msgid1, &retlen);

		  if (retval != NULL)
		    {
		      domain = domain->successor[cnt];
		      break;
		    }
		}
	    }

	  if (retval != NULL)
	    {
	      /* Found the translation of MSGID1 in domain DOMAIN:
		 starting at RETVAL, RETLEN bytes.  */
	      FREE_BLOCKS (block_list);
	      __set_errno (saved_errno);
#if defined HAVE_TSEARCH || defined _LIBC
	      if (foundp == NULL)
		{
		  /* Create a new entry and add it to the search tree.  */
		  struct known_translation_t *newp;

		  newp = (struct known_translation_t *)
		    malloc (offsetof (struct known_translation_t, msgid)
			    + msgid_len + domainname_len + 1);
		  if (newp != NULL)
		    {
		      newp->domainname =
			mempcpy (newp->msgid, msgid1, msgid_len);
		      memcpy (newp->domainname, domainname, domainname_len + 1);
		      newp->category = category;
		      newp->counter = _nl_msg_cat_cntr;
		      newp->domain = domain;
		      newp->translation = retval;
		      newp->translation_length = retlen;

		      /* Insert the entry in the search tree.  */
		      foundp = (struct known_translation_t **)
			tsearch (newp, &root, transcmp);
		      if (foundp == NULL
			  || __builtin_expect (*foundp != newp, 0))
			/* The insert failed.  */
			free (newp);
		    }
		}
	      else
		{
		  /* We can update the existing entry.  */
		  (*foundp)->counter = _nl_msg_cat_cntr;
		  (*foundp)->domain = domain;
		  (*foundp)->translation = retval;
		  (*foundp)->translation_length = retlen;
		}
#endif
	      /* Now deal with plural.  */
	      if (plural)
		retval = plural_lookup (domain, n, retval, retlen);

	      __libc_rwlock_unlock (_nl_state_lock);
	      return retval;
	    }
	}
    }
  /* NOTREACHED */
}


char *
internal_function
_nl_find_msg (domain_file, domainbinding, msgid, lengthp)
     struct loaded_l10nfile *domain_file;
     struct binding *domainbinding;
     const char *msgid;
     size_t *lengthp;
{
  struct loaded_domain *domain;
  size_t act;
  char *result;
  size_t resultlen;

  if (domain_file->decided == 0)
    _nl_load_domain (domain_file, domainbinding);

  if (domain_file->data == NULL)
    return NULL;

  domain = (struct loaded_domain *) domain_file->data;

  /* Locate the MSGID and its translation.  */
  if (domain->hash_size > 2 && domain->hash_tab != NULL)
    {
      /* Use the hashing table.  */
      nls_uint32 len = strlen (msgid);
      nls_uint32 hash_val = hash_string (msgid);
      nls_uint32 idx = hash_val % domain->hash_size;
      nls_uint32 incr = 1 + (hash_val % (domain->hash_size - 2));

      while (1)
	{
	  nls_uint32 nstr = W (domain->must_swap, domain->hash_tab[idx]);

	  if (nstr == 0)
	    /* Hash table entry is empty.  */
	    return NULL;

	  /* Compare msgid with the original string at index nstr-1.
	     We compare the lengths with >=, not ==, because plural entries
	     are represented by strings with an embedded NUL.  */
	  if (W (domain->must_swap, domain->orig_tab[nstr - 1].length) >= len
	      && (strcmp (msgid,
			  domain->data + W (domain->must_swap,
					    domain->orig_tab[nstr - 1].offset))
		  == 0))
	    {
	      act = nstr - 1;
	      goto found;
	    }

	  if (idx >= domain->hash_size - incr)
	    idx -= domain->hash_size - incr;
	  else
	    idx += incr;
	}
      /* NOTREACHED */
    }
  else
    {
      /* Try the default method:  binary search in the sorted array of
	 messages.  */
      size_t top, bottom;

      bottom = 0;
      act = 0;
      top = domain->nstrings;
      while (bottom < top)
	{
	  int cmp_val;

	  act = (bottom + top) / 2;
	  cmp_val = strcmp (msgid, (domain->data
				    + W (domain->must_swap,
					 domain->orig_tab[act].offset)));
	  if (cmp_val < 0)
	    top = act;
	  else if (cmp_val > 0)
	    bottom = act + 1;
	  else
	    goto found;
	}
      /* No translation was found.  */
      return NULL;
    }

 found:
  /* The translation was found at index ACT.  If we have to convert the
     string to use a different character set, this is the time.  */
  result = ((char *) domain->data
	    + W (domain->must_swap, domain->trans_tab[act].offset));
  resultlen = W (domain->must_swap, domain->trans_tab[act].length) + 1;

#if defined _LIBC || HAVE_ICONV
  if (domain->codeset_cntr
      != (domainbinding != NULL ? domainbinding->codeset_cntr : 0))
    {
      /* The domain's codeset has changed through bind_textdomain_codeset()
	 since the message catalog was initialized or last accessed.  We
	 have to reinitialize the converter.  */
      _nl_free_domain_conv (domain);
      _nl_init_domain_conv (domain_file, domain, domainbinding);
    }

  if (
# ifdef _LIBC
      domain->conv != (__gconv_t) -1
# else
#  if HAVE_ICONV
      domain->conv != (iconv_t) -1
#  endif
# endif
      )
    {
      /* We are supposed to do a conversion.  First allocate an
	 appropriate table with the same structure as the table
	 of translations in the file, where we can put the pointers
	 to the converted strings in.
	 There is a slight complication with plural entries.  They
	 are represented by consecutive NUL terminated strings.  We
	 handle this case by converting RESULTLEN bytes, including
	 NULs.  */

      if (domain->conv_tab == NULL
	  && ((domain->conv_tab = (char **) calloc (domain->nstrings,
						    sizeof (char *)))
	      == NULL))
	/* Mark that we didn't succeed allocating a table.  */
	domain->conv_tab = (char **) -1;

      if (__builtin_expect (domain->conv_tab == (char **) -1, 0))
	/* Nothing we can do, no more memory.  */
	goto converted;

      if (domain->conv_tab[act] == NULL)
	{
	  /* We haven't used this string so far, so it is not
	     translated yet.  Do this now.  */
	  /* We use a bit more efficient memory handling.
	     We allocate always larger blocks which get used over
	     time.  This is faster than many small allocations.   */
	  __libc_lock_define_initialized (static, lock)
# define INITIAL_BLOCK_SIZE	4080
	  static unsigned char *freemem;
	  static size_t freemem_size;

	  const unsigned char *inbuf;
	  unsigned char *outbuf;
	  int malloc_count;
# ifndef _LIBC
	  transmem_block_t *transmem_list = NULL;
# endif

	  __libc_lock_lock (lock);

	  inbuf = (const unsigned char *) result;
	  outbuf = freemem + sizeof (size_t);

	  malloc_count = 0;
	  while (1)
	    {
	      transmem_block_t *newmem;
# ifdef _LIBC
	      size_t non_reversible;
	      int res;

	      if (freemem_size < sizeof (size_t))
		goto resize_freemem;

	      res = __gconv (doma