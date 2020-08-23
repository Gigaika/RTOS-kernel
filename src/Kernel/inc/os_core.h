//
// Created by Aleksi on 12/02/2020.
//

#ifndef MRTOS_OS_CORE_H
#define MRTOS_OS_CORE_H


#include "mrtos_config.h"
#include "stdint.h"

/* --------------------------------------- Type definitions and structures --------------------------------------- */
typedef enum {
    READY,
    BLOCKED,
    ASLEEP,
    INACTIVE
} OS_StateTypeDef;
// Forward definition as OS_SemaphoreObjectTypeDef depends on OS_TCBTypeDef, and vice versa
typedef struct OS_SemaphoreStruct OS_SemaphoreObjectTypeDef;

typedef struct OS_TCBStruct OS_TCBTypeDef;
struct OS_TCBStruct {
    StackElementTypeDef *stkPtr;
    OS_TCBTypeDef *next;
    OS_TCBTypeDef *prev;
    const char *identifier;
    uint8_t id;
    uint32_t stackSize;
    uint32_t basePriority;
    uint32_t priority;
    OS_SemaphoreObjectTypeDef *blockPtr;
    uint32_t sleep;
    uint32_t basePeriod;
    uint32_t period;
    uint32_t hasFullyRan;
    OS_StateTypeDef state;
};


/* -------------------------------------------- Test helper functions -------------------------------------------- */
#if TEST
void OS_ResetState(void);
void SysTick_Handler(void);
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


/* --------------------------------------------- Utility functions ---------------------------------------------- */
uint64_t OS_GetSysTickCount(void);


#endif //MRTOS_OS_CORE_H
