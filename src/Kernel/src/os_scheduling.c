//
// Created by Aleksi on 12/02/2020.
//

#include <assert.h>
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

static void onIdleCondition(void) {
    // TODO: Detect idle conditions, and run idle task in response
}

void OS_Schedule(void) {
    uint32_t pri = OS_CriticalEnter();
    // TODO: Implement scheduler and scheduling algorithms
    OS_CriticalExit(pri);
}
