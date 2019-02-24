// LzmaBench.cpp

#include "StdAfx.h"

#include <stdio.h>

#ifndef WIN32
#include <time.h>
#endif

#include "../../../Common/CRC.h"
#include "../LZMA/LZMADecoder.h"
#include "../LZMA/LZMAEncoder.h"

static const UInt32 kAdditionalSize = (6 << 20);
static const UInt32 kCompressedAdditionalSize = (1 << 10);
static const UInt32 kMaxLzmaPropSize = 10;

class CRandomGenerator
{
  UInt32 A1;
  UInt32 A2;
public:
  CRandomGenerator() { Init(); }
  void Init() { A1 = 362436069; A2 = 521288629;}
  UInt32 GetRnd() 
  {
    return 
      ((A1 = 36969 * (A1 & 0xffff) + (A1 >> 16)) << 16) ^
      ((A2 = 18000 * (A2 & 0xffff) + (A2 >> 16)) );
  }
};

class CBitRandomGenerator
{
  CRandomGenerator RG;
  UInt32 Value;
  int NumBits;
public:
  void Init()
  {
    Value = 0;
    NumBits = 0;
  }
  UInt32 GetRnd(int numBits) 
  {
    if (NumBits > numBits)
    {
      UInt32 result = Value & ((1 << numBits) - 1);
      Value >>= numBits;
      NumBits -= numBits;
      return result;
    }
    numBits -= NumBits;
    UInt32 result = (Value << numBits);
    Value = RG.GetRnd();
    result |= Value & ((1 << numBits) - 1);
    Value >>= numBits;
    NumBits = 32 - numBits;
    return result;
  }
};

class CBenchRandomGenerator
{
  CBitRandomGenerator RG;
  UInt32 Pos;
public:
  UInt32 BufferSize;
  Byte *Buffer;
  CBenchRandomGenerator(): Buffer(0) {} 
  ‾CBenchRandomGenerator() { delete []Buffer; }
  void Init() { RG.Init(); }
  void Set(UInt32 bufferSize) 
  {
    delete []Buffer;
    Buffer = 0;
    Buffer = new Byte[bufferSize];
    Pos = 0;
    BufferSize = bufferSize;
  }
  UInt32 GetRndBit() { return RG.GetRnd(1); }
  /*
  UInt32 GetLogRand(int maxLen)
  {
    UInt32 len = GetRnd() % (maxLen + 1);
    return GetRnd() & ((1 << len) - 1);
  }
  */
  UInt32 GetLogRandBits(int numBits)
  {
    UInt32 len = RG.GetRnd(numBits);
    return RG.GetRnd(len);
  }
  UInt32 GetOffset()
  {
    if (GetRndBit() == 0)
      return GetLogRandBits(4);
    return (GetLogRandBits(4) << 10) | RG.GetRnd(10);
  }
  UInt32 GetLen()
  {
    if (GetRndBit() == 0)
      return RG.GetRnd(2);
    if (GetRndBit() == 0)
      return 4 + RG.GetRnd(3);
    return 12 + RG.GetRnd(4);
  }
  void Generate()
  {
    while(Pos < BufferSize)
    {
      if (GetRndBit() == 0 || Pos < 1)
        Buffer[Pos++] = Byte(RG.GetRnd(8));
      else
      {
        UInt32 offset = GetOffset();
        while (offset >= Pos)
          offset >>= 1;
        offset += 1;
        UInt32 len = 2 + GetLen();
        for (UInt32 i = 0; i < len && Pos < BufferSize; i++, Pos++)
          Buffer[Pos] = Buffer[Pos - offset];
      }
    }
  }
};

class CBenchmarkInStream: 
  public ISequentialInStream,
  public CMyUnknownImp
{
  const Byte *Data;
  UInt32 Pos;
  UInt32 Size;
public:
  MY_UNKNOWN_IMP
  void Init(const Byte *data, UInt32 size)
  {
    Data = data;
    Size = size;
    Pos = 0;
  }
  STDMETHOD(Read)(void *data, UInt32 size, UInt32 *processedSize);
  STDMETHOD(ReadPart)(void *data, UInt32 size, UInt32 *processedSize);
};

STDMETHODIMP CBenchmarkInStream::Read(void *data, UInt32 size, UInt32 *processedSize)
{
  UInt32 remain = Size - Pos;
  if (size > remain)
    size = remain;
  for (UInt32 i = 0; i < size; i++)
  {
    ((Byte *)data)[i] = Data[Pos + i];
  }
  Pos += size;
  if(processedSize != NULL)
    *processedSize = size;
  return S_OK;
}
  
STDMETHODIMP CBenchmarkInStream::ReadPart(void *data, UInt32 size, UInt32 *processedSize)
{
  return Read(data, size, processedSize);
}

class CBenchmarkOutStream: 
  public ISequentialOutStream,
  public CMyUnknownImp
{
  UInt32 BufferSize;
public:
  UInt32 Pos;
  Byte *Buffer;
  CBenchmarkOutStream(): Buffer(0) {} 
  ‾CBenchmarkOutStream() { delete []Buffer; }
  void Init(UInt32 bufferSize) 
  {
    delete []Buffer;
    Buffer = 0;
    Buffer = new Byte[bufferSize];
    Pos = 0;
    BufferSize = bufferSize;
  }
  MY_UNKNOWN_IMP
  STDMETHOD(Write)(const void