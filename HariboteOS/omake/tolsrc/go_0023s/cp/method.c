/* Handle the hair of processing (but not expanding) inline functions.
   Also manage function and variable name overloading.
   Copyright (C) 1987, 1989, 1992, 1993, 1994, 1995, 1996, 1997, 1998,
   1999, 2000 Free Software Foundation, Inc.
   Contributed by Michael Tiemann (tiemann@cygnus.com)

This file is part of GNU CC.
   
GNU CC is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

GNU CC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU CC; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */

/* !kawai! */
/* Handle method declarations.  */
#include "../gcc/config.h"
#include "../gcc/system.h"
#include "../gcc/tree.h"
#include "cp-tree.h"
#include "../include/obstack.h"
#include "../gcc/rtl.h"
#include "../gcc/expr.h"
#include "../gcc/output.h"
#include "../gcc/flags.h"
#include "../gcc/toplev.h"
#include "../gcc/ggc.h"
#include "../gcc/tm_p.h"
/* end of !kawai! */

/* Various flags to control the mangling process.  */

enum mangling_flags
{
  /* No flags.  */
  mf_none = 0,
  /* The thing we are presently mangling is part of a template type,
     rather than a fully instantiated type.  Therefore, we may see
     complex expressions where we would normally expect to see a
     simple integer constant.  */
  mf_maybe_uninstantiated = 1,
  /* When mangling a numeric value, use the form `_XX_' (instead of
     just `XX') if the value has more than one digit.  */
  mf_use_underscores_around_value = 2,
};

typedef enum mangling_flags mangling_flags;

#define obstack_chunk_alloc xmalloc
#define obstack_chunk_free free

static void do_build_assign_ref PARAMS ((tree));
static void do_build_copy_constructor PARAMS ((tree));
static tree synthesize_exception_spec PARAMS ((tree, tree (*) (tree, void *), void *));
static tree locate_dtor PARAMS ((tree, void *));
static tree locate_ctor PARAMS ((tree, void *));
static tree locate_copy PARAMS ((tree, void *));

/* Called once to initialize method.c.  */

void
init_method ()
{
  init_mangle ();
}


/* Set the mangled name (DECL_ASSEMBLER_NAME) for DECL.  */

void
set_mangled_name_for_decl (decl)
     tree decl;
{
  if (processing_template_decl)
    /* There's no need to mangle the name of a template function.  */
    return;

  mangle_decl (decl);
}


/* Given a tree_code CODE, and some arguments (at least one),
   attempt to use an overloaded operator on the arguments.

   For unary operators, only the first argument need be checked.
   For binary operators, both arguments may need to be checked.

   Member functions can convert class references to class pointers,
   for one-level deep indirection.  More than that is not supported.
   Operators [](), ()(), and ->() must be member functions.

   We call function call building calls with LOOKUP_COMPLAIN if they
   are our only hope.  This is true when we see a vanilla operator
   applied to something of aggregate type.  If this fails, we are free
   to return `error_mark_node', because we will have reported the
   error.

   Operators NEW and DELETE overload in funny ways: operator new takes
   a single `size' parameter, and operator delete takes a pointer to the
   storage being deleted.  When overloading these operators, success is
   assumed.  If there is a failure, report an error message and return
   `error_mark_node'.  */

/* NOSTRICT */
tree
build_opfncall (code, flags, xarg1, xarg2, arg3)
     enum tree_code code;
     int flags;
     tree xarg1, xarg2, arg3;
{
  return build_new_op (code, flags, xarg1, xarg2, arg3);
}
