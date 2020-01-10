[ORG 0x00]	; set code start addr 0x00
[BITS 16]	; set code 16 bit

SECTION .text	; define text segment

jmp 0x07C0:START	; copy 0x07C0 to CS register and jump START label

TOTAL_SECTOR_COUNT: dw 0x02 ; os image size (sector count)
KERNEL32_SECTOR_COUNT: dw 0x02 ; prevention mode kernel size (sector count)
BOOTSTRAP_PROCESSOR: db 0x01  ; BootStrap Processor Flag (0x00 = AP)

;-----------------------------------
; code section begin
START:
  mov ax, 0x07C0	; set DS register 0x7C00 (bootloader addr)
  mov ds, ax
  mov ax, 0xB800	; set ES register 0xB800 (video memory addr)
  mov es, ax

  ; init stack
  mov ax, 0x0000  ; set stack addr 0x0000
  mov ss, ax
  mov sp, 0xFFFE  ; SP, BP 64KB
  mov bp, 0xFFFE

  ; clear screen
  mov si, 0		
.SCREEN_CLEAR_LOOP:
  mov byte[es:si], 0	; set ES(video char) to zero
  mov byte[es:si+1], 0x0A	; set ES+1(video property) 
  add si, 2
  cmp si, 80 * 25 * 2	; Row * Col * (char + property) 
  jl .SCREEN_CLEAR_LOOP

  ; print welcom message
  push WELCOM_MESSAGE
  push 0
  push 0
  call PRINT_MESSAGE
  add sp, 6

  ; print loading message
  push IMAGE_LOADING_MESSAGE
  push 1
  push 0
  call PRINT_MESSAGE
  add sp, 6

  ; load os image from disk

  ; first reset before to read
RESET_DISK:
  ; call BIOS reset function
  mov ax, 0
  mov dl, 0
  int 0x13
  jc HANDLE_DISK_ERROR

  ; read disk sector
  mov si, 0x1000  ; addr(x10000) to copy os image
  mov es, si 
  mov bx, 0x0000  ; addr 0x1000:0000

  mov di, word [TOTAL_SECTOR_COUNT] ; DI (sector count to read)

READ_DATA:
  cmp di, 0
  je READ_END
  sub di, 0x1

  ; BIOS Read Function  
  mov ah, 0x02  ; function num (0x02 read sector)
  mov al, 0x1   ; sector count to read (0x1 one at time)
  mov ch, byte[TRACK_NUMBER] ; track idx to read
  mov cl, byte[SECTOR_NUMBER] ; sector idx to read
  mov dh, byte[HEAD_NUMBER] ; header idx to read
  mov dl, 0x00  ; drive id (floppy : 0x0)
  int 0x13      ; make software interrupt (call function)
  jc HANDLE_DISK_ERROR  ; if CF : 1 call error handle function

  ; calc dest addr, track, head, sector idx
  add si, 0x0020 ; add 0x200(512) to dest addr
  mov es, si

  ; add 1 sector idx
  mov al, byte[SECTOR_NUMBER]
  add al, 0x01
  mov byte[SECTOR_NUMBER], al
  cmp al, 19
  jl READ_DATA

  ; if sector read done 1 ~ 18
  mov byte[SECTOR_NUMBER], 0x01  ; set sector idx to 1
  xor byte[HEAD_NUMBER], 0x01  ; toggle header idx 0->1, 1->0

  cmp byte[HEAD_NUMBER], 0x00
  jne READ_DATA

  ; if header read done 0~1
  add byte[TRACK_NUMBER], 0x01 ; add 1 track idx
  jmp READ_DATA
READ_END:

  ; print load finished message
  push LOADING_COMPLETE_MESSAGE
  push 1
  push 20
  call PRINT_MESSAGE
  add sp, 6

  ; run loaded os image
  jmp 0x1000:0x0000

; code section end
;-----------------------------------

;-----------------------------------
; data section begin

; about message
WELCOM_MESSAGE: db 'OS Boot Loader Start...', 0
DISK_ERROR_MESSAGE: db 'DISK Error....', 0
IMAGE_LOADING_MESSAGE: db 'OS Image Loading...', 0
LOADING_COMPLETE_MESSAGE: db 'OS Image Loading Complete...', 0

; about disk read
SECTOR_NUMBER: db 0x02 ; os image start sector
HEAD_NUMBER: db 0x00 ; os image start header
TRACK_NUMBER: db 0x00  ; os image start track

; data section end
;-----------------------------------

;-----------------------------------
; function code section begin

HANDLE_DISK_ERROR:  ; disk read error handler
  push DISK_ERROR_MESSAGE
  push 1
  push 20
  call PRINT_MESSAGE

  jmp $

; message print function
;   PARAM: x coord, y coord, msg
PRINT_MESSAGE: 
  push bp   
  mov bp, sp  
    
  push es     ; register backup
  push si
  push di
  push ax
  push cx
  push dx

  ; set ES register (video memory : 0xB8000)
  mov ax, 0xB800
  mov es, ax

  ; calc dest addr
  ; Y coor
  mov ax, word[bp+6]  ; y param
  mov si, 160         ; one line (2 * 80)
  mul si
  mov di, ax          ; result to DI

  ; X coord
  mov ax, word[bp+4]  ; x param
  mov si, 2           ; char + property
  mul si
  add di, ax          ; Y addr + X addr to DI

  ; msg param
  mov si, word[bp+8]
.MESSAGE_LOOP:
  mov cl, byte[si]    ; terminate check
  cmp cl, 0
  je .MESSAGE_END

  mov byte[es:di], cl ; video + Y + X
  add si, 1           ; msg addr + 1
  add di, 2           ; video addr + 2

  jmp .MESSAGE_LOOP

.MESSAGE_END:
  pop dx
  pop cx
  pop ax
  pop di
  pop si
  pop es
  pop bp
  ret

; function code section end
;-----------------------------------


  times 510 - ( $ - $$ ) db 0x00

  db 0x55	; boot sector sign 0x55 0xAA
  db 0xAA