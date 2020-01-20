[BITS 64]           ; set code 64 bit

SECTION .text       ; define text segment

extern kProcessSystemCall

global kSystemCallEntryPoint, kSystemCallTestTask


; System Call Entry Point 
;   PARAM: ServiceNumber, Parameter
kSystemCallEntryPoint:
  push rcx
  push r11
  mov cx, ds
  push cx
  mov cx, es
  push cx
  push fs
  push gs

  mov cx, 0x10
  mov ds, cx
  mov es, cx
  mov fs, cx
  mov gs, cx

  call kProcessSystemCall

  pop gs
  pop fs
  pop cx
  mov es, cx
  pop cx
  mov ds, cx
  pop r11
  pop rcx

  o64 sysret

; Test 
kSystemCallTestTask:
    mov rdi, 0xFFFFFFFF 
    mov rsi, 0x00  
    syscall        

    mov rdi, 0xFFFFFFFF 
    mov rsi, 0x00  
    syscall        

    mov rdi, 0xFFFFFFFF
    mov rsi, 0x00  
    syscall         

    mov rdi, 24    
    mov rsi, 0x00   
    syscall    
    jmp $
