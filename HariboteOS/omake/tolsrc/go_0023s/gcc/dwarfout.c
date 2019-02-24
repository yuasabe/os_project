/* Output Dwarf format symbol table information from the GNU C compiler.
   Copyright (C) 1992, 1993, 1995, 1996, 1997, 1998,
   1999, 2000, 2001 Free Software Foundation, Inc.
   Contributed by Ron Guilmette (rfg@monkeys.com) of Network Computing Devices.

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

/*

 Notes on the GNU Implementation of DWARF Debugging Information
 --------------------------------------------------------------
 Last Major Update: Sun Jul 17 08:17:42 PDT 1994 by rfg@segfault.us.com
 ------------------------------------------------------------

 This file describes special and unique aspects of the GNU implementation of
 the DWARF Version 1 debugging information language, as provided in the GNU
 version 2.x compiler(s).

 For general information about the DWARF debugging information language,
 you should obtain the DWARF version 1.1 specification document (and perhaps
 also the DWARF version 2 draft specification document) developed by the
 (now defunct) UNIX International Programming Languages Special Interest Group.

 To obtain a copy of the DWARF Version 1 and/or DWARF Version 2
 specification, visit the web page for the DWARF Version 2 committee, at

   http://www.eagercon.com/dwarf/dwarf2std.htm

 The generation of DWARF debugging information by the GNU version 2.x C
 compiler has now been tested rather extensively for m88k, i386, i860, and
 Sparc targets.  The DWARF output of the GNU C compiler appears to inter-
 operate well with the standard SVR4 SDB debugger on these kinds of target
 systems (but of course, there are no guarantees).

 DWARF 1 generation for the GNU g++ compiler is implemented, but limited.
 C++ users should definitely use DWARF 2 instead.

 Future plans for the dwarfout.c module of the GNU compiler(s) includes the
 addition of full support for GNU FORTRAN.  (This should, in theory, be a
 lot simpler to add than adding support for g++... but we'll see.)

 Many features of the DWARF version 2 specification have been adapted to
 (and used in) the GNU implementation of DWARF (version 1).  In most of
 these cases, a DWARF version 2 approach is used in place of (or in addition
 to) DWARF version 1 stuff simply because it is apparent that DWARF version
 1 is not sufficiently expressive to provide the kinds of information which
 may be necessary to support really robust debugging.  In all of these cases
 however, the use of DWARF version 2 features should not interfere in any
 way with the interoperability (of GNU compilers) with generally available
 "classic" (pre version 1) DWARF consumer tools (e.g. SVR4 SDB).

 The DWARF generation enhancement for the GNU compiler(s) was initially
 donated to the Free Software Foundation by Network Computing Devices.
 (Thanks NCD!) Additional development and maintenance of dwarfout.c has
 been largely supported (i.e. funded) by Intel Corporation.  (Thanks Intel!)

 If you have questions or comments about the DWARF generation feature, please
 send mail to me <rfg@netcom.com>.  I will be happy to investigate any bugs
 reported and I may even provide fixes (but of course, I can make no promises).

 The DWARF debugging information produced by GCC may deviate in a few minor
 (but perhaps significant) respects from the DWARF debugging information
 currently produced by other C compilers.  A serious attempt has been made
 however to conform to the published specifications, to existing practice,
 and to generally accepted norms in the GNU implementation of DWARF.

     ** IMPORTANT NOTE **    ** IMPORTANT NOTE **    ** IMPORTANT NOTE **

 Under normal circumstances, the DWARF information generated by the GNU
 compilers (in an assembly language file) is essentially impossible for
 a human being to read.  This fact can make it very difficult to debug
 certain DWARF-related problems.  In order to overcome this difficulty,
 a feature has been added to dwarfout.c (enabled by the -dA
 option) which causes additional comments to be placed into the assembly
 language output file, out to the right-hand side of most bits of DWARF
 material.  The comments indicate (far more clearly that the obscure
 DWARF hex codes do) what is actually being encoded in DWARF.  Thus, the
 -dA option can be highly useful for those who must study the
 DWARF output from the GNU compilers in detail.

 ---------

 (Footnote: Within this file, the term `Debugging Information Entry' will
 be abbreviated as `DIE'.)


 Release Notes  (aka known bugs)
 -------------------------------

 In one very obscure case involving dynamically sized arrays, the DWARF
 "location information" for such an array may make it appear that the
 array has been totally optimized out of existence, when in fact it
 *must* actually exist.  (This only happens when you are using *both* -g
 *and* -O.)  This is due to aggressive dead store elimination in the
 compiler, and to the fact that the DECL_RTL expressions associated with
 variables are not always updated to correctly reflect the effects of
 GCC's aggressive dead store elimination.

 -------------------------------

 When attempting to set a breakpoint at the "start" of a function compiled
 with -g1, the debugger currently has no way of knowing exactly where the
 end of the prologue code for the function is.  Thus, for most targets,
 all the debugger can do is to set the breakpoint at the AT_low_pc address
 for the function.  But if you stop there and then try to look at one or
 more of the formal parameter values, they may not have been "homed" yet,
 so you may get inaccurate answers (or perhaps even addressing errors).

 Some people may consider this simply a non-feature, but I consider it a
 bug, and I hope to provide some GNU-specific attributes (on function
 DIEs) which will specify the address of the end of the prologue and the
 address of the beginning of the epilogue in a future release.

 -------------------------------

 It is believed at this time that old bugs relating to the AT_bit_offset
 values for bit-fields have been fixed.

 There may still be some very obscure bugs relating to the DWARF description
 of type `long long' bit-fields for target machines (e.g. 80x86 machines)
 where the alignment of type `long long' data objects is different from
 (and less than) the size of a type `long long' data object.

 Please report any problems with the DWARF description of bit-fields as you
 would any other GCC bug.  (Procedures for bug reporting are given in the
 GNU C compiler manual.)

 --------------------------------

 At this time, GCC does not know how to handle the GNU C "nested functions"
 extension.  (See the GCC manual for more info on this extension to ANSI C.)

 --------------------------------

 The GNU compilers now represent inline functions (and inlined instances
 thereof) in exactly the manner described by the current DWARF version 2
 (draft) specification.  The version 1 specification for handling inline
 functions (and inlined instances) was known to be brain-damaged (by the
 PLSIG) when the version 1 spec was finalized, but it was simply too late
 in the cycle to get it removed before the version 1 spec was formally
 released to the public (by UI).

 --------------------------------

 At this time, GCC does not generate the kind of really precise information
 about the exact declared types of entities with signed integral types which
 is required by the current DWARF draft specification.

 Specifically, the current DWARF draft specification seems to require that
 the type of an non-unsigned integral bit-field member of a struct or union
 type be represented as either a "signed" type or as a "plain" type,
 depending upon the exact set of keywords that were used in the
 type specification for the given bit-field member.  It was felt (by the
 UI/PLSIG) that this distinction between "plain" and "signed" integral types
 could have some significance (in the case of bit-fields) because ANSI C
 does not constrain the signedness of a plain bit-field, whereas it does
 constrain the signedness of an explicitly "signed" bit-field.  For this
 reason, the current DWARF specification calls for compilers to produce
 type information (for *all* integral typed entities... not just bit-fields)
 which explicitly indicates the signedness of the relevant type to be
 "signed" or "plain" or "unsigned".

 Unfortunately, the GNU DWARF implementation is currently incapable of making
 such distinctions.

 --------------------------------


 Known Interoperability Problems
 -------------------------------

 Although the GNU implementation of DWARF conforms (for the most part) with
 the current UI/PLSIG DWARF version 1 specification (with many compatible
 version 2 features added in as "vendor specific extensions" just for good
 measure) there are a few known cases where GCC's DWARF output can cause
 some confusion for "classic" (pre version 1) DWARF consumers such as the
 System V Release 4 SDB debugger.  These cases are described in this section.

 --------------------------------

 The DWARF version 1 specification includes the fundamental type codes
 FT_ext_prec_float, FT_complex, FT_dbl_prec_complex, and FT_ext_prec_complex.
 Since GNU C is only a C compiler (and since C doesn't provide any "complex"
 data types) the only one of these fundamental type codes which GCC ever
 generates is FT_ext_prec_float.  This fundamental type code is generated
 by GCC for the `long double' data type.  Unfortunately, due to an apparent
 bug in the SVR4 SDB debugger, SDB can become very confused wherever any
 attempt is made to print a variable, parameter, or field whose type was
 given in terms of FT_ext_prec_float.

 (Actually, SVR4 SDB fails to understand *any* of the four fundamental type
 codes mentioned here.  This will fact will cause additional problems when
 there is a GNU FORTRAN front-end.)

 --------------------------------

 In general, it appears that SVR4 SDB is not able to effectively ignore
 fundamental type codes in the "implementation defined" range.  This can
 cause problems when a program being debugged uses the `long long' data
 type (or the signed or unsigned varieties thereof) because these types
 are not defined by ANSI C, and thus, GCC must use its own private fundamental
 type codes (from the implementation-defined range) to represent these types.

 --------------------------------


 General GNU DWARF extensions
 ----------------------------

 In the current DWARF version 1 specification, no mechanism is specified by
 which accurate information about executable code from include files can be
 properly (and fully) described.  (The DWARF version 2 specification *does*
 specify such a mechanism, but it is about 10 times more complicated than
 it needs to be so I'm not terribly anxious to try to implement it right
 away.)

 In the GNU implementation of DWARF version 1, a fully downward-compatible
 extension has been implemented which permits the GNU compilers to specify
 which executable lines come from which files.  This extension places
 additional information (about source file names) in GNU-specific sections
 (which should be totally ignored by all non-GNU DWARF consumers) so that
 this extended information can be provided (to GNU DWARF consumers) in a way
 which is totally transparent (and invisible) to non-GNU DWARF consumers
 (e.g. the SVR4 SDB debugger).  The additional information is placed *only*
 in specialized GNU-specific sections, where it should never even be seen
 by non-GNU DWARF consumers.

 To understand this GNU DWARF extension, imagine that the sequence of entries
 in the .lines section is broken up into several subsections.  Each contiguous
 sequence of .line entries which relates to a sequence of lines (or statements)
 from one particular file (either a `base' file or an `include' file) could
 be called a `line entries chunk' (LEC).

 For each LEC there is one entry in the .debug_srcinfo section.

 Each normal entry in the .debug_srcinfo section consists of two 4-byte
 words of data as follows:

	 (1)	The starting address (relative to the entire .line section)
		 of the first .line entry in the relevant LEC.

	 (2)	The starting address (relative to the entire .debug_sfnames
		 section) of a NUL terminated string representing the
		 relevant filename.  (This filename name be either a
		 relative or an absolute filename, depending upon how the
		 given source file was located during compilation.)

 Obviously, each .debug_srcinfo entry allows you to find the relevant filename,
 and it also points you to the first .line entry that was generated as a result
 of having compiled a given source line from the given source file.

 Each subsequent .line entry should also be assumed to have been produced
 as a result of compiling yet more lines from the same file.  The end of
 any given LEC is easily found by looking at the first 4-byte pointer in
 the *next* .debug_srcinfo entry.  That next .debug_srcinfo entry points
 to a new and different LEC, so the preceding LEC (implicitly) must have
 ended with the last .line section entry which occurs at the 2 1/2 words
 just before the address given in the first pointer of the new .debug_srcinfo
 entry.

 The following picture may help to clarify this feature.  Let's assume that
 `LE' stands for `.line entry'.  Also, assume that `* 'stands for a pointer.


	 .line section	   .debug_srcinfo section     .debug_sfnames section
	 ----------------------------------------------------------------

	 LE  <---------------------- *
	 LE			    * -----------------> "foobar.c" <---
	 LE								|
	 LE								|
	 LE  <---------------------- *					|
	 LE			    * -----------------> "foobar.h" <|	|
	 LE							     |	|
	 LE							     |	|
	 LE  <---------------------- *				     |	|
	 LE			    * ----------------->  "inner.h"  |	|
	 LE							     |	|
	 LE  <---------------------- *				     |	|
	 LE			    * -------------------------------	|
	 LE								|
	 LE								|
	 LE								|
	 LE								|
	 LE  <---------------------- *					|
	 LE			    * -----------------------------------
	 LE
	 LE
	 LE

 In effect, each entry in the .debug_srcinfo section points to *both* a
 filename (in the .debug_sfnames section) and to the start of a block of
 consecutive LEs (in the .line section).

 Note that just like in the .line section, there are specialized first and
 last entries in the .debug_srcinfo section for each object file.  These
 special first and last entries for the .debug_srcinfo section are very
 different from the normal .debug_srcinfo section entries.  They provide
 additional information which may be helpful to a debugger when it is
 interpreting the data in the .debug_srcinfo, .debug_sfnames, and .line
 sections.

 The first entry in the .debug_srcinfo section for each compilation unit
 consists of five 4-byte words of data.  The contents of these five words
 should be interpreted (by debuggers) as follows:

	 (1)	The starting address (relative to the entire .line section)
		 of the .line section for this compilation unit.

	 (2)	The starting address (relative to the entire .debug_sfnames
		 section) of the .debug_sfnames section for this compilation
		 unit.

	 (3)	The starting address (in the execution virtual address space)
		 of the .text section for this compilation unit.

	 (4)	The ending address plus one (in the execution virtual address
		 space) of the .text section for this compilation unit.

	 (5)	The d