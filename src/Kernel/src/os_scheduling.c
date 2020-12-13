//
// Created by Aleksi on 12/02/2020.
//

#include <stddef.h>
#include "os_scheduling.h"
#include "os_core.h"
#include "os_threads.h"
#include "bsp.h"

uint32_t firstSwitch = 1;

void OS_Suspend(OS_Suspend_Cause cause) {
    // When a periodic thread has ran fully and given up control, it should be removed from ready list to prevent it from running again
    if (cause == OS_SUSPEND_RELINQUISH) {
        if (runPtr->basePeriod != 0) {
            runPtr->hasFullyRan = 1;
            OS_ReadyListRemove(runPtr);
        }
    }

    BSP_TriggerPendSV();
}

void OS_Sleep(uint32_t milliseconds) {
    uint32_t priority = OS_CriticalEnter();
    runPtr->sleep = milliseconds;
    OS_ReadyListRemove(runPtr);
    OS_SleepListInsert(runPtr);
    OS_CriticalExit(priority);
    OS_Suspend(OS_SUSPEND_SLEEP);
}

void OS_Schedule(void) {
    uint32_t pri = OS_CriticalEnter();
    OS_TCBTypeDef *nextToRun = idlePtr;
    OS_TCBTypeDef *tmpPtr = readyHeadPtr;

    if (tmpPtr == NULL) {
        runPtr = idlePtr;
        OS_CriticalExit(pri);
        return;
    }
    
    if (firstSwitch) {
      firstSwitch = 0;
    }

    nextToRun = tmpPtr;
    // If currently running thread is highest priority, check if the next thread in ready list has same priority (for round robin)
    if (tmpPtr == runPtr) {
        if (tmpPtr->next != NULL) {
            if (tmpPtr->next->priority == runPtr->priority) {
                nextToRun = tmpPtr->next;
            }
        }
    }

    runPtr = nextToRun;
    OS_CriticalExit(pri);
}
