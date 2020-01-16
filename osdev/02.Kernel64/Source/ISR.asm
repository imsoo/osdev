[BITS 64]	; set code 64 bit

SECTION .text	; define text segment

extern kCommonExceptionHandler, kCommonInterruptHandler, kKeyboardHandler, kMouseHandler, kTimerHandler, kDeviceNotAvailableHandler, kHDDHandler

; Exception ISR
global kISRDivideError, kISRDebug, kISRNMI, kISRBreakPoint, kISROverflow
global kISRBoundRangeExceeded, kISRInvalidOpcode, kISRDeviceNotAvailable, kISRDoubleFault,
global kISRCoprocessorSegmentOverrun, kISRInvalidTSS, kISRSegmentNotPresent
global kISRStackSegmentFault, kISRGeneralProtection, kISRPageFault, kISR15
global kISRFPUError, kISRAlignmentCheck, kISRMachineCheck, kISRSIMDError, kISRETCException

; Interrup ISR
global kISRTimer, kISRKeyboard, kISRSlavePIC, kISRSerial2, kISRSerial1, kISRParallel2
global kISRFloppy, kISRParallel1, kISRRTC, kISRReserved, kISRNotUsed1, kISRNotUsed2
global kISRMouse, kISRCoprocessor, kISRHDD1, kISRHDD2, kISRETCInterrupt

; MACRO : save context
%macro KSAVECONTEXT 0       
    push rbp
    mov rbp, rsp
    push rax
    push rbx
    push rcx
    push rdx
    push rdi
    push rsi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    mov ax, ds     
    push rax       
    mov ax, es
    push rax
    push fs
    push gs

    ; change segment selector
    mov ax, 0x10    ; set kernel data segment
    mov ds, ax      ; 
    mov es, ax      ; 
    mov gs, ax
    mov fs, ax
%endmacro


; MACRO : reload context
%macro KLOADCONTEXT 0   
    pop gs
    pop fs
    pop rax
    mov es, ax      
    pop rax         
    mov ds, ax

    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rsi
    pop rdi
    pop rdx
    pop rcx
    pop rbx
    pop rax
    pop rbp
%endmacro  



; Exception Handler
; #0, Divide Error ISR
kISRDivideError:
    KSAVECONTEXT

    mov rdi, 0 ; exception number
    call kCommonExceptionHandler

    KLOADCONTEXT    
    iretq         

; #1, Debug ISR
kISRDebug:
    KSAVECONTEXT   

    mov rdi, 1 ; exception number
    call kCommonExceptionHandler

    KLOADCONTEXT    
    iretq          

; #2, NMI ISR
kISRNMI:
    KSAVECONTEXT    

    mov rdi, 2 ; exception number
    call kCommonExceptionHandler

    KLOADCONTEXT    
    iretq           

; #3, BreakPoint ISR
kISRBreakPoint:
    KSAVECONTEXT    

    mov rdi, 3 ; exception number
    call kCommonExceptionHandler

    KLOADCONTEXT    
    iretq         

; #4, Overflow ISR
kISROverflow:
    KSAVECONTEXT    

    mov rdi, 4 ; exception number
    call kCommonExceptionHandler

    KLOADCONTEXT   
    iretq          

; #5, Bound Range Exceeded ISR
kISRBoundRangeExceeded:
    KSAVECONTEXT    
    
    mov rdi, 5 ; exception number
    call kCommonExceptionHandler

    KLOADCONTEXT    
    iretq           

; #6, Invalid Opcode ISR
kISRInvalidOpcode:
    KSAVECONTEXT    

    mov rdi, 6 ; exception number
    call kCommonExceptionHandler

    KLOADCONTEXT    
    iretq         

; #7, Device Not Available ISR
kISRDeviceNotAvailable:
    KSAVECONTEXT   

    mov rdi, 7 ; exception number
    call kDeviceNotAvailableHandler

    KLOADCONTEXT    
    iretq           

; #8, Double Fault ISR
kISRDoubleFault:
    KSAVECONTEXT    

    mov rdi, 8 ; exception number
    mov rsi, qword [ rbp + 8 ]  ; error code
    call kCommonExceptionHandler

    KLOADCONTEXT    
    add rsp, 8  ; remove error code
    iretq           

; #9, Coprocessor Segment Overrun ISR
kISRCoprocessorSegmentOverrun:
    KSAVECONTEXT  

    mov rdi, 9 ; exception number
    call kCommonExceptionHandler

    KLOADCONTEXT
    iretq          

; #10, Invalid TSS ISR
kISRInvalidTSS:
    KSAVECONTEXT    

    mov rdi, 10 ; exception number
    mov rsi, qword [ rbp + 8 ]
    call kCommonExceptionHandler

    KLOADCONTEXT    
    add rsp, 8  ; remove error code      
    iretq           

; #11, Segment Not Present ISR
kISRSegmentNotPresent:
    KSAVECONTEXT    

    mov rdi, 11 ; exception number
    mov rsi, qword [ rbp + 8 ]  ; error code
    call kCommonExceptionHandler

    KLOADCONTEXT    
    add rsp, 8  ; remove error code      
    iretq           

; #12, Stack Segment Fault ISR
kISRStackSegmentFault:
    KSAVECONTEXT    

    mov rdi, 12 ; exception number
    mov rsi, qword [ rbp + 8 ]  ; error code
    call kCommonExceptionHandler

    KLOADCONTEXT    
    add rsp, 8  ; remove error code      
    iretq           

; #13, General Protection ISR
kISRGeneralProtection:
    KSAVECONTEXT    

    mov rdi, 13 ; exception number
    mov rsi, qword [ rbp + 8 ]  ; error code
    call kCommonExceptionHandler

    KLOADCONTEXT    
    add rsp, 8  ; remove error code     
    iretq          

; #14, Page Fault ISR
kISRPageFault:
    KSAVECONTEXT    

    mov rdi, 14 ; exception number
    mov rsi, qword [ rbp + 8 ]  ; error code
    call kCommonExceptionHandler

    KLOADCONTEXT    
    add rsp, 8  ; remove error code      
    iretq           

; #15, Reserved ISR
kISR15:
    KSAVECONTEXT   

    mov rdi, 15 ; exception number
    call kCommonExceptionHandler

    KLOADCONTEXT    
    iretq      

; #16, FPU Error ISR
kISRFPUError:
    KSAVECONTEXT    

    mov rdi, 16 ; exception number
    call kCommonExceptionHandler

    KLOADCONTEXT    
    iretq           

; #17, Alignment Check ISR
kISRAlignmentCheck:
    KSAVECONTEXT    

    mov rdi, 17 ; exception number
    mov rsi, qword [ rbp + 8 ]  ; error code
    call kCommonExceptionHandler

    KLOADCONTEXT    
    add rsp, 8  ; remove error code      
    iretq          

; #18, Machine Check ISR
kISRMachineCheck:
    KSAVECONTEXT   

    mov rdi, 18 ; exception number
    call kCommonExceptionHandler

    KLOADCONTEXT    
    iretq           

; #19, SIMD Floating Point Exception ISR
kISRSIMDError:
    KSAVECONTEXT    

    mov rdi, 19 ; exception number
    call kCommonExceptionHandler

    KLOADCONTEXT   
    iretq           

; #20~#31, Reserved ISR
kISRETCException:
    KSAVECONTEXT    

    mov rdi, 20 ; exception number
    call kCommonExceptionHandler

    KLOADCONTEXT    
    iretq         
    

;---------------------------
; Interrupt ISR
; #32, Timer ISR
kISRTimer:
    KSAVECONTEXT    

    mov rdi, 32
    call kTimerHandler

    KLOADCONTEXT    
    iretq           

; #33, Keyboard ISR
kISRKeyboard:
    KSAVECONTEXT     

    mov rdi, 33
    call kKeyboardHandler

    KLOADCONTEXT     
    iretq            

; #34, Slave PIC ISR
kISRSlavePIC:
    KSAVECONTEXT     

    mov rdi, 34
    call kCommonInterruptHandler

    KLOADCONTEXT     
    iretq            

; #35, Serial Port 2 ISR
kISRSerial2:
    KSAVECONTEXT     

    mov rdi, 35
    call kCommonInterruptHandler

    KLOADCONTEXT     
    iretq            

; #36, Serial Port 1 ISR
kISRSerial1:
    KSAVECONTEXT     

    mov rdi, 36
    call kCommonInterruptHandler

    KLOADCONTEXT     
    iretq            

; #37, Parallel Port 2 ISR
kISRParallel2:
    KSAVECONTEXT     

    mov rdi, 37
    call kCommonInterruptHandler

    KLOADCONTEXT     
    iretq            

; #38, Floppy disk controller ISR
kISRFloppy:
    KSAVECONTEXT     

    mov rdi, 38
    call kCommonInterruptHandler

    KLOADCONTEXT     
    iretq            

; #39, Parallel Port 1 ISR
kISRParallel1:
    KSAVECONTEXT     

    mov rdi, 39
    call kCommonInterruptHandler

    KLOADCONTEXT     
    iretq            

; #40, RTC ISR
kISRRTC:
    KSAVECONTEXT     

    mov rdi, 40
    call kCommonInterruptHandler

    KLOADCONTEXT     
    iretq            

; #41, RESERVED ISR
kISRReserved:
    KSAVECONTEXT     

    mov rdi, 41
    call kCommonInterruptHandler

    KLOADCONTEXT     
    iretq            

; #42, Unused
kISRNotUsed1:
    KSAVECONTEXT     

    mov rdi, 42
    call kCommonInterruptHandler

    KLOADCONTEXT     
    iretq            

; #43, Unused
kISRNotUsed2:
    KSAVECONTEXT     
 
    mov rdi, 43
    call kCommonInterruptHandler

    KLOADCONTEXT     
    iretq            

; #44, Mouse ISR
kISRMouse:
    KSAVECONTEXT     

    mov rdi, 44
    call kMouseHandler

    KLOADCONTEXT     
    iretq            

; #45, CoProcessor ISR
kISRCoprocessor:
    KSAVECONTEXT     
 
    mov rdi, 45
    call kCommonInterruptHandler

    KLOADCONTEXT     
    iretq            

; #46, HDD 1 ISR
kISRHDD1:
    KSAVECONTEXT     
     
    mov rdi, 46
    call kHDDHandler

    KLOADCONTEXT     
    iretq            

; #47, HDD 2 ISR
kISRHDD2:
    KSAVECONTEXT     
 
    mov rdi, 47
    call kHDDHandler

    KLOADCONTEXT     
    iretq            

; #48 ETC ISR
kISRETCInterrupt:
    KSAVECONTEXT     
 
    mov rdi, 48
    call kCommonInterruptHandler

    KLOADCONTEXT     
    iretq            
