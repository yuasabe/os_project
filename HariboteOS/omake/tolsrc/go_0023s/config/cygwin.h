/* Operating system specific defines to be used when targeting GCC for
   hosting on Windows32, using a Unix style C library and tools.
   Copyright (C) 1995, 1996, 1997, 1998, 1999, 2000, 2001, 2002
   Free Software Foundation, Inc.

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
#define READONLY_DATA_SECTION() data_section ()

#undef CPP_PREDEFINES
#define CPP_PREDEFINES "-D_X86_=1 -Asystem=winnt"
/* Support the __declspec keyword by turning them into attributes.
   We currently only support: dllimport and dllexport.
   Note that the current way we do this may result in a collision with
   predefined attributes later on.  This can be solved by using one attribute,
   say __declspec__, and passing args to it.  The problem with that approach
   is that args are not accumulated: each new appearance would clobber any
   existing args.  */

#undef CPP_SPEC
#define CPP_SPEC "%(cpp_cpu) %{posix:-D_POSIX_SOURCE} ¥
  -D__stdcall=__attribute__((__stdcall__)) ¥
  -D__fastcall=__attribute__((__fastcall__)) ¥
  -D__cdecl=__attribute__((__cdecl__)) ¥
  %{!ansi:-D_stdcall=__attribute__((__stdcall__)) ¥
    -D_fastcall=__attribute__((__fastcall__)) ¥
    -D_cdecl=__attribute__((__cdecl__))} ¥
  -D__declspec(x)=__attribute__((x)) ¥
  -D__i386__ -D__i386 ¥
  %{mno-win32:%{mno-cygwin: %emno-cygwin and mno-win32 are not compatible}} ¥
  %{mno-cygwin:-D__MSVCRT__ -D__MINGW32__ %{!ansi:%{mthreads:-D_MT}}}¥
  %{!mno-cygwin:-D__CYGWIN32__ -D__CYGWIN__ %{!ansi:-Dunix} -D__unix__ -D__unix }¥
  %{mwin32|mno-cygwin:-DWIN32 -D_WIN32 -D__WIN32 -D__WIN32__ %{!ansi:-DWINNT}}¥
  %{!mno-win32|mno-cygwin:-isystem ../include/w32api%s -isystem ../../include/w32api%s}¥
"

#undef STARTFILE_SPEC
#define STARTFILE_SPEC "¥
  %{shared|mdll: %{mno-cygwin:dllcrt2%O%s}}¥
  %{!shared: %{!mdll: %{!mno-cygwin:crt0%O%s} %{mno-cygwin:crt2%O%s}¥
  %{pg:gcrt0%O%s}}} crtbegin%O%s"

#undef ENDFILE_SPEC
#define ENDFILE_SPEC "crtend%O%s"

/* Normally, -lgcc is not needed since everything in it is in the DLL, but we
   want to allow things to be added to it when installing new versions of
   GCC without making a new CYGWIN.DLL, so we leave it.  Profiling is handled
   by calling the init function from the prologue.  */

#undef LIBGCC_SPEC
#define LIBGCC_SPEC ¥
  "%{mno-cygwin: %{mthreads:-lmingwthrd} -lmingw32} -lgcc %{mno-cygwin:-lmoldname -lmingwex -lmsvcrt}"

/* This macro defines names of additional specifications to put in the specs
   that can be used in various specifications like CC1_SPEC.  Its definition
   is an initializer with a subgrouping for each command option.

   Each subgrouping contains a string constant, that defines the
   specification name, and a string constant that used by the GNU CC driver
   program.

   Do not define this macro if it does not need to do anything.  */

#undef  SUBTARGET_EXTRA_SPECS
#define SUBTARGET_EXTRA_SPECS ¥
  { "mingw_include_path", DEFAULT_TARGET_MACHINE }

/* We have to dynamic link to get to the system DLLs.  All of libc, libm and
   the Unix stuff is in cygwin.dll.  The import library is called
   'libcygwin.a'.  For Windows applications, include more libraries, but
   always include kernel32.  We'd like to specific subsystem windows to
   ld, but that doesn't work just yet.  */

#undef LIB_SPEC
#define LIB_SPEC "¥
  %{pg:-lgmon} ¥
  %{!mno-cygwin:-lcygwin} ¥
  %{mno-cygwin:%{mth