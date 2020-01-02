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
  ; set CR4 PAE bit (bit 5)
  mov eax, cr4
  or eax, 0x20
  mov cr4, eax

  ; set CR3 PML4 table addr and enable cache
  mov eax, 0x100000
  mov cr3, eax

  ; set IA32_EFER.LME (Enable IA-32e mode)
  mov ecx, 0xC0000080
  rdmsr
  or eax, 0x0100  ; set lme bit
  wrmsr

  ; CR0 NW(29) = 0, CD(30) = 0, PG(31) = 1
  ; cache, paging enable
  mov eax, cr0
  or eax, 0xE0000000  ; set NW, CD, PG
  xor eax, 0x60000000 ; clear NW, CD
  mov cr0, eax

  ; run 64 bit kernel
  jmp 0x08:0x200000

  ;
  jmp $