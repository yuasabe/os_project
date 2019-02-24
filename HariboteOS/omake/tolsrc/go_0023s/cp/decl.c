/* Process declarations and variables for C compiler.
   Copyright (C) 1988, 1992, 1993, 1994, 1995, 1996, 1997, 1998, 1999, 2000,
   2001, 2002  Free Software Foundation, Inc.
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


/* Process declarations and symbol lookup for C front end.
   Also constructs types; the standard scalar types at initialization,
   and structure, union, array and enum types when they are declared.  */

/* ??? not all decl nodes are given the most useful possible
   line numbers.  For example, the CONST_DECLs for enum values.  */

/* !kawai! */
#include "../gcc/config.h"
#include "../gcc/system.h"
#include "../gcc/tree.h"
#include "../gcc/rtl.h"
#include "../gcc/expr.h"
#include "../gcc/flags.h"
#include "cp-tree.h"
#include "../gcc/tree-inline.h"
#include "decl.h"
#include "lex.h"
#include "../gcc/output.h"
#include "../gcc/except.h"
#include "../gcc/toplev.h"
#include "../gcc/hash.h"
#include "../gcc/ggc.h"
#include "../gcc/tm_p.h"
#include "../gcc/target.h"
#include "../gcc/c-common.h"
#include "../gcc/c-pragma.h"
#include "../gcc/diagnostic.h"
/* end of !kawai! */

extern const struct attribute_spec *lang_attribute_table;

static tree grokparms				PARAMS ((tree));
static const char *redeclaration_error_message	PARAMS ((tree, tree));

static void push_binding_level PARAMS ((struct binding_level *, int,
				      int));
static void pop_binding_level PARAMS ((void));
static void suspend_binding_level PARAMS ((void));
static void resume_binding_level PARAMS ((struct binding_level *));
static struct binding_level *make_binding_level PARAMS ((void));
static void declare_namespace_level PARAMS ((void));
static int decl_jump_unsafe PARAMS ((tree));
static void storedecls PARAMS ((tree));
static void require_complete_types_for_parms PARAMS ((tree));
static int ambi_op_p PARAMS ((enum tree_code));
static int unary_op_p PARAMS ((enum tree_code));
static tree store_bindings PARAMS ((tree, tree));
static tree lookup_tag_reverse PARAMS ((tree, tree));
static tree obscure_complex_init PARAMS ((tree, tree));
static tree lookup_name_real PARAMS ((tree, int, int, int));
static void push_local_name PARAMS ((tree));
static void warn_extern_redeclared_static PARAMS ((tree, tree));
static tree grok_reference_init PARAMS ((tree, tree, tree));
static tree grokfndecl PARAMS ((tree, tree, tree, tree, int,
			      enum overload_flags, tree,
			      tree, int, int, int, int, int, int, tree));
static tree grokvardecl PARAMS ((tree, tree, RID_BIT_TYPE *, int, int, tree));
static tree follow_tag_typedef PARAMS ((tree));
static tree lookup_tag PARAMS ((enum tree_code, tree,
			      struct binding_level *, int));
static void set_identifier_type_value_with_scope
	PARAMS ((tree, tree, struct binding_level *));
static void record_unknown_type PARAMS ((tree, const char *));
static tree builtin_function_1 PARAMS ((const char *, tree, tree, int,
                                      enum built_in_class, const char *));
static tree build_library_fn_1 PARAMS ((tree, enum tree_code, tree));
static int member_function_or_else PARAMS ((tree, tree, enum overload_flags));
static void bad_specifiers PARAMS ((tree, const char *, int, int, int, int,
				  int));
static tree maybe_process_template_type_declaration PARAMS ((tree, int, struct binding_level*));
static void check_for_uninitialized_const_var PARAMS ((tree));
static unsigned long typename_hash PARAMS ((hash_table_key));
static bool typename_compare PARAMS ((hash_table_key, hash_table_key));
static void push_binding PARAMS ((tree, tree, struct binding_level*));
static int add_binding PARAMS ((tree, tree));
static void pop_binding PARAMS ((tree, tree));
static tree local_variable_p_walkfn PARAMS ((tree *, int *, void *));
static tree find_binding PARAMS ((tree, tree));
static tree select_decl PARAMS ((tree, int));
static int lookup_flags PARAMS ((int, int));
static tree qualify_lookup PARAMS ((tree, int));
static tree record_builtin_java_type PARAMS ((const char *, int));
static const char *tag_name PARAMS ((enum tag_types code));
static void find_class_binding_level PARAMS ((void));
static struct binding_level *innermost_nonclass_level PARAMS ((void));
static void warn_about_implicit_typename_lookup PARAMS ((tree, tree));
static int walk_namespaces_r PARAMS ((tree, walk_namespaces_fn, void *));
static int walk_globals_r PARAMS ((tree, void *));
static void add_decl_to_level PARAMS ((tree, struct binding_level *));
static tree make_label_decl PARAMS ((tree, int));
static void use_label PARAMS ((tree));
static void check_previous_goto_1 PARAMS ((tree, struct binding_level *, tree,
					   const char *, int));
static void check_previous_goto PARAMS ((struct named_label_use_list *));
static void check_switch_goto PARAMS ((struct binding_level *));
static void check_previous_gotos PARAMS ((tree));
static void pop_label PARAMS ((tree, tree));
static void pop_labels PARAMS ((tree));
static void maybe_deduce_size_from_array_init PARAMS ((tree, tree));
static void layout_var_decl PARAMS ((tree));
static void maybe_commonize_var PARAMS ((tree));
static tree check_initializer PARAMS ((tree, tree));
static void make_rtl_for_nonlocal_decl PARAMS ((tree, tree, const char *));
static void push_cp_function_context PARAMS ((struct function *));
static void pop_cp_function_context PARAMS ((struct function *));
static void mark_binding_level PARAMS ((void *));
static void mark_named_label_lists PARAMS ((void *, void *));
static void mark_cp_function_context PARAMS ((struct function *));
static void mark_saved_scope PARAMS ((void *));
static void mark_lang_function PARAMS ((struct cp_language_function *));
static void save_function_data PARAMS ((tree));
static void check_function_type PARAMS ((tree, tree));
static void destroy_local_var PARAMS ((tree));
static void begin_constructor_body PARAMS ((void));
static void finish_constructor_body PARAMS ((void));
static void begin_destructor_body PARAMS ((void));
static void finish_destructor_body PARAMS ((void));
static tree create_array_type_for_decl PARAMS ((tree, tree, tree));
static tree get_atexit_node PARAMS ((void));
static tree get_dso_handle_node PARAMS ((void));
static tree start_cleanup_fn PARAMS ((void));
static void end_cleanup_fn PARAMS ((void));
static tree cp_make_fname_decl PARAMS ((tree, int));
static void initialize_predefined_identifiers PARAMS ((void));
static tree check_special_function_return_type
  PARAMS ((special_function_kind, tree, tree));
static tree push_cp_library_fn PARAMS ((enum tree_code, tree));
static tree build_cp_library_fn PARAMS ((tree, enum tree_code, tree));
static void store_parm_decls PARAMS ((tree));
static int cp_missing_noreturn_ok_p PARAMS ((tree));

#if defined (DEBUG_CP_BINDING_LEVELS)
static void indent PARAMS ((void));
#endif

/* Erroneous argument lists can use this *IFF* they do not modify it.  */
tree error_mark_list;

/* The following symbols are subsumed in the cp_global_trees array, and
   listed here individually for documentation purposes.

   C++ extensions
	tree wchar_decl_node;

	tree vtable_entry_type;
	tree delta_type_node;
	tree __t_desc_type_node;
        tree ti_desc_type_node;
	tree bltn_desc_type_node, ptr_desc_type_node;
	tree ary_desc_type_node, func_desc_type_node, enum_desc_type_node;
	tree class_desc_type_node, si_class_desc_type_node, vmi_class_desc_type_node;
	tree ptm_desc_type_node;
	tree base_desc_type_node;

	tree class_type_node, record_type_node, union_type_node, enum_type_node;
	tree unknown_type_node;

   Array type `vtable_entry_type[]'

	tree vtbl_type_node;
	tree vtbl_ptr_type_node;

   Namespaces,

	tree std_node;
	tree abi_node;

   A FUNCTION_DECL which can call `abort'.  Not necessarily the
   one that the user will declare, but sufficient to be called
   by routines that want to abort the program.

	tree abort_fndecl;

   The FUNCTION_DECL for the default `::operator delete'.

	tree global_delete_fndecl;

   Used by RTTI
	tree type_info_type_node, tinfo_decl_id, tinfo_decl_type;
	tree tinfo_var_id;

*/

tree cp_global_trees[CPTI_MAX];

/* Indicates that there is a type value in some namespace, although
   that is not necessarily in scope at the moment.  */

static tree global_type_node;

/* Expect only namespace names now. */
static int only_namespace_names;

/* Used only for jumps to as-yet undefined labels, since jumps to
   defined labels can have their validity checked immediately.  */

struct named_label_use_list
{
  struct binding_level *binding_level;
  tree names_in_scope;
  tree label_decl;
  const char *filename_o_goto;
  int lineno_o_goto;
  struct named_label_use_list *next;
};

#define named_label_uses cp_function_chain->x_named_label_uses

#define local_names cp_function_chain->x_local_names

/* A list of objects which have constructors or destructors
   which reside in the global scope.  The decl is stored in
   the TREE_VALUE slot and the initializer is stored
   in the TREE_PURPOSE slot.  */
tree static_aggregates;

/* -- end of C++ */

/* A node for the integer constants 2, and 3.  */

tree integer_two_node, integer_three_node;

/* Similar, for last_function_parm_tags.  */
tree last_function_parms;

/* A list of all LABEL_DECLs in the function that have names.  Here so
   we can clear out their names' definitions at the end of the
   function, and so we can check the validity of jumps to these labels.  */

struct named_label_list
{
  struct binding_level *binding_level;
  tree names_in_scope;
  tree old_value;
  tree label_decl;
  tree bad_decls;
  struct named_label_list *next;
  unsigned int in_try_scope : 1;
  unsigned int in_catch_scope : 1;
};

#define named_labels cp_function_chain->x_named_labels

/* Nonzero means use the ISO C94 dialect of C.  */

int flag_isoc94;

/* Nonzero means use the ISO C99 dialect of C.  */

int flag_isoc99;

/* Nonzero means we are a hosted implementation for code shared with C.  */

int flag_hosted = 1;

/* Nonzero means add default format_arg attributes for functions not
   in ISO C.  */

int flag_noniso_default_format_attributes = 1;

/* Nonzero if we want to conserve space in the .o files.  We do this
   by putting uninitialized data and runtime initialized data into
   .common instead of .data at the expense of not flagging multiple
   definitions.  */
extern int flag_conserve_space;

/* C and C++ flags are in decl2.c.  */

/* A expression of value 0 with the same precision as a sizetype
   node, but signed.  */
tree signed_size_zero_node;

/* The name of the anonymous namespace, throughout this translation
   unit.  */
tree anonymous_namespace_name;

/* The number of function bodies which we are currently processing.
   (Zero if we are at namespace scope, one inside the body of a
   function, two inside the body of a function in a local class, etc.)  */
int function_depth;

/* States indicating how grokdeclarator() should handle declspecs marked
   with __attribute__((deprecated)).  An object declared as
   __attribute__((deprecated)) suppresses warnings of uses of other
   deprecated items.  */
   
enum deprecated_states {
  DEPRECATED_NORMAL,
  DEPRECATED_SUPPRESS
};

static enum deprecated_states deprecated_state = DEPRECATED_NORMAL;

/* Set by add_implicitly_declared_members() to keep those members from
   being flagged as deprecated or reported as using deprecated
   types.  */
int adding_implicit_members = 0;

/* For each binding contour we allocate a binding_level structure
   which records the names defined in that contour.
   Contours include:
    0) the global one
    1) one for each function definition,
       where internal declarations of the parameters appear.
    2) one for each compound statement,
       to record its declarations.

   The current meaning of a name can be found by searching the levels
   from the current one out to the global one.

   Off to the side, may be the class_binding_level.  This exists only
   to catch class-local declarations.  It is otherwise nonexistent.

   Also there may be binding levels that catch cleanups that must be
   run when exceptions occur.  Thus, to see whether a name is bound in
   the current scope, it is not enough to look in the
   CURRENT_BINDING_LEVEL.  You should use lookup_name_current_level
   instead.  */

/* Note that the information in the `names' component of the global contour
   is duplicated in the IDENTIFIER_GLOBAL_VALUEs of all identifiers.  */

struct binding_level
  {
    /* A chain of _DECL nodes for all variables, constants, functions,
       and typedef types.  These are in the reverse of the order
       supplied.  There may be OVERLOADs on this list, too, but they
       are wrapped in TREE_LISTs; the TREE_VALUE is the OVERLOAD.  */
    tree names;

    /* A list of structure, union and enum definitions, for looking up
       tag names.
       It is a chain of TREE_LIST nodes, each of whose TREE_PURPOSE is a name,
       or NULL_TREE; and whose TREE_VALUE is a RECORD_TYPE, UNION_TYPE,
       or ENUMERAL_TYPE node.

       C++: the TREE_VALUE nodes can be simple types for
       component_bindings.  */
    tree tags;

    /* A list of USING_DECL nodes. */
    tree usings;

    /* A list of used namespaces. PURPOSE is the namespace,
       VALUE the common ancestor with this binding_level's namespace. */
    tree using_directives;

    /* If this binding level is the binding level for a class, then
       class_shadowed is a TREE_LIST.  The TREE_PURPOSE of each node
       is the name of an entity bound in the class.  The TREE_TYPE is
       the DECL bound by this name in the class.  */
    tree class_shadowed;

    /* Similar to class_shadowed, but for IDENTIFIER_TYPE_VALUE, and
       is used for all binding levels. In addition the TREE_VALUE is the
       IDENTIFIER_TYPE_VALUE before we entered the class.  */
    tree type_shadowed;

    /* A TREE_LIST.  Each TREE_VALUE is the LABEL_DECL for a local
       label in this scope.  The TREE_PURPOSE is the previous value of
       the IDENTIFIER_LABEL VALUE.  */
    tree shadowed_labels;

    /* For each level (except not the global one),
       a chain of BLOCK nodes for all the levels
       that were entered and exited one level down.  */
    tree blocks;

    /* The _TYPE node for this level, if parm_flag == 2.  */
    tree this_class;

    /* The binding level which this one is contained in (inherits from).  */
    struct binding_level *level_chain;

    /* List of VAR_DECLS saved from a previous for statement.
       These would be dead in ISO-conforming code, but might
       be referenced in ARM-era code.  These are stored in a
       TREE_LIST; the TREE_VALUE is the actual declaration.  */
    tree dead_vars_from_for;

    /* 1 for the level that holds the parameters of a function.
       2 for the level that holds a class declaration.  */
    unsigned parm_flag : 2;

    /* 1 means make a BLOCK for this level regardless of all else.
       2 for temporary binding contours created by the compiler.  */
    unsigned keep : 2;

    /* Nonzero if this level "doesn't exist" for tags.  */
    unsigned tag_transparent : 1;

    /* Nonzero if this level can safely have additional
       cleanup-needing variables added to it.  */
    unsigned more_cleanups_ok : 1;
    unsigned have_cleanups : 1;

    /* Nonzero if this scop