// PatMain.h

#include "../../../../Common/Defs.h"

namespace PAT_NAMESPACE {

STDMETHODIMP CPatricia::SetCallback(IMatchFinderCallback *callback)
{
  m_Callback = callback;
  return S_OK;
}

void CPatricia::BeforeMoveBlock()
{
  if (m_Callback)
    m_Callback->BeforeChangingBufferPos();
  CLZInWindow::BeforeMoveBlock();
}

void CPatricia::AfterMoveBlock()
{
  if (m_Callback)
    m_Callback->AfterChangingBufferPos();
  CLZInWindow::AfterMoveBlock();
}

const UInt32 kMatchStartValue2 = 2;
const UInt32 kDescendantEmptyValue2 = kMatchStartValue2 - 1;
const UInt32 kDescendantsNotInitilized2 = kDescendantEmptyValue2 - 1;

#ifdef __HASH_3

static const UInt32 kNumHashBytes = 3;
static const UInt32 kHashSize = 1 << (8 * kNumHashBytes);

static const UInt32 kNumHash2Bytes = 2;
static const UInt32 kHash2Size = 1 << (8 * kNumHash2Bytes);
static const UInt32 kPrevHashSize = kNumHash2Bytes;

#else

static const UInt32 kNumHashBytes = 2;
static const UInt32 kHashSize = 1 << (8 * kNumHashBytes);
static const UInt32 kPrevHashSize = 0;

#endif


CPatricia::CPatricia():
  m_HashDescendants(0),
  #ifdef __HASH_3
  m_Hash2Descendants(0),
  #endif
  m_TmpBacks(0),
  m_Nodes(0)
{
}

CPatricia::‾CPatricia()
{
  FreeMemory();
}

void CPatricia::FreeMemory()
{
  delete []m_TmpBacks;
  m_TmpBacks = 0;

  #ifdef WIN32
  if (m_Nodes != 0)
    VirtualFree(m_Nodes, 0, MEM_RELEASE);
  m_Nodes = 0;
  #else
  m_AlignBuffer.Free();
  #endif

  delete []m_HashDescendants;
  m_HashDescendants = 0;

  #ifdef __HASH_3

  delete []m_Hash2Descendants;
  m_Hash2Descendants = 0;

  #endif
}
  
STDMETHODIMP CPatricia::Create(UInt32 historySize, UInt32 keepAddBufferBefore, 
    UInt32 matchMaxLen, UInt32 keepAddBufferAfter)
{
  FreeMemory();
  int kNumBitsInNumSameBits = sizeof(CSameBitsType) * 8;
  if (kNumBitsInNumSameBits < 32 && ((matchMaxLen * MY_BYTE_SIZE) > ((UInt32)1 << kNumBitsInNumSameBits)))
    return E_INVALIDARG;

  const UInt32 kAlignMask = (1 << 16) - 1;
  UInt32 windowReservSize = historySize;
  windowReservSize += kAlignMask;
  windowReservSize &= ‾(kAlignMask);

  const UInt32 kMinReservSize = (1 << 19);
  if (windowReservSize < kMinReservSize)
    windowReservSize = kMinReservSize;
  windowReservSize += 256;

  try 
  {
    CLZInWindow::Create(historySize + keepAddBufferBefore, 
      matchMaxLen + keepAddBufferAfter, windowReservSize);
    _sizeHistory = historySize;
    _matchMaxLen = matchMaxLen;
    m_HashDescendants = new CDescendant[kHashSize + 1];
    #ifdef __HASH_3
    m_Hash2Descendants = new CDescendant[kHash2Size + 1];
    #endif

    #ifdef __AUTO_REMOVE
   
    #ifdef __HASH_3
    m_NumNodes = historySize + _sizeHistory * 4 / 8 + (1 << 19);
    #else
    m_NumNodes = historySize + _sizeHistory * 4 / 8 + (1 << 10);
    #endif

    #else

    UInt32 m_NumNodes = historySize;
    
    #endif
    
    const UInt32 kMaxNumNodes = UInt32(1) << (sizeof(CIndex) * 8 - 1);
    if (m_NumNodes + 32 > kMaxNumNodes)
      return E_INVALIDARG;

    #ifdef WIN32
    m_Nodes = (CNode *)::VirtualAlloc(0, (m_NumNodes + 2) * sizeof(CNode), MEM_COMMIT, PAGE_READWRITE);
    if (m_Nodes == 0)
      throw 1; // CNewException();
    #else
    m_Nodes = (CNode *)m_AlignBuffer.Allocate((m_NumNodes + 2) * sizeof(CNode), 0x3F);
    #endif

    m_TmpBacks = new UInt32[_matchMaxLen + 1];
    return S_OK;
  }
  catch(...)
  {
    FreeMemory();
    return E_OUTOFMEMORY;
  }
}

STDMETHODIMP CPatricia::Init(ISequentialInStream *aStream)
{
  RINOK(CLZInWindow::Init(aStream));

  // memset(m_HashDescendants, 0xFF, kHashSize * sizeof(m_HashDescendants[0]));

  #ifdef __HASH_3
  for (UInt32 i = 0; i < kHash2Size; i++)
    m_Hash2Descendants[i].MatchPointer = kDescendantsNotInitilized2;
  #else
  for (UInt32 i = 0; i < kHashSize; i++)
    m_HashDescendants[i].MakeEmpty();
  #endif

  m_Nodes[0].NextFreeNode = 1;
  m_FreeNode = 0;
  m_FreeNodeMax = 0;
  #ifdef __AUTO_REMOVE
  m_NumUsedNodes = 0;
  #els