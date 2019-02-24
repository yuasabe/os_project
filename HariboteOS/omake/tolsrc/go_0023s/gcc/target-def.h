/* Default initializers for a generic GCC target.
   Copyright (C) 2001, 2002 Free Software Foundation, Inc.

This program is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 2, or (at your option) any
later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

 In other words, you are welcome to use, share and improve this program.
 You are forbidden to forbid anyone else to use, share and improve
 what you give them.   Help stamp out software-hoarding!  */

/* See target.h for a description of what this file contains and how to
   use it.

   We want to have non-NULL default definitions of all hook functions,
   even if they do nothing.  */

/* Note that if one of these macros must be defined in an OS .h file
   rather than the .c file, then we need to wrap the default
   definition in a #ifndef, since files include tm.h before this one.  */

/* Assembler output.  */
#define TARGET_ASM_OPEN_PAREN "("
#define TARGET_ASM_CLOSE_PAREN ")"
#define TARGET_ASM_BYTE_OP "¥t.byte¥t"

#define TARGET_ASM_ALIGNED_HI_OP "¥t.short¥t"
#define TARGET_ASM_ALIGNED_SI_OP "¥t.long¥t"
#define TARGET_ASM_ALIGNED_DI_OP NULL
#define TARGET_ASM_ALIGNED_TI_OP NULL

/* GAS and SYSV4 assemblers accept these.  */
#if defined (OBJECT_FORMAT_ELF) || defined (OBJECT_FORMAT_ROSE)
#define TARGET_ASM_UNALIGNED_HI_OP "¥t.2byte¥t"
#define TARGET_ASM_UNALIGNED_SI_OP "¥t.4byte¥t"
#define TARGET_ASM_UNALIGNED_DI_OP "¥t.8byte¥t"
#define TARGET_ASM_UNALIGNED_TI_OP NULL
#else
#define TARGET_ASM_UNALIGNED_HI_OP NULL
#define TARGET_ASM_UNALIGNED_SI_OP NULL
#define TARGET_ASM_UNALIGNED_DI_OP NULL
#define TARGET_ASM_UNALIGNED_TI_OP NULL
#endif /* OBJECT_FORMAT_ELF || OBJECT_FORMAT_ROSE */

#define TARGET_ASM_INTEGER default_assemble_integer

#define TARGET_ASM_FUNCTION_PROLOGUE default_function_pro_epilogue
#define TARGET_ASM_FUNCTION_EPILOGUE default_function_pro_epilogue
#define TARGET_ASM_FUNCTION_END_PROLOGUE no_asm_to_stream
#define TARGET_ASM_FUNCTION_BEGIN_EPILOGUE no_asm_to_stream

#if !defined(TARGET_ASM_CONSTRUCTOR) && !defined(USE_COLLECT2)
# ifdef CTORS_SECTION_ASM_OP
#  define TARGET_ASM_CONSTRUCTOR default_ctor_section_asm_out_constructor
# else
#  ifdef TARGET_ASM_NAMED_SECTION
#   define TARGET_ASM_CONSTRUCTOR default_named_section_asm_out_constructor
#  else
#   define TARGET_ASM_CONSTRUCTOR default_stabs_asm_out_constructor
#  endif
# endif
#endif

#if !defined(TARGET_ASM_DESTRUCTOR) && !defined(USE_COLLECT2)
# ifdef DTORS_SECTION_ASM_OP
#  define TARGET_ASM_DESTRUCTOR default_dtor_section_asm_out_destructor
# else
#  ifdef TARGET_ASM_NAMED_SECTION
#   define TARGET_ASM_DESTRUCTOR default_named_section_asm_out_destructor
#  else
#   define TARGET_ASM_DESTRUCTOR default_stabs_asm_out_destructor
#  endif
# endif
#endif

#if defined(TARGET_ASM_CONSTRUCTOR) && defined(TARGET_ASM_DESTRUCTOR)
#define TARGET_HAVE_CTORS_DTORS true
#else
#define TARGET_HAVE_CTORS_DTORS false
#define TARGET_ASM_CONSTRUCTOR NULL
#define TARGET_ASM_DESTRUCTOR NULL
#endif

#ifdef TARGET_ASM_NAMED_SECTION
#define TARGET_HAVE_NAMED_SECTIONS true
#else
#define TARGET_ASM_NAMED_SECTION default_no_named_section
#define TARGET_HAVE_NAMED_SECTIONS false
#endif

#ifndef TARGET_ASM_EXCEPTION_SECTION
#define TARGET_ASM_EXCEPTION_SECTION default_exception_section
#endif

#ifndef TARGET_ASM_EH_FRAME_SECTION
#define TARGET_ASM_EH_FRAME_SECTION default_eh_frame_section
#endif

#define TARGET_ASM_ALIGNED_INT_OP				¥
		       {TARGET_ASM_ALIGNED_HI_OP,		¥
			TARGET_ASM_ALIGNED_SI_OP,		¥
		