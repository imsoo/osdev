[BITS 64]	; set code 64 bit

SECTION .text	; define text segment

extern Main

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

  call Main

  jmp $