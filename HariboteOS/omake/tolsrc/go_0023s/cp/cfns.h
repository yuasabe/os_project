/* C code produced by gperf version 2.7 */
/* Command-line: gperf -o -C -E -k 1-6,$ -j1 -D -N libc_name_p ../../../egcs-CVS20000404/gcc/cp/cfns.gperf  */
#ifdef __GNUC__
__inline
#endif
static unsigned int hash PARAMS ((const char *, unsigned int));
#ifdef __GNUC__
__inline
#endif
const char * libc_name_p PARAMS ((const char *, unsigned int));
/* maximum key range = 1020, duplicates = 1 */

#ifdef __GNUC__
__inline
#endif
static unsigned int
hash (str, len)
     register const char *str;
     register unsigned int len;
{
  static const unsigned short asso_values[] =
    {
      1038, 1038, 1038, 1038, 1038, 1038, 1038, 1038, 1038, 1038,
      1038, 1038, 1038, 1038, 1038, 1038, 1038, 1038, 1038, 1038,
      1038, 1038, 1038, 1038, 1038, 1038, 1038, 1038, 1038, 1038,
      1038, 1038, 1038, 1038, 1038, 1038, 1038, 1038, 1038, 1038,
      1038, 1038, 1038, 1038, 1038, 1038, 1038, 1038,    0,    1,
         0, 1038, 1038, 1038, 1038, 1038, 1038, 1038, 1038, 1038,
      1038, 1038, 1038, 1038, 1038, 1038, 1038, 1038, 1038, 1038,
      1038, 1038, 1038, 1038, 1038, 1038, 1038, 1038, 1038, 1038,
      1038, 1038, 1038, 1038, 1038, 1038, 1038, 1038, 1038, 1038,
      1038, 1038, 1038, 1038, 1038, 1038, 1038,  247,  218,  144,
         0,    0,   40,    7,  126,  184,    2,   15,  146,   67,
         9,   60,    0,    0,    3,    0,    7,    8,  197,    1,
        40,    8, 1038, 1038, 1038, 1038, 1038, 1038, 1038, 1038,
      1038, 1038, 1038, 1038, 1038, 1038, 1038, 1038, 1038, 1038,
      1038, 1038, 1038, 1038, 1038, 1038, 1038, 1038, 1038, 1038,
      1038, 1038, 1038, 1038, 1038, 1038, 1038, 1038, 1038, 1038,
      1038, 1038, 1038, 1038, 1038, 1038, 1038, 1038, 1038, 1038,
      1038, 1038, 1038, 1038, 1038, 1038, 1038, 1038, 1038, 1038,
      1038, 1038, 1038, 1038, 1038, 1038, 1038, 1038, 1038, 1038,
      1038, 1038, 1038, 1038, 1038, 1038, 1038, 1038, 1038, 1038,
      1038, 1038, 1038, 1038, 1038, 1038, 1038, 1038, 1038, 1038,
      1038, 1038, 1038, 1038, 1038, 1038, 1038, 1038, 1038, 1038,
      1038, 1038, 1038, 1038, 1038, 1038, 1038, 1038, 1038, 1038,
      1038, 1038, 1038, 1038, 1038, 1038, 1038, 1038, 1038, 1038,
      1038, 1038, 1038, 1038, 1038, 1038, 1038, 1038, 1038, 1038,
      1038, 1038, 1038, 1038, 1038, 1038
    };
  register int hval = len;

  switch (hval)
    {
      default:
      case 6:
        hval += asso_values[(unsigned char)str[5]];
      case 5:
        hval += asso_values[(unsigned char)str[4]];
      case 4:
        hval += asso_values[(unsigned char)str[3]];
      case 3:
        hval += asso_values[(unsigned char)str[2]];
      case 2:
        hval += asso_values[(unsigned char)str[1]];
      case 1:
        hval += asso_values[(unsigned char)str[0]];
        break;
    }
  return hval + asso_values[(unsigned char)str[len - 1]];
}

#ifdef __GNUC__
__inline
#endif
const char *
libc_name_p (str, len)
     register const char *str;
     register unsigned int len;
{
  enum
    {
      TOTAL_KEYWORDS = 207,
      MIN_WORD_LENGTH = 3,
      MAX_WORD_LENGTH = 10,
      MIN_HASH_VALUE = 18,
      MAX_HASH_VALUE = 1037
    };

  static const char * const wordlist[] =
    {
      "gets",
      "puts",
      "sqrt",
      "strerror",
      "strstr",
      "strspn",
      "exp",
      "free",
      "fgets",
      "fputs",
      "fgetws",
      "fputws",
      "pow",
      "fseek",
      "perror",
      "strtod",
      "toupper",
      "towupper",
      "frexp",
      "strtok",
      "fsetpos",
      "ferror",
      "freopen",
      "fgetpos",
      "fopen",
      "wmemset",
      "memset",
      "system",
      "wcsstr",
      "wctype",
      "strxfrm",
      "wcsspn",
      "strcspn",
      "fmod",
      "strcpy",
      "strncpy",
      "strlen",
      "ungetwc",
      "feof",
      "ldexp",
      "isupper",
      "rewind",
      "iswupper",
      "sin",
      "cos",
      "modf",
      "iswpunct",
      "wcstod",
      "log10",
      "log",
      "wcsrtomb