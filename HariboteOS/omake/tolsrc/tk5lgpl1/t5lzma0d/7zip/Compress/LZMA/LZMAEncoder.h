// LZMA/Encoder.h

#ifndef __LZMA_ENCODER_H
#define __LZMA_ENCODER_H

#include "../../../Common/MyCom.h"
#include "../../ICoder.h"
#include "../LZ/IMatchFinder.h"
#include "../RangeCoder/RangeCoderBitTree.h"

#include "LZMA.h"

namespace NCompress {
namespace NLZMA {

typedef NRangeCoder::CBitEncoder<kNumMoveBits> CMyBitEncoder;

class CMatchFinderException
{
public:
  HRESULT ErrorCode;
  CMatchFinderException(HRESULT errorCode): ErrorCode(errorCode) {}
};

class CBaseState
{
protected:
  CState _state;
  Byte _previousByte;
  bool _peviousIsMatch;
  UInt32 _repDistances[kNumRepDistances];
  void Init()
  {
    _state.Init();
    _previousByte = 0;
    _peviousIsMatch = false;
    for(int i = 0 ; i < kNumRepDistances; i++)
      _repDistances[i] = 0;
  }
};

struct COptimal
{
  CState State;

  bool Prev1IsChar;
  bool Prev2;

  UInt32 PosPrev2;
  UInt32 BackPrev2;     

  UInt32 Price;    
  UInt32 PosPrev;         // posNext;
  UInt32 BackPrev;     
  UInt32 Backs[kNumRepDistances];
  void MakeAsChar() { BackPrev = UInt32(-1); Prev1IsChar = false; }
  void MakeAsShortRep() { BackPrev = 0; ; Prev1IsChar = false; }
  bool IsShortRep() { return (BackPrev == 0); }
};


extern Byte g_FastPos[1024];
inline UInt32 GetPosSlot(UInt32 pos)
{
  if (pos < (1 << 10))
    return g_FastPos[pos];
  if (pos < (1 << 19))
    return g_FastPos[pos >> 9] + 18;
  return g_FastPos[pos >> 18] + 36;
}

inline UInt32 GetPosSlot2(UInt32 pos)
{
  if (pos < (1 << 16))
    return g_FastPos[pos >> 6] + 12;
  if (pos < (1 << 25))
    return g_FastPos[pos >> 15] + 30;
  return g_FastPos[pos >> 24] + 48;
}

const UInt32 kIfinityPrice = 0xFFFFFFF;

const UInt32 kNumOpts = 1 << 12;


class CLiteralEncoder2
{
  CMyBitEncoder _encoders[0x300];
public:
  void Init()
  {
    for (int i = 0; i < 0x300; i++)
      _encoders[i].Init();
  }
  void Encode(NRangeCoder::CEncoder *rangeEncoder, bool matchMode, Byte matchByte, Byte symbol);
  UInt32 GetPrice(bool matchMode, Byte matchByte, Byte symbol) const;
};

class CLiteralEncoder
{
  CLiteralEncoder2 *_coders;
  int _numPrevBits;
  int _numPosBits;
  UInt32 _posMask;
public:
  CLiteralEncoder(): _coders(0) {}
  ‾CLiteralEncoder()  { Free(); }
  void Free()
  { 
    delete []_coders;
    _coders = 0;
  }
  void Create(int numPosBits, int numPrevBits)
  {
    if (_coders == 0 || (numPosBits + numPrevBits) != 
        (_numPrevBits + _numPosBits) )
    {
      Free();
      UInt32 numStates = 1 << (numPosBits + numPrevBits);
      _coders = new CLiteralEncoder2[numStates];
    }
    _numPosBits = numPosBits;
    _posMask = (1 << numPosBits) - 1;
    _numPrevBits = numPrevBits;
  }
  void Init()
  {
    UInt32 numStates = 1 << (_numPrevBits + _numPosBits);
    for (UInt32 i = 0; i < numStates; i++)
      _coders[i].Init();
  }
  UInt32 GetState(UInt32 pos, Byte prevByte) const
    { return ((pos & _posMask) << _numPrevBits) + (prevByte >> (8 - _numPrevBits)); }
  void Encode(NRangeCoder::CEncoder *rangeEncoder, UInt32 pos, Byte prevByte, 
      bool matchMode, Byte matchByte, Byte symbol)
    { _coders[GetState(pos, prevByte)].Encode(rangeEncoder, matchMode, 
          matchByte, symbol); }
  UInt32 GetPrice(UInt32 pos, Byte prevByte, bool matchMode, Byte matchByte, Byte symbol) const
    { return _coders[GetState(pos, prevByte)].GetPrice(matchMode, matchByte, symbol); }
};

namespace NLength {

class CEncoder
{
  CMyBitEncoder _choice;
  NRangeCoder::CBitTreeEncoder<kNumMoveBits, kNumLowBits>  _lowCoder[kNumPosStatesEncodingMax];
  CMyBitEncoder  _choice2;
  NRangeCoder::CBitTreeEncoder<kNumMoveBits, kNumMidBits>  _midCoder[kNumPosStatesEncodingMax];
  NRangeCoder::CBitTreeEncoder<kNumMoveBits, kNumHighBits>  _highCoder;
public:
  void Init(UInt32 numPosStates);
  void Encode(NRangeCoder::CEncoder *rangeEncoder, UInt32 symbol, UInt32 posState);
  UInt32 GetPrice(UInt32 symbol, UInt32 posState) const;
};

const UInt32 kNumSpecSymbols = kNumLowSymbols + kNumMidSy