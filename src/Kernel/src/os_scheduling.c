//
// Created by Aleksi on 12/02/2020.
//

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include "os_scheduling.h"
#include "os_core.h"
#include "os_threads.h"
#include "bsp.h"

void OS_Suspend(void) {
    BSP_TriggerPendSV();
}

void OS_Sleep(uint32_t milliseconds) {
    uint32_t priority = OS_CriticalEnter();
    runPtr->sleep = milliseconds;
    OS_ReadyListRemove(runPtr);
    OS_SleepListInsert(runPtr);
    OS_CriticalExit(priority);
    OS_Suspend();
}

void OS_Schedule(void) {
    uint32_t pri = OS_CriticalEnter();
    OS_TCBTypeDef *nextToRun = idlePtr;
    OS_TCBTypeDef *tmpPtr = readyHeadPtr;

    if (readyHeadPtr == NULL) {
        runPtr = idlePtr;
        OS_CriticalExit(pri);
        return;
    }

    if (runPtr == idlePtr) {
        tmpPtr = readyHeadPtr;
    }

    while(tmpPtr != NULL) {
        if (tmpPtr->prev == runPtr) {
            // To conserve round robin when runPtr has threads following it with same priority
            if (tmpPtr->priority <= nextToRun->priority) {
                nextToRun = tmpPtr;
            }
        } else {
            if (tmpPtr->priority < nextToRun->priority) {
                nextToRun = tmpPtr;
            }
        }

        tmpPtr = tmpPtr->next;
    }

    runPtr = nextToRun;
    OS_CriticalExit(pri);
}
