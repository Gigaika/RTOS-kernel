  EXTERN runPtr
  EXTERN firstSwitch
  EXTERN OS_Schedule
  
  PUBLIC PendSV_Handler
  PUBLIC OS_CriticalEnter
  PUBLIC OS_CriticalExit
  PUBLIC OS_DisableInterrupts
  PUBLIC OS_EnableInterrupt
  
  SECTION .text:CODE:NOROOT(3)
  THUMB

; Does the context switch for the OS, scheduling (updating value of runPt) is done by OS_Schedule (C-code)
PendSV_Handler
    CPSID I            ; Disable interrupts
    MOV R1,#0x1
    LDR R0,=firstSwitch
    LDR R2,[R0]
    LDR R0,=runPtr     ; Load the runPt address to R0
    CMP R1,R2
    BEQ Schedule
    PUSH {R4-R11}      ; Push the R4-R11 registers which are not pushed automatically
    LDR R1,[R0]        ; Load the current TCB address to R1
    STR SP,[R1]        ; Update the TCB stack pointer to the current value of the SP register
Schedule
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

OS_CriticalEnter
    MRS R0, PRIMASK     ;Save old interrupt mask priority to R0 (Return register)
    CPSID I             ;Disable interrupts
    BX LR

OS_CriticalExit
    MSR PRIMASK, R0     ;Load old interrupt mask priority from R0 (1st param register)
    CPSIE I             ;Enable interrupts
    BX LR

OS_DisableInterrupts
    CPSID I

OS_EnableInterrupt
    CPSIE I

  END