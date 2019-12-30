[ORG 0x00]	; set code start addr 0x00
[BITS 16]	; set code 16 bit

SECTION .text	; define text segment

;-----------------------------------
; code section begin
START:
  mov ax, 0x1000 ; protected mode addr 0x10000
  mov ds, ax  ; ds 0x1000
  mov es, ax  ; es 0x1000

  ; A20 Gate Enable Using BIOS
  mov ax, 0x2401	; A20 gate enable service
  int 0x15			; BIOS SWI

  jc .A20_GATE_ERROR	; error handle
  jmp .A20_GATE_SUCCESS

.A20_GATE_ERROR:
  ; if A20 gate enable using bios is fail, then using system control port
  in al, 0x92	; read from system control port
  or al, 0x02	; set A20 gate bit
  and al, 0xFE  ; clear system reset bit (0)
  out 0x92, al	; write 

.A20_GATE_SUCCESS:

  cli	; interrupt off
  lgdt [GDTR]	; GDTR load

  ; enter protected mode
  ; Disable paging, cache, internal FPU, align check
  mov eax, 0x4000003B ; PG=0, CD=1, NW=0, AM=0, WP=0, NE=1, ET=1, TS=1, EM=0, MP=1, PE=1
  mov cr0, eax		; set cr0 register

  jmp dword 0x08:(PROTECTED_MODE - $$ + 0x10000)

[BITS 32]
PROTECTED_MODE:
  mov ax, 0x10	; 0x10 data segment descriptor
  mov ds, ax
  mov es, ax
  mov fs, ax
  mov gs, ax

  ; init stack
  mov ss, ax
  mov esp, 0xFFFE
  mov ebp, 0xFFFE

  ; print enter PM
  push (SWITCH_SUCCESS_MESSAGE - $$ + 0x10000)
  push 2
  push 0
  call PRINT_MESSAGE
  add esp, 12

  jmp dword 0x08:0x10200	; jump C kernel (0x10200)

;-----------------------------------
; function code section begin

; message print function
;   PARAM: x coord, y coord, msg
PRINT_MESSAGE: 
  push ebp   
  mov ebp, esp  
    
  push esi     ; register backup
  push edi
  push eax
  push ecx
  push edx

  ; calc dest addr
  ; Y coor
  mov eax, dword[ebp+12]  ; y param
  mov esi, 160         ; one line (2 * 80)
  mul esi
  mov edi, eax          ; result to EDI

  ; X coord
  mov eax, dword[ebp+8]  ; x param
  mov esi, 2           ; char + property
  mul esi
  add edi, eax          ; Y addr + X addr to EDI

  ; msg param
  mov esi, dword[ebp+16]
.MESSAGE_LOOP:
  mov cl, byte[esi]    ; terminate check
  cmp cl, 0
  je .MESSAGE_END

  mov byte[edi + 0xB8000], cl ; video + Y + X
  add esi, 1           ; msg addr + 1
  add edi, 2           ; video addr + 2

  jmp .MESSAGE_LOOP

.MESSAGE_END:
  pop edx
  pop ecx
  pop eax
  pop edi
  pop esi
  pop ebp
  ret

; function code section end
;-----------------------------------

;-----------------------------------
; data section begin

align 8, db 0

dw 0x0000

  ; GDTR
GDTR:
  dw GDT_END - GDT - 1
  dd (GDT - $$ + 0x10000)

  ; GDT Table define
GDT:
  ; NULL descriptor
  NULL_Descriptor:
    dw 0x0000
    dw 0x0000
	db 0x00
	db 0x00
	db 0x00
	db 0x00
  
  ; PM CODE descriptor
  CODE_DESCRIPTOR:
    dw 0xFFFF	; Limit [15:0]
	dw 0x0000	; Base [15:0]
	db 0x00		; Base [23:16]
	db 0x9A		; P=1, DPL=0, Code Segment, Execute/Read
	db 0xCF		; G=1, D=1, L=0, Limit[19:16]
	db 0x00		; Base [31:24]

  ; PM Data descriptor
  DATA_DESCRIPTOR:
    dw 0xFFFF	; Limit [15:0]
	dw 0x0000	; Base [15:0]
	db 0x00		; Base [23:16]
	db 0x92		; P=1, DPL=0, Data Segment, Read/Write
	db 0xCF		; G=1, D=1, L=0, Limit[19:16]
	db 0x00		; Base [31:24]
GDT_END:

; msg
SWITCH_SUCCESS_MESSAGE: db 'Switch to protected mode success...', 0

; data section end
;-----------------------------------

times 512 - ( $ - $$ ) db 0x00