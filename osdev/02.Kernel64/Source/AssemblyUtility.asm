[BITS 64]	; set code 64 bit

SECTION .text	; define text segment

global kInPortByte, kOutPortByte

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