/* Subroutines used for code generation on IA-32.
   Copyright (C) 1988, 1992, 1994, 1995, 1996, 1997, 1998, 1999, 2000, 2001,
   2002 Free Software Foundation, Inc.

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
#include "../gcc/rtl.h"
#include "../gcc/tree.h"
#include "../gcc/tm_p.h"
#include "../gcc/regs.h"
#include "../gcc/hard-reg-set.h"
#include "../gcc/real.h"
#include "../gcc/insn-config.h"
#include "../gcc/conditions.h"
#include "../gcc/output.h"
#include "../gcc/insn-attr.h"
#include "../gcc/flags.h"
#include "../gcc/except.h"
#include "../gcc/function.h"
#include "../gcc/recog.h"
#include "../gcc/expr.h"
#include "../gcc/optabs.h"
#include "../gcc/toplev.h"
#include "../gcc/basic-block.h"
#include "../gcc/ggc.h"
#include "../gcc/target.h"
#include "../gcc/target-def.h"
/* end of !kawai! */

#ifndef CHECK_STACK_LIMIT
#define CHECK_STACK_LIMIT (-1)
#endif

/* Processor costs (relative to an add) */
static const 
struct processor_costs size_cost = {	/* costs for tunning for size */
  2,					/* cost of an add instruction */
  3,					/* cost of a lea instruction */
  2,					/* variable shift costs */
  3,					/* constant shift costs */
  3,					/* cost of starting a multiply */
  0,					/* cost of multiply per each bit set */
  3,					/* cost of a divide/mod */
  3,					/* cost of movsx */
  3,					/* cost of movzx */
  0,					/* "large" insn */
  2,					/* MOVE_RATIO */
  2,					/* cost for loading QImode using movzbl */
  {2, 2, 2},				/* cost of loading integer registers
					   in QImode, HImode and SImode.
					   Relative to reg-reg move (2).  */
  {2, 2, 2},				/* cost of storing integer registers */
  2,					/* cost of reg,reg fld/fst */
  {2, 2, 2},				/* cost of loading fp registers
					   in SFmode, DFmode and XFmode */
  {2, 2, 2},				/* cost of loading integer registers */
  3,					/* cost of moving MMX register */
  {3, 3},				/* cost of loading MMX registers
					   in SImode and DImode */
  {3, 3},				/* cost of storing MMX registers
					   in SImode and DImode */
  3,					/* cost of moving SSE register */
  {3, 3, 3},				/* cost of loading SSE registers
					   in SImode, DImode and TImode */
  {3, 3, 3},				/* cost of storing SSE registers
					   in SImode, DImode and TImode */
  3,					/* MMX or SSE register to integer */
  0,					/* size of prefetch block */
  0,					/* number of parallel prefetches */
};
/* Processor costs (relative to an add) */
static const 
struct processor_costs i386_cost = {	/* 386 specific costs */
  1,					/* cost of an add instruction */
  1,					/* cost of a lea instruction */
  3,					/* variable shift costs */
  2,					/* constant shift costs */
  6,					/* cost of starting a multiply */
  1,					/* cost of multiply per each bit set */
  23,					/* cost of a divide/mod */
  3,					/* cost of movsx */
  2,					/* cost of movzx */
  15,					/* "large" insn */
  3,					/* MOVE_RATIO */
  4,					/* cost for loading QImode using movzbl */
  {2, 4, 2},				/* cost of loading integer registers
					   in QImode, HImode and SImode.
					   Relative to reg-reg move (2).  */
  {2, 4, 2},				/* cost of storing integer registers */
  2,					/* cost of reg,reg fld/fst */
  {8, 8, 8},				/* cost of loading fp registers
					   in SFmode, DFmode and XFmode */
  {8, 8, 8},				/* cost of loading integer registers */
  2,					/* cost of moving MMX register */
  {4, 8},				/* cost of loading MMX registers
					   in SImode and DImode */
  {4, 8},				/* cost of storing MMX registers
					   in SImode and DImode */
  2,					/* cost of moving SSE register */
  {4, 8, 16},				/* cost of loading SSE registers
					   in SImode, DImode and TImode */
  {4, 8, 16},				/* cost of storing SSE registers
					   in SImode, DImode and TImode */
  3,					/* MMX or SSE register to integer */
  0,					/* size of prefetch block */
  0,					/* number of parallel prefetches */
};

static const 
struct processor_costs i486_cost = {	/* 486 specific costs */
  1,					/* cost of an add instruction */
  1,					/* cost of a lea instruction */
  3,					/* variable shift costs */
  2,					/* constant shift costs */
  12,					/* cost of starting a multiply */
  1,					/* cost of multiply per each bit set */
  40,					/* cost of a divide/mod */
  3,					/* cost of movsx */
  2,					/* cost of movzx */
  15,					/* "large" insn */
  3,					/* MOVE_RATIO */
  4,					/* cost for loading QImode using movzbl */
  {2, 4, 2},				/* cost of loading integer registers
					   in QImode, HImode and SImode.
					   Relative to reg-reg move (2).  */
  {2, 4, 2},				/* cost of storing integer registers */
  2,					/* cost of reg,reg fld/fst */
  {8, 8, 8},				/* cost of loading fp registers
					   in SFmode, DFmode and XFmode */
  {8, 8, 8},				/* cost of loading integer registers */
  2,					/* cost of moving MMX register */
  {4, 8},				/* cost of loading MMX registers
					   in SImode and DImode */
  {4, 8},				/* cost of storing MMX registers
					   in SImode and DImode */
  2,					/* cost of moving SSE register */
  {4, 8, 16},				/* cost of loading SSE registers
					   in SImode, DImode and TImode */
  {4, 8, 16},				/* cost of storing SSE registers
					   in SImode, DImode and TImode */
  3,					/* MMX or SSE register to integer */
  0,					/* size of prefetch block */
  0,					/* number of parallel prefetches */
};

static const 
struct processor_costs pentium_cost = {
  1,					/* cost of an add instruction */
  1,					/* cost of a lea instruction */
  4,					/* variable shift costs */
  1,					/* constant shift costs */
  11,					/* cost of starting a multiply */
  0,					/* cost of multiply per each bit set */
  25,					/* cost of a divide/mod */
  3,					/* cost of movsx */
  2,					/* cost of movzx */
  8,					/* "large" insn */
  6,					/* MOVE_RATIO */
  6,					/* cost for loading QImode using movzbl */
  {2, 4, 2},				/* cost of loading integer registers
					   in QImode, HImode and SImode.
					   Relative to reg-reg move (2).  */
  {2, 4, 2},				/* cost of storing integer registers */
  2,					/* cost of reg,reg fld/fst */
  {2, 2, 6},				/* cost of loading fp registers
					   in SFmode, DFmode and XFmode */
  {4, 4, 6},				/* cost of loading integer registers */
  8,					/* cost of moving MMX register */
  {8, 8},				/* cost of loading MMX registers
					   in SImode and DImode */
  {8, 8},				/* cost of storing MMX registers
					   in SImode and DImode */
  2,					/* cost of moving SSE register */
  {4, 8, 16},				/* cost of loading SSE registers
					   in SImode, DImode and TImode */
  {4, 8, 16},				/* cost of storing SSE registers
					   in SImode, DImode and TImode */
  3,					/* MMX or SSE register to integer */
  0,					/* size of prefetch block */
  0,					/* number of parallel prefetches */
};

static const 
struct processor_costs pentiumpro_cost = {
  1,					/* cost of an add instruction */
  1,					/* cost of a lea instruction */
  1,					/* variable shift costs */
  1,					/* constant shift costs */
  4,					/* cost of starting a multiply */
  0,					/* cost of multiply per each bit set */
  17,					/* cost of a divide/mod */
  1,					/* cost of movsx */
  1,					/* cost of movzx */
  8,					/* "large" insn */
  6,					/* MOVE_RATIO */
  2,					/* cost for loading QImode using movzbl */
  {4, 4, 4},				/* cost of loading integer registers
					   in QImode, HImode and SImode.
					   Relative to reg-reg move (2).  */
  {2, 2, 2},				/* cost of storing integer registers */
  2,					/* cost of reg,reg fld/fst */
  {2, 2, 6},				/* cost of loading fp registers
					   in SFmode, DFmode and XFmode */
  {4, 4, 6},				/* cost of loading integer registers */
  2,					/* cost of moving MMX register */
  {2, 2},				/* cost of loading MMX registers
					   in SImode and DImode */
  {2, 2},				/* cost of storing MMX registers
					   in SImode and DImode */
  2,					/* cost of moving SSE register */
  {2, 2, 8},				/* cost of loading SSE registers
					   in SImode, DImode and TImode */
  {2, 2, 8},				/* cost of storing SSE registers
					   in SImode, DImode and TImode */
  3,					/* MMX or SSE register to integer */
  32,					/* size of prefetch block */
  6,					/* number of parallel prefetches */
};

static const 
struct processor_costs k6_cost = {
  1,					/* cost of an add instruction */
  2,					/* cost of a lea instruction */
  1,					/* variable shift costs */
  1,					/* constant shift costs */
  3,					/* cost of starting a multiply */
  0,					/* cost of multiply per each bit set */
  18,					/* cost of a divide/mod */
  2,					/* cost of movsx */
  2,					/* cost of movzx */
  8,					/* "large" insn */
  4,					/* MOVE_RATIO */
  3,					/* cost for loading QImode using movzbl */
  {4, 5, 4},				/* cost of loading integer registers
					   in QImode, HImode and SImode.
					   Relative to reg-reg move (2).  */
  {2, 3, 2},				/* cost of storing integer registers */
  4,					/* cost of reg,reg fld/fst */
  {6, 6, 6},				/* cost of loading fp registers
					   in SFmode, DFmode and XFmode */
  {4, 4, 4},				/* cost of loading integer registers */
  2,					/* cost of moving MMX register */
  {2, 2},				/* cost of loading MMX registers
					   in SImode and DImode */
  {2, 2},				/* cost of storing MMX registers
					   in SImode and DImode */
  2,					/* cost of moving SSE register */
  {2, 2, 8},				/* cost of loading SSE registers
					   in SImode, DImode and TImode */
  {2, 2, 8},				/* cost of storing SSE registers
					   in SImode, DImode and TImode */
  6,					/* MMX or SSE register to integer */
  32,					/* size of prefetch block */
  1,					/* number of parallel prefetches */
};

static const 
struct processor_costs athlon_cost = {
  1,					/* cost of an add instruction */
  2,					/* cost of a lea instruction */
  1,					/* variable shift costs */
  1,					/* constant shift costs */
  5,					/* cost of starting a multiply */
  0,					/* cost of multiply per each bit set */
  42,					/* cost of a divide/mod */
  1,					/* cost of movsx */
  1,					/* cost of movzx */
  8,					/* "large" insn */
  9,					/* MOVE_RATIO */
  4,					/* cost for loading QImode using movzbl */
  {4, 5, 4},				/* cost of loading integer registers
					   in QImode, HImode and SImode.
					   Relative to reg-reg move (2).  */
  {2, 3, 2},				/* cost of storing integer registers */
  4,					/* cost of reg,reg fld/fst */
  {6, 6, 20},				/* cost of loading fp registers
					   in SFmode, DFmode and XFmode */
  {4, 4, 16},				/* cost of loading integer registers */
  2,					/* cost of moving MMX register */
  {2, 2},				/* cost of loading MMX registers
					   in SImode and DImode */
  {2, 2},				/* cost of storing MMX registers
					   in SImode and DImode */
  2,					/* cost of moving SSE register */
  {2, 2, 8},				/* cost of loading SSE registers
					   in SImode, DImode and TImode */
  {2, 2, 8},				/* cost of storing SSE registers
					   in SImode, DImode and TImode */
  6,					/* MMX or SSE register to integer */
  64,					/* size of prefetch block */
  6,					/* number of parallel prefetches */
};

static const 
struct processor_costs pentium4_cost = {
  1,					/* cost of an add instruction */
  1,					/* cost of a lea instruction */
  8,					/* variable shift costs */
  8,					/* constant shift costs */
  30,	