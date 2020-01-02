[BITS 64]	; set code 64 bit

SECTION .text	; define text segment

global kInPortByte, kOutPortByte, kLoadGDTR, kLoadTR, kLoadIDTR
global kEnableInterrupt, kDisableInterrupt, kReadFLAGS

; read from port one byte
; PARAM port_num
kInPortByte:
  push rdx

  mov rdx, rdi
  mov rax, 0
  in al, dx

  pop rdx
  ret

; write to port one byte
; PARAM port_num, data
kOutPortByte:
  push rdx
  push rax

  mov rdx, rdi
  mov rax, rsi
  out dx, al

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