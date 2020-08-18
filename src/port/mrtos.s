  EXTERN runPt
  EXTERN OS_Schedule
  
  SECTION .text:CODE:NOROOT(2)
  THUMB

; PendSV_Handler
; Does the context switch for the OS, scheduling (updating value of runPt) is done by OS_Schedule (C-code)
PendSV_Handler
    CPSID I            ; Disable interrupts
    PUSH {R4-R11}      ; Push the R4-R11 registers which are not pushed automatically
    LDR R0,=runPt      ; Load the runPt address to R0
    LDR R1,[R0]        ; Load the current TCB address to R1
    STR SP,[R1]        ; Update the TCB stack pointer to the current value of the SP register

    PUSH {LR}
    PUSH {R0}
    BL OS_Schedule
    POP {R0}
    POP {LR}

    LDR R1,[R0]        ; R0 points to runPt, which points to a TCB, that was updated by the OS_Schedule function
    LDR SP,[R1]        ; Load the stack pointer of the current thread to the SP register
    POP {R4-R11}       ; Pop the R4-R11 registers which were pushed when the thread was previously switched from
    CPSIE I            ; Enable interrupts
    BX LR              ; Return from handler
DisableInterrupts
    CPSID I
EnableInterrupts
    CPSIE I
OS_CriticalEnter
    MRS R0, PRIMASK     ;Save old interrupt mask priority to R0 (Return register)
    CPSID I             ;Disable interrupts
    BX LR
OS_CriticalExit
    MSR PRIMASK, R0     ;Load old interrupt mask priority from R0 (1st param register)
    CPSIE I             ;Enable interrupts
    BX LR
  END