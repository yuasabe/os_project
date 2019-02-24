/* alloca.c -- allocate automatically reclaimed memory
   (Mostly) portable public-domain implementation -- D A Gwyn

   This implementation of the PWB library alloca function,
   which is used to allocate space off the run-time stack so
   that it is automatically reclaimed upon procedure exit,
   was inspired by discussions with J. Q. Johnson of Cornell.
   J.Otto Tennant <jot@cray.com> contributed the Cray support.

   There are some preprocessor constants that can
   be defined when compiling for your specific system, for
   improved efficiency; however, the defaults should be okay.

   The general concept of this implementation is to keep
   track of all alloca-allocated blocks, and reclaim any
   that are found to be deeper in the stack than the current
   invocation.  This heuristic does not reclaim storage as
   soon as it becomes invalid, but it will do so eventually.

   As a special case, alloca(0) reclaims storage without
   allocating any.  It is a good idea to use alloca(0) in
   your main control loop, etc. to force garbage collection.  */

/*

@deftypefn Replacement void* alloca (size_t @var{size})

This function allocates memory which will be automatically reclaimed
after the procedure exits.  The @libib{} implementation does not free
the memory immediately but will do so eventually during subsequent
calls to this function.  Memory is allocated using @code{xmalloc} under
normal circumstances.

The header file @file{alloca-conf.h} can be used in conjunction with the
GNU Autoconf test @code{AC_FUNC_ALLOCA} to test for and properly make
available this function.  The @code{AC_FUNC_ALLOCA} test requires that
client code use a block of preprocessor code to be safe (see the Autoconf
manual for more); this header incorporates that logic and more, including
the possibility of a GCC built-in function.

@end deftypefn

*/

/* !kawai! */
#include "../include/string.h"
#include "../include/stdlib.h"

#include "config.h"
#include "../include/libiberty.h"

/* find_stack_direction() is removed. */

/* end of !kawai! */

/* These variables are used by the ASTRDUP implementation that relies
   on C_alloca.  */
const char *libiberty_optr;
char *libiberty_nptr;
unsigned long libiberty_len;

/* If your stack is a linked list of frames, you have to
   provide an "address metric" ADDRESS_FUNCTION macro.  */

#if defined (CRAY) && defined (CRAY_STACKSEG_END)
static long i00afunc ();
#define ADDRESS_FUNCTION(arg) (char *) i00afunc (&(arg))
#else
#define ADDRESS_FUNCTION(arg) &(arg)
#endif

#ifndef NULL
#define	NULL	0
#endif

#define	STACK_DIR	STACK_DIRECTION	/* Known at compile-time.  */

/* An "alloca header" is used to:
   (a) chain together all alloca'ed blocks;
   (b) keep track of stack depth.

   It is very important that sizeof(header) agree with malloc
   alignment chunk size.  The following default should work okay.  */

#ifndef	ALIGN_SIZE
#define	ALIGN_SIZE	sizeof(double)
#endif

typedef union hdr
{
  char align[ALIGN_SIZE];	/* To force sizeof(header).  */
  struct
    {
      union hdr *next;		/* For chaining headers.  */
      char *deep;		/* For stack depth measure.  */
    } h;
} header;

static header *last_alloca_header = NULL;	/* -> last alloca header.  */

/* Return a pointer to at least SIZE bytes of storage,
   which will be automatically reclaimed upon exit from
   the procedure that called alloca.  Originally, this space
   was supposed to be taken from the current stack frame of the
   caller, but that method cannot be made to work for some
   implementations of C, for example under Gould's UTX/32.  */

/* @undocumented C_alloca */

PTR
C_alloca (size)
     size_t size;
{
  auto char probe;		/* Probes stack depth: */
  register char *depth = ADDRESS_FUNCTION (probe);

/* !kawai! */
/* end of !kawai! */

  /* Reclaim garbage, defined as all alloca'd storage that
     was allocated from deeper in the stack than currently.  */

  {
    register header *hp;	/* Traverses linked list.  */

    for (hp = last_alloca_header; hp != NULL;)
      if ((STACK_DIR > 0 && hp->h.deep > depth)
	  || (STACK_DIR < 0 && hp->h.deep < depth))
	{
	  register header *np = hp->h.next;

	  free ((PTR) hp);	/* Collect garbage.  */

	  hp = np;		/* -> next header.  */
	}
      else
	break;			/* Rest are not deeper.  */

    last_alloca_header = hp;	/* -> last valid storage.  */
  }

  if (size == 0)
    return NULL;		/* No allocation required.  */

  /* Allocate combined header + user data storage.  */

  {
    register PTR new = xmalloc (sizeof (header) + size);
    /* Address of header.  */

    if (new == 0)
      abort();

    ((header *) new)->h.next = last_alloca_header;
    ((header *) new)->h.deep = depth;

    last_alloca_header = (header *) new;

    /* User storage begins just after header.  */

    return (PTR) ((char *) new + sizeof (header));
  }
}

#if defined (CRAY) && defined (CRAY_STACKSEG_END)

#ifdef DEBUG_I00AFUNC
#include <stdio.h>
#endif

#ifndef CRAY_STACK
#define CRAY_STACK
#ifndef CRAY2
/* Stack structures for CRAY-1, CRAY X-MP, and CRAY Y-MP */
struct stack_control_header
  {
    long shgrow:32;		/* Number of times stack has grown.  */
    long shaseg:32;		/* Size of increments to stack.  */
    long shhwm:32;		/* High water mark of stack.  */
    long shsize:32;		/* Current size of stack (all segments).  */
  };

/* The stack segment linkage control information occurs at
   the high-address end of a stack segment.  (The stack
   grows from low addresses to high addresses.)  The initial
   part of the stack segment linkage control information is
   0200 (octal) words.  This provides for register storage
   for the routine which overflows the stack.  */

struct stack_segment_linkage
  {
    long ss[0200];		/* 0200 overflow words.  */
    long sssize:32;		/* Number of words in this segment.  */
    long ssbase:32;		/* Offset to stack base.  */
    long:32;
    long sspseg:32;		/* Offset to linkage control of previous
				   segment of stack.  */
    long:32;
    long sstcpt:32;		/* Pointer to task common address block.  */
    long sscsnm;		/* Private control structure number for
				   microtasking.  */
    long ssusr1;		/* Reserved for user.  */
    long ssusr2;		/* Reserved for user.  */
    long sstpid;		/* Process ID for pid based multi-tasking.  */
    long ssgvup;		/* Pointer to multitasking thread giveup.  */
    long sscray[7];		/* Reserved for Cray Research.  */
    long ssa0;
    long ssa1;
    long ssa2;
    long ssa3;
    long ssa4;
    long ssa5;
    long ssa6;
    long ssa7;
    long sss0;
    long sss1;
    long sss2;
    long sss3;
    long sss4;
    long sss5;
    long sss6;
    long sss7;
  };

#else /* CRAY2 */
/* The following structure defines the vector of words
   returned by the STKSTAT library routine.  */
struct stk_stat
  {
    long now;			/* Current total stack size.  */
    long maxc;			/* Amount of contiguous space which would
				   be required to satisfy the maximum
				   stack demand to date.  */
    long high_water;		/* Stack high-water mark.  */
    long overflows;		/* Number of stack overflow ($STKOFEN) calls.  */
    long hits;			/* Number of internal buffer hits.  */
    long extends;		/* Number of block extensions.  */
    long stko_mallocs;		/* Block allocations by $STKOFEN.  */
    long underflows;		/* Number of stack underflow calls ($STKRETN).  */
    long stko_free;		/* Number of deallocations by $STKRETN.  */
    long stkm_free;		/* Number of deallocations by $STKMRET.  */
    long segments;		/* Current number of stack segments.  */
    long maxs;			/* Maximum number of stack segments so far.  */
    long pad_size;		/* Stack pad size.  */
    long current_address;	/* Current stack segment address.  */
    long current_size;		/* Current stack segment size.  This
				   number is actually corrupted by STKSTAT to
				   include the fifteen word trailer area.  */
    long initial_address;	/* Address of initial segment.  */
    long initia