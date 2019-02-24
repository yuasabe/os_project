// LzmaAlone.cpp

#include "StdAfx.h"

#ifdef WIN32
#include <initguid.h>
#else
#define INITGUID
#endif

#include "../../../Common/MyWindows.h"

// #include <limits.h>
#include <stdio.h>

#if defined(WIN32) || defined(OS2) || defined(MSDOS)
#include <fcntl.h>
#include <io.h>
#define MY_SET_BINARY_MODE(file) setmode(fileno(file),O_BINARY)
#else
#define MY_SET_BINARY_MODE(file)
#endif

#include "../../../Common/CommandLineParser.h"
#include "../../../Common/StringConvert.h"
#include "../../../Common/StringToInt.h"

// #include "Windows/PropVariant.h"

#include "../../Common/FileStreams.h"

#include "../LZMA/LZMADecoder.h"
#include "../LZMA/LZMAEncoder.h"

#include "LzmaBench.h"

using namespace NCommandLineParser;

namespace NKey {
enum Enum
{
  kHelp1 = 0,
  kHelp2,
  kMode,
  kDictionary,
  kFastBytes,
  kLitContext,
  kLitPos,
  kPosBits,
  kMatchFinder,
  kEOS,
  kStdIn,
  kStdOut
};
}

static const CSwitchForm kSwitchForms[] = 
{
  { L"?",  NSwitchType::kSimple, false },
  { L"H",  NSwitchType::kSimple, false },
  { L"A", NSwitchType::kUnLimitedPostString, false, 1 },
  { L"D", NSwitchType::kUnLimitedPostString, false, 1 },
  { L"FB", NSwitchType::kUnLimitedPostString, false, 1 },
  { L"LC", NSwitchType::kUnLimitedPostString, false, 1 },
  { L"LP", NSwitchType::kUnLimitedPostString, false, 1 },
  { L"PB", NSwitchType::kUnLimitedPostString, false, 1 },
  { L"MF", NSwitchType::kUnLimitedPostString, false, 1 },
  { L"EOS", NSwitchType::kSimple, false },
  { L"SI",  NSwitchType::kSimple, false },
  { L"SO",  NSwitchType::kSimple, false }
};

static const int kNumSwitches = sizeof(kSwitchForms) / sizeof(kSwitchForms[0]);

static void PrintHelp()
{
  fprintf(stderr, "¥nUsage:  t5lzma <e|d> inputFile outputFile [<switches>...]¥n"
             "  e: encode file¥n"
             "  d: decode file¥n"
             "  b: Benchmark¥n"
    "<Switches>¥n"
    "  -a{N}:  set compression mode - [0, 2], default: 2 (max)¥n"
    "  -d{N}:  set dictionary - [0,28], default: 23 (8MB)¥n"
    "  -fb{N}: set number of fast bytes - [5, 255], default: 128¥n"
    "  -lc{N}: set number of literal context bits - [0, 8], default: 0¥n"
    "  -lp{N}: set number of literal pos bits - [0, 4], default: 0¥n"
    "  -pb{N}: set number of pos bits - [0, 4], default: 0¥n"
    "  -mf{MF_ID}: set Match Finder: [bt2, bt3, bt4, bt4b, pat2r, pat2,¥n"
    "              pat2h, pat3h, pat4h, hc3, hc4], default: bt4¥n"
    "  -eos:   write End Of Stream marker¥n"
    "  -si:    Read data from stdin¥n"
    "  -so:    Write data to stdout¥n"
    );
}

static void PrintHelpAndExit(const char *s)
{
  fprintf(stderr, "¥nError: %s¥n¥n", s);
  PrintHelp();
  throw -1;
}

static void IncorrectCommand()
{
  PrintHelpAndExit("Incorrect command");
}

static void WriteArgumentsToStringList(int numArguments, const char *arguments[], 
    UStringVector &strings)
{
  for(int i = 1; i < numArguments; i++)
    strings.Add(MultiByteToUnicodeString(arguments[i]));
}

static bool GetNumber(const wchar_t *s, UInt32 &value)
{
  value = 0;
  if (MyStringLen(s) == 0)
    return false;
  const wchar_t *end;
  UInt64 res = ConvertStringToUINT64(s, &end);
  if (*end != L'¥0')
    return false;
  if (res > 0xFFFFFFFF)
    return false;
  value = UInt32(res);
  return true;
}

int main2(int n, const char *args[])
{
	if (strcmp(args[n - 1], "-notitle") == 0)
		n--; 
	else {
		fprintf(stderr,
			"¥nt5lzma 1.00 Copyright (C) 2004 H.Kawai¥n"
			" --- based : LZMA 4.03 Copyright (c) 1999-2004 Igor Pavlov  2004-06-18¥n"
		);
	}

  if (n == 1)
  {
    PrintHelp();
    return 0;
  }

  if (sizeof(Byte) != 1 || sizeof(UInt32) < 4 || sizeof(UInt64) < 4)
  {
    fprintf(stderr, "Unsupported base types. Edit Common/Types.h and recompile");
    return 1;
  }   

  UStringVector commandStrings;
  WriteArgumentsToStringList(n, args, commandStrings);
  CParser parser(kNumSwitches);
  try
  {
    parser.ParseStrings(kSwitchForms, commandStrings);
  }
  c