[BITS 64]           ; set code 64 bit

SECTION .text       ; define text segment

global ExecuteSystemCall

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
