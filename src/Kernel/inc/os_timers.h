//
// Created by Aleksi on 12/02/2020.
//

#ifndef MRTOS_OS_TIMERS_H
#define MRTOS_OS_TIMERS_H

#include "mrtos_config.h"
#include "stdint.h"
#include "os_semaphore.h"

#if TEST
void OS_ResetTimers();
#endif

typedef struct OS_PeriodicEventStruct {
    uint32_t used;
    OS_SemaphoreObjectTypeDef *semaphore;
    void (*callback)(void);
    uint32_t basePeriod;
    uint32_t period;
} OS_PeriodicEventTypeDef;

// Global timers
extern OS_PeriodicEventTypeDef periodicEvents[NUM_SOFT_TIMERS];

/***
 * @brief: Creates a software timer that executes the callback periodically. The timer is derived from SysTick,
 *         so the jitter will vary depending on the SysTick period.
 * @param periodMillis: The period for the timer in milliseconds
 * @param callback: The function that should be called. Will be called from an established critical section.
 * @return: An unique identifier that can be used to disable the timer later
 */
int32_t OS_CreateSoftwareTimer(void (*callback)(void), uint32_t periodMillis);

/***
 * @brief: Destroys the software timer corresponding to the identifier
 * @param identifier: The identifier that was received when the timer was created
 */
void OS_DestroySoftwareTimer(uint32_t identifier);


/***
 * @brief: Signals the provided semaphore periodically. The timer is derived from SysTick,
 *         so the jitter will vary depending on the SysTick period.
 * @param periodMillis: The period for the timer in milliseconds
 * @param semaphore: The semaphore that should be signalled
 * @return: An unique identifier that can be used to disable the timer late
 */
int32_t OS_StartPeriodicSignal(OS_SemaphoreObjectTypeDef *semaphore, uint32_t periodMillis);

/***
 * @brief: Disables signalling a specific semaphore, that corresponds to the identifier
 * @param identifier: The identifier that was received when the periodic signalling was initiated
 */
void OS_StopPeriodicSignal(uint32_t identifier);


#endif //MRTOS_OS_TIMERS_H
