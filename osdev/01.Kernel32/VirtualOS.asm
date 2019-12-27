[ORG 0x00]	; set code start addr 0x00
[BITS 16]	; set code 16 bit

SECTION .text	; define text segment

jmp 0x1000:START	; copy 0x1000 to CS register and jump START label

SECTOR_COUNT: dw 0x000 ; current sector num
TOTAL_SECTOR_COUNT: equ 1024  ; total sector num of virtual os

;-----------------------------------
; code section begin
START:
  mov ax, cs ; ax 0x1000
  mov ds, ax  ; ds 0x1000
  mov ax, 0xB800
  mov es, ax  ; es 0xB800


  ; make each sector code
  %assign i 0
  %rep TOTAL_SECTOR_COUNT

  %assign i i+1
    mov ax, 2
    mul word[SECTOR_COUNT]
    mov si, ax
    mov byte[es:si + (160 * 2)], '0' + (i % 10)

    add word[SECTOR_COUNT], 1

    ; last sector 
    %if i == TOTAL_SECTOR_COUNT 
      jmp $
    %else
      jmp (0x1000 + i * 0x20): 0x0000
    %endif

    times (512 - ( $ - $$ ) % 512) db 0x00

  %endrep
; code section end
;-----------------------------------