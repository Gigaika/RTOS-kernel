//
// Created by Aleksi on 12/02/2020.
//

#ifndef MRTOS_OS_THREADS_H
#define MRTOS_OS_THREADS_H


#include "mrtos_config.h"
#include "os_core.h"


/* ----------------------------------------------- Global variables ----------------------------------------------- */
extern OS_TCBTypeDef *idlePtr;
extern OS_TCBTypeDef *runPtr;

extern OS_TCBTypeDef *readyHeadPtr;
extern OS_TCBTypeDef *readyTailPtr;

extern OS_TCBTypeDef *sleepHeadPtr;
extern OS_TCBTypeDef *sleepTailPtr;

extern OS_TCBTypeDef *blockHeadPtr;
extern OS_TCBTypeDef *blockTailPtr;


/* -------------------------------------------- Test helper functions -------------------------------------------- */
#if TEST
void OS_ResetThreads();
OS_TCBTypeDef *OS_GetReadyThreadByIdentifier(const char *identifier);
OS_TCBTypeDef *OS_GetBlockedThreadByIdentifier(const char *identifier);
OS_TCBTypeDef *OS_GetSleepingThreadByIdentifier(const char *identifier);
#endif


/* ----------------------------------------- Thread management functions ------------------------------------------ */
/**
 * @brief: Creates a new thread and adds it to the ready thread list
 * @param function: Pointer to the thread function, which takes a single void pointer argument,
 *                  that can be used to pass in data. The function is responsible for interpreting the pointer correctly
 * @param priority: The fixed priority to be assigned for the task
 * @param stackPtr: Pointer to the pre-allocated stack memory
 * @param: stackSize: How many elements the stack has been allocated for
 * @param: identifier: Should be a string literal that can be used to identify this thread
 *
 */
void OS_CreateThread(void (*function)(void *), StackElementTypeDef *stkPtr, uint32_t stackSize, uint32_t priority, const char *identifier);

void OS_CreatePeriodicThread(void (*function)(void *), uint32_t priority, StackElementTypeDef *stackPtr, uint32_t periodMillis);

/***
 * @brief: Creates the idle thread from the parameters. Only called once during the initialization sequence.+
 * @param function: Pointer to the thread function, which takes a single void pointer argument,
 *                  that can be used to pass in data. The function is responsible for interpreting the pointer correctly
 * @param stackPtr: Pointer to the pre-allocated stack memory
 * @param: stackSize: How many elements the stack has been allocated for
 */
void OS_CreateIdleThread(void (*idleFunction)(void *), StackElementTypeDef *idleStkPtr, uint32_t stackSize);

void OS_ReadyListInsert(OS_TCBTypeDef *thread);
void OS_ReadyListRemove(OS_TCBTypeDef *thread);
void OS_SleepListInsert(OS_TCBTypeDef *thread);
void OS_SleepListRemove(OS_TCBTypeDef *thread);
void OS_BlockedListInsert(OS_TCBTypeDef *thread);
void OS_BlockedListRemove(OS_TCBTypeDef *thread);


#endif //MRTOS_OS_THREADS_H
