[BITS 32]	; set code 32 bit

; export for C lang
global kReadCPUID, kSwitchAndExecute64bitKernel

SECTION .text ; define text segment

; return CPUID
;   PARAM : DWORD dwEAX, DWORD* pdwEAX, * pdwEBX, * pdwECX, * pdwEDX
kReadCPUID:
  push ebp
  mov ebp, esp
  push eax
  push ebx
  push ecx
  push edx
  push esi

  ; execute CPUID
  mov eax, dword[ebp+8]
  cpuid

  ; save result to param
  mov esi, dword[ebp+12]
  mov dword[esi], eax

  mov esi, dword[ebp+16]
  mov dword[esi], ebx

  mov esi, dword[ebp+20]
  mov dword[esi], ecx

  mov esi, dword[ebp+24]
  mov dword[esi], edx

  pop esi
  pop edx
  pop ecx
  pop ebx
  pop eax
  pop ebp
  ret

; Switch to IA-32e mode and run 64 bit kernel
;   PARAM: None
kSwitchAndExecute64bitKernel:
  ; set CR4 PAE bit (bit 5), 
  ; For FPU : OSXMMECPT (bit 10), OSFXSR (bit 9)
  mov eax, cr4
  or eax, 0x620
  mov cr4, eax

  ; set CR3 PML4 table addr and enable cache
  mov eax, 0x100000
  mov cr3, eax

  ; set IA32_EFER.LME(Bit 10) (Enable IA-32e mode)
  ; set IA32_EFER.SCE(Bit 0) (Enable SystemCall)
  mov ecx, 0xC0000080
  rdmsr
  or eax, 0x0101  ; set LME, SCE bit
  wrmsr

  ; CR0 NW(29) = 0, CD(30) = 0, PG(31) = 1
  ; cache, paging enable
  ; For FPU : TS(3) = 1, EM(2) = 0, MP(1) = 1
  mov eax, cr0
  or eax, 0xE000000E  ; set NW, CD, PG, TS, EM, MP
  xor eax, 0x60000004 ; clear NW, CD, EM
  mov cr0, eax

  ; run 64 bit kernel
  jmp 0x08:0x200000

  ;
  jmp $