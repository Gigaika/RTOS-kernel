; PendSV_Handler
; Does the context switch for the OS, scheduling (updating value of runPt) is done by OS_Schedule (C-code)
PendSV_Handler:
    CPSID               ; Disable interrupts
    PUSH {R4, R11}      ; Push the R4-R11 registers which are not pushed automatically
    LDR R0, =runPt      ; Load the runPt address to R0
    LDR R1, [R0]        ; Load the current TCB address to R1
    STR [R1], SP        ; Update the TCB stack pointer to the current value of the SP register

    PUSH {R0, LR}
    BL OS_Schedule
    POP {R0, LR}

    LDR R1, [R0]        ; R0 points to runPt, which points to a TCB, that was updated by the OS_Schedule function
    LDR SP, [R1]        ; Load the stack pointer of the current thread to the SP register
    POP {R4, R11}       ; Pop the R4-R11 registers which were pushed when the thread was previously switched from
    CPSIE               ; Enable interrupts
    BX  LR              ; Return from handler

DisableInterrupts:
    CPSID

EnableInterrupts:
    CPSIE

OS_CriticalEnter:
    MRS R0, PRIMASK     ; Save old interrupt mask priority to R0 (Return register)
    CPSID               ; Disable interrupts
    BX LR

OS_CriticalExit:
    MSR PRIMASK, R0     ; Load old interrupt mask priority from R0 (1st param register)
    CPSIE               ; Enable interrupts
    BX LR