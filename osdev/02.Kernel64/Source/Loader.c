#include "Loader.h"
#include "FileSystem.h"
#include "DynamicMemory.h"

/*
  Load ELF File and Execute
*/
QWORD kExecuteProgram(const char* pcFileName, const char* pcArgumentString, BYTE bAffinity)
{
  DIR* pstDirectory;
  struct dirent* pstEntry;
  DWORD dwFileSize;
  BYTE* pbTempFileBuffer;
  FILE* pstFile;
  DWORD dwReadSize;
  QWORD qwEntryPointAddress;
  QWORD qwApplicationMemory;
  QWORD qwMemorySize;
  TCB* pstTask;

  // Open Root Dir and Find File
  pstDirectory = opendir("/");
  dwFileSize = 0;
  while (1)
  {
    pstEntry = readdir(pstDirectory);
    if (pstEntry == NULL)
    {
      break;
    }

    // Compare File name
    if ((kStrLen(pstEntry->d_name) == kStrLen(pcFileName)) &&
      (kMemCmp(pstEntry->d_name, pcFileName, kStrLen(pcFileName))
        == 0))
    {
      dwFileSize = pstEntry->dwFileSize;
      break;
    }
  }
  closedir(pstDirectory);

  if (dwFileSize == 0)
  {
    kPrintf("%s file doesn't exist or size is zero\n",
      pcFileName);
    return TASK_INVALIDID;
  }

  // Allocate Buffer for read file
  pbTempFileBuffer = (BYTE*)kAllocateMemory(dwFileSize);
  if (pbTempFileBuffer == NULL) {
    kPrintf("Memory %dbytes allocate fail\n", dwFileSize);
    return TASK_INVALIDID;
  }

  // open file
  pstFile = fopen(pcFileName, "r");
  if ((pstFile != NULL) && (fread(pbTempFileBuffer, 1, dwFileSize, pstFile) == dwFileSize)) {
    fclose(pstFile);
    kPrintf("%s file read success\n", pcFileName);
  }
  else {
    kPrintf("%s file read fail\n", pcFileName);
    kFreeMemory(pbTempFileBuffer);
    fclose(pstFile);
    return TASK_INVALIDID;
  }

  // Load and Relocation
  if (kLoadProgramAndRelocation(pbTempFileBuffer, &qwApplicationMemory,
    &qwMemorySize, &qwEntryPointAddress) == FALSE) {
    kPrintf("%s file is invalid application file or loading fail\n", pcFileName);
    kFreeMemory(pbTempFileBuffer);
    return TASK_INVALIDID;
  }
  kFreeMemory(pbTempFileBuffer);

  // Create Task
  pstTask = kCreateTask(TASK_FLAGS_USERLEVEL | TASK_FLAGS_PROCESS,
    (void*)qwApplicationMemory, qwMemorySize, qwEntryPointAddress, bAffinity);
  if (pstTask == NULL) {
    kFreeMemory((void*)qwApplicationMemory);
    return TASK_INVALIDID;
  }

  // Add argument in Task Stack
  kAddArgumentStringToTask(pstTask, pcArgumentString);

  return pstTask->stLink.qwID;
}

/*
  File Load in Memory and Relocation
*/
static BOOL kLoadProgramAndRelocation(BYTE* pbFileBuffer,
  QWORD* pqwApplicationMemoryAddress, QWORD* pqwApplicationMemorySize,
  QWORD* pqwEntryPointAddress)
{
  Elf64_Ehdr* pstELFHeader;
  Elf64_Shdr* pstSectionHeader;
  Elf64_Shdr* pstSectionNameTableHeader;
  Elf64_Xword qwLastSectionSize;
  Elf64_Addr qwLastSectionAddress;
  int i;
  QWORD qwMemorySize;
  QWORD qwStackAddress;
  BYTE* pbLoadedAddress;

  // Get Header
  pstELFHeader = (Elf64_Ehdr*)pbFileBuffer;
  pstSectionHeader = (Elf64_Shdr*)(pbFileBuffer + pstELFHeader->e_shoff);
  pstSectionNameTableHeader = pstSectionHeader + pstELFHeader->e_shstrndx;

  // Print ELF Header Info
  kPrintf("========================= ELF Header Info =========================\n");
  kPrintf("Magic Number [%c%c%c] Section Header Count [%d]\n",
    pstELFHeader->e_ident[1], pstELFHeader->e_ident[2],
    pstELFHeader->e_ident[3], pstELFHeader->e_shnum);
  kPrintf("File Type [%d]\n", pstELFHeader->e_type);
  kPrintf("Section Header Offset [0x%X] Size [0x%X]\n", pstELFHeader->e_shoff,
    pstELFHeader->e_shentsize);
  kPrintf("Program Header Offset [0x%X] Size [0x%X]\n", pstELFHeader->e_phoff,
    pstELFHeader->e_phentsize);
  kPrintf("Section Name String Table Section Index [%d]\n", pstELFHeader->e_shstrndx);

  // Check file is valid
  if ((pstELFHeader->e_ident[EI_MAG0] != ELFMAG0) ||
    (pstELFHeader->e_ident[EI_MAG1] != ELFMAG1) ||
    (pstELFHeader->e_ident[EI_MAG2] != ELFMAG2) ||
    (pstELFHeader->e_ident[EI_MAG3] != ELFMAG3) ||
    (pstELFHeader->e_ident[EI_CLASS] != ELFCLASS64) ||
    (pstELFHeader->e_ident[EI_DATA] != ELFDATA2LSB) ||
    (pstELFHeader->e_type != ET_REL))
  {
    return FALSE;
  }

  // Calc Memory Size to Allocate
  qwLastSectionAddress = 0;
  qwLastSectionSize = 0;
  for (i = 0; i < pstELFHeader->e_shnum; i++) {
    if ((pstSectionHeader[i].sh_flags & SHF_ALLOC) &&
      (pstSectionHeader[i].sh_addr >= qwLastSectionAddress)) {
      qwLastSectionAddress = pstSectionHeader[i].sh_addr;
      qwLastSectionSize = pstSectionHeader[i].sh_size;
    }
  }
  kPrintf("\n========================= Load & Relocaion ========================\n");
  kPrintf("Last Section Address [0x%q] Size [0x%q]\n", qwLastSectionAddress,
    qwLastSectionSize);
  qwMemorySize = (qwLastSectionAddress + qwLastSectionSize + 0x1000 - 1) &
    0xfffffffffffff000;
  kPrintf("Aligned Memory Size [0x%q]\n", qwMemorySize);

  // Allocate Memory
  pbLoadedAddress = (char*)kAllocateMemory(qwMemorySize);
  if (pbLoadedAddress == NULL)
  {
    kPrintf("Memory allocate fail\n");
    return FALSE;
  }
  else
  {
    kPrintf("Loaded Address [0x%q]\n", pbLoadedAddress);
  }

  // Load Section (0 Section is NULL Section)
  for (i = 1; i < pstELFHeader->e_shnum; i++) {

    // don't allocate section
    if (!(pstSectionHeader[i].sh_flags & SHF_ALLOC) ||
      (pstSectionHeader[i].sh_size == 0)) {
      continue;
    }

    pstSectionHeader[i].sh_addr += (Elf64_Addr)pbLoadedAddress;

    // SHT_NOBITS (.bss)
    if (pstSectionHeader[i].sh_type == SHT_NOBITS) {
      // Set 0
      kMemSet(pstSectionHeader[i].sh_addr, 0, pstSectionHeader[i].sh_size);
    }
    else {
      // Copy
      kMemCpy(pstSectionHeader[i].sh_addr,
        pbFileBuffer + pstSectionHeader[i].sh_offset,
        pstSectionHeader[i].sh_size);
    }

    kPrintf("Section [%x] Virtual Address [%q] File Address [%q] Size [%q]\n",
      i,
      pstSectionHeader[i].sh_addr,
      pbFileBuffer + pstSectionHeader[i].sh_offset,
      pstSectionHeader[i].sh_size);
  }
  kPrintf("Program load success\n");

  // Relocation
  if (kRelocation(pbFileBuffer) == FALSE)
  {
    kPrintf("Relocation fail\n");
    return FALSE;
  }
  else
  {
    kPrintf("Relocation success\n");
  }

  *pqwApplicationMemoryAddress = (QWORD)pbLoadedAddress;
  *pqwApplicationMemorySize = qwMemorySize;
  *pqwEntryPointAddress = pstELFHeader->e_entry + (QWORD)pbLoadedAddress;

  return TRUE;
}

/*
  ELF Relocation
*/
static BOOL kRelocation(BYTE* pbFileBuffer)
{
  Elf64_Ehdr* pstELFHeader;
  Elf64_Shdr* pstSectionHeader;
  int i;
  int j;
  int iSymbolTableIndex;
  int iSectionIndexInSymbol;
  int iSectionIndexToRelocation;
  Elf64_Addr ulOffset;
  Elf64_Xword ulInfo;
  Elf64_Sxword lAddend;
  Elf64_Sxword lResult;
  int iNumberOfBytes;
  Elf64_Rel* pstRel;
  Elf64_Rela* pstRela;
  Elf64_Sym* pstSymbolTable;

  pstELFHeader = (Elf64_Ehdr*)pbFileBuffer;
  pstSectionHeader = (Elf64_Shdr*)(pbFileBuffer + pstELFHeader->e_shoff);

  for (i = 1; i < pstELFHeader->e_shnum; i++) {
    if ((pstSectionHeader[i].sh_type != SHT_RELA) &&
      (pstSectionHeader[i].sh_type != SHT_REL)) {
      continue;
    }

    iSectionIndexToRelocation = pstSectionHeader[i].sh_info;
    iSymbolTableIndex = pstSectionHeader[i].sh_link;
    pstSymbolTable = (Elf64_Sym*)(pbFileBuffer + pstSectionHeader[iSymbolTableIndex].sh_offset);

    for (j = 0; j < pstSectionHeader[i].sh_size; ) {
      // REL
      if (pstSectionHeader[i].sh_type == SHT_REL) {
        pstRel = (Elf64_Rel*)(pbFileBuffer + pstSectionHeader[i].sh_offset);
        ulOffset = pstRel->r_offset;
        ulInfo = pstRel->r_info;
        lAddend = 0;

        j += sizeof(Elf64_Rel);
      }
      // RELA
      else {
        pstRela = (Elf64_Rela*)
          (pbFileBuffer + pstSectionHeader[i].sh_offset + j);
        ulOffset = pstRela->r_offset;
        ulInfo = pstRela->r_info;
        lAddend = pstRela->r_addend;

        j += sizeof(Elf64_Rela);
      }

      if (pstSymbolTable[RELOCATION_UPPER32(ulInfo)].st_shndx == SHN_ABS)
      {
        continue;
      }
      else if (pstSymbolTable[RELOCATION_UPPER32(ulInfo)].st_shndx == SHN_COMMON)
      {
        kPrintf("Common symbol is not supported\n");
        return FALSE;
      }

      // Calc Write offset
      switch (RELOCATION_LOWER32(ulInfo))
      {
        // S(st_value) + A(r_addend)
      case R_X86_64_64:
      case R_X86_64_32:
      case R_X86_64_32S:
      case R_X86_64_16:
      case R_X86_64_8:
        iSectionIndexInSymbol =
          pstSymbolTable[RELOCATION_UPPER32(ulInfo)].st_shndx;

        lResult = (pstSymbolTable[RELOCATION_UPPER32(ulInfo)].st_value +
          pstSectionHeader[iSectionIndexInSymbol].sh_addr) + lAddend;
        break;

        // S(st_value) + A(r_addend) - P(r_offset)
      case R_X86_64_PC32:
      case R_X86_64_PC16:
      case R_X86_64_PC8:
      case R_X86_64_PC64:
        iSectionIndexInSymbol =
          pstSymbolTable[RELOCATION_UPPER32(ulInfo)].st_shndx;

        lResult = (pstSymbolTable[RELOCATION_UPPER32(ulInfo)].st_value +
          pstSectionHeader[iSectionIndexInSymbol].sh_addr) + lAddend -
          (ulOffset + pstSectionHeader[iSectionIndexToRelocation].sh_addr);
        break;

        // B(sh_addr) + A(r_ddend)
      case R_X86_64_RELATIVE:
        lResult = pstSectionHeader[i].sh_addr + lAddend;
        break;

        // Z(st_size) + A(r_addend)
      case R_X86_64_SIZE32:
      case R_X86_64_SIZE64:
        lResult = pstSymbolTable[RELOCATION_UPPER32(ulInfo)].st_size +
          lAddend;
        break;

      default:
        kPrintf("Unsupported relocation type [%X]\n",
          RELOCATION_LOWER32(ulInfo));
        return FALSE;
        break;
      }

      // Calc Write Size
      switch (RELOCATION_LOWER32(ulInfo))
      {
        // 64
      case R_X86_64_64:
      case R_X86_64_PC64:
      case R_X86_64_SIZE64:
        iNumberOfBytes = 8;
        break;

        // 32
      case R_X86_64_PC32:
      case R_X86_64_32:
      case R_X86_64_32S:
      case R_X86_64_SIZE32:
        iNumberOfBytes = 4;
        break;

        // 16
      case R_X86_64_16:
      case R_X86_64_PC16:
        iNumberOfBytes = 2;
        break;

        // 8
      case R_X86_64_8:
      case R_X86_64_PC8:
        iNumberOfBytes = 1;
        break;

      default:
        kPrintf("Unsupported relocation type [%X]\n",
          RELOCATION_LOWER32(ulInfo));
        return FALSE;
        break;
      }

      // Write
      switch (iNumberOfBytes)
      {
      case 8:
        *((Elf64_Sxword*)
          (pstSectionHeader[iSectionIndexToRelocation].sh_addr +
            ulOffset)) += lResult;
        break;

      case 4:
        *((int*)
          (pstSectionHeader[iSectionIndexToRelocation].sh_addr +
            ulOffset)) += (int)lResult;
        break;

      case 2:
        *((short*)
          (pstSectionHeader[iSectionIndexToRelocation].sh_addr +
            ulOffset)) += (short)lResult;
        break;

      case 1:
        *((char*)
          (pstSectionHeader[iSectionIndexToRelocation].sh_addr +
            ulOffset)) += (char)lResult;
        break;

      default:
        kPrintf("Relocation error. Relocation byte size is [%d]byte\n",
          iNumberOfBytes);
        return FALSE;
        break;
      }
    }
  }
  return TRUE;
}

/*
  Add Argument in Stack
*/
static void kAddArgumentStringToTask(TCB* pstTask, const char* pcArgumentString)
{
  int iLength;
  int iAlignedLength;
  QWORD qwNewRSPAddress;

  // Get Argument Len
  if (pcArgumentString == NULL)
  {
    iLength = 0;
  }
  else
  {
    iLength = kStrLen(pcArgumentString);

    if (iLength > 1023)
    {
      iLength = 1023;
    }
  }
  iAlignedLength = (iLength + 7) & 0xFFFFFFF8;

  // Copy to Stack Area
  qwNewRSPAddress = pstTask->stContext.vqRegister[TASK_RSPOFFSET] -
    (QWORD)iAlignedLength;
  kMemCpy((void*)qwNewRSPAddress, pcArgumentString, iLength);
  *((BYTE*)qwNewRSPAddress + iLength) = '\0';

  // Update RSP, RBP
  pstTask->stContext.vqRegister[TASK_RSPOFFSET] = qwNewRSPAddress;
  pstTask->stContext.vqRegister[TASK_RBPOFFSET] = qwNewRSPAddress;

  // RDI = New RSP (Argument)
  pstTask->stContext.vqRegister[TASK_RDIOFFSET] = qwNewRSPAddress;
}

/*
  Create User Thread
*/
QWORD kCreateThread(QWORD qwEntryPoint, QWORD qwArgument, BYTE bAffinity, QWORD qwExitFunction)
{
  TCB* pstTask;

  // Create Tasks (thread)
  pstTask = kCreateTask(TASK_FLAGS_USERLEVEL | TASK_FLAGS_THREAD, NULL, 0,
    qwEntryPoint, bAffinity);
  if (pstTask == NULL)
  {
    return TASK_INVALIDID;
  }

  // Chagne kEndTask to qwExitFunction
  *((QWORD*)pstTask->stContext.vqRegister[TASK_RSPOFFSET]) =
    qwExitFunction;

  // RDI = Argument
  pstTask->stContext.vqRegister[TASK_RDIOFFSET] = qwArgument;

  return pstTask->stLink.qwID;
}