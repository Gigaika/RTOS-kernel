//
// Created by Aleksi on 12/02/2020.
//

#ifndef MRTOS_OS_CORE_H
#define MRTOS_OS_CORE_H


#include "mrtos_config.h"
#include "stdint.h"


/* --------------------------------------- Type definitions and structures --------------------------------------- */
// Forward definition as OS_SemaphoreObjectTypeDef depends on OS_TCBTypeDef, and vice versa
typedef struct OS_SemaphoreStruct OS_SemaphoreObjectTypeDef;

typedef struct OS_TCBStruct OS_TCBTypeDef;
struct OS_TCBStruct {
    StackElementTypeDef *stkPtr;
    OS_TCBTypeDef *next;
    OS_TCBTypeDef *prev;
    const char *identifier;
    uint32_t stackSize;
    uint32_t basePriority;
    uint32_t priority;
    OS_SemaphoreObjectTypeDef *blockPtr;
    uint32_t sleep;
};


/* -------------------------------------------- Test helper functions -------------------------------------------- */
#if TEST
void OS_ResetState(void);
#endif


/* -------------------------------------------- OS Startup functions --------------------------------------------- */
/**
 * @brief: The first function that should be called. Sets up the hardware interrupts needed by the OS using the BSP,
 *         and creates an idle thread using the parameters, that will be ran when there is nothing else to do.
 * @param: idleFunction: pointer to the thread function, which takes a single void pointer argument,
 *                  that can be used to pass in data. The function is responsible for interpreting the pointer correctly.
 * @param: stackPtr: Pointer to the pre-allocated stack memory
 * @param: idleStackSize: How many elements the stack has been allocated for
 * @return None
 */
void OS_Init(void (*idleFunction)(void *), StackElementTypeDef *idleStackPtr, uint32_t idleStackSize);

/**
 * @brief: Launches the operating system by scheduling the first thread to run
 */
void OS_Launch(void);

/* --------------------------------------------- Critical sections  ---------------------------------------------- */
/**
 * @brief: Enters a critical section (disables interrupts), written in assembly
 * @return: The value of PRIMASK register before disabling interrupts
 */
uint32_t OS_CriticalEnter(void);

/**
 * @brief: Exits a critical section (enables interrupts), written in assembly
 * @param priority: The priority level that the PRIMASK register should be set to
 */
void OS_CriticalExit(uint32_t priority);
void EnableInterrupts(void);    // Assembly
void DisableInterrupts(void);   // Assembly

/* --------------------------------------------- Utility functions ---------------------------------------------- */


#endif //MRTOS_OS_CORE_H
