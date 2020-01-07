[BITS 64]	; set code 64 bit

SECTION .text	; define text segment

global kInPortByte, kInPortWord, kOutPortByte, kOutPortWord, kLoadGDTR, kLoadTR, kLoadIDTR
global kEnableInterrupt, kDisableInterrupt, kReadRFLAGS
; TSC
global kReadTSC

; Task
global kSwitchContext, kHlt, kTestAndSet

; FPU
global kInitializeFPU, kSaveFPUContext, kLoadFPUContext, kSetTS, kClearTS

; read from port one byte
;   PARAM port_num
kInPortByte:
  push rdx

  mov rdx, rdi
  mov rax, 0
  in al, dx

  pop rdx
  ret

; read from port one WORD
;   PARAM port_num
kInPortWord:
  push rdx

  mov rdx, rdi
  mov rax, 0
  in ax, dx

  pop rdx
  ret

; write to port one byte
;   PARAM port_num, data
kOutPortByte:
  push rdx
  push rax

  mov rdx, rdi
  mov rax, rsi
  out dx, al

  pop rax
  pop rdx
  ret

; write to port one WORD
;   PARAM port_num, data
kOutPortWord:
  push rdx
  push rax

  mov rdx, rdi
  mov rax, rsi
  out dx, ax

  pop rax
  pop rdx
  ret

; load GDT in GDTR
;   PARAM gdt address
kLoadGDTR:
  lgdt [rdi]
  ret

; load TSS in TR
;   PARAM TSS offset
kLoadTR:
  ltr di
  ret

; load IDT in IDTR
;   PARAM IDT address
kLoadIDTR:
  lidt [rdi]
  ret

; enable interrupt
;   PARAM None
kEnableInterrupt:
  sti
  ret

; disable interrupt
;   PARAM None
kDisableInterrupt:
  cli
  ret

; read RFLAGS and get
;   PARAM None
kReadRFLAGS:
  pushfq
  pop rax
  ret

; Read time stamp counter and return
;   PARAM None
kReadTSC:
  push rdx

  rdtsc

  shl rdx, 32
  or rax, rdx

  pop rdx
  ret

; ---------
; Task

; Push Context
%macro KSAVECONTEXT 0
  push rbp
  push rax
  push rbx
  push rcx
  push rdx
  push rdi
  push rsi

  push r8
  push r9
  push r10
  push r11
  push r12
  push r13
  push r14
  push r15

  mov ax, ds
  push rax

  mov ax, es
  push rax
  
  push fs
  push gs
%endmacro

; pop Context
%macro KLOADCONTEXT 0
  pop gs
  pop fs
  
  pop rax
  mov es, ax

  pop rax
  mov ds, ax

  pop r15
  pop r14
  pop r13
  pop r12
  pop r11
  pop r10
  pop r9
  pop r8

  pop rsi
  pop rdi
  pop rdx
  pop rcx
  pop rbx
  pop rax
  pop rbp
%endmacro


; Context Save to Current Context & Load Context from Next Context
;   PARAM Current Context, Next Context
kSwitchContext:
  push rbp;
  mov rbp, rsp

  ; if current context is NULL go to Load Context
  pushfq
  cmp rdi, 0
  je .LoadContext
  popfq

  push rax

  ; SS
  mov ax, ss
  mov qword[rdi + (23 * 8)], rax

  ; RSP
  mov rax, rbp
  add rax, 16 ; RBP + Reurn Addr Size = 16
  mov qword[rdi + (22 * 8)], rax

  ; RFLAGS
  pushfq
  pop rax
  mov qword[rdi + (21 * 8)], rax

  ; CS
  mov ax, cs
  mov qword[rdi + (20 * 8)], rax

  ; RIP
  mov rax, qword[rbp + 8] ; Return address 
  mov qword[rdi + (19 * 8)], rax

  pop rax
  pop rbp

  ; RSP change for KSAVECONTEX
  add rdi, (19 * 8)
  mov rsp, rdi
  sub rdi, (19 * 8)

  KSAVECONTEXT

.LoadContext:
  ; Next Context
  mov rsp, rsi

  KLOADCONTEXT
  iretq

; Processor to ready state
;   PARAM None
kHlt:
  hlt
  hlt
  ret

; compare Destination and Compare, then if same, assign source
;   PARAM Destination(rdi), Compare(rsi), Source(rdx)
kTestAndSet:
  mov rax, rsi  

  lock cmpxchg byte[rdi], dl
  je .SUCCESS

.NOTSAME: ; Destination and Compare are diff
  mov rax, 0x00
  ret

.SUCCESS: ; Destination and Compare are same
  mov rax, 0x01
  ret

; Init FPU
;   PARAM : None
kInitializeFPU:
  finit
  ret

; Save FPU Context to rdi
;   PARAM : Buffer Address(rdi)
kSaveFPUContext:
  fxsave [rdi]
  ret

; Load FPU Context to rdi
;   PARAM : Buffer Address(rdi)
kLoadFPUContext:
  fxrstor [rdi]
  ret

; Set TS Bit(7) in CR0 
;   PARAM : None
kSetTS:
  push rax

  mov rax, cr0
  or rax, 0x08
  mov cr0, rax

  pop rax
  ret

; Clear TS Bit(1) in CR0 
;   PARAM : None
kClearTS:
  clts
  ret