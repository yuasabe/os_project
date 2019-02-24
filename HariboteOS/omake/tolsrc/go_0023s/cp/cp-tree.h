/* Definitions for C++ parsing and type checking.
   Copyright (C) 1987, 1992, 1993, 1994, 1995, 1996, 1997, 1998, 1999,
   2000, 2001, 2002 Free Software Foundation, Inc.
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
#include "../gcc/function.h"
#include "../include/hashtab.h"
#include "../include/splay-tree.h"
#include "../gcc/varray.h"

#ifndef GCC_CP_TREE_H
#define GCC_CP_TREE_H


#include "../gcc/c-common.h"
/* end of !kawai! */

/* Usage of TREE_LANG_FLAG_?:
   0: BINFO_MARKED (BINFO nodes).
      IDENTIFIER_MARKED (IDENTIFIER_NODEs)
      NEW_EXPR_USE_GLOBAL (in NEW_EXPR).
      DELETE_EXPR_USE_GLOBAL (in DELETE_EXPR).
      LOOKUP_EXPR_GLOBAL (in LOOKUP_EXPR).
      TREE_INDIRECT_USING (in NAMESPACE_DECL).
      LOCAL_BINDING_P (in CPLUS_BINDING)
      ICS_USER_FLAG (in _CONV)
      CLEANUP_P (in TRY_BLOCK)
      AGGR_INIT_VIA_CTOR_P (in AGGR_INIT_EXPR)
      CTOR_BEGIN_P (in CTOR_STMT)
      BV_USE_VCALL_INDEX_P (in the BINFO_VIRTUALS TREE_LIST)
      PTRMEM_OK_P (in ADDR_EXPR, OFFSET_REF)
      PARMLIST_ELLIPSIS_P (in PARMLIST)
   1: IDENTIFIER_VIRTUAL_P.
      TI_PENDING_TEMPLATE_FLAG.
      TEMPLATE_PARMS_FOR_INLINE.
      DELETE_EXPR_USE_VEC (in DELETE_EXPR).
      (TREE_CALLS_NEW) (in _EXPR or _REF) (commented-out).
      TYPE_BASE_CONVS_MAY_REQUIRE_CODE_P (in _TYPE).
      INHERITED_VALUE_BINDING_P (in CPLUS_BINDING)
      BASELINK_P (in TREE_LIST)
      ICS_ELLIPSIS_FLAG (in _CONV)
      BINFO_ACCESS (in BINFO)
   2: IDENTIFIER_OPNAME_P.
      TYPE_POLYMORHPIC_P (in _TYPE)
      ICS_THIS_FLAG (in _CONV)
      BINDING_HAS_LEVEL_P (in CPLUS_BINDING)
      BINFO_LOST_PRIMARY_P (in BINFO)
      TREE_PARMLIST (in TREE_LIST)
   3: TYPE_USES_VIRTUAL_BASECLASSES (in a class TYPE).
      BINFO_VTABLE_PATH_MARKED.
      BINFO_PUSHDECLS_MARKED.
      (TREE_REFERENCE_EXPR) (in NON_LVALUE_EXPR) (commented-out).
      ICS_BAD_FLAG (in _CONV)
      FN_TRY_BLOCK_P (in TRY_BLOCK)
      IDENTIFIER_CTOR_OR_DTOR_P (in IDENTIFIER_NODE)
   4: BINFO_NEW_VTABLE_MARKED.
      TREE_HAS_CONSTRUCTOR (in INDIRECT_REF, SAVE_EXPR, CONSTRUCTOR,
          or FIELD_DECL).
      NEED_TEMPORARY_P (in REF_BIND, BASE_CONV)
      IDENTIFIER_TYPENAME_P (in IDENTIFIER_NODE)
   5: C_IS_RESERVED_WORD (in IDENTIFIER_NODE)
   6: BINFO_ACCESS (in BINFO)

   Usage of TYPE_LANG_FLAG_?:
   0: C_TYPE_FIELDS_READONLY (in RECORD_TYPE or UNION_TYPE).
   1: TYPE_HAS_CONSTRUCTOR.
   2: TYPE_HAS_DESTRUCTOR.
   3: TYPE_FOR_JAVA.
   4: TYPE_HAS_NONTRIVIAL_DESTRUCTOR
   5: IS_AGGR_TYPE.
   6: TYPE_BUILT_IN.

   Usage of DECL_LANG_FLAG_?:
   0: DECL_ERROR_REPORTED (in VAR_DECL).
      DECL_TEMPLATE_PARM_P (in PARM_DECL, CONST_DECL, TYPE_DECL, or TEMPLATE_DECL)
      DECL_LOCAL_FUNCTION_P (in FUNCTION_DECL)
      DECL_MUTABLE_P (in FIELD_DECL)
   1: C_TYPEDEF_EXPLICITLY_SIGNED (in TYPE_DECL).
      DECL_TEMPLATE_INSTANTIATED (in a VAR_DECL or a FUNCTION_DECL)
      DECL_C_BITFIELD (in FIELD_DECL)
   2: DECL_THIS_EXTERN (in VAR_DECL or FUNCTION_DECL).
      DECL_IMPLICIT_TYPEDEF_P (in a TYPE_DECL)
   3: DECL_IN_AGGR_P.
   4: DECL_C_BIT_FIELD
   5: DECL_INTERFACE_KNOWN.
   6: DECL_THIS_STATIC (in VAR_DECL or FUNCTION_DECL).
   7: DECL_DEAD_FOR_LOCAL (in VAR_DECL).
      DECL_THUNK_P (in a member FUNCTION_DECL)

   Usage of language-independent fields in a language-dependent manner:

   TREE_USED
     This field is BINFO_INDIRECT_PRIMARY_P in a BINFO.

   TYPE_ALIAS_SET
     This field is used by TYPENAME_TYPEs, TEMPLATE_TYPE_PARMs, and so
     forth as a substitute for the mark bits provided in `lang_type'.
     At present, only the six low-order bits are used.

   TYPE_BINFO
     For an ENUMERAL_TYPE, this is ENUM_TEMPLATE_INFO.
     For a FUNCTION_TYPE or METHOD_TYPE, this is TYPE_RAISES_EXCEPTIONS

  BINFO_VIRTUALS
     For a binfo, this is a TREE_LIST.  The BV_DELTA of each node
     gives the amount by which to adjust the `this' pointer when
     calling the function.  If the method is an overriden version of a
     base class method, then it is assumed that, prior to adjustment,
     the this pointer points to an object of the base class.

     The BV_VCALL_INDEX of each node, if non-NULL, gives the vtable
     index of the vcall offset for this entry.  If
     BV_USE_VCALL_INDEX_P then the corresponding vtable entry should
     use a virtual thunk, as opposed to an ordinary thunk.

     The BV_FN is the declaration for the virtual function itself.

   BINFO_VTABLE
     This is an expression with POINTER_TYPE that gives the value
     to which the vptr should be initialized.  Use get_vtbl_decl_for_binfo
     to extract the VAR_DECL for the complete vtable.

   DECL_ARGUMENTS
     For a VAR_DECL this is DECL_ANON_UNION_ELEMS.

   DECL_VINDEX
     This field is NULL for a non-virtual function.  For a virtual
     function, it is eventually set to an INTEGER_CST indicating the
     index in the vtable at which this function can be found.  When
     a virtual function is declared, but before it is known what
     function is overriden, this field is the error_mark_node.

     Temporarily, it may be set to a TREE_LIST whose TREE_VALUE is
     the virtual function this one overrides, and whose TREE_CHAIN is
     the old DECL_VINDEX.  */

/* Language-specific tree checkers. */

#if defined ENABLE_TREE_CHECKING && (GCC_VERSION >= 2007)

#define VAR_OR_FUNCTION_DECL_CHECK(NODE)			¥
({  const tree __t = (NODE);					¥
    enum tree_code const __c = TREE_CODE(__t);			¥
    if (__c != VAR_DECL && __c != FUNCTION_DECL)		¥
      tree_check_failed (__t, VAR_DECL, __FILE__, __LINE__,	¥
			 __FUNCTION__);				¥
    __t; })

#define VAR_FUNCTION_OR_PARM_DECL_CHECK(NODE)			¥
({  const tree __t = (NODE);					¥
    enum tree_code const __c = TREE_CODE(__t);			¥
    if (__c != VAR_DECL						¥
	&& __c != FUNCTION_DECL					¥
        && __c != PARM_DECL)					¥
      tree_check_failed (__t, VAR_DECL, __FILE__, __LINE__,	¥
			 __FUNCTION__);				¥
    __t; })

#define VAR_TEMPL_TYPE_OR_FUNCTION_DECL_CHECK(NODE)		¥
({  const tree __t = (NODE);					¥
    enum tree_code const __c = TREE_CODE(__t);			¥
    if (__c != VAR_DECL						¥
	&& __c != FUNCTION_DECL					¥
	&& __c != TYPE_DECL					¥
	&& __c != TEMPLATE_DECL)				¥
      tree_check_failed (__t, VAR_DECL, __FILE__, __LINE__,	¥
			 __FUNCTION__);				¥
    __t; })

#define RECORD_OR_UNION_TYPE_CHECK(NODE)			¥
({  const tree __t = (NODE);					¥
    enum tree_code const __c = TREE_CODE(__t);			¥
    if (__c != RECORD_TYPE && __c != UNION_TYPE)		¥
      tree_check_failed (__t, RECORD_TYPE, __FILE__, __LINE__,	¥
			 __FUNCTION__);				¥
    __t; })

#define BOUND_TEMPLATE_TEMPLATE_PARM_TYPE_CHECK(NODE)		¥
({  const tree __t = (NODE);					¥
    enum tree_code const __c = TREE_CODE(__t);			¥
    if (__c != BOUND_TEMPLATE_TEMPLATE_PARM)			¥
      tree_check_failed (__t, BOUND_TEMPLATE_TEMPLATE_PARM,	¥
			 __FILE__, __LINE__, __FUNCTION__);	¥
    __t; })

#else /* not ENABLE_TREE_CHECKING, or not gcc */

#define VAR_OR_FUNCTION_DECL_CHECK(NODE)		(NODE)
#define VAR_FUNCTION_OR_PARM_DECL_CHECK(NODE)   	(NODE)
#define VAR_TEMPL_TYPE_OR_FUNCTION_DECL_CHECK(NODE)	(NODE)
#define RECORD_OR_UNION_TYPE_CHECK(NODE)		(NODE)
#define BOUND_TEMPLATE_TEMPLATE_PARM_TYPE_CHECK(NODE)	(NODE)

#endif


/* ABI control.  */

/* Nonzero to use __cxa_atexit, rather than atexit, to register
   destructors for local statics and global objects.  */

extern int flag_use_cxa_atexit;

/* Nonzero means generate 'rtti' that give run-time type information.  */

extern int flag_rtti;

/* Nonzero if we want to support huge (> 2^(sizeof(short)*8-1) bytes)
   objects.  */

extern int flag_huge_objects;


/* Language-dependent contents of an identifier.  */

struct lang_identifier
{
  struct c_common_identifier ignore;
  tree namespace_bindings;
  tree bindings;
  tree class_value;
  tree class_template_info;
  struct lang_id2 *x;
};

/* In an IDENTIFIER_NODE, nonzero if this identifier is actually a
   keyword.  C_RID_CODE (node) is then the RID_* value of the keyword,
   and C_RID_YYCODE is the token number wanted by Yacc.  */

#define C_IS_RESERVED_WORD(ID) TREE_LANG_FLAG_5 (ID)

extern const short rid_to_yy[RID_MAX];
#define C_RID_YYCODE(ID) rid_to_yy[C_RID_CODE (ID)]

#define LANG_IDENTIFIER_CAST(NODE) ¥
	((struct lang_identifier*)IDENTIFIER_NODE_CHECK (NODE))

struct lang_id2
{
  tree label_value, implicit_decl;
  tree error_locus;
};

typedef struct
{
  tree t;
  int new_type_flag;
  tree lookups;
} flagged_type_tree;

typedef struct
{
  struct tree_common common;
  HOST_WIDE_INT index;
  HOST_WIDE_INT level;
  HOST_WIDE_INT orig_level;
  tree decl;
} template_parm_index;

typedef struct ptrmem_cst
{
  struct tree_common common;
  /* This isn't used, but the middle-end expects all constants to have
     this field.  */
  rtx rtl;
  tree member;
}* ptrmem_cst_t;

/* Nonzero if this binding is for a local scope, as opposed to a class
   or namespace scope.  */
#define LOCAL_BINDING_P(NODE) TREE_LANG_FLAG_0 (NODE)

/* Nonzero if BINDING_VALUE is from a base class of the class which is
   currently being defined.  */
#define INHERITED_VALUE_BINDING_P(NODE) TREE_LANG_FLAG_1 (NODE)

/* For a binding between a name and an entity at a non-local scope,
   defines the scope where the binding is declared.  (Either a class
   _TYPE node, or a NAMESPACE_DECL.)  This macro should be used only
   for namespace-level bindings; on the IDENTIFIER_BINDING list
   BINDING_LEVEL is used instead.  */
#define BINDING_SCOPE(NODE) ¥
  (((struct tree_binding*)CPLUS_BINDING_CHECK (NODE))->scope.scope)

/* Nonzero if NODE has BINDING_LEVEL, rather than BINDING_SCOPE.  */
#define BINDING_HAS_LEVEL_P(NODE) TREE_LANG_FLAG_2 (NODE)

/* This is the declaration bound to the name. Possible values:
   variable, overloaded function, namespace, template, enumerator.  */
#define BINDING_VALUE(NODE) ¥
  (((struct tree_binding*)CPLUS_BINDING_CHECK (NODE))->value)

/* If name is bound to a type, this is the type (struct, union, enum).  */
#define BINDING_TYPE(NODE)     TREE_TYPE (NODE)

#define IDENTIFIER_GLOBAL_VALUE(NODE) ¥
  namespace_binding ((NODE), global_namespace)
#define SET_IDENTIFIER_GLOBAL_VALUE(NODE, VAL) ¥
  set_namespace_binding ((NODE), global_namespace, (VAL))
#define IDENTIFIER_NAMESPACE_VALUE(NODE) ¥
  namespace_binding ((NODE), current_namespace)
#define SET_IDENTIFIER_NAMESPACE_VALUE(NODE, VAL) ¥
  set_namespace_binding ((NODE), current_namespace, (VAL))

#define CLEANUP_P(NODE)         TREE_LANG_FLAG_0 (TRY_BLOCK_CHECK (NODE))

/* Returns nonzero iff TYPE1 and TYPE2 are the same type, in the usual
   sense of `same'.  */
#define same_type_p(TYPE1, TYPE2) ¥
  comptypes ((TYPE1), (TYPE2), COMPARE_STRICT)

/* Returns nonzero iff TYPE1 and TYPE2 are the same type, ignoring
   top-level qualifiers.  */
#define same_type_ignoring_top_level_qualifiers_p(TYPE1, TYPE2) ¥
  same_type_p (TYPE_MAIN_VARIANT (TYPE1), TYPE_MAIN_VARIANT (TYPE2))

/* Non-zero if we are presently building a statement tree, rather
   than expanding each statement as we encounter it.  */
#define building_stmt_tree() (last_tree != NULL_TREE)

/* Returns non-zero iff NODE is a declaration for the global function
   `main'.  */
#define DECL_MAIN_P(NODE)				¥
   (DECL_EXTERN_C_FUNCTION_P (NODE)                     ¥
    && DECL_NAME (NODE) != NULL_TREE			¥
    && MAIN_NAME_P (DECL_NAME (NODE)))


struct tree_binding
{
  struct tree_common common;
  union {
    tree scope;
    struct binding_level *level;
  } scope;
  tree value;
};

/* The overloaded FUNCTION_DECL. */
#define OVL_FUNCTION(NODE) ¥
  (((struct tree_overload*)OVERLOAD_CHECK (NODE))->function)
#define OVL_CHAIN(NODE)      TREE_CHAIN (NODE)
/* Polymorphic access to FUNCTION and CHAIN. */
#define OVL_CURRENT(NODE)     ¥
  ((TREE_CODE (NODE) == OVERLOAD) ? OVL_FUNCTION (NODE) : (NODE))
#define OVL_NEXT(NODE)        ¥
  ((TREE_CODE (NODE) == OVERLOAD) ? TREE_CHAIN (NODE) : NULL_TREE)
/* If set, this was imported in a using declaration.
   This is not to confuse with being used somewhere, which
   is not important for this node. */
#define OVL_USED(NODE)        TREE_USED (NODE)

struct tree_overload
{
  struct tree_common common;
  tree function;
};

/* A `baselink' is a TREE_LIST whose TREE_PURPOSE is a BINFO
   indicating a particular base class, and whose TREE_VALUE is a
   (possibly overloaded) function from that base class.  */
#define BASELINK_P(NODE) ¥
  (TREE_CODE (NODE) == TREE_LIST && TREE_LANG_FLAG_1 (NODE))
#define SET_BASELINK_P(NODE) ¥
  (TREE_LANG_FLAG_1 (NODE) = 1)

#define WRAPPER_PTR(NODE) (((struct tree_wrapper*)WRAPPER_CHECK (NODE))->u.ptr)
#define WRAPPER_INT(NODE) (((struct tree_wrapper*)WRAPPER_CHECK (NODE))->u.i)

struct tree_wrapper
{
  struct tree_common common;
  union {
    void *ptr;
    int i;
  } u;
};

#define SRCLOC_FILE(NODE) (((struct tree_srcloc*)SRCLOC_CHECK (NODE))->filename)
#define SRCLOC_LINE(NODE) (((struct tree_srcloc*)SRCLOC_CHECK (NODE))->linenum)
struct tree_srcloc
{
  struct tree_common common;
  const char *filename;
  int linenum;
};

/* Macros for access to language-specific slots in an identifier.  */

#define IDENTIFIER_NAMESPACE_BINDINGS(NODE)	¥
  (LANG_IDENTIFIER_CAST (NODE)->namespace_bindings)
#define IDENTIFIER_TEMPLATE(NODE)	¥
  (LANG_IDENTIFIER_CAST (NODE)->class_template_info)

/* The IDENTIFIER_BINDING is the innermost CPLUS_BINDING for the
    identifier.  It's TREE_CHAIN is the next outermost binding.  Each
    BINDING_VALUE is a DECL for the associated declaration.  Thus,
    name lookup consists simply of pulling off the node at the front
    of the list (modulo oddities for looking up the names of types,
    and such.)  You can use BINDING_SCOPE or BINDING_LEVEL to
    determine the scope that bound the name.  */
#define IDENTIFIER_BINDING(NODE) ¥
  (LANG_IDENTIFIER_CAST (NODE)->bindings)

/* The IDENTIFIER_VALUE is the value of the IDENTIFIER_BINDING, or
   NULL_TREE if there is no binding.  */
#define IDENTIFIER_VALUE(NODE)			¥
  (IDENTIFIER_BINDING (NODE)			¥
   ? BINDING_VALUE (IDENTIFIER_BINDING (NODE))	¥
   : NULL_TREE)

/* If IDENTIFIER_CLASS_VALUE is set, then NODE is bound in the current
   class, and IDENTIFIER_CLASS_VALUE is the value binding.  This is
   just a pointer to the BINDING_VALUE of one of the bindings in the
   IDENTIFIER_BINDINGs list, so any time that this is non-NULL so is
   IDENTIFIER_BINDING.  */
#define IDENTIFIER_CLASS_VALUE(NODE) ¥
  (LANG_IDENTIFIER_CAST (NODE)->class_value)

/* TREE_TYPE only indicates on local and class scope the current
   type. For namespace scope, the presence of a type in any namespace
   is indicated with global_type_node, and the real type behind must
   be found through lookup. */
#define IDENTIFIER_TYPE_VALUE(NODE) identifier_type_value (NODE)
#define REAL_IDENTIFIER_TYPE_VALUE(NODE) TREE_TYPE (NODE)
#define SET_IDENTIFIER_TYPE_VALUE(NODE,TYPE) (TREE_TYPE (NODE) = (TYPE))
#define IDENTIFIER_HAS_TYPE_VALUE(NODE) (IDENTIFIER_TYPE_VALUE (NODE) ? 1 : 0)

#define LANG_ID_FIELD(NAME, NODE)			¥
  (LANG_IDENTIFIER_CAST (NODE)->x			¥
   ? LANG_IDENTIFIER_CAST (NODE)->x->NAME : 0)

#define SET_LANG_ID(NODE, VALUE, NAME)					  ¥
  (LANG_IDENTIFIER_CAST (NODE)->x == 0				  	  ¥
   ? LANG_IDENTIFIER_CAST (NODE)->x					  ¥
      = (struct lang_id2 *)perm_calloc (1, sizeo