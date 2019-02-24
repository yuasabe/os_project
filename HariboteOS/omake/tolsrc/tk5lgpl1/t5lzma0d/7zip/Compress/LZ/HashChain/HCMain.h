// HC.h

#include "../../../../Common/Defs.h"
#include "../../../../Common/CRC.h"

namespace HC_NAMESPACE {

#ifdef HASH_ARRAY_2
  static const UInt32 kHash2Size = 1 << 10;
  #ifdef HASH_ARRAY_3
    static const UInt32 kNumHashDirectBytes = 0;
    static const UInt32 kNumHashBytes = 4;
    static const UInt32 kHash3Size = 1 << 18;
    #ifdef HASH_BIG
    static const UInt32 kHashSize = 1 << 23;
    #else
    static const UInt32 kHashSize = 1 << 20;
    #endif
  #else
    static const UInt32 kNumHashBytes = 3;
    // static const UInt32 kNumHashDirectBytes = 3;
    // static const UInt32 kHashSize = 1 << (8 * kNumHashBytes);
    static const UInt32 kNumHashDirectBytes = 0;
    static const UInt32 kHashSize = 1 << (16);
  #endif
#else
  #ifdef HASH_ZIP 
    static const UInt32 kNumHashDirectBytes = 0;
    static const UInt32 kNumHashBytes = 3;
    static const UInt32 kHashSize = 1 << 16;
  #else
    static const UInt32 kNumHashDirectBytes = 2;
    static const UInt32 kNumHashBytes = 2;
    static const UInt32 kHashSize = 1 << (8 * kNumHashBytes);
  #endif
#endif


CInTree::CInTree():
  #ifdef HASH_ARRAY_2
  _hash2(0),
  #ifdef HASH_ARRAY_3
  _hash3(0),
  #endif
  #endif
  _hash(0),
  _chain(0),
  _cutValue(16)
{
}

void CInTree::FreeMemory()
{
  #ifdef WIN32
  if (_chain != 0)
    VirtualFree(_chain, 0, MEM_RELEASE);
  if (_hash != 0)
    VirtualFree(_hash, 0, MEM_RELEASE);
  #else
  delete []_chain;
  delete []_hash;
  #endif
  _chain = 0;
  _hash = 0;
  CLZInWindow::Free();
}

CInTree::‾CInTree()
{ 
  FreeMemory();
}

HRESULT CInTree::Create(UInt32 historySize, UInt32 keepAddBufferBefore, 
    UInt32 matchMaxLen, UInt32 keepAddBufferAfter, UInt32 sizeReserv)
{
  FreeMemory();
  try
  {
    CLZInWindow::Create(historySize + keepAddBufferBefore, 
      matchMaxLen + keepAddBufferAfter, sizeReserv);
    
    if (_blockSize + 256 > kMaxValForNormalize)
      return E_INVALIDARG;
    
    _historySize = historySize;
    _matchMaxLen = matchMaxLen;
    _cyclicBufferSize = historySize + 1;
    
    
    UInt32 size = kHashSize;
    #ifdef HASH_ARRAY_2
    size += kHash2Size;
    #ifdef HASH_ARRAY_3
    size += kHash3Size;
    #endif
    #endif
    
    #ifdef WIN32
    _chain = (CIndex *)::VirtualAlloc(0, (_cyclicBufferSize + 1) * sizeof(CIndex), MEM_COMMIT, PAGE_READWRITE);
    if (_chain == 0)
      throw 1; // CNewException();
    _hash = (CIndex *)::VirtualAlloc(0, (size + 1) * sizeof(CIndex), MEM_COMMIT, PAGE_READWRITE);
    if (_hash == 0)
      throw 1; // CNewException();
    #else
    _chain = new CIndex[_cyclicBufferSize + 1];
    _hash = new CIndex[size + 1];
    #endif
    
    // m_RightBase = &m_LeftBase[_blockSize];
    
    // _hash = &m_RightBase[_blockSize];
    #ifdef HASH_ARRAY_2
    _hash2 = &_hash[kHashSize]; 
    #ifdef HASH_ARRAY_3
    _hash3 = &_hash2[kHash2Size]; 
    #endif
    #endif
    return S_OK;
  }
  catch(...)
  {
    FreeMemory();
    return E_OUTOFMEMORY;
  }
}

static const UInt32 kEmptyHashValue = 0;

HRESULT CInTree::Init(ISequentialInStream *aStream)
{
  RINOK(CLZInWindow::Init(aStream));
  int i;
  for(i = 0; i < kHashSize; i++)
    _hash[i] = kEmptyHashValue;

  #ifdef HASH_ARRAY_2
  for(i = 0; i < kHash2Size; i++)
    _hash2[i] = kEmptyHashValue;
  #ifdef HASH_ARRAY_3
  for(i = 0; i < kHash3Size; i++)
    _hash3[i] = kEmptyHashValue;
  #endif
  #endif

  _cyclicBufferPos = 0;

  ReduceOffsets(-1);
  return S_OK;
}


#ifdef HASH_ARRAY_2
#ifdef HASH_ARRAY_3
inline UInt32 Hash(const Byte *pointer, UInt32 &hash2Value, UInt32 &hash3Value)
{
  UInt32 temp = CCRC::Table[pointer[0]] ^ pointer[1];
  hash2Value = temp & (kHash2Size - 1);
  hash3Value = (temp ^ (UInt32(pointer[2]) << 8)) & (kHash3Size - 1);
  return (temp ^ (UInt32(pointer[2]) << 8) ^ (CCRC::Table[pointer[3]] << 5)) & 
      (kHashSize - 1);
}
#else // no HASH_ARRAY_3
inline UInt32 Hash(const Byte *pointer, UInt32 &hash2Value)
{
  UInt32 temp = CCRC::Table[pointer[0]