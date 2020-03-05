//
// Created by Aleksi on 12/02/2020.
//

#ifndef MRTOS_OS_SCHEDULING_H
#define MRTOS_OS_SCHEDULING_H

#include "stdint.h"

typedef enum {
    OS_SUSPEND_BLOCK       = 0x0,
    OS_SUSPEND_UNBLOCK     = 0x1,
    OS_SUSPEND_SLEEP       = 0x2,
    OS_SUSPEND_RELINQUISH  = 0x3
} OS_Suspend_Cause;

/**
 * @brief: Suspends the current task and forces the scheduler to run.
 */
void OS_Suspend(OS_Suspend_Cause cause);

/**
 * @brief: Updates the sleep field of the currently executing task, and suspends the current thread afterwards
 * @param milliseconds: Time to sleep in milliseconds
 * @return : None
 */
void OS_Sleep(uint32_t milliseconds);

void OS_Schedule(void);

#endif //MRTOS_OS_SCHEDULING_H
