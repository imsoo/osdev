[BITS 64]	; set code 64 bit

SECTION .text	; define text segment

extern Main

; In MultiProcessor
extern g_qwAPICIDAddress, g_iWakeUpApplicationProcessorCount


;-----------------------------------
; code section begin
START:
  mov ax, 0x10
  mov ds, ax
  mov es, ax
  mov fs, ax
  mov gs, ax

  mov ss, ax
  mov rsp, 0x6FFFF8
  mov rbp, 0x6FFFF8

  ; if Bootstrap Processor move to Kernel directly
  cmp byte[0x7C09], 0x01
  je .BOOTSTRAP_PROCESSOR_START_POINT

  ; if AP
  ; Read APIC ID from APIC ID Register
  mov rax, 0
  mov rbx, qword[g_qwAPICIDAddress]
  mov eax, dword[rbx]
  shr rax, 24

  ; Calc Stack Pointer 
  mov rbx, 0x10000
  mul rbx

  ; Set Stack
  sub rsp, rax
  sub rbp, rax

  lock inc dword[g_iWakeUpApplicationProcessorCount]

.BOOTSTRAP_PROCESSOR_START_POINT:
  call Main

  jmp $

