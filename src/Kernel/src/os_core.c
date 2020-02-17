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
static void OS_SysTickCallback(void);


/* -------------------------------------------- Function definitions ---------------------------------------------- */
/**
 * @brief: Resets the internal state of the operating system. Only compiled for tests.
 */
void OS_ResetState() {
    OS_ResetThreads();
}

/* --------------------------------------------- OS Startup functions --------------------------------------------- */
void OS_Init(void (*idleFunction)(void *), StackElementTypeDef *idleStackPtr, uint32_t idleStackSize) {
    // Make sure interrupts are disabled during initialization
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
    // Will also trigger scheduler to run, if it unblocks periodic events
    // TODO: implement in such way that it does not interfere with time slice scheduling as much
    OS_SysTickCallback();
    // Since the time slice for each thread might be longer than the SysTick period, check if enough SysTicks have
    // been observed since the last time scheduler was ran
    if (tickCount * SYS_TICK_PERIOD_MILLIS >= THREAD_TIME_SLICE_MILLIS) {
        BSP_TriggerPendSV();
        tickCount = 0;
    }
}

static void OS_SysTickCallback() {
    uint32_t priority = OS_CriticalEnter();

    // Iterate through the complete thread list and decrement all nonzero sleep counters by the period of the SysTick
    OS_TCBTypeDef *tmpPtr = sleepHeadPtr;
    while (tmpPtr != NULL) {
         if (tmpPtr->sleep <= SYS_TICK_PERIOD_MILLIS) {   // If value goes to zero (or rolls over), make thread ready
             OS_SleepListRemove(tmpPtr);
             OS_ReadyListInsert(tmpPtr);
         } else {
             tmpPtr->sleep -= SYS_TICK_PERIOD_MILLIS;
         }

        tmpPtr = tmpPtr->next;
    }

    // Iterate through the software timer array
    OS_PeriodicEventTypeDef *current;
    for (int i = 0; i < NUM_SOFT_TIMERS; i++) {
        current = &periodicEvents[i];

        if (!(current->used)) {
            continue;
        }

        // If the time since last trigger of the timer is bigger than the period, trigger it now and reset the tick counter
        if (++current->tickCount * SYS_TICK_PERIOD_MILLIS >= current->period) {
            if (current->callback != NULL) {
                (*current->callback)();
            } else if (current->semaphore != NULL) {
                OS_Signal(current->semaphore);
            }

            current->tickCount = 0;
        }
    }

    OS_CriticalExit(priority);
}