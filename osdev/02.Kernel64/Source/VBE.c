#include "VBE.h"

static VBEMODEINFOBLOCK* gs_pstVBEModeBlockInfo = (VBEMODEINFOBLOCK*)VBE_MODEINFOBLOCKADDRESS;

inline VBEMODEINFOBLOCK* kGetVBEModeInfoBlock(void)
{
  return gs_pstVBEModeBlockInfo;
}