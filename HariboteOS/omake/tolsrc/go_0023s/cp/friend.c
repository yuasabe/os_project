/* Help friends in C++.
   Copyright (C) 1997, 1998, 1999, 2000, 2001 Free Software Foundation, Inc.

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
#include "../gcc/config.h"
#include "../gcc/system.h"
#include "../gcc/tree.h"
#include "../gcc/rtl.h"
#include "../gcc/expr.h"
#include "cp-tree.h"
#include "../gcc/flags.h"
#include "../gcc/output.h"
#include "../gcc/toplev.h"
/* end of !kawai! */

/* Friend data structures are described in cp-tree.h.  */

/* Returns non-zero if SUPPLICANT is a friend of TYPE.  */

int
is_friend (type, supplicant)
     tree type, supplicant;
{
  int declp;
  register tree list;
  tree context;

  if (supplicant == NULL_TREE || type == NULL_TREE)
    return 0;

  declp = DECL_P (supplicant);

  if (declp)
    /* It's a function decl.  */
    {
      tree list = DECL_FRIENDLIST (TYPE_MAIN_DECL (type));
      tree name = DECL_NAME (supplicant);

      for (; list ; list = TREE_CHAIN (list))
	{
	  if (name == FRIEND_NAME (list))
	    {
	      tree friends = FRIEND_DECLS (list);
	      for (; friends ; friends = TREE_CHAIN (friends))
		{
		  if (TREE_VALUE (friends) == NULL_TREE)
		    continue;

		  if (supplicant == TREE_VALUE (friends))
		    return 1;

		  /* Temporarily, we are more lenient to deal with
		     nested friend functions, for which there can be
		     more than one FUNCTION_DECL, despite being the
		     same function.  When that's fixed, this bit can
		     go.  */
		  if (DECL_FUNCTION_MEMBER_P (supplicant)
		      && same_type_p (TREE_TYPE (supplicant),
				      TREE_TYPE (TREE_VALUE (friends))))
		    return 1;

		  if (TREE_CODE (TREE_VALUE (friends)) == TEMPLATE_DECL
		      && is_specialization_of (supplicant, 
					       TREE_VALUE (friends)))
		    return 1;
		}
	      break;
	    }
	}
    }
  else
    /* It's a type.  */
    {
      /* Nested classes are implicitly friends of their enclosing types, as
	 per core issue 45 (this is a change from the standard).  */
      for (context = supplicant;
	   context && TYPE_P (context);
	   context = TYPE_CONTEXT (context))
	if (type == context)
	  return 1;
      
      list = CLASSTYPE_FRIEND_CLASSES (TREE_TYPE (TYPE_MAIN_DECL (type)));
      for (; list ; list = TREE_CHAIN (list))
	{
	  tree t = TREE_VALUE (list);

	  if (TREE_CODE (t) == TEMPLATE_DECL ? 
	      is_specialization_of (TYPE_MAIN_DECL (supplicant), t) :
	      same_type_p (supplicant, t))
	    return 1;
	}
    }      

  if (declp && DECL_FUNCTION_MEMBER_P (supplicant))
    context = DECL_CONTEXT (supplicant);
  else if (! declp)
    /* Local classes have the same access as the enclosing function.  */
    context = decl_function_context (TYPE_MAIN_DECL (supplicant));
  else
    context = NULL_TREE;

  /* A namespace is not friend to anybody. */
  if (context && TREE_CODE (context) == NAMESPACE_DECL)
    context = NULL_TREE;

  if (context)
    return is_friend (type, context);

  return 0;
}

/* Add a new friend to the friends of the aggregate type TYPE.
   DECL is the FUNCTION_DECL of the friend being added.  */

void
add_friend (type, decl)
     tree type, decl;
{
  tree typedecl;
  tree list;
  tree name;

  if (decl == error_mark_node)
    return;

  typedecl = TYPE_MAIN_DECL (type);
  list = DECL_FRIENDLIST (typedecl);
  name = DECL_NAME (decl);
  type = TREE_T