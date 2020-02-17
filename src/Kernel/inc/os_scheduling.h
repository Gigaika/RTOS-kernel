//
// Created by Aleksi on 12/02/2020.
//

#ifndef MRTOS_OS_SCHEDULING_H
#define MRTOS_OS_SCHEDULING_H

#include "stdint.h"

/**
 * @brief: Suspends the current task and forces the scheduler to run.
 */
void OS_Suspend(void);

/**
 * @brief: Updates the sleep field of the currently executing task, and suspends the current thread afterwards
 * @param milliseconds: Time to sleep in milliseconds
 * @return : None
 */
void OS_Sleep(uint32_t milliseconds);

#endif //MRTOS_OS_SCHEDULING_H
