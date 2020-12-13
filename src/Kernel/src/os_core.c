//
// Created by Aleksi on 12/02/2020.
//


#include <assert.h>
#include "os_core.h"
#include "stddef.h"
#include "os_threads.h"


/* --------------------------------------------- Private variables ----------------------------------------------- */
static uint64_t sysTickCount = 0;  // The amount of SysTicks since last scheduler execution


/* ---------------------------------------- Private function declarations ----------------------------------------- */
/***
 * @brief: Gets called from the SysTick interrupt. Should handle all the housekeeping tasks that are synchronized
 *         with the SysTick timer (thread sleep handling, periodic OS timer handling)
 */
static uint32_t OS_SysTickCallback(void);


/* -------------------------------------------- Function definitions ---------------------------------------------- */
/**
 * @brief: Resets the internal state of the operating system. Only compiled for tests.
 */
void OS_ResetState() {
    OS_ResetThreads();
}


/* --------------------------------------------- OS Startup functions --------------------------------------------- */
void OS_Init(void (*idleFunction)(void *), StackElementTypeDef *idleStackPtr, uint32_t idleStackSize) {
    DisableInterrupts();
    // Configure system clock using the board support package
    BSP_SysClockConfig();
    // Configure hardware interrupts needed by OS (PendSV, SysTick)
    BSP_HardwareInit();
    // Create the idle thread and initialize the idlePtr
    OS_CreateIdleThread(idleFunction, idleStackPtr, idleStackSize);
}

void OS_Launch(void) {
    // All initialization has been completed at this point, enable interrupts
    EnableInterrupts();
    // Run the scheduler (PendSV ISR written in assembly)
    BSP_TriggerPendSV();
}

/* ----------------------------------------- SysTick handler and callback ----------------------------------------- */
uint64_t OS_GetSysTickCount(void) {
    return sysTickCount;
}

/***
 * @brief: Handler for the SysTick interrupt, is responsible for triggering scheduler (PendSV) after a thread has
 *         used its time slice. Also used for deriving software timers and implementing thread sleeping.
 */
void SysTick_Handler() {
    static uint32_t currentTickCount = 0;  // The amount of SysTicks since last scheduler execution
    currentTickCount++;
    sysTickCount++;

    uint32_t shouldRunScheduler = 0;
    shouldRunScheduler = OS_SysTickCallback();
    // Since the time slice for each thread might be longer than the SysTick period, check if enough SysTicks have
    // been observed since the last time scheduler was ran
    if (currentTickCount * SYS_TICK_PERIOD_MILLIS >= THREAD_TIME_SLICE_MILLIS || shouldRunScheduler) {
        BSP_TriggerPendSV();
        currentTickCount = 0;
    }
}

static uint32_t OS_SysTickCallback() {
    uint32_t priority = OS_CriticalEnter();
    uint32_t shouldRunScheduler = 0;

    // Iterate through the complete thread list and decrement all sleep counters by the period of the SysTick
    OS_TCBTypeDef *tmpPtr = sleepHeadPtr;
    OS_TCBTypeDef *tmpPtr2;
    while (tmpPtr != NULL) {
        // Because tmpPtr->next might point to a ready thread at the end of loop if tmpPtr stops sleeping
        tmpPtr2 = tmpPtr->next;
        // If value would go to zero (or roll over), make thread ready
        if (tmpPtr->sleep <= SYS_TICK_PERIOD_MILLIS) {
            OS_SleepListRemove(tmpPtr);
            OS_ReadyListInsert(tmpPtr);
            tmpPtr->sleep = 0;
            // After scheduler has been flagged to run no reason the check for it anymore
            if (!shouldRunScheduler) {
                // If new ready to run thread higher priority then runPtr, schedule it to run afterwards
                if (tmpPtr->priority < runPtr->priority) {
                    shouldRunScheduler = 1;
                }
            }
        } else {
            tmpPtr->sleep -= SYS_TICK_PERIOD_MILLIS;
        }

        tmpPtr = tmpPtr2;
    }

    // Iterate through the periodic thread list and decrement all period counters
    OS_TCBTypeDef **listPtr = getPeriodicListPtr();
    while (*listPtr != NULL) {
        // If value would go to zero (or roll over), make thread ready
        if ((*listPtr)->period <= SYS_TICK_PERIOD_MILLIS) {
            (*listPtr)->period = (*listPtr)->basePeriod;

            // Check to avoid double insertion to ready list, in case thread is still executing (in ready list)
            if ((*listPtr)->hasFullyRan) {
                OS_ReadyListInsert((*listPtr));
                (*listPtr)->hasFullyRan = 0;
                // After scheduler has been flagged to run no reason the check for it anymore
                if (!shouldRunScheduler) {
                    // If new ready to run thread higher priority then runPtr, schedule it to run afterwards
                    if ((*listPtr)->priority < runPtr->priority) {
                        shouldRunScheduler = 1;
                    }
                }
            }
        } else {
            (*listPtr)->period -= SYS_TICK_PERIOD_MILLIS;
        }

        listPtr++;
    }

    OS_CriticalExit(priority);
    return shouldRunScheduler;
}