/* Form lists of pseudo register references for autoinc optimization
   for GNU compiler.  This is part of flow optimization.  
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.
   Contributed by Michael P. Hayes (m.hayes@elec.canterbury.ac.nz)

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

#define DF_RD		 1	/* Reaching definitions.  */
#define DF_RU		 2	/* Reaching uses.  */
#define DF_LR		 4	/* Live registers.  */
#define DF_DU_CHAIN	 8	/* Def-use chain.  */
#define DF_UD_CHAIN     16	/* Use-def chain.  */
#define DF_REG_INFO	32	/* Register info.  */
#define DF_RD_CHAIN	64	/* Reg-def chain.  */
#define DF_RU_CHAIN    128	/* Reg-use chain.  */
#define DF_ALL	       255
#define DF_HARD_REGS  1024
#define DF_EQUIV_NOTES 2048	/* Mark uses present in EQUIV/EQUAL notes.  */

enum df_ref_type {DF_REF_REG_DEF, DF_REF_REG_USE, DF_REF_REG_MEM_LOAD,
		  DF_REF_REG_MEM_STORE};

#define DF_REF_TYPE_NAMES {"def", "use", "mem load", "mem store"}

/* ???> Perhaps all these data structures should be made private
   to enforce the interface.  */


/* Link on a def-use or use-def chain.  */
struct df_link
{
  struct df_link *next;
  struct ref *ref;
};

enum df_ref_flags
  {
    DF_REF_READ_WRITE = 1
  };

/* Define a register reference structure.  */
struct ref
{
  rtx reg;			/* The register referenced.  */
  rtx insn;			/* Insn containing ref.  */
  rtx *loc;			/* Loc is the location of the reg.  */
  struct df_link *chain;	/* Head of def-use or use-def chain.  */
  enum df_ref_type type;	/* Type of ref.  */
  int id;			/* Ref index.  */
  enum df_ref_flags flags;	/* Various flags.  */
};


/* One of these structures is allocated for every insn.  */
struct insn_info
{
  struct df_link *defs;		/* Head of insn-def chain.  */
  struct df_link *uses;		/* Head of insn-use chain.  */
  /* ???? The following luid field should be considerd private so that
     we can change it on the fly to accommodate new insns?  */
  int luid;			/* Logical UID.  */
#if 0
  rtx insn;			/* Backpointer to the insn.  */
#endif
};


/* One of these structures is allocated for every reg.  */
struct reg_info
{
  struct df_link *defs;		/* Head of reg-def chain.  */
  struct df_link *uses;		/* Head of reg-use chain.  */
  int lifetime;
  int n_defs;
  int n_uses;
};


/* One of these structures is allocated for every basic block.  */
struct bb_info
{
  /* Reaching def bitmaps have def_id elements.  */
  bitmap rd_kill;
  bitmap rd_gen;
  bitmap rd_in;
  bitmap rd_out;
  /* Reaching use bitmaps have use_id elements.  */
  bitmap ru_kill;
  bitmap ru_gen;
  bitmap ru_in;
  bitmap ru_out;
  /* Live variable bitmaps have n_regs elements.  */
  bitmap lr_def;
  bitmap lr_use;
  bitmap lr_in;
  bitmap lr_out;
  int rd_valid;
  int ru_valid;
  int lr_valid;
};


struct df
{
  int flags;			/* Indicates what's recorded.  */
  struct bb_info *bbs;		/* Basic block table.  */
  struct ref **defs;		/* Def table, indexed by def_id.  */
  struct ref **uses;		/* Use table, indexed by use_id.  */
  struct ref **reg_def_last;	/* Indexed by regno.  */
  struct reg_info *regs;	/* Regs table, index by regno.  */
  unsigned int reg_size;	/* Size of regs table.  */
  struct insn_info *insns;	/* Insn table, indexed by insn UID.  */
  unsigned int insn_size;	/* Size of insn table.  */
  unsigned int def_id;		/* Next def ID.  