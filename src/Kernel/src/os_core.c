//
// Created by Aleksi on 12/02/2020.
//

#include "os_core.h"
#include "stddef.h"
#include "os_threads.h"
#include "os_timers.h"
#include "bsp.h"

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
    OS_ResetTimers();
}

/* --------------------------------------------- OS Startup functions --------------------------------------------- */
void OS_Init(void (*idleFunction)(void *), StackElementTypeDef *idleStackPtr, uint32_t idleStackSize) {
    DisableInterrupts();
    // Configure system clock using the board support package
    BSP_SysClockConfig(SYSCLOCK_FREQUENCY);
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
/***
 * @brief: Handler for the SysTick interrupt, is responsible for triggering scheduler (PendSV) after a thread has
 *         used its time slice. Also used for deriving software timers and implementing thread sleeping.
 */
void SysTickHandler() {
    static uint32_t tickCount = 0;  // The amount of SysTicks since last scheduler execution
    tickCount++;

    uint32_t shouldRunScheduler = 0;
    shouldRunScheduler = OS_SysTickCallback();
    // Since the time slice for each thread might be longer than the SysTick period, check if enough SysTicks have
    // been observed since the last time scheduler was ran
    if (tickCount * SYS_TICK_PERIOD_MILLIS >= THREAD_TIME_SLICE_MILLIS || shouldRunScheduler) {
        BSP_TriggerPendSV();
        tickCount = 0;
    }
}

static uint32_t OS_SysTickCallback() {
    uint32_t priority = OS_CriticalEnter();
    uint32_t shouldRunScheduler = 0;

    // Iterate through the complete thread list and decrement all nonzero sleep counters by the period of the SysTick
    OS_TCBTypeDef *tmpPtr = sleepHeadPtr;
    while (tmpPtr != NULL) {
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

        tmpPtr = tmpPtr->next;
    }

    // Iterate through the periodic thread list and decrement all period counters
    OS_TCBTypeDef **listPtr = periodicListPtr;
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

    // Iterate through the software timer array
    OS_PeriodicEventTypeDef *current;
    for (int i = 0; i < NUM_SOFT_TIMERS; i++) {
        current = &periodicEvents[i];

        if (!(current->used)) {
            continue;
        }

        // If the time since last trigger of the timer is bigger than the period, trigger it now and reset the tick counter
        if (current->period <= SYS_TICK_PERIOD_MILLIS) {
            if (current->callback != NULL) {
                (*current->callback)();
            } else if (current->semaphore != NULL) {
                // No need to flag scheduler to run as signal will end up triggering PendSV through OS_Suspend if needed
                OS_Signal(current->semaphore);
            }
            current->period = current->basePeriod;
        } else {
            current->period -= SYS_TICK_PERIOD_MILLIS;
        }
    }

    OS_CriticalExit(priority);
    return shouldRunScheduler;
}