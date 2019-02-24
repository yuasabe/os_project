// Common/String.h

#ifndef __COMMON_STRING_H
#define __COMMON_STRING_H

#include <string.h>
// #include <wchar.h>

#include "Vector.h"

static const char *kTrimDefaultCharSet  = " ¥n¥t";

template <class T>
inline size_t MyStringLen(const T *s)
{ 
  int i;
  for (i = 0; s[i] != '¥0'; i++);
  return i;
}

template <class T>
inline T * MyStringCopy(T *dest, const T *src)
{ 
  T *destStart = dest;
  while((*dest++ = *src++) != 0);
  return destStart;
}

inline wchar_t* MyStringGetNextCharPointer(wchar_t *p)
  { return (p + 1); }
inline const wchar_t* MyStringGetNextCharPointer(const wchar_t *p)
  { return (p + 1); }
inline wchar_t* MyStringGetPrevCharPointer(const wchar_t *base, wchar_t *p)
  { return (p - 1); }
inline const wchar_t* MyStringGetPrevCharPointer(const wchar_t *base, const wchar_t *p)
  { return (p - 1); }

#ifdef WIN32

inline char* MyStringGetNextCharPointer(char *p)
  { return CharNextA(p); }
inline const char* MyStringGetNextCharPointer(const char *p)
  { return CharNextA(p); }

inline char* MyStringGetPrevCharPointer(char *base, char *p)
  { return CharPrevA(base, p); }
inline const char* MyStringGetPrevCharPointer(const char *base, const char *p)
  { return CharPrevA(base, p); }

inline char MyCharUpper(char c)
  { return (char)(unsigned int)CharUpperA((LPSTR)(unsigned int)(unsigned char)c); }
#ifdef _UNICODE
inline wchar_t MyCharUpper(wchar_t c)
  { return (wchar_t)CharUpperW((LPWSTR)c); }
#else
wchar_t MyCharUpper(wchar_t c);
#endif

inline char MyCharLower(char c)
  { return (char)(unsigned int)CharLowerA((LPSTR)(unsigned int)(unsigned char)c); }
#ifdef _UNICODE
inline wchar_t MyCharLower(wchar_t c)
  { return (wchar_t)CharLowerW((LPWSTR)c); }
#else
wchar_t MyCharLower(wchar_t c);
#endif

inline char * MyStringUpper(char *s) { return CharUpperA(s); }
#ifdef _UNICODE
inline wchar_t * MyStringUpper(wchar_t *s) { return CharUpperW(s); }
#else
wchar_t * MyStringUpper(wchar_t *s);
#endif

inline char * MyStringLower(char *s) { return CharLowerA(s); }
#ifdef _UNICODE
inline wchar_t * MyStringLower(wchar_t *s) { return CharLowerW(s); }
#else
wchar_t * MyStringLower(wchar_t *s);
#endif

#else // Standard-C
wchar_t MyCharUpper(wchar_t c);
#endif

//////////////////////////////////////
// Compare

int MyStringCollate(const char *s1, const char *s2);
int MyStringCollate(const wchar_t *s1, const wchar_t *s2);
int MyStringCollateNoCase(const char *s1, const char *s2);
int MyStringCollateNoCase(const wchar_t *s1, const wchar_t *s2);

int MyStringCompare(const char *s1, const char  *s2);
int MyStringCompare(const wchar_t *s1, const wchar_t *s2);

template <class T>
inline int MyStringCompareNoCase(const T *s1, const T *s2)
  { return MyStringCollateNoCase(s1, s2); }

template <class T>
class CStringBase
{
  void TrimLeftWithCharSet(const CStringBase &charSet)
  {
    const T *p = _chars;
    while (charSet.Find(*p) >= 0 && (*p != 0))
      p = GetNextCharPointer(p);
    Delete(0, p - _chars);
  }
  void TrimRightWithCharSet(const CStringBase &charSet)
  {
    const T *p = _chars;
    const T *pLast = NULL;
    while (*p != 0)
    {
      if (charSet.Find(*p) >= 0)
      {
        if (pLast == NULL)
          pLast = p;
      }
      else
        pLast = NULL;
      p = GetNextCharPointer(p);
    }
    if(pLast != NULL)
    {
      int i = pLast - _chars;
      Delete(i, _length - i);
    }

  }
  void MoveItems(int destIndex, int srcIndex)
  {
    memmove(_chars + destIndex, _chars + srcIndex, 
        sizeof(T) * (_length - srcIndex + 1));
  }
  
  void InsertSpace(int &index, int size)
  {
    CorrectIndex(index);
    GrowLength(size);
    MoveItems(index + size, index);
  }

  static T *GetNextCharPointer(T *p)
    { return MyStringGetNextCharPointer(p); }
  static const T *GetNextCharPointer(const T *p)
    { return MyStringGetNextCharPointer(p); }
  static T *GetPrevCharPointer(T *base, T *p)
    { return MyStringGetPrevCharPointer(base, p); }
  static const T *