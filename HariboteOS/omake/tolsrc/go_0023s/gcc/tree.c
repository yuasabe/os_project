/* Language-independent node constructors for parse phase of GNU compiler.
   Copyright (C) 1987, 1988, 1992, 1993, 1994, 1995, 1996, 1997, 1998,
   1999, 2000, 2001, 2002 Free Software Foundation, Inc.

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

/* This file contains the low level primitives for operating on tree nodes,
   including allocation, list operations, interning of identifiers,
   construction of data type nodes and statement nodes,
   and construction of type conversion nodes.  It also contains
   tables index by tree code that describe how to take apart
   nodes of that code.

   It is intended to be language-independent, but occasionally
   calls language-dependent routines defined (for C) in typecheck.c.

   The low-level allocation routines oballoc and permalloc
   are used also for allocating many other kinds of objects
   by all passes of the compiler.  */

/* !kawai! */
#include "config.h"
#include "system.h"
#include "flags.h"
#include "tree.h"
#include "tm_p.h"
#include "function.h"
#include "../include/obstack.h"
#include "toplev.h"
#include "ggc.h"
#include "../include/hashtab.h"
#include "output.h"
#include "target.h"
#include "langhooks.h"
/* end of !kawai! */

#define obstack_chunk_alloc xmalloc
#define obstack_chunk_free free
/* obstack.[ch] explicitly declined to prototype this.  */
extern int _obstack_allocated_p PARAMS ((struct obstack *h, PTR obj));

static void unsave_expr_now_r PARAMS ((tree));

/* Objects allocated on this obstack last forever.  */

struct obstack permanent_obstack;

/* Table indexed by tree code giving a string containing a character
   classifying the tree code.  Possibilities are
   t, d, s, c, r, <, 1, 2 and e.  See tree.def for details.  */

#define DEFTREECODE(SYM, NAME, TYPE, LENGTH) TYPE,

char tree_code_type[MAX_TREE_CODES] = {
#include "tree.def"
};
#undef DEFTREECODE

/* Table indexed by tree code giving number of expression
   operands beyond the fixed part of the node structure.
   Not used for types or decls.  */

#define DEFTREECODE(SYM, NAME, TYPE, LENGTH) LENGTH,

int tree_code_length[MAX_TREE_CODES] = {
#include "tree.def"
};
#undef DEFTREECODE

/* Names of tree components.
   Used for printing out the tree and error messages.  */
#define DEFTREECODE(SYM, NAME, TYPE, LEN) NAME,

const char *tree_code_name[MAX_TREE_CODES] = {
#include "tree.def"
};
#undef DEFTREECODE

/* Statistics-gathering stuff.  */
typedef enum
{
  d_kind,
  t_kind,
  b_kind,
  s_kind,
  r_kind,
  e_kind,
  c_kind,
  id_kind,
  perm_list_kind,
  temp_list_kind,
  vec_kind,
  x_kind,
  lang_decl,
  lang_type,
  all_kinds
} tree_node_kind;

int tree_node_counts[(int) all_kinds];
int tree_node_sizes[(int) all_kinds];

static const char * const tree_node_kind_names[] = {
  "decls",
  "types",
  "blocks",
  "stmts",
  "refs",
  "exprs",
  "constants",
  "identifiers",
  "perm_tree_lists",
  "temp_tree_lists",
  "vecs",
  "random kinds",
  "lang_decl kinds",
  "lang_type kinds"
};

/* Unique id for next decl created.  */
static int next_decl_uid;
/* Unique id for next type created.  */
static int next_type_uid = 1;

/* Since we cannot rehash a type after it is in the table, we have to
   keep the hash code.  */

struct type_hash
{
  unsigned long hash;
  tree type;
};

/* Initial size of the hash table (rounded to next prime).  */
#define TYPE_HASH_INITIAL_SIZE 1000

/* Now here is the hash table.  When recording a type, it is added to
   the slot whose index is the hash code.  Note that the hash table is
   used for several kinds of types (function types, array types and
   array index range types, for now).  While all these live in the
   same table, they are completely independent, and the hash code is
   computed differently for each of these.  */

htab_t type_hash_table;

static void build_real_from_int_cst_1 PARAMS ((PTR));
static void set_type_quals PARAMS ((tree, int));
static void append_random_chars PARAMS ((char *));
static int type_hash_eq PARAMS ((const void*, const void*));
static unsigned int type_hash_hash PARAMS ((const void*));
static void print_type_hash_statistics PARAMS((void));
static void finish_vector_type PARAMS((tree));
static tree make_vector PARAMS ((enum machine_mode, tree, int));
static int type_hash_marked_p PARAMS ((const void *));
static void type_hash_mark PARAMS ((const void *));
static int mark_tree_hashtable_entry PARAMS((void **, void *));

/* If non-null, these are language-specific helper functions for
   unsave_expr_now.  If present, LANG_UNSAVE is called before its
   argument (an UNSAVE_EXPR) is to be unsaved, and all other
   processing in unsave_expr_now is aborted.  LANG_UNSAVE_EXPR_NOW is
   called from unsave_expr_1 for language-specific tree codes.  */
void (*lang_unsave) PARAMS ((tree *));
void (*lang_unsave_expr_now) PARAMS ((tree));

/* If non-null, these are language-specific helper functions for
   unsafe_for_reeval.  Return negative to not handle some tree.  */
int (*lang_unsafe_for_reeval) PARAMS ((tree));

/* Set the DECL_ASSEMBLER_NAME for a node.  If it is the sort of thing
   that the assembler should talk about, set DECL_ASSEMBLER_NAME to an
   appropriate IDENTIFIER_NODE.  Otherwise, set it to the
   ERROR_MARK_NODE to ensure that the assembler does not talk about
   it.  */
void (*lang_set_decl_assembler_name)     PARAMS ((tree));

tree global_trees[TI_MAX];
tree integer_types[itk_none];

/* Set the DECL_ASSEMBLER_NAME for DECL.  */
void
set_decl_assembler_name (decl)
     tree decl;
{
  /* The language-independent code should never use the
     DECL_ASSEMBLER_NAME for lots of DECLs.  Only FUNCTION_DECLs and
     VAR_DECLs for variables with static storage duration need a real
     DECL_ASSEMBLER_NAME.  */
  if (TREE_CODE (decl) == FUNCTION_DECL
      || (TREE_CODE (decl) == VAR_DECL 
	  && (TREE_STATIC (decl) 
	      || DECL_EXTERNAL (decl) 
	      || TREE_PUBLIC (decl))))
    /* By default, assume the name to use in assembly code is the
       same as that used in the source language.  (That's correct
       for C, and GCC used to set DECL_ASSEMBLER_NAME to the same
       value as DECL_NAME in build_decl, so this choice provides
       backwards compatibility with existing front-ends.  */
    SET_DECL_ASSEMBLER_NAME (decl, DECL_NAME (decl));
  else
    /* Nobody should ever be asking for the DECL_ASSEMBLER_NAME of
       these DECLs -- unless they're in language-dependent code, in
       which case lang_set_decl_assembler_name should handle things.  */
    abort ();
}

/* Init the principal obstacks.  */

void
init_obstacks ()
{
  gcc_obstack_init (&permanent_obstack);

  /* Initialize the hash table of types.  */
  type_hash_table = htab_create (TYPE_HASH_INITIAL_SIZE, type_hash_hash,
				 type_hash_eq, 0);
  ggc_add_deletable_htab (type_hash_table, type_hash_marked_p,
			  type_hash_mark);
  ggc_add_tree_root (global_trees, TI_MAX);
  ggc_add_tree_root (integer_types, itk_none);

  /* Set lang_set_decl_set_assembler_name to a default value.  */
  lang_set_decl_assembler_name = set_decl_assembler_name;
}


/* Allocate SIZE bytes in the permanent obstack
   and return a pointer to them.  */

char *
permalloc (size)
     int size;
{
  return (char *) obstack_alloc (&permanent_obstack, size);
}

/* Allocate NELEM items of SIZE bytes in the permanent obstack
   and return a pointer to them.  The storage is cleared before
   returning the value.  */

char *
perm_calloc (nelem, size)
     int nelem;
     long size;
{
  char *rval = (char *) obstack_alloc (&permanent_obstack, nelem * size);
  memset (rval, 0, nelem * size);
  return rval;
}

/* Compute the number of bytes occupied by 'node'.  This routine only
   looks at TREE_CODE and, if the code is TREE_VEC, TREE_VEC_LENGTH.  */
size_t
tree_size (node)
     tree node;
{
  enum tree_code code = TREE_CODE (node);

  switch (TREE_CODE_CLASS (code))
    {
    case 'd':  /* A decl node */
      return sizeof (struct tree_decl);

    case 't':  /* a type node */
      return sizeof (struct tree_type);

    case 'b':  /* a lexical block node */
      return sizeof (struct tree_block);

    case 'r':  /* a reference */
    case 'e':  /* an expression */
    case 's':  /* an expression with side effects */
    case '<':  /* a comparison expression */
    case '1':  /* a unary arithmetic expression */
    case '2':  /* a binary arithmetic expression */
      return (sizeof (struct tree_exp)
	      + (TREE_CODE_LENGTH (code) - 1) * sizeof (char *));

    case 'c':  /* a constant */
      /* We can't use TREE_CODE_LENGTH for INTEGER_CST, since the number of
	 words is machine-dependent due to varying length of HOST_WIDE_INT,
	 which might be wider than a pointer (e.g., long long).  Similarly
	 for REAL_CST, since the number of words is machine-dependent due
	 to varying size and alignment of `double'.  */
      if (code == INTEGER_CST)
	return sizeof (struct tree_int_cst);
      else if (code == REAL_CST)
	return sizeof (struct tree_real_cst);
      else
	return (sizeof (struct tree_common)
		+ TREE_CODE_LENGTH (code) * sizeof (char *));

    case 'x':  /* something random, like an identifier.  */
      {
	  size_t length;
	  length = (sizeof (struct tree_common)
		    + TREE_CODE_LENGTH (code) * sizeof (char *));
	  if (code == TREE_VEC)
	    length += (TREE_VEC_LENGTH (node) - 1) * sizeof (char *);
	  return length;
      }

    default:
      abort ();
    }
}

/* Return a newly allocated node of code CODE.
   For decl and type nodes, some other fields are initialized.
   The rest of the node is initialized to zero.

   Achoo!  I got a code in the node.  */

tree
make_node (code)
     enum tree_code code;
{
  tree t;
  int type = TREE_CODE_CLASS (code);
  size_t length;
#ifdef GATHER_STATISTICS
  tree_node_kind kind;
#endif
  struct tree_common ttmp;
  
  /* We can't allocate a TREE_VEC without knowing how many elements
     it will have.  */
  if (code == TREE_VEC)
    abort ();
  
  TREE_SET_CODE ((tree)&ttmp, code);
  length = tree_size ((tree)&ttmp);

#ifdef GATHER_STATISTICS
  switch (type)
    {
    case 'd':  /* A decl node */
      kind = d_kind;
      break;

    case 't':  /* a type node */
      kind = t_kind;
      break;

    case 'b':  /* a lexical block */
      kind = b_kind;
      break;

    case 's':  /* an expression with side effects */
      kind = s_kind;
      break;

    case 'r':  /* a reference */
      kind = r_kind;
      break;

    case 'e':  /* an expression */
    case '<':  /* a comparison expression */
    case '1':  /* a unary arithmetic expression */
    case '2':  /* a binary arithmetic expression */
      kind = e_kind;
      break;

    case 'c':  /* a constant */
      kind = c_kind;
      break;

    case 'x':  /* something random, like an identifier.  */
      if (code == IDENTIFIER_NODE)
	kind = id_kind;
      else if (code == TREE_VEC)
	kind = vec_kind;
      else
	kind = x_kind;
      break;

    default:
      abort ();
    }

  tree_node_counts[(int) kind]++;
  tree_node_sizes[(int) kind] += length;
#endif

  t = ggc_alloc_tree (length);

  memset ((PTR) t, 0, length);

  TREE_SET_CODE (t, code);

  switch (type)
    {
    case 's':
      TREE_SIDE_EFFECTS (t) = 1;
      TREE_TYPE (t) = void_type_node;
      break;

    case 'd':
      if (code != FUNCTION_DECL)
	DECL_ALIGN (t) = 1;
      DECL_USER_ALIGN (t) = 0;
      DECL_IN_SYSTEM_HEADER (t) = in_system_header;
      DECL_SOURCE_LINE (t) = lineno;
      DECL_SOURCE_FILE (t) =
	(input_filename) ? input_filename : "<built-in>";
      DECL_UID (t) = next_decl_uid++;

      /* We have not yet computed the alias set for this declaration.  */
      DECL_POINTER_ALIAS_SET (t) = -1;
      break;

    case 't':
      TYPE_UID (t) = next_type_uid++;
      TYPE_ALIGN (t) = char_type_node ? TYPE_ALIGN (char_type_node) : 0;
      TYPE_USER_ALIGN (t) = 0;
      TYPE_MAIN_VARIANT (t) = t;

      /* Default to no attributes for type, but let target change that.  */
      TYPE_ATTRIBUTES (t) = NULL_TREE;
      (*targetm.set_default_type_attributes) (t);

      /* We have not yet computed the alias set for this type.  */
      TYPE_ALIAS_SET (t) = -1;
      break;

    case 'c':
      TREE_CONSTANT (t) = 1;
      break;

    case 'e':
      switch (code)
	{
	case INIT_EXPR:
	case MODIFY_EXPR:
	case VA_ARG_EXPR:
	case RTL_EXPR:
	case PREDECREMENT_EXPR:
	case PREINCREMENT_EXPR:
	case POSTDECREMENT_EXPR:
	case POSTINCREMENT_EXPR:
	  /* All of these have side-effects, no matter what their
	     operands are.  */
	  TREE_SIDE_EFFECTS (t) = 1;
	  break;

	default:
	  break;
	}
      break;
    }

  return t;
}

/* A front-end can reset this to an appropriate function if types need
   special handling.  */

tree (*make_lang_type_fn) PARAMS ((enum tree_code)) = make_node;

/* Return a new type (with the indicated CODE), doing whatever
   language-specific processing is required.  */

tree
make_lang_type (code)
     enum tree_code code;
{
  return (*make_lang_type_fn) (code);
}

/* Return a new node with the same contents as NODE except that its
   TREE_CHAIN is zero and it has a fresh uid.  */

tree
copy_node (node)
     tree node;
{
  tree t;
  enum tree_code code = TREE_CODE (node);
  size_t length;

  length = tree_size (node);
  t = ggc_alloc_tree (length);
  memcpy (t, node, length);

  TREE_CHAIN (t) = 0;
  TREE_ASM_WRITTEN (t) = 0;

  if (TREE_CODE_CLASS (code) == 'd')
    DECL_UID (t) = next_decl_uid++;
  else if (TREE_CODE_CLASS (code) == 't')
    {
      TYPE_UID (t) = next_type_uid++;
      /* The following is so that the debug code for
	 the copy is different from the original type.
	 The two statements usually duplicate each other
	 (because they clear fields of the same union),
	 but the optimizer should catch that.  */
      TYPE_SYMTAB_POINTER (t) = 0;
      TYPE_SYMTAB_ADDRESS (t) = 0;
    }

  return t;
}

/* Return a copy of a chain of nodes, chained through the TREE_CHAIN field.
   For example, this can copy a list made of TREE_LIST nodes.  */

tree
copy_list (list)
     tree list;
{
  tree head;
  tree prev, next;

  if (list == 0)
    return 0;

  head = prev = copy_node (list);
  next = TREE_CHAIN (list);
  while (next)
    {
      TREE_CHAIN (prev) = copy_node (next);
      prev = TREE_CHAIN (prev);
      next = TREE_CHAIN (next);
    }
  return head;
}


/* Return a newly constructed INTEGER_CST node whose constant value
   is specified by the two ints LOW and HI.
   The TREE_TYPE is set to `int'.

   This function should be used via the `build_int_2' macro.  */

tree
build_int_2_wide (low, hi)
     unsigned HOST_WIDE_INT low;
     HOST_WIDE_INT hi;
{
  tree t = make_node (INTEGER_CST);

  TREE_INT_CST_LOW (t) = low;
  TREE_INT_CST_HIGH (t) = hi;
  TREE_TYPE (t) = integer_type_node;
  return t;
}

/* Return a new VECTOR_CST node whose type is TYPE and whose values
   are in a list pointed by VALS.  */

tree
build_vector (type, vals)
     tree type, vals;
{
  tree v = make_node (VECTOR_CST);
  int over1 = 0, over2 = 0;
  tree link;

  TREE_VECTOR_CST_ELTS (v) = vals;
  TREE_TYPE (v) = type;

  /* Iterate through elements and check for overflow.  */
  for (link = vals; link; link = TREE_CHAIN (link))
    {
      tree value = TREE_VALUE (link