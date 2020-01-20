[BITS 64]           ; set code 64 bit

SECTION .text       ; define text segment

global _START, ExecuteSystemCall

extern Main, exit

; Entry Point
;   Call Application Main Function, call ExitTask System Call
_START:
  call Main

  mov rdi, rax
  call exit

  jmp $

  ret

; Execute System Call
;   PARAM: ServiceNumber, pstParameter
ExecuteSystemCall:
    push rcx        
    push r11        
    

    ; syscall
    syscall         
    
    ; restore context
    pop r11         
    pop rcx
    ret
