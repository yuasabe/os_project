/* Demangler for IA64 / g++ V3 ABI.
   Copyright (C) 2000, 2001, 2002 Free Software Foundation, Inc.
   Written by Alex Samuel <samuel@codesourcery.com>. 

   This file is part of GNU CC.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   In addition to the permissions in the GNU General Public License, the
   Free Software Foundation gives you unlimited permission to link the
   compiled version of this file into combinations with other programs,
   and to distribute those combinations without any restriction coming
   from the use of this file.  (The General Public License restrictions
   do apply in other respects; for example, they cover modification of
   the file, and distribution when not linked into a combined
   executable.)

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. 
*/

/* This file implements demangling of C++ names mangled according to
   the IA64 / g++ V3 ABI.  Use the cp_demangle function to
   demangle a mangled name, or compile with the preprocessor macro
   STANDALONE_DEMANGLER defined to create a demangling filter
   executable (functionally similar to c++filt, but includes this
   demangler only).  */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif


#ifdef HAVE_STDLIB_H
#include "../include/stdlib.h"
#endif

#include "../include/stdio.h"

#ifdef HAVE_STRING_H
#include "../include/string.h"
#endif

/* !kawai! */
#include "../include/ansidecl.h"
#include "../include/libiberty.h"
#include "../include/dyn-string.h"
#include "../include/demangle.h"
/* end of !kawai! */

/* If CP_DEMANGLE_DEBUG is defined, a trace of the grammar evaluation,
   and other debugging output, will be generated. */
#ifdef CP_DEMANGLE_DEBUG
#define DEMANGLE_TRACE(PRODUCTION, DM)                                  ¥
  fprintf (stderr, " -> %-24s at position %3d¥n",                       ¥
           (PRODUCTION), current_position (DM));
#else
#define DEMANGLE_TRACE(PRODUCTION, DM)
#endif

/* Don't include <ctype.h>, to prevent additional unresolved symbols
   from being dragged into the C++ runtime library.  */
#define IS_DIGIT(CHAR) ((CHAR) >= '0' && (CHAR) <= '9')
#define IS_ALPHA(CHAR)                                                  ¥
  (((CHAR) >= 'a' && (CHAR) <= 'z')                                     ¥
   || ((CHAR) >= 'A' && (CHAR) <= 'Z'))

/* The prefix prepended by GCC to an identifier represnting the
   anonymous namespace.  */
#define ANONYMOUS_NAMESPACE_PREFIX "_GLOBAL_"

/* Character(s) to use for namespace separation in demangled output */
#define NAMESPACE_SEPARATOR (dm->style == DMGL_JAVA ? "." : "::")

/* If flag_verbose is zero, some simplifications will be made to the
   output to make it easier to read and supress details that are
   generally not of interest to the average C++ programmer.
   Otherwise, the demangled representation will attempt to convey as
   much information as the mangled form.  */
static int flag_verbose;

/* If flag_strict is non-zero, demangle strictly according to the
   specification -- don't demangle special g++ manglings.  */
static int flag_strict;

/* String_list_t is an extended form of dyn_string_t which provides a
   link field and a caret position for additions to the string.  A
   string_list_t may safely be cast to and used as a dyn_string_t.  */

struct string_list_def
{
  /* The dyn_string; must be first.  */
  struct dyn_string string;

  /* The position at which additional text is added to this string
     (using the result_add* macros).  This value is an offset from the
     end of the string, not the beginning (and should be
     non-positive).  */
  int caret_position;

  /* The next string in the list.  */
  struct string_list_def *next;
};

typedef struct string_list_def *string_list_t;

/* Data structure representing a potential substitution.  */

struct substitution_def
{
  /* The demangled text of the substitution.  */
  dyn_string_t text;

  /* Whether this substitution represents a template item.  */
  int template_p : 1;
};

/* Data structure representing a template argument list.  */

struct template_arg_list_def
{
  /* The next (lower) template argument list in the stack of currently
     active template arguments.  */
  struct template_arg_list_def *next;

  /* The first element in the list of template arguments in
     left-to-right order.  */
  string_list_t first_argument;

  /* The last element in the arguments lists.  */
  string_list_t last_argument;
};

typedef struct template_arg_list_def *template_arg_list_t;

/* Data structure to maintain the state of the current demangling.  */

struct demangling_def
{
  /* The full mangled name being mangled.  */
  const char *name;

  /* Pointer into name at the current position.  */
  const char *next;

  /* Stack for strings containing demangled result generated so far.
     Text is emitted to the topmost (first) string.  */
  string_list_t result;

  /* The number of presently available substitutions.  */
  int num_substitutions;

  /* The allocated size of the substitutions array.  */
  int substitutions_allocated;

  /* An array of available substitutions.  The number of elements in
     the array is given by num_substitions, and the allocated array
     size in substitutions_size.  

     The most recent substition is at the end, so

       - `S_'  corresponds to substititutions[num_substitutions - 1] 
       - `S0_' corresponds to substititutions[num_substitutions - 2]

     etc. */
  struct substitution_def *substitutions;

  /* The stack of template argument lists.  */
  template_arg_list_t template_arg_lists;

  /* The most recently demangled source-name.  */
  dyn_string_t last_source_name;
  
  /* Language style to use for demangled output. */
  int style;

  /* Set to non-zero iff this name is a constructor.  The actual value
     indicates what sort of constructor this is; see demangle.h.  */
  enum gnu_v3_ctor_kinds is_constructor;

  /* Set to non-zero iff this name is a destructor.  The actual value
     indicates what sort of destructor this is; see demangle.h.  */
  enum gnu_v3_dtor_kinds is_destructor;

};

typedef struct demangling_def *demangling_t;

/* This type is the standard return code from most functions.  Values
   other than STATUS_OK contain descriptive messages.  */
typedef const char *status_t;

/* Special values that can be used as a status_t.  */
#define STATUS_OK                       NULL
#define STATUS_ERROR                    "Error."
#define STATUS_UNIMPLEMENTED            "Unimplemented."
#define STATUS_INTERNAL_ERROR           "Internal error."

/* This status code indicates a failure in malloc or realloc.  */
static const char *const status_allocation_failed = "Allocation failed.";
#define STATUS_ALLOCATION_FAILED        status_allocation_failed

/* Non-zero if STATUS indicates that no error has occurred.  */
#define STATUS_NO_ERROR(STATUS)         ((STATUS) == STATUS_OK)

/* Evaluate EXPR, which must produce a status_t.  If the status code
   indicates an error, return from the current function with that
   status code.  */
#define RETURN_IF_ERROR(EXPR)                                           ¥
  do                                                                    ¥
    {                                                                   ¥
      status_t s = EXPR;                                                ¥
      if (!STATUS_NO_ERROR (s))                                         ¥
	return s;                                                       ¥
    }                                                                   ¥
  while (0)

static status_t int_to_dyn_string 
  PARAMS ((int, dyn_string_t));
static string_list_t string_list_new
  PARAMS ((int));
static void string_list_delete
  PARAMS ((string_list_t));
static status_t result_add_separated_char
  PARAMS ((demangling_t, int));
static status_t result_push
  PARAMS ((demangling_t));
static string_list_t result_pop
  PARAMS ((demangling_t));
static int substitution_start
  PARAMS ((demangling_t));
static status_t substitution_add
  PARAMS ((demangling_t, int, int));
static dyn_string_t substitution_get
  PARAMS ((demangling_t, int, int *));
#ifdef CP_DEMANGLE_DEBUG
static void substitutions_print 
  PARAMS ((demangling_t, FILE *));
#endif
static template_arg_list_t template_arg_list_new
  PARAMS ((void));
static void template_arg_list_delete
  PARAMS ((template_arg_list_t));
static void template_arg_list_add_arg 
  PARAMS ((template_arg_list_t, string_list_t));
static string_list_t template_arg_list_get_arg
  PARAMS ((template_arg_list_t, int));
static void push_template_arg_list
  PARAMS ((demangling_t, template_arg_list_t));
static void pop_to_template_arg_list
  PARAMS ((demangling_t, template_arg_list_t));
#ifdef CP_DEMANGLE_DEBUG
static void template_arg_list_print
  PARAMS ((template_arg_list_t, FILE *));
#endif
static template_arg_list_t current_template_arg_list
  PARAMS ((demangling_t));
static demangling_t demangling_new
  PARAMS ((const char *, int));
static void demangling_delete 
  PARAMS ((demangling_t));

/* The last character of DS.  Warning: DS is evaluated twice.  */
#define dyn_string_last_char(DS)                                        ¥
  (dyn_string_buf (DS)[dyn_string_length (DS) - 1])

/* Append a space character (` ') to DS if it does not already end
   with one.  Evaluates to 1 on success, or 0 on allocation failure.  */
#define dyn_string_append_space(DS)                                     ¥
      ((dyn_string_length (DS) > 0                                      ¥
        && dyn_string_last_char (DS) != ' ')                            ¥
       ? dyn_string_append_char ((DS), ' ')                             ¥
       : 1)

/* Returns the index of the current position in the mangled name.  */
#define current_position(DM)    ((DM)->next - (DM)->name)

/* Returns the character at the current position of the mangled name.  */
#define peek_char(DM)           (*((DM)->next))

/* Returns the character one past the current position of the mangled
   name.  */
#define peek_char_next(DM)                                              ¥
  (peek_char (DM) == '¥0' ? '¥0' : (*((DM)->next + 1)))

/* Returns the character at the current position, and advances the
   current position to the next character.  */
#define next_char(DM)           (*((DM)->next)++)

/* Returns non-zero if the current position is the end of the mangled
   name, i.e. one past the last character.  */
#define end_of_name_p(DM)       (peek_char (DM) == '¥0')

/* Advances the current position by one character.  */
#define advance_char(DM)        (++(DM)->next)

/* Returns the string containing the current demangled result.  */
#define result_string(DM)       (&(DM)->result->string)

/* Returns the position at which new text is inserted into the
   demangled result.  */
#define result_caret_pos(DM)                                            ¥
  (result_length (DM) +                                                 ¥
   ((string_list_t) result_string (DM))->caret_position)

/* Adds a dyn_string_t to the demangled result.  */
#define result_add_string(DM, STRING)                                   ¥
  (dyn_string_insert (&(DM)->result->string,                            ¥
		      result_caret_pos (DM), (STRING))                  ¥
   ? STATUS_OK : STATUS_ALLOCATION_FAILED)

/* Adds NUL-terminated string CSTR to the demangled result.    */
#define result_add(DM, CSTR)                                            ¥
  (dyn_string_insert_cstr (&(DM)->result->string,                       ¥
			   result_caret_pos (DM), (CSTR))               ¥
   ? STATUS_OK : STATUS_ALLOCATION_FAILED)

/* Adds character CHAR to the demangled result.  */
#define result_add_char(DM, CHAR)                                       ¥
  (dyn_string_insert_char (&(DM)->result->string,                       ¥
			   result_caret_pos (DM), (CHAR))               ¥
   ? STATUS_OK : STATUS_ALLOCATION_FAILED)

/* Inserts a dyn_string_t to the demangled result at position POS.  */
#define result_insert_string(DM, POS, STRING)                           ¥
  (dyn_string_insert (&(DM)->result->string, (POS), (STRING))           ¥
   ? STATUS_OK : STATUS_ALLOCATION_FAILED)

/* Inserts NUL-terminated string CSTR to the demangled result at
   position POS.  */
#define result_insert(DM, POS, CSTR)                                    ¥
  (dyn_string_insert_cstr (&(DM)->result->string, (POS), (CSTR))        ¥
   ? STATUS_OK : STATUS_ALLOCATION_FAILED)

/* Inserts character CHAR to the demangled result at position POS.  */
#define result_insert_char(DM, POS, CHAR)                               ¥
  (dyn_string_insert_char (&(DM)->result->string, (POS), (CHAR))        ¥
   ? STATUS_OK : STATUS_ALLOCATION_FAILED)

/* The length of the current demangled result.  */
#define result_length(DM)                                               ¥
  dyn_string_length (&(DM)->result->string)

/* Appends a (less-than, greater-than) character to the result in DM
   to (open, close) a template argument or parameter list.  Appends a
   space first if necessary to prevent spurious elision of angle
   brackets with the previous character.  */
#define result_open_template_list(DM) result_add_separated_char(DM, '<')
#define result_close_template_list(DM) result_add_separated_char(DM, '>')

/* Appends a base 10 representation of VALUE to DS.  STATUS_OK on
   success.  On failure, deletes DS and returns an error code.  */

static status_t
int_to_dyn_string (value, ds)
     int value;
     dyn_string_t ds;
{
  int i;
  int mask = 1;

  /* Handle zero up front.  */
  if (value == 0)
    {
      if (!dyn_string_append_char (ds, '0'))
	return STATUS_ALLOCATION_FAILED;
      return STATUS_OK;
    }

  /* For negative numbers, emit a minus sign.  */
  if (value < 0)
    {
      if (!dyn_string_append_char (ds, '-'))
	return STATUS_ALLOCATION_FAILED;
      value = -value;
    }
  
  /* Find the power of 10 of the first digit.  */
  i = value;
  while (i > 9)
    {
      mask *= 10;
      i /= 10;
    }

  /* Write the digits.  */
  while (mask > 0)
    {
      int digit = value / mask;

      if (!dyn_string_append_char (ds, '0' + digit))
	return STATUS_ALLOCATION_FAILED;

      value -= digit * mask;
      mask /= 10;
    }

  return STATUS_OK;
}

/* Creates a new string list node.  The contents of the string are
   empty, but the initial buffer allocation is LENGTH.  The string
   list node should be deleted with string_list_delete.  Returns NULL
   if allocation fails.  */

static string_list_t 
string_list_new (length)
     int length;
{
  string_list_t s = (string_list_t) malloc (sizeof (struct string_list_def));
  s->caret_position = 0;
  if (s == NULL)
    return NULL;
  if (!dyn_string_init ((dyn_string_t) s, length))
    return NULL;
  return s;
}  

/* Deletes the entire string list starting at NODE.  */

static void
string_list_delete (node)
     string_list_t node;
{
  while (node != NULL)
    {
      string_list_t next = node->next;
      dyn_string_delete ((dyn_string_t) node);
      node = next;
    }
}

/* Appends CHARACTER to the demangled result.  If the current trailing
   character of the result is CHARACTER, a space is inserted first.  */

static status_t
result_add_separated_char (dm, character)
     demangling_t dm;
     int character;
{
  char *result = dyn_string_buf (result_string (dm));
  