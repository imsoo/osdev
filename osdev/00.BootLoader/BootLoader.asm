[ORG 0x00]
[BITS 16]

SECTION .text

  jmp 0x07C0:START

START:
  mov ax, 0x07C0
  mov ds, ax
  mov ax, 0xB800
  mov es, ax


  mov si, 0
.SCREENCLEARLOOP:
  mov byte[es:si], 0
  mov byte[es:si+1], 0x0A
  add si, 2
  cmp si, 80 * 25 * 2
  jl .SCREENCLEARLOOP


  mov si, 0
  mov di, 0
.MESSAGELOOP:
  mov cl, byte[si + MESSAGE1]
  cmp cl, 0
  je .MESSAGEEND

  mov byte[es:di], cl

  add si, 1
  add di, 2

  jmp .MESSAGELOOP
.MESSAGEEND:

  jmp $

MESSAGE1: db 'MINT64 OS Boot Loader Start~!!', 0

  times 510 - ( $ - $$ ) db 0x00

  db 0x55
  db 0xAA