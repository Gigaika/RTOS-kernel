//
// Created by Aleksi on 12/02/2020.
//

#include "os_timers.h"
#include "stddef.h"
#include "memory.h"
#include <assert.h>


/* ---------------------------------------- Private function declarations ----------------------------------------- */
static int32_t initializePeriodicEvent(uint32_t periodMillis, void (*callback)(void), OS_SemaphoreObjectTypeDef *semaphore);
static void findAndReset(uint32_t identifier);


/* ----------------------------------------------- Global variables ----------------------------------------------- */
OS_PeriodicEventTypeDef periodicEvents[NUM_SOFT_TIMERS] = {0};   // pre-allocate memory for the software timers


/* ---------------------------------------------- Private variables ----------------------------------------------- */
static uint32_t timersActive = 0;


/* -------------------------------------------- Function definitions ---------------------------------------------- */
/**
 * @brief: Resets the internal state of the module. Only compiled for tests.
 */
void OS_ResetTimers(void) {
    memset(&periodicEvents, 0, sizeof(periodicEvents));
    timersActive = 0;
}

int32_t OS_CreateSoftwareTimer(void (*callback)(void), uint32_t periodMillis) {
    return initializePeriodicEvent(periodMillis, callback, NULL);
}

void OS_DestroySoftwareTimer(uint32_t identifier) {
    findAndReset(identifier);
}

int32_t OS_StartPeriodicSignal(OS_SemaphoreObjectTypeDef *semaphore, uint32_t periodMillis) {
    return initializePeriodicEvent(periodMillis, NULL, semaphore);
}

void OS_StopPeriodicSignal(uint32_t identifier) {
    findAndReset(identifier);
}

static void findAndReset(uint32_t identifier) {
    OS_PeriodicEventTypeDef *target = &periodicEvents[identifier];
    target->used = 0;
    target->period = 0;
    target->basePeriod = 0;
    target->semaphore = NULL;
    target->callback = NULL;
    timersActive--;
}

static int32_t initializePeriodicEvent(uint32_t periodMillis, void (*callback)(void), OS_SemaphoreObjectTypeDef *semaphore) {
    assert(timersActive + 1 <= NUM_SOFT_TIMERS);
    for (int i = 0; i < NUM_SOFT_TIMERS; i++) {
        if (!(periodicEvents[i].used)) {
            OS_PeriodicEventTypeDef *new = &periodicEvents[i];
            new->used = 1;
            new->period = periodMillis;
            new->basePeriod = periodMillis;
            new->semaphore = semaphore;
            new->callback = callback;
            timersActive++;
            return i;
        }
    }

    return -1;
}