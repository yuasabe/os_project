/* Convert RTL to assembler code and output it, for GNU compiler.
   Copyright (C) 1987, 1988, 1989, 1992, 1993, 1994, 1995, 1996, 1997,
   1998, 1999, 2000, 2001, 2002 Free Software Foundation, Inc.

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

/* This is the final pass of the compiler.
   It looks at the rtl code for a function and outputs assembler code.

   Call `final_start_function' to output the assembler code for function entry,
   `final' to output assembler code for some RTL code,
   `final_end_function' to output assembler code for function exit.
   If a function is compiled in several pieces, each piece is
   output separately with `final'.

   Some optimizations are also done at this level.
   Move instructions that were made unnecessary by good register allocation
   are detected and omitted from the output.  (Though most of these
   are removed by the last jump pass.)

   Instructions to set the condition codes are omitted when it can be
   seen that the condition codes already had the desired values.

   In some cases it is sufficient if the inherited condition codes
   have related values, but this may require the following insn
   (the one that tests the condition codes) to be modified.

   The code for the function prologue and epilogue are generated
   directly in assembler by the target functions function_prologue and
   function_epilogue.  Those instructions never exist as rtl.  */

#include "config.h"
#include "system.h"

#include "tree.h"
#include "rtl.h"
#include "tm_p.h"
#include "regs.h"
#include "insn-config.h"
#include "insn-attr.h"
#include "recog.h"
#include "conditions.h"
#include "flags.h"
#include "real.h"
#include "hard-reg-set.h"
#include "output.h"
#include "except.h"
#include "function.h"
#include "toplev.h"
#include "reload.h"
#include "intl.h"
#include "basic-block.h"
#include "target.h"
#include "debug.h"
#include "expr.h"

#ifdef XCOFF_DEBUGGING_INFO
#include "xcoffout.h"		/* Needed for external data
				   declarations for e.g. AIX 4.x.  */
#endif

#if defined (DWARF2_UNWIND_INFO) || defined (DWARF2_DEBUGGING_INFO)
#include "dwarf2out.h"
#endif

/* If we aren't using cc0, CC_STATUS_INIT shouldn't exist.  So define a
   null default for it to save conditionalization later.  */
#ifndef CC_STATUS_INIT
#define CC_STATUS_INIT
#endif

/* How to start an assembler comment.  */
#ifndef ASM_COMMENT_START
#define ASM_COMMENT_START ";#"
#endif

/* Is the given character a logical line separator for the assembler?  */
#ifndef IS_ASM_LOGICAL_LINE_SEPARATOR
#define IS_ASM_LOGICAL_LINE_SEPARATOR(C) ((C) == ';')
#endif

#ifndef JUMP_TABLES_IN_TEXT_SECTION
#define JUMP_TABLES_IN_TEXT_SECTION 0
#endif

/* Last insn processed by final_scan_insn.  */
static rtx debug_insn;
rtx current_output_insn;

/* Line number of last NOTE.  */
static int last_linenum;

/* Highest line number in current block.  */
static int high_block_linenum;

/* Likewise for function.  */
static int high_function_linenum;

/* Filename of last NOTE.  */
static const char *last_filename;

/* Number of instrumented arcs when profile_arc_flag is set.  */
extern int count_instrumented_edges;

extern int length_unit_log; /* This is defined in insn-attrtab.c.  */

/* Nonzero while outputting an `asm' with operands.
   This means that inconsistencies are the user's fault, so don't abort.
   The precise value is the insn being output, to pass to error_for_asm.  */
static rtx this_is_asm_operands;

/* Number of operands of this insn, for an `asm' with operands.  */
static unsigned int insn_noperands;

/* Compare optimization flag.  */

static rtx last_ignored_compare = 0;

/* Flag indicating this insn is the start of a new basic block.  */

static int new_block = 1;

/* Assign a unique number to each insn that is output.
   This can be used to generate unique local labels.  */

static int insn_counter = 0;

#ifdef HAVE_cc0
/* This variable contains machine-dependent flags (defined in tm.h)
   set and examined by output routines
   that describe how to interpret the condition codes properly.  */

CC_STATUS cc_status;

/* During output of an insn, this contains a copy of cc_status
   from before the insn.  */

CC_STATUS cc_prev_status;
#endif

/* Indexed by hardware reg number, is 1 if that register is ever
   used in the current function.

   In life_analysis, or in stupid_life_analysis, this is set
   up to record the hard regs used explicitly.  Reload adds
   in the hard regs used for holding pseudo regs.  Final uses
   it to generate the code in the function prologue and epilogue
   to save and restore registers as needed.  */

char regs_ever_live[FIRST_PSEUDO_REGISTER];

/* Nonzero means current function must be given a frame pointer.
   Set in stmt.c if anything is allocated on the stack there.
   Set in reload1.c if anything is allocated on the stack there.  */

int frame_pointer_needed;

/* Number of unmatched NOTE_INSN_BLOCK_BEG notes we have seen.  */

static int block_depth;

/* Nonzero if have enabled APP processing of our assembler output.  */

static int app_on;

/* If we are outputting an insn sequence, this contains the sequence rtx.
   Zero otherwise.  */

rtx final_sequence;

#ifdef ASSEMBLER_DIALECT

/* Number of the assembler dialect to use, starting at 0.  */
static int dialect_number;
#endif

/* Indexed by line number, nonzero if there is a note for that line.  */

static char *line_note_exists;

#ifdef HAVE_conditional_execution
/* Nonnull if the insn currently being emitted was a COND_EXEC pattern.  */
rtx current_insn_predicate;
#endif

#ifdef HAVE_ATTR_length
static int asm_insn_count	PARAMS ((rtx));
#endif
static void profile_function	PARAMS ((FILE *));
static void profile_after_prologue PARAMS ((FILE *));
static void notice_source_line	PARAMS ((rtx));
static rtx walk_alter_subreg	PARAMS ((rtx *));
static void output_asm_name	PARAMS ((void));
static tree get_mem_expr_from_op	PARAMS ((rtx, int *));
static void output_asm_operand_names PARAMS ((rtx *, int *, int));
static void output_operand	PARAMS ((rtx, int));
#ifdef LEAF_REGISTERS
static void leaf_renumber_regs	PARAMS ((rtx));
#endif
#ifdef HAVE_cc0
static int alter_cond		PARAMS ((rtx));
#endif
#ifndef ADDR_VEC_ALIGN
static int final_addr_vec_align PARAMS ((rtx));
#endif
#ifdef HAVE_ATTR_length
static int align_fuzz		PARAMS ((rtx, rtx, int, unsigned));
#endif

/* Initialize data in final at the beginning of a compilation.  */

void
init_final (filename)
     const char *filename ATTRIBUTE_UNUSED;
{
  app_on = 0;
  final_sequence = 0;

#ifdef ASSEMBLER_DIALECT
  dialect_number = ASSEMBLER_DIALECT;
#endif
}

/* Called at end of source file,
   to output the block-profiling table for this entire compilation.  */

void
end_final (filename)
     const char *filename;
{
  if (profile_arc_flag)
    {
      char name[20];
      int align = exact_log2 (BIGGEST_ALIGNMENT / BITS_PER_UNIT);
      int size, rounded;
      int long_bytes = LONG_TYPE_SIZE / BITS_PER_UNIT;
      int gcov_type_bytes = GCOV_TYPE_SIZE / BITS_PER_UNIT;
      int pointer_bytes = POINTER_SIZE / BITS_PER_UNIT;
      unsigned int align2 = LONG_TYPE_SIZE;

      size = gcov_type_bytes * count_instrumented_edges;
      rounded = size;

      rounded += (BIGGEST_ALIGNMENT / BITS_PER_UNIT) - 1;
      rounded = (rounded / (BIGGEST_ALIGNMENT / BITS_PER_UNIT)
		 * (BIGGEST_ALIGNMENT / BITS_PER_UNIT));

      /* ??? This _really_ ought to be done with a structure layout
	 and with assemble_constructor.  If long_bytes != pointer_bytes
	 we'll be emitting unaligned data at some point.  */
      if (long_bytes != pointer_bytes)
	abort ();

      data_section ();

      /* Output the main header, of 11 words:
	 0:  1 if this file is initialized, else 0.
	 1:  address of file name (LPBX1).
	 2:  address of table of counts (LPBX2).
	 3:  number of counts in the table.
	 4:  always 0, for compatibility with Sun.

         The following are GNU extensions:

	 5:  address of table of start addrs of basic blocks (LPBX3).
	 6:  Number of bytes in this header.
	 7:  address of table of function names (LPBX4).
	 8:  address of table of line numbers (LPBX5) or 0.
	 9:  address of table of file names (LPBX6) or 0.
	10:  space reserved for basic block profiling.  */

      ASM_OUTPUT_ALIGN (asm_out_file, align);

      ASM_OUTPUT_INTERNAL_LABEL (asm_out_file, "LPBX", 0);

      /* Zero word.  */
      assemble_integer (const0_rtx, long_bytes, align2, 1);

      /* Address of filename.  */
      ASM_GENERATE_INTERNAL_LABEL (name, "LPBX", 1);
      assemble_integer (gen_rtx_SYMBOL_REF (Pmode, name), pointer_bytes,
			align2, 1);

      /* Address of count table.  */
      ASM_GENERATE_INTERNAL_LABEL (name, "LPBX", 2);
      assemble_integer (gen_rtx_SYMBOL_REF (Pmode, name), pointer_bytes,
			align2, 1);

      /* Count of the # of instrumented arcs.  */
      assemble_integer (GEN_INT (count_instrumented_edges),
			long_bytes, align2, 1);

      /* Zero word (link field).  */
      assemble_integer (const0_rtx, pointer_bytes, align2, 1);

      assemble_integer (const0_rtx, pointer_bytes, align2, 1);

      /* Byte count for extended structure.  */
      assemble_integer (GEN_INT (11 * UNITS_PER_WORD), long_bytes, align2, 1);

      /* Address of function name table.  */
      assemble_integer (const0_rtx, pointer_bytes, align2, 1);

      /* Address of line number and filename tables if debugging.  */
      assemble_integer (const0_rtx, pointer_bytes, align2, 1);
      assemble_integer (const0_rtx, pointer_bytes, align2, 1);

      /* Space for extension ptr (link field).  */
      assemble_integer (const0_rtx, UNITS_PER_WORD, align2, 1);

      /* Output the file name changing the suffix to .d for
	 Sun tcov compatibility.  */
      ASM_OUTPUT_INTERNAL_LABEL (asm_out_file, "LPBX", 1);
      {
	char *cwd = getpwd ();
	int len = strlen (filename) + strlen (cwd) + 1;
	char *data_file = (char *) alloca (len + 4);

	strcpy (data_file, cwd);
	strcat (data_file, "/");
	strcat (data_file, filename);
	strip_off_ending (data_file, len);
	strcat (data_file, ".da");
	assemble_string (data_file, strlen (data_file) + 1);
      }

      /* Make space for the table of counts.  */
      if (size == 0)
	{
	  /* Realign data section.  */
	  ASM_OUTPUT_ALIGN (asm_out_file, align);
	  ASM_OUTPUT_INTERNAL_LABEL (asm_out_file, "LPBX", 2);
	  if (size != 0)
	    assemble_zeros (size);
	}
      else
	{
	  ASM_GENERATE_INTERNAL_LABEL (name, "LPBX", 2);
#ifdef ASM_OUTPUT_SHARED_LOCAL
	  if (flag_shared_data)
	    ASM_OUTPUT_SHARED_LOCAL (asm_out_file, name, size, rounded);
	  else
#endif
#ifdef ASM_OUTPUT_ALIGNED_DECL_LOCAL
	    ASM_OUTPUT_ALIGNED_DECL_LOCAL (asm_out_file, NULL_TREE, name,
					   size, BIGGEST_ALIGNMENT);
#else
#ifdef ASM_OUTPUT_ALIGNED_LOCAL
	    ASM_OUTPUT_ALIGNED_LOCAL (asm_out_file, name, size,
				      BIGGEST_ALIGNMENT);
#else
	    ASM_OUTPUT_LOCAL (asm_out_file, name, size, rounded);
#endif
#endif
	}
    }
}

/* Default target function prologue and epilogue assembler output.

   If not overridden for epilogue code, then the function body itself
   contains return instructions wherever needed.  */
void
default_function_pro_epilogue (file, size)
     FILE *file ATTRIBUTE_UNUSED;
     HOST_WIDE_INT size ATTRIBUTE_UNUSED;
{
}

/* Default target hook that outputs nothing to a stream.  */
void
no_asm_to_stream (file)
     FILE *file ATTRIBUTE_UNUSED;
{
}

/* Enable APP processing of subsequent output.
   Used before the output from an `asm' statement.  */

void
app_enable ()
{
  if (! app_on)
    {
      fputs (ASM_APP_ON, asm_out_file);
      app_on = 1;
    }
}

/* Disable APP processing of subsequent output.
   Called from varasm.c before most kinds of output.  */

void
app_disable ()
{
  if (app_on)
    {
      fputs (ASM_APP_OFF, asm_out_file);
      app_on = 0;
    }
}

/* Return the number of slots filled in the current
   delayed branch sequence (we don't count the insn needing the
   delay slot).   Zero if not in a delayed branch sequence.  */

#ifdef DELAY_SLOTS
int
dbr_sequence_length ()
{
  if (final_sequence != 0)
    return XVECLEN (final_sequence, 0) - 1;
  else
    return 0;
}
#endif

/* The next two pages contain routines used to compute the length of an insn
   and to shorten branches.  */

/* Arrays for insn lengths, and addresses.  The latter is referenced by
   `insn_current_length'.  */

static int *insn_lengths;

#ifdef HAVE_ATTR_length
varray_type insn_addresses_;
#endif

/* Max uid for which the above arrays are valid.  */
static int insn_lengths_max_uid;

/* Address of insn being processed.  Used by `insn_current_length'.  */
int insn_current_address;

/* Address of insn being processed in previous iteration.  */
int insn_last_address;

/* known invariant alignment of insn being processed.  */
int insn_current_align;

/* After shorten_branches, for any insn, uid_align[INSN_UID (insn)]
   gives the next following alignment insn that increases the known
   alignment, or NULL_RTX if there is no such insn.
   For any alignment obtained this way, we can again index uid_align with
   its uid to obtain the next following align that in turn increases the
   alignment, till we reach NULL_RTX; the sequence obtained this way
   for each insn we'll call the alignment chain of this insn in the following
   comments.  */

struct label_alignment
{
  short alignment;
  short max_skip;
};

static rtx *uid_align;
static int *uid_shuid;
static struct label_alignment *label_align;

/* Indicate that branch shortening hasn't yet been done.  */

void
init_insn_lengths ()
{
  if (uid_shuid)
    {
      free (uid_shuid);
      uid_shuid = 0;
    }
  if (insn_lengths)
    {
      free (insn_lengths);
      insn_lengths = 0;
      insn_lengths_max_uid = 0;
    }
#ifdef HAVE_ATTR_length
  INSN_ADDRESSES_FREE ();
#endif
  if (uid_align)
    {
      free (uid_align);
      uid_align = 0;
    }
}

/* Obtain the current length of an insn.  If branch shortening has been done,
   get its actual length.  Otherwise, get its maximum length.  */

int
get_attr_length (insn)
     rtx insn ATTRIBUTE_UNUSED;
{
#ifdef HAVE_ATTR_length
  rtx body;
  int i;
  int length = 0;

  if (insn_lengths_max_uid > INSN_UID (insn))
    return insn_lengths[INSN_UID (insn)];
  else
    switch (GET_CODE (insn))
      {
      case NOTE:
      case BARRIER:
      case CODE_LABEL:
	return 0;

      case CALL_INSN:
	length = insn_default_length (insn);
	break;

      case JUMP_INSN:
	body = PATTERN (insn);
        if (GET_CODE (body) == ADDR_VEC || GET_CODE (body) == ADDR_DIFF_VEC)
	  {
	    /* Alignment is machine-dependent and should be handled by
	       ADDR_VEC_ALIGN.  */
	  }
	else
	  length = insn_default_length (insn);
	break;

      case INSN:
	body = PATTERN (insn);
	if (GET_CODE (body) == USE || GET_CODE (body) == CLOBBER)
	  return 0;

	else if (GET_CODE (body) == ASM_INPUT || asm_noperands (body) >= 0)
	  length = asm_insn_count (body) * insn_default_length (insn);
	else if (GET_CODE (body) == SEQUENCE)
	  for (i = 0; i < XVECLEN (body, 0); i++)
	    length += get_attr_length (XVECEXP (body, 0, i));
	else
	  length = insn_default_length (insn);
	break;

      default:
	break;
      }

#ifdef ADJUST_INSN_LENGTH
  ADJUST_INSN_LENGTH (insn, length);
#endif
  return length;
#else /* not HAVE_ATTR_length */
  return 0;
#endif /* not HAVE_ATTR_length */
}

/* Code to handle alignment inside shorten_branches.  */

/* Here is an explanation how the algorithm in align_fuzz can give
   proper results:

   Call a sequence of instructions beginning with alignment point X
   and continuing until the next alignment point `block X'.  When `X'
   is used in an expression, it means the alignment value of the
   alignment point.

   Call the distance between the start of the first insn of block X, and
   the end of the last insn of block X `IX', for the `inner size of X'.
   This is clearly the sum of the instruction lengths.

   Likewise with the next alignment-delimited block following X, which we
   shall call block Y.

   Call the distance between the start of the first insn of block X, and
   the start of the first insn of block Y `OX', for the `outer size of X'.

   The estimated padding is then OX - IX.

   OX can be safely estimated as

           if (X >= Y)
                   OX = round_up(IX, Y)
           else
                   OX = round_up(IX, X) + Y - X

   Clearly est(IX) >= real(IX), because that only depends on the
   instruction lengths, and those being overestimated is a given.

   Clearly round_up(foo, Z) >= round_up(bar, Z) if foo >= bar, so
   we needn't worry about that when thinking about OX.

   When X >= Y, the alignment provided by Y adds no uncertainty factor
   for branch ranges starting before X, so we can just round what we have.
   But when X < Y, we don't know anything about the, so to speak,
   `middle bits', so we have to assume the worst when aligning up from an
   address mod X to one mod Y, which is Y - X.  */

#ifndef LABEL_ALIGN
#define LABEL_ALIGN(LABEL) align_labels_log
#endif

#ifndef LABEL_ALIGN_MAX_SKIP
#define LABEL_ALIGN_MAX_SKIP align_labels_max_skip
#endif

#ifndef LOOP_ALIGN
#define LOOP_ALIGN(LABEL) align_loops_log
#endif

#ifndef LOOP_ALIGN_MAX_SKIP
#define LOOP_ALIGN_MAX_SKIP align_loops_max_skip
#endif

#ifndef LABEL_ALIGN_AFTER_BARRIER
#define LABEL_ALIGN_AFTER_BARRIER(LABEL) 0
#endif

#ifndef LABEL_ALIGN_AFTER_BARRIER_MAX_SKIP
#define LABEL_ALIGN_AFTER_BARRIER_MAX_SKIP 0
#endif

#ifndef JUMP_ALIGN
#define JUMP_ALIGN(LABEL) align_jumps_log
#endif

#ifndef JUMP_ALIGN_MAX_SKIP
#define JUMP_ALIGN_MAX_SKIP align_jumps_max_skip
#endif

#ifndef ADDR_VEC_ALIGN
static int
final_addr_vec_align (addr_vec)
     rtx addr_vec;
{
  int align = GET_MODE_SIZE (GET_MODE (PATTERN (addr_vec)));

  if (align > BIGGEST_ALIGNMENT / BITS_PER_UNIT)
    align = BIGGEST_ALIGNMENT / BITS_PER_UNIT;
  return exact_log2 (align);

}

#define ADDR_VEC_ALIGN(ADDR_VEC) final_addr_vec_align (ADDR_VEC)
#endif

#ifndef INSN_LENGTH_ALIGNMENT
#define INSN_LENGTH_ALIGNMENT(INSN) length_unit_log
#endif

#define INSN_SHUID(INSN) (uid_shuid[INSN_UID (INSN)])

static int min_labelno, max_labelno;

#define LABEL_TO_ALIGNMENT(LABEL) ¥
  (label_align[CODE_LABEL_NUMBER (LABEL) - min_labelno].alignment)

#define LABEL_TO_MAX_SKIP(LABEL) ¥
  (label_align[CODE_LABEL_NUMBER (LABEL) - min_labelno].max_skip)

/* For the benefit of port specific code do this also as a function.  */

int
label_to_alignment (label)
     rtx label;
{
  return LABEL_TO_ALIGNMENT (label);
}

#ifdef HAVE_ATTR_length
/* The differences in addresses
   between a branch and its target might grow or shrink depending on
   the alignment the start insn of the range (the branch for a forward
   branch or the label for a backward branch) starts out on; if these
   differences are used naively, they can even oscillate infinitely.
   We therefore want to compute a 'worst case' address difference that
   is independent of the alignment the start insn of the range end
   up on, and that is at least as large as the actual difference.
   The function align_fuzz calculates the amount we have to add to the
   naively computed difference, by traversing the part of the alignment
   chain of the start insn of the range that is in front of the end insn
   of the range, and considering for each alignment the maximum amount
   that it might contribute to a size increase.

   For casesi tables, we also want to know worst case minimum amounts of
   address difference, in case a machine description wants to introduce
   some common offset that is added to all offsets in a table.
   For this purpose, align_fuzz with a growth argument of 0 computes the
   appropriate adjustment.  */

/* Compute the maximum delta by which the difference of the addresses of
   START and END might grow / shrink due to a different address for start
   which changes the size of alignment insns between START and END.
   KNOWN_ALIGN_LOG is the alignment known for START.
   GROWTH should be ‾0 if the objective is to compute potential code size
   increase, and 0 if the objective is to compute potential shrink.
   The return value is undefined for any other value of GROWTH.  */

static int
align_fuzz (start, end, known_align_log, growth)
     rtx start, end;
     int known_align_log;
     unsigned growth;
{
  int uid = INSN_UID (start);
  rtx align_label;
  int known_align = 1 << known_align_log;
  int end_shuid = INSN_SHUID (end);
  int fuzz = 0;

  for (align_label = uid_align[uid]; align_label; align_label = uid_align[uid])
    {
      int align_addr, new_align;

      uid = INSN_UID (align_label);
      align_addr = INSN_ADDRESSES (uid) - insn_lengths[uid];
      if (uid_shuid[uid] > end_shuid)
	break;
      known_align_log = LABEL_TO_ALIGNMENT (align_label);
      new_align = 1 << known_align_log;
      if (new_align < known_align)
	continue;
      fuzz += (-align_addr ^ growth) & (new_align - known_align);
      known_align = new_align;
    }
  return fuzz;
}

/* Compute a worst-case reference address of a branch so that it
   can be safely used in the presence of aligned labels.  Since the
   size of the branch itself is unknown, the size of the branch is
   not included in the range.  I.e. for a forward branch, the reference
   address is the end address of the branch as known from the previous
   branch shortening pass, minus a value to account for possible size
   increase due to alignment.  For a backward branch, it is the start
   address of the branch as known from the current pass, plus a value
   to account for possible size increase due to alignment.
   NB.: Therefore, the maximum offset allowed for backward branches needs
   to exclude the branch size.  */

int
insn_current_reference_address (branch)
     rtx branch;
{
  rtx dest, seq;
  int seq_uid;

  if (! INSN_ADDRESSES_SET_P ())
    return 0;

  seq = NEXT_INSN (PREV_INSN (branch));
  seq_uid = INSN_UID (seq);
  if (GET_CODE (branch) != JUMP_INSN)
    /* This can happen for example on the PA; the objective is to know the
       offset to address something in front of the start of the function.
       Thus, we can treat it like a backward branch.
       We assume here that FUNCTION_BOUNDARY / BITS_PER_UNIT is larger than
       any alignment we'd encounter, so we skip the call to align_fuzz.  */
    return insn_current_address;
  dest = JUMP_LABEL (branch);

  /* BRANCH has no proper alignment chain set, so use SEQ.
     BRANCH also has no INSN_SHUID.  */
  if (INSN_SHUID (seq) < INSN_SHUID (dest))
    {
      /* Forward branch.  */
      return (insn_last_address + insn_lengths[seq_uid]
	      - align_fuzz (seq, dest, length_unit_log, ‾0));
    }
  else
    {
      /* Backward branch.  */
      return (insn_current_address
	      + align_fuzz (dest, seq, length_unit_log, ‾0));
    }
}
#endif /* HAVE_ATTR_length */

void
compute_alignments ()
{
  int i;
  int log, max_skip, max_log;

  if (label_align)
    {
      free (label_align);
      label_align = 0;
    }

  max_labelno = max_label_num ();
  min_labelno = get_first_label_num ();
  label_align = (struct label_alignment *)
    xcalloc (max_labelno - min_labelno + 1, sizeof (struct label_alignment));

  /* If not optimizing or optimizing for size, don't assign any alignments.  */
  if (! optimize || optimize_size)
    return;

  for (i = 0; i < n_basic_blocks; i++)
    {
      basic_block bb = BASIC_BLOCK (i);
      rtx label = bb->head;
      int fallthru_frequency = 0, branch_frequency = 0, has_fallthru = 0;
      edge e;

      if (GET_CODE (label) != CODE_LABEL)
	continue;
      max_log = LABEL_ALIGN (label);
      max_skip = LABEL_ALIGN_MAX_SKIP;

      for (e = bb->pred; e; e = e->pred_next)
	{
	  if (e->flags & EDGE_FALLTHRU)
	    has_fallthru = 1, fallthru_frequency += EDGE_FREQUENCY (e);
	  else
	    branch_frequency += EDGE_FREQUENCY (e);
	}

      /* There are two purposes to align block with no fallthru incoming edge:
	 1) to avoid fetch stalls when branch destination is near cache boundary
	 2) to improve cache efficiency in case the previous block is not executed
	    (so it does not need to be in the cache).

	 We to catch first case, we align frequently executed blocks.
	 To catch the second, we align blocks that are executed more frequently
	 than the predecessor and the predecessor is likely to not be executed
	 when function is called.  */

      if (!has_fallthru
	  && (branch_frequency > BB_FREQ_MAX / 10
	      || (bb->frequency > BASIC_BLOCK (i - 1)->frequency * 10
		  && (BASIC_BLOCK (i - 1)->frequency
		      <= ENTRY_BLOCK_PTR->frequency / 2))))
	{
	  log = JUMP_ALIGN (label);
	  if (max_log < log)
	    {
	      max_log = log;
	      max_skip = JUMP_ALIGN_MAX_SKIP;
	    }
	}
      /* In case block is frequent and reached mostly by non-fallthru edge,
	 align it.  It is most likely an first block of loop.  */
      if (has_fallthru
	  && branch_frequency + fallthru_frequency > BB_FREQ_MAX / 10
	  && branch_frequency > fallthru_frequency * 5)
	{
	  log = LOOP_ALIGN (label);
	  if (max_log < log)
	    {
	      max_log = log;
	      max_skip = LOOP_ALIGN_MAX_SKIP;
	    }
	}
      LABEL_TO_ALIGNMENT (label) = max_log;
      LABEL_TO_MAX_SKIP (label) = max_skip;
    }
}

/* Make a pass over all insns and compute their actual lengths by shortening
   any branches of variable length if possible.  */

/* Give a default value for the lowest address in a function.  */

#ifndef FIRST_INSN_ADDRESS
#define FIRST_INSN_ADDRESS 0
#endif

/* shorten_branches might be called multiple times:  for example, the SH
   port splits out-of-range conditional branches in MACHINE_DEPENDENT_REORG.
   In order to do this, it needs proper length information, which it obtains
   by calling shorten_branches.  This cannot be collapsed with
   shorten_branches itself into a single pass unless we also want to integrate
   reorg.c, since the branch splitting exposes new instructions with delay
   slots.  */

void
shorten_branches (first)
     rtx first ATTRIBUTE_UNUSED;
{
  rtx insn;
  int max_uid;
  int i;
  int max_log;
  int max_skip;
#ifdef HAVE_ATTR_length
#define MAX_CODE_ALIGN 16
  rtx seq;
  int something_changed = 1;
  char *varying_length;
  rtx body;
  int uid;
  rtx align_tab[MAX_CODE_ALIGN];

#endif

  /* Compute maximum UID and allocate label_align / uid_shuid.  */
  max_uid = get_max_uid ();

  uid_shuid = (int *) xmalloc (max_uid * sizeof *uid_shuid);

  if (max_labelno != max_label_num ())
    {
      int old = max_labelno;
      int n_labels;
      int n_old_labels;

      max_labelno = max_label_num ();

      n_labels = max_labelno - min_labelno + 1;
      n_old_labels = old - min_labelno + 1;

      label_align = (struct label_alignment *) xrealloc
	(label_align, n_labels * sizeof (struct label_alignment));

      /* Range of labels grows monotonically in the function.  Abort here
         means that the initialization of array got lost.  */
      if (n_old_labels > n_labels)
	abort ();

      memset (label_align + n_old_labels, 0,
	      (n_labels - n_old_labels) * sizeof (struct label_alignment));
    }

  /* Initialize label_align and set up uid_shuid to be strictly
     monotonically rising with insn order.  */
  /* We use max_log here to keep track of the maximum alignment we want to
     impose on the next CODE_LABEL (or the current one if we are processing
     the CODE_LABEL itself).  */

  max_log = 0;
  max_skip = 0;

  for (insn = get_insns (), i = 1; insn; insn = NEXT_INSN (insn))
    {
      int log;

      INSN_SHUID (insn) = i++;
      if (INSN_P (insn))
	{
	  /* reorg might make the first insn of a loop being run once only,
             and delete the label in front of it.  Then we want to apply
             the loop alignment to the new label created by reorg, which
             is separated by the former loop start insn from the
	     NOTE_INSN_LOOP_BEG.  */
	}
      else if (GET_CODE (insn) == CODE_LABEL)
	{
	  rtx next;

	  /* Merge in alignments computed by compute_alignments.  */
	  log = LABEL_TO_ALIGNMENT (insn);
	  if (max_log < log)
	    {
	      max_log = log;
	      max_skip = LABEL_TO_MAX_SKIP (insn);
	    }

	  log = LABEL_ALIGN (insn);
	  if (max_log < log)
	    {
	      max_log = log;
	      max_skip = LABEL_ALIGN_MAX_SKIP;
	    }
	  next = NEXT_INSN (insn);
	  /* ADDR_VECs only take room if read-only data goes into the text
	     section.  */
	  if (JUMP_TABLES_IN_TEXT_SECTION
#if !defined(READONLY_DATA_SECTION)
	      || 1
#endif
	      )
	    if (next && GET_CODE (next) == JUMP_INSN)
	      {
		rtx nextbody = PATTERN (next);
		if (GET_CODE (nextbody) == ADDR_VEC
		    || GET_CODE (nextbody) == ADDR_DIFF_VEC)
		  {
		    log = ADDR_VEC_ALIGN (next);
		    if (max_log < log)
		      {
			max_log = log;
			max_skip = LABEL_ALIGN_MAX_SKIP;
		      }
		  }
	      }
	  LABEL_TO_ALIGNMENT (insn) = max_log;
	  LABEL_TO_MAX_SKIP (insn) = max_skip;
	  max_log = 0;
	  max_skip = 0;
	}
      else if (GET_CODE (insn) == BARRIER)
	{
	  rtx label;

	  for (label = insn; label && ! INSN_P (label);
	       label = NEXT_INSN (label))
	    if (GET_CODE (label) == CODE_LABEL)
	      {
		log = LABEL_ALIGN_AFTER_BARRIER (insn);
		if (max_log < log)
		  {
		    max_log = log;
		    max_skip = LABEL_ALIGN_AFTER_BARRIER_MAX_SKIP;
		  }
		break;
	      }
	}
    }
#ifdef HAVE_ATTR_length

  /* Allocate the rest of the arrays.  */
  insn_lengths = (int *) xmalloc (max_uid * sizeof (*insn_lengths));
  insn_lengths_max_uid = max_uid;
  /* Syntax errors can lead to labels being outside of the main insn stream.
     Initialize insn_addresses, so that we get reproducible results.  */
  INSN_ADDRESSES_ALLOC (max_uid);

  varying_length = (char *) xcalloc (max_uid, sizeof (char));

  /* Initialize uid_align.  We scan instructions
     from end to start, and keep in align_tab[n] the last seen insn
     that does an alignment of at least n+1, i.e. the successor
     in the alignment chain for an insn that does / has a known
     alignment of n.  */
  uid_align = (rtx *) xcalloc (max_uid, sizeof *uid_align);

  for (i = MAX_CODE_ALIGN; --i >= 0;)
    align_tab[i] = NULL_RTX;
  seq = get_last_insn ();
  for (; seq; seq = PREV_INSN (seq))
    {
      int uid = INSN_UID (seq);
      int log;
      log = (GET_CODE (seq) == CODE_LABEL ? LABEL_TO_ALIGNMENT (seq) : 0);
      uid_align[uid] = align_tab[0];
      if (log)
	{
	  /* Found an alignment label.  */
	  uid_align[uid] = align_tab[log];
	  for (i = log - 1; i >= 0; i--)
	    align_tab[i] = seq;
	}
    }
#ifdef CASE_VECTOR_SHORTEN_MODE
  if (optimize)
    {
      /* Look for ADDR_DIFF_VECs, and initialize their minimum and maximum
         label fields.  */

      int min_shuid = INSN_SHUID (get_insns ()) - 1;
      int max_shuid = INSN_SHUID (get_last_insn ()) + 1;
      int rel;

      for (insn = first; insn != 0; insn = NEXT_INSN (insn))
	{
	  rtx min_lab = NULL_RTX, max_lab = NULL_RTX, pat;
	  int len, i, min, max, insn_shuid;
	  int min_align;
	  addr_diff_vec_flags flags;

	  if (GET_CODE (insn) != JUMP_INSN
	      || GET_CODE (PATTERN (insn)) != ADDR_DIFF_VEC)
	    continue;
	  pat = PATTERN (insn);
	  len = XVECLEN (pat, 1);
	  if (len <= 0)
	    abort ();
	  min_align = MAX_CODE_ALIGN;
	  for (min = max_shuid, max = min_shuid, i = len - 1; i >= 0; i--)
	    {
	      rtx lab = XEXP (XVECEXP (pat, 1, i), 0);
	      int shuid = INSN_SHUID (lab);
	      if (shuid < min)
		{
		  min = shuid;
		  min_lab = lab;
		}
	      if (shuid > max)
		{
		  max = shuid;
		  max_lab = lab;
		}
	      if (min_align > LABEL_TO_ALIGNMENT (lab))
		min_align = LABEL_TO_ALIGNMENT (lab);
	    }
	  XEXP (pat, 2) = gen_rtx_LABEL_REF (VOIDmode, min_lab);
	  XEXP (pat, 3) = gen_rtx_LABEL_REF (VOIDmode, max_lab);
	  insn_shuid = INSN_SHUID (insn);
	  rel = INSN_SHUID (XEXP (XEXP (pat, 0), 0));
	  flags.min_align = min_align;
	  flags.base_after_vec = rel > insn_shuid;
	  flags.min_after_vec  = min > insn_shuid;
	  flags.max_after_vec  = max > insn_shuid;
	  flags.min_after_base = min > rel;
	  flags.max_after_base = max > rel;
	  ADDR_DIFF_VEC_FLAGS (pat) = flags;
	}
    }
#endif /* CASE_VECTOR_SHORTEN_MODE */

  /* Compute initial lengths, addresses, and varying flags for each insn.  */
  for (insn_current_address = FIRST_INSN_ADDRESS, insn = first;
       insn != 0;
       insn_current_address += insn_lengths[uid], insn = NEXT_INSN (insn))
    {
      uid = INSN_UID (insn);

      insn_lengths[uid] = 0;

      if (GET_CODE (insn) == CODE_LABEL)
	{
	  int log = LABEL_TO_ALIGNMENT (insn);
	  if (log)
	    {
	      int align = 1 << log;
	      int new_address = (insn_current_address + align - 1) & -align;
	      insn_lengths[uid] = new_address - insn_current_address;
	    }
	}

      INSN_ADDRESSES (uid) = insn_current_address;

      if (GET_CODE (insn) == NOTE || GET_CODE (insn) == BARRIER
	  || GET_CODE (insn) == CODE_LABEL)
	continue;
      if (INSN_DELETED_P (insn))
	continue;

      body = PATTERN (insn);
      if (GET_CODE (body) == ADDR_VEC || GET_CODE (body) == ADDR_DIFF_VEC)
	{
	  /* This only takes room if read-only data goes into the text
	     section.  */
	  if (JUMP_TABLES_IN_TEXT_SECTION
#if !defined(READONLY_DATA_SECTION)
	      || 1
#endif
	      )
	    insn_lengths[uid] = (XVECLEN (body,
					  GET_CODE (body) == ADDR_DIFF_VEC)
				 * GET_MODE_SIZE (GET_MODE (body)));
	  /* Alignment is handled by ADDR_VEC_ALIGN.  */
	}
      else if (GET_CODE (body) == ASM_INPUT || asm_noperands (body) >= 0)
	insn_lengths[uid] = asm_insn_count (body) * insn_default_length (insn);
      else if (GET_CODE (body) == SEQUENCE)
	{
	  int i;
	  int const_delay_slots;
#ifdef DELAY_SLOTS
	  const_delay_slots = const_num_delay_slots (XVECEXP (body, 0, 0));
#else
	  const_delay_slots = 0;
#endif
	  /* Inside a delay slot sequence, we do not do any branch shortening
	     if the shortening could change the number of delay slots
	     of the branch.  */
	  for (i = 0; i < XVECLEN (body, 0); i++)
	    {
	      rtx inner_insn = XVECEXP (body, 0, i);
	      int inner_uid = INSN_UID (inner_insn);
	      int inner_length;

	      if (GET_CODE (body) == ASM_INPUT
		  || asm_noperands (PATTERN (XVECEXP (body, 0, i))) >= 0)
		inner_length = (asm_insn_count (PATTERN (inner_insn))
				* insn_default_length (inner_insn));
	      else
		inner_length = insn_default_length (inner_insn);

	      insn_lengths[inner_uid] = inner_length;
	      if (const_delay_slots)
		{
		  if ((varying_length[inner_uid]
		       = insn_variable_length_p (inner_insn)) != 0)
		    varying_length[uid] = 1;
		  INSN_ADDRESSES (inner_uid) = (insn_current_address
						+ insn_lengths[uid]);
		}
	      else
		varying_length[inner_uid] = 0;
	      insn_lengths[uid] += inner_length;
	    }
	}
      else if (GET_CODE (body) != USE && GET_CODE (body) != CLOBBER)
	{
	  insn_lengths[uid] = insn_default_length (insn);
	  varying_length[uid] = insn_variable_length_p (insn);
	}

      /* If needed, do any adjustment.  */
#ifdef ADJUST_INSN_LENGTH
      ADJUST_INSN_LENGTH (insn, insn_lengths[uid]);
      if (insn_lengths[uid] < 0)
	fatal_insn ("negative insn length", insn);
#endif
    }

  /* Now loop over all the insns finding varying length insns.  For each,
     get the current insn length.  If it has changed, reflect the change.
     When nothing changes for a full pass, we are done.  */

  while (something_changed)
    {
      something_changed = 0;
      insn_current_align = MAX_CODE_ALIGN - 1;
      for (insn_current_address = FIRST_INSN_ADDRESS, insn = first;
	   insn != 0;
	   insn = NEXT_INSN (insn))
	{
	  int new_length;
#ifdef ADJUST_INSN_LENGTH
	  int tmp_length;
#endif
	  int length_align;

	  uid = INSN_UID (insn);

	  if (GET_CODE (insn) == CODE_LABEL)
	    {
	      int log = LABEL_TO_ALIGNMENT (insn);
	      if (log > insn_current_align)
		{
		  int align = 1 << log;
		  int new_address= (insn_current_address + align - 1) & -align;
		  insn_lengths[uid] = new_address - insn_current_address;
		  insn_current_align = log;
		  insn_current_address = new_address;
		}
	      else
		insn_lengths[uid] = 0;
	      INSN_ADDRESSES (uid) = insn_current_address;
	      continue;
	    }

	  length_align = INSN_LENGTH_ALIGNMENT (insn);
	  if (length_align < insn_current_align)
	    insn_current_align = length_align;

	  insn_last_address = INSN_ADDRESSES (uid);
	  INSN_ADDRESSES (uid) = insn_current_address;

#ifdef CASE_VECTOR_SHORTEN_MODE
	  if (optimize && GET_CODE (insn) == JUMP_INSN
	      && GET_CODE (PATTERN (insn)) == ADDR_DIFF_VEC)
	    {
	      rtx body = PATTERN (insn);
	      int old_length = insn_lengths[uid];
	      rtx rel_lab = XEXP (XEXP (body, 0), 0);
	      rtx min_lab = XEXP (XEXP (body, 2), 0);
	      rtx max_lab = XEXP (XEXP (body, 3), 0);
	      int rel_addr = INSN_ADDRESSES (INSN_UID (rel_lab));
	      int min_addr = INSN_ADDRESSES (INSN_UID (min_lab));
	      int max_addr = INSN_ADDRESSES (INSN_UID (max_lab));
	      rtx prev;
	      int rel_align = 0;
	      addr_diff_vec_flags flags;

	      /* Avoid automatic aggregate initialization.  */
	      flags = ADDR_DIFF_VEC_FLAGS (body);

	      /* Try to find a known alignment for rel_lab.  */
	      for (prev = rel_lab;
		   prev
		   && ! insn_lengths[INSN_UID (prev)]
		   && ! (varying_length[INSN_UID (prev)] & 1);
		   prev = PREV_INSN (prev))
		if (varying_length[INSN_UID (prev)] & 2)
		  {
		    rel_align = LABEL_TO_ALIGNMENT (prev);
		    break;
		  }

	      /* See the comment on addr_diff_vec_flags in rtl.h for the
		 meaning of the flags values.  base: REL_LAB   vec: INSN  */
	      /* Anything after INSN has still addresses from the last
		 pass; adjust these so that they reflect our current
		 estimate for this pass.  */
	      if (flags.base_after_vec)
		rel_addr += insn_current_address - insn_last_address;
	      if (flags.min_after_vec)
		min_addr += insn_current_address - insn_last_address;
	      if (flags.max_after_vec)
		max_addr += insn_current_address - insn_last_address;
	      /* We want to know the worst case, i.e. lowest possible value
		 for the offset of MIN_LAB.  If MIN_LAB is after REL_LAB,
		 its offset is positive, and we have to be wary of code shrink;
		 otherwise, it is negative, and we have to be vary of code
		 size increase.  */
	      if (flags.min_after_base)
		{
		  /* If INSN is between REL_LAB and MIN_LAB, the size
		     changes we are about to make can change the alignment
		     within the observed offset, therefore we have to break
		     it up into two parts that are independent.  */
		  if (! flags.base_after_vec && flags.min_after_vec)
		    {
		      min_addr -= align_fuzz (rel_lab, insn, rel_align, 0);
		      min_addr -= align_fuzz (insn, min_lab, 0, 0);
		    }
		  else
		    min_addr -= align_fuzz (rel_lab, min_lab, rel_align, 0);
		}
	      else
		{
		  if (flags.base_after_vec && ! flags.min_after_vec)
		    {
		      min_addr -= align_fuzz (min_lab, insn, 0, ‾0);
		      min_addr -= align_fuzz (insn, rel_lab, 0, ‾0);
		    }
		  else
		    min_addr -= align_fuzz (min_lab, rel_lab, 0, ‾0);
		}
	      /* Likewise, determine the highest lowest possible value
		 for the offset of MAX_LAB.  */
	      if (flags.max_after_base)
		{
		  if (! flags.base_after_vec && flags.max_after_vec)
		    {
		      max_addr += align_fuzz (rel_lab, insn, rel_align, ‾0);
		      max_addr += align_fuzz (insn, max_lab, 0, ‾0);
		    }
		  else
		    max_addr += align_fuzz (rel_lab, max_lab, rel_align, ‾0);
		}
	      else
		{
		  if (flags.base_after_vec && ! flags.max_after_vec)
		    {
		      max_addr += align_fuzz (max_lab, insn, 0, 0);
		      max_addr += align_fuzz (insn, rel_lab, 0, 0);
		    }
		  else
		    max_addr += align_fuzz (max_lab, rel_lab, 0, 0);
		}
	      PUT_MODE (body, CASE_VECTOR_SHORTEN_MODE (min_addr - rel_addr,
							max_addr - rel_addr,
							body));
	      if (JUMP_TABLES_IN_TEXT_SECTION
#if !defined(READONLY_DATA_SECTION)
		  || 1
#endif
		  )
		{
		  insn_lengths[uid]
		    = (XVECLEN (body, 1) * GET_MODE_SIZE (GET_MODE (body)));
		  insn_current_address += insn_lengths[uid];
		  if (insn_lengths[uid] != old_length)
		    something_changed = 1;
		}

	      continue;
	    }
#endif /* CASE_VECTOR_SHORTEN_MODE */

	  if (! (varying_length[uid]))
	    {
	      if (GET_CODE (insn) == INSN
		  && GET_CODE (PATTERN (insn)) == SEQUENCE)
		{
		  int i;

		  body = PATTERN (insn);
		  for (i = 0; i < XVECLEN (body, 0); i++)
		    {
		      rtx inner_insn = XVECEXP (body, 0, i);
		      int inner_uid = INSN_UID (inner_insn);

		      INSN_ADDRESSES (inner_uid) = insn_current_address;

		      insn_current_address += insn_lengths[inner_uid];
		    }
                }
	      else
		insn_current_address += insn_lengths[uid];

	      continue;
	    }

	  if (GET_CODE (insn) == INSN && GET_CODE (PATTERN (insn)) == SEQUENCE)
	    {
	      int i;

	      body = PATTERN (insn);
	      new_length = 0;
	      for (i = 0; i < XVECLEN (body, 0); i++)
		{
		  rtx inner_insn = XVECEXP (body, 0, i);
		  int inner_uid = INSN_UID (inner_insn);
		  int inner_length;

		  INSN_ADDRESSES (inner_uid) = insn_current_address;

		  /* insn_current_length returns 0 for insns with a
		     non-varying length.  */
		  if (! varying_length[inner_uid])
		    inner_length = insn_lengths[inner_uid];
		  else
		    inner_length = insn_current_length (inner_insn);

		  if (inner_length != insn_lengths[inner_uid])
		    {
		      insn_lengths[inner_uid] = inner_length;
		      something_changed = 1;
		    }
		  insn_current_address += insn_lengths[inner_uid];
		  new_length += inner_length;
		}
	    }
	  else
	    {
	      new_length = insn_current_length (insn);
	      insn_current_address += new_length;
	    }

#ifdef ADJUST_INSN_LENGTH
	  /* If needed, do any adjustment.  */
	  tmp_length = new_length;
	  ADJUST_INSN_LENGTH (insn, new_length);
	  insn_current_address += (new_length - tmp_length);
#endif

	  if (new_length != insn_lengths[uid])
	    {
	      insn_lengths[uid] = new_length;
	      something_changed = 1;
	    }
	}
      /* For a non-optimizing compile, do only a single pass.  */
      if (!optimize)
	break;
    }

  free (varying_length);

#endif /* HAVE_ATTR_length */
}

#ifdef HAVE_ATTR_length
/* Given the body of an INSN known to be generated by an ASM statement, return
   the number of machine instructions likely to be generated for this insn.
   This is used to compute its length.  */

static int
asm_insn_count (body)
     rtx body;
{
  const char *template;
  int count = 1;

  if (GET_CODE (body) == ASM_INPUT)
    template = XSTR (body, 0);
  else
    template = decode_asm_operands (body, NULL, NULL, NULL, NULL);

  for (; *template; template++)
    if (IS_ASM_LOGICAL_LINE_SEPARATOR (*template) || *template == '¥n')
      count++;

  return count;
}
#endif

/* Output assembler code for the start of a function,
   and initialize some of the variables in this file
   for the new function.  The label for the function and associated
   assembler pseudo-ops have already been output in `assemble_start_function'.

   FIRST is the first insn of the rtl for the function being compiled.
   FILE is the file to write assembler code to.
   OPTIMIZE is nonzero if we should eliminate redundant
     test and compare insns.  */

void
final_start_function (first, file, optimize)
     rtx first;
     FILE *file;
     int optimize ATTRIBUTE_UNUSED;
{
  block_depth = 0;

  this_is_asm_operands = 0;

#ifdef NON_SAVING_SETJMP
  /* A function that calls setjmp should save and restore all the
     call-saved registers on a system where longjmp clobbers them.  */
  if (NON_SAVING_SETJMP && current_function_calls_setjmp)
    {
      int i;

      for (i = 0; i < FIRST_PSEUDO_REGISTER; i++)
	if (!call_used_regs[i])
	  regs_ever_live[i] = 1;
    }
#endif

  if (NOTE_LINE_NUMBER (first) != NOTE_INSN_DELETED)
    notice_source_line (first);
  high_block_linenum = high_function_linenum = last_linenum;

  (*debug_hooks->begin_prologue) (last_linenum, last_filename);

#if defined (DWARF2_UNWIND_INFO) || defined (IA64_UNWIND_INFO)
  if (write_symbols != DWARF2_DEBUG && write_symbols != VMS_AND_DWARF2_DEBUG)
    dwarf2out_begin_prologue (0, NULL);
#endif

#ifdef LEAF_REG_REMAP
  if (current_function_uses_only_leaf_regs)
    leaf_renumber_regs (first);
#endif

  /* The Sun386i and perhaps other machines don't work right
     if the profiling code comes after the prologue.  */
#ifdef PROFILE_BEFORE_PROLOGUE
  if (current_function_profile)
    profile_function (file);
#endif /* PROFILE_BEFORE_PROLOGUE */

#if defined (DWARF2_UNWIND_INFO) && defined (HAVE_prologue)
  if (dwarf2out_do_frame ())
    dwarf2out_frame_debug (NULL_RTX);
#endif

  /* If debugging, assign block numbers to all of the blocks in this
     function.  */
  if (write_symbols)
    {
      remove_unnecessary_notes ();
      reorder_blocks ();
      number_blocks (current_function_decl);
      /* We never actually put out begin/end notes for the top-level
	 block in the function.  But, conceptually, that block is
	 always needed.  */
      TREE_ASM_WRITTEN (DECL_INITIAL (current_function_decl)) = 1;
    }

  /* First output the function prologue: code to set up the stack frame.  */
  (*targetm.asm_out.function_prologue) (file, get_frame_size ());

#ifdef VMS_DEBUGGING_INFO
  /* Output label after the prologue of the function.  */
  if (write_symbols == VMS_DEBUG || write_symbols == VMS_AND_DWARF2_DEBUG)
    vmsdbgout_after_prologue ();
#endif

  /* If the machine represents the prologue as RTL, the profiling code must
     be emitted when NOTE_INSN_PROLOGUE_END is scanned.  */
#ifdef HAVE_prologue
  if (! HAVE_prologue)
#endif
    profile_after_prologue (file);
}

static void
profile_after_prologue (file)
     FILE *file ATTRIBUTE_UNUSED;
{
#ifndef PROFILE_BEFORE_PROLOGUE
  if (current_function_profile)
    profile_function (file);
#endif /* not PROFILE_BEFORE_PROLOGUE */
}

static void
profile_function (file)
     FILE *file ATTRIBUTE_UNUSED;
{
#ifndef NO_PROFILE_COUNTERS
  int align = MIN (BIGGEST_ALIGNMENT, LONG_TYPE_SIZE);
#endif
#if defined(ASM_OUTPUT_REG_PUSH)
#if defined(STRUCT_VALUE_INCOMING_REGNUM) || defined(STRUCT_VALUE_REGNUM)
  int sval = current_function_returns_struct;
#endif
#if defined(STATIC_CHAIN_INCOMING_REGNUM) || defined(STATIC_CHAIN_REGNUM)
  int cxt = current_function_needs_context;
#endif
#endif /* ASM_OUTPUT_REG_PUSH */

#ifndef NO_PROFILE_COUNTERS
  data_section ();
  ASM_OUTPUT_ALIGN (file, floor_log2 (align / BITS_PER_UNIT));
  ASM_OUTPUT_INTERNAL_LABEL (file, "LP", current_function_profile_label_no);
  assemble_integer (const0_rtx, LONG_TYPE_SIZE / BITS_PER_UNIT, align, 1);
#endif

  function_section (current_function_decl);

#if defined(STRUCT_VALUE_INCOMING_REGNUM) && defined(ASM_OUTPUT_REG_PUSH)
  if (sval)
    ASM_OUTPUT_REG_PUSH (file, STRUCT_VALUE_INCOMING_REGNUM);
#else
#if defined(STRUCT_VALUE_REGNUM) && defined(ASM_OUTPUT_REG_PUSH)
  if (sval)
    {
      ASM_OUTPUT_REG_PUSH (file, STRUCT_VALUE_REGNUM);
    }
#endif
#endif

#if defined(STATIC_CHAIN_INCOMING_REGNUM) && defined(ASM_OUTPUT_REG_PUSH)
  if (cxt)
    ASM_OUTPUT_REG_PUSH (file, STATIC_CHAIN_INCOMING_REGNUM);
#else
#if defined(STATIC_CHAIN_REGNUM) && defined(ASM_OUTPUT_REG_PUSH)
  if (cxt)
    {
      ASM_OUTPUT_REG_PUSH (file, STATIC_CHAIN_REGNUM);
    }
#endif
#endif

  FUNCTION_PROFILER (file, current_function_profile_label_no);

#if defined(STATIC_CHAIN_INCOMING_REGNUM) && defined(ASM_OUTPUT_REG_PUSH)
  if (cxt)
    ASM_OUTPUT_REG_POP (file, STATIC_CHAIN_INCOMING_REGNUM);
#else
#if defined(STATIC_CHAIN_REGNUM) && defined(ASM_OUTPUT_REG_PUSH)
  if (cxt)
    {
      ASM_OUTPUT_REG_POP (file, STATIC_CHAIN_REGNUM);
    }
#endif
#endif

#if defined(STRUCT_VALUE_INCOMING_REGNUM) && defined(ASM_OUTPUT_REG_PUSH)
  if (sval)
    ASM_OUTPUT_REG_POP (file, STRUCT_VALUE_INCOMING_REGNUM);
#else
#if defined(STRUCT_VALUE_REGNUM) && defined(ASM_OUTPUT_REG_PUSH)
  if (sval)
    {
      ASM_OUTPUT_REG_POP (file, STRUCT_VALUE_REGNUM);
    }
#endif
#endif
}

/* Output assembler code for the end of a function.
   For clarity, args are same as those of `final_start_function'
   even though not all of them are needed.  */

void
final_end_function ()
{
  app_disable ();

  (*debug_hooks->end_function) (high_function_linenum);

  /* Finally, output the function epilogue:
     code to restore the stack frame and return to the caller.  */
  (*targetm.asm_out.function_epilogue) (asm_out_file, get_frame_size ());

  /* And debug output.  */
  (*debug_hooks->end_epilogue) ();

#if defined (DWARF2_UNWIND_INFO)
  if (write_symbols != DWARF2_DEBUG && write_symbols != VMS_AND_DWARF2_DEBUG
      && dwarf2out_do_frame ())
    dwarf2out_end_epilogue ();
#endif
}

/* Output assembler code for some insns: all or part of a function.
   For description of args, see `final_start_function', above.

   PRESCAN is 1 if we are not really outputting,
     just scanning as if we were outputting.
   Prescanning deletes and rearranges insns just like ordinary output.
   PRESCAN is -2 if we are outputting after having prescanned.
   In this case, don't try to delete or rearrange insns
   because that has already been done.
   Prescanning is done only on certain machines.  */

void
final (first, file, optimize, prescan)
     rtx first;
     FILE *file;
     int optimize;
     int prescan;
{
  rtx insn;
  int max_line = 0;
  int max_uid = 0;

  last_ignored_compare = 0;
  new_block = 1;

  /* Make a map indicating which line numbers appear in this function.
     When producing SDB debugging info, delete troublesome line number
     notes from inlined functions in other files as well as duplicate
     line number notes.  */
#ifdef SDB_DEBUGGING_INFO
  if (write_symbols == SDB_DEBUG)
    {
      rtx last = 0;
      for (insn = first; insn; insn = NEXT_INSN (insn))
	if (GET_CODE (insn) == NOTE && NOTE_LINE_NUMBER (insn) > 0)
	  {
	    if ((RTX_INTEGRATED_P (insn)
		 && strcmp (NOTE_SOURCE_FILE (insn), main_input_filename) != 0)
		 || (last != 0
		     && NOTE_LINE_NUMBER (insn) == NOTE_LINE_NUMBER (last)
		     && NOTE_SOURCE_FILE (insn) == NOTE_SOURCE_FILE (last)))
	      {
		delete_insn (insn);	/* Use delete_note.  */
		continue;
	      }
	    last = insn;
	    if (NOTE_LINE_NUMBER (insn) > max_line)
	      max_line = NOTE_LINE_NUMBER (insn);
	  }
    }
  else
#endif
    {
      for (insn = first; insn; insn = NEXT_INSN (insn))
	if (GET_CODE (insn) == NOTE && NOTE_LINE_NUMBER (insn) > max_line)
	  max_line = NOTE_LINE_NUMBER (insn);
    }

  line_note_exists = (char *) xcalloc (max_line + 1, sizeof (char));

  for (insn = first; insn; insn = NEXT_INSN (insn))
    {
      if (INSN_UID (insn) > max_uid)       /* find largest UID */
	max_uid = INSN_UID (insn);
      if (GET_CODE (insn) == NOTE && NOTE_LINE_NUMBER (insn) > 0)
	line_note_exists[NOTE_LINE_NUMBER (insn)] = 1;
#ifdef HAVE_cc0
      /* If CC tracking across branches is enabled, record the insn which
	 jumps to each branch only reached from one place.  */
      if (optimize && GET_CODE (insn) == JUMP_INSN)
	{
	  rtx lab = JUMP_LABEL (insn);
	  if (lab && LABEL_NUSES (lab) == 1)
	    {
	      LABEL_REFS (lab) = insn;
	    }
	}
#endif
    }

  init_recog ();

  CC_STATUS_INIT;

  /* Output the insns.  */
  for (insn = NEXT_INSN (first); insn;)
    {
#ifdef HAVE_ATTR_length
      if ((unsigned) INSN_UID (insn) >= INSN_ADDRESSES_SIZE ())
	{
#ifdef STACK_REGS
	  /* Irritatingly, the reg-stack pass is creating new instructions
	     and because of REG_DEAD note abuse it has to run after
	     shorten_branches.  Fake address of -1 then.  */
	  insn_current_address = -1;
#else
	  /* This can be triggered by bugs elsewhere in the compiler if
	     new insns are created after init_insn_lengths is called.  */
	  abort ();
#endif
	}
      else
	insn_current_address = INSN_ADDRESSES (INSN_UID (insn));
#endif /* HAVE_ATTR_length */

      insn = final_scan_insn (insn, file, optimize, prescan, 0);
    }

  free (line_note_exists);
  line_note_exists = NULL;
}

const char *
get_insn_template (code, insn)
     int code;
     rtx insn;
{
  const void *output = insn_data[code].output;
  switch (insn_data[code].output_format)
    {
    case INSN_OUTPUT_FORMAT_SINGLE:
      return (const char *) output;
    case INSN_OUTPUT_FORMAT_MULTI:
      return ((const char *const *) output)[which_alternative];
    case INSN_OUTPUT_FORMAT_FUNCTION:
      if (insn == NULL)
	abort ();
      return (*(insn_output_fn) output) (recog_data.operand, insn);

    default:
      abort ();
    }
}

/* The final scan for one insn, INSN.
   Args are same as in `final', except that INSN
   is the insn being scanned.
   Value returned is the next insn to be scanned.

   NOPEEPHOLES is the flag to disallow peephole processing (currently
   used for within delayed branch sequence output).  */

rtx
final_scan_insn (insn, file, optimize, prescan, nopeepholes)
     rtx insn;
     FILE *file;
     int optimize ATTRIBUTE_UNUSED;
     int prescan;
     int nopeepholes ATTRIBUTE_UNUSED;
{
#ifdef HAVE_cc0
  rtx set;
#endif

  insn_counter++;

  /* Ignore deleted insns.  These can occur when we split insns (due to a
     template of "#") while not optimizing.  */
  if (INSN_DELETED_P (insn))
    return NEXT_INSN (insn);

  switch (GET_CODE (insn))
    {
    case NOTE:
      if (prescan > 0)
	break;

      switch (NOTE_LINE_NUMBER (insn))
	{
	case NOTE_INSN_DELETED:
	case NOTE_INSN_LOOP_BEG:
	case NOTE_INSN_LOOP_END:
	case NOTE_INSN_LOOP_END_TOP_COND:
	case NOTE_INSN_LOOP_CONT:
	case NOTE_INSN_LOOP_VTOP:
	case NOTE_INSN_FUNCTION_END:
	case NOTE_INSN_REPEATED_LINE_NUMBER:
	case NOTE_INSN_RANGE_BEG:
	case NOTE_INSN_RANGE_END:
	case NOTE_INSN_LIVE:
	case NOTE_INSN_EXPECTED_VALUE:
	  break;

	case NOTE_INSN_BASIC_BLOCK:
#ifdef IA64_UNWIND_INFO
	  IA64_UNWIND_EMIT (asm_out_file, insn);
#endif
	  if (flag_debug_asm)
	    fprintf (asm_out_file, "¥t%s basic block %d¥n",
		     ASM_COMMENT_START, NOTE_BASIC_BLOCK (insn)->index);
	  break;

	case NOTE_INSN_EH_REGION_BEG:
	  ASM_OUTPUT_DEBUG_LABEL (asm_out_file, "LEHB",
				  NOTE_EH_HANDLER (insn));
	  break;

	case NOTE_INSN_EH_REGION_END:
	  ASM_OUTPUT_DEBUG_LABEL (asm_out_file, "LEHE",
				  NOTE_EH_HANDLER (insn));
	  break;

	case NOTE_INSN_PROLOGUE_END:
	  (*targetm.asm_out.function_end_prologue) (file);
	  profile_after_prologue (file);
	  break;

	case NOTE_INSN_EPILOGUE_BEG:
	  (*targetm.asm_out.function_begin_epilogue) (file);
	  break;

	case NOTE_INSN_FUNCTION_BEG:
	  app_disable ();
	  (*debug_hooks->end_prologue) (last_linenum);
	  break;

	case NOTE_INSN_BLOCK_BEG:
	  if (debug_info_level == DINFO_LEVEL_NORMAL
	      || debug_info_level == DINFO_LEVEL_VERBOSE
	      || write_symbols == DWARF_DEBUG
	      || write_symbols == DWARF2_DEBUG
	      || write_symbols == VMS_AND_DWARF2_DEBUG
	      || write_symbols == VMS_DEBUG)
	    {
	      int n = BLOCK_NUMBER (NOTE_BLOCK (insn));

	      app_disable ();
	      ++block_depth;
	      high_block_linenum = last_linenum;

	      /* Output debugging info about the symbol-block beginning.  */
	      (*debug_hooks->begin_block) (last_linenum, n);

	      /* Mark this block as output.  */
	      TREE_ASM_WRITTEN (NOTE_BLOCK (insn)) = 1;
	    }
	  break;

	case NOTE_INSN_BLOCK_END:
	  if (debug_info_level == DINFO_LEVEL_NORMAL
	      || debug_info_level == DINFO_LEVEL_VERBOSE
	      || write_symbols == DWARF_DEBUG
	      || write_symbols == DWARF2_DEBUG
	      || write_symbols == VMS_AND_DWARF2_DEBUG
	      || write_symbols == VMS_DEBUG)
	    {
	      int n = BLOCK_NUMBER (NOTE_BLOCK (insn));

	      app_disable ();

	      /* End of a symbol-block.  */
	      --block_depth;
	      if (block_depth < 0)
		abort ();

	      (*debug_hooks->end_block) (high_block_linenum, n);
	    }
	  break;

	case NOTE_INSN_DELETED_LABEL:
	  /* Emit the label.  We may have deleted the CODE_LABEL because
	     the label could be proved to be unreachable, though still
	     referenced (in the form of having its address taken.  */
	  ASM_OUTPUT_DEBUG_LABEL (file, "L", CODE_LABEL_NUMBER (insn));
	  break;

	case 0:
	  break;

	default:
	  if (NOTE_LINE_NUMBER (insn) <= 0)
	    abort ();

	  /* This note is a line-number.  */
	  {
	    rtx note;
	    int note_after = 0;

	    /* If there is anything real after this note, output it.
	       If another line note follows, omit this one.  */
	    for (note = NEXT_INSN (insn); note; note = NEXT_INSN (note))
	      {
		if (GET_CODE (note) != NOTE && GET_CODE (note) != CODE_LABEL)
		  break;

		/* These types of notes can be significant
		   so make sure the preceding line number stays.  */
		else if (GET_CODE (note) == NOTE
			 && (NOTE_LINE_NUMBER (note) == NOTE_INSN_BLOCK_BEG
			     || NOTE_LINE_NUMBER (note) == NOTE_INSN_BLOCK_END
			     || NOTE_LINE_NUMBER (note) == NOTE_INSN_FUNCTION_BEG))
		  break;
		else if (GET_CODE (note) == NOTE && NOTE_LINE_NUMBER (note) > 0)
		  {
		    /* Another line note follows; we can delete this note
		       if no intervening line numbers have notes elsewhere.  */
		    int num;
		    for (num = NOTE_LINE_NUMBER (insn) + 1;
		         num < NOTE_LINE_NUMBER (note);
		         num++)
		      if (line_note_exists[num])
			break;

		    if (num >= NOTE_LINE_NUMBER (note))
		      note_after = 1;
		    break;
		  }
	      }

	    /* Output this line note if it is the first or the last line
	       note in a row.  */
	    if (!note_after)
	      {
		notice_source_line (insn);
		(*debug_hooks->source_line) (last_linenum, last_filename);
	      }
	  }
	  break;
	}
      break;

    case BARRIER:
#if defined (DWARF2_UNWIND_INFO)
      if (dwarf2out_do_frame ())
	dwarf2out_frame_debug (insn);
#endif
      break;

    case CODE_LABEL:
      /* The target port might emit labels in the output function for
	 some insn, e.g. sh.c output_branchy_insn.  */
      if (CODE_LABEL_NUMBER (insn) <= max_labelno)
	{
	  int align = LABEL_TO_ALIGNMENT (insn);
#ifdef ASM_OUTPUT_MAX_SKIP_ALIGN
	  int max_skip = LABEL_TO_MAX_SKIP (insn);
#endif

	  if (align && NEXT_INSN (insn))
	    {
#ifdef ASM_OUTPUT_MAX_SKIP_ALIGN
	      ASM_OUTPUT_MAX_SKIP_ALIGN (file, align, max_skip);
#else
	      ASM_OUTPUT_ALIGN (file, align);
#endif
	    }
	}
#ifdef HAVE_cc0
      CC_STATUS_INIT;
      /* If this label is reached from only one place, set the condition
	 codes from the instruction just before the branch.  */

      /* Disabled because some insns set cc_status in the C output code
	 and NOTICE_UPDATE_CC alone can set incorrect status.  */
      if (0 /* optimize && LABEL_NUSES (insn) == 1*/)
	{
	  rtx jump = LABEL_REFS (insn);
	  rtx barrier = prev_nonnote_insn (insn);
	  rtx prev;
	  /* If the LABEL_REFS field of this label has been set to point
	     at a branch, the predecessor of the branch is a regular
	     insn, and that branch is the only way to reach this label,
	     set the condition codes based on the branch and its
	     p