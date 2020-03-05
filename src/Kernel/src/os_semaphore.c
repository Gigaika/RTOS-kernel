//
// Created by aleksi on 02/02/2020.
//


#include <assert.h>
#include "os_semaphore.h"
#include "stddef.h"
#include "os_scheduling.h"
#include "os_core.h"
#include "os_threads.h"


/* ---------------------------------------- Private function declarations ---------------------------------------- */
/***
 * @brief: Temporarily makes the semaphore object owners priority match the currently running threads priority
 * @param semaphoreObject: The semaphore object whose owners priority will be elevated
 */
static void grantDynamicPriorityToOwner(OS_SemaphoreObjectTypeDef *semaphoreObject);

/***
 * @brief: Reverts the threads priority level to either the base priority, or to the next highest priority level,
 *         if another semaphore still wants the priority to be elevated.
 * @param semaphoreObject: The semaphore whose owners priority will be modified
 */
static void removeDynamicPriorityFromOwner(OS_SemaphoreObjectTypeDef *semaphoreObject);

/**
 * @brief: Iterates the list of blocked threads, and unblocks the highest priority task that is waiting for the semaphore.
 *         If multiple waiting threads have the same priority, the longest waiting one is chosen.
 * @param semaphore: Pointer to the semaphore that the blocked threads should be waiting for
 * @return: 1 if the unblocked thread has higher priority than the currently running task
 */
static int32_t unblockThread(OS_SemaphoreObjectTypeDef *semaphoreObject);

static void semaphoreSetOwner(OS_SemaphoreObjectTypeDef *semaphoreObject, OS_TCBTypeDef *newOwner);

static void semaphoreRemoveOwner(OS_SemaphoreObjectTypeDef *semaphoreObject);


/* -------------------------------- Semaphore initialization and modification ------------------------------------ */
void OS_InitSemaphore(OS_SemaphoreObjectTypeDef *semaphoreObject, SemaphoreType type) {
    if (type == SEMAPHORE_MUTEX) {
        semaphoreObject->value = 1;
        semaphoreObject->type = SEMAPHORE_MUTEX;
    } else if (type == SEMAPHORE_FLAG) {
        semaphoreObject->value = 0;
        semaphoreObject->type = SEMAPHORE_FLAG;
    }

    semaphoreObject->owner = NULL;
    semaphoreObject->priorityHasBeenGranted = 0;
    semaphoreObject->priorityLevelGranted = 0;
}

static void semaphoreSetOwner(OS_SemaphoreObjectTypeDef *semaphoreObject, OS_TCBTypeDef *newOwner) {
    if (semaphoreObject->type != SEMAPHORE_FLAG) {
        semaphoreObject->owner = newOwner;
    }
}

static void semaphoreRemoveOwner(OS_SemaphoreObjectTypeDef *semaphoreObject) {
    semaphoreObject->owner = NULL;
}


/* ---------------------------------------- Semaphore relinquishing ---------------------------------------------- */
static int32_t unblockThread(OS_SemaphoreObjectTypeDef *semaphoreObject) {
    OS_TCBTypeDef *tmpPtr = blockHeadPtr;

    // Iterate through the list of blocked threads, and find the highest priority thread blocked by this semaphore
    while (tmpPtr != NULL) {
        // Ordered round robin list, first element found is highest priority or longest waiting
        if (tmpPtr->blockPtr == semaphoreObject) {
            tmpPtr->blockPtr = NULL;
            OS_BlockedListRemove(tmpPtr);
            OS_ReadyListInsert(tmpPtr);
            semaphoreSetOwner(semaphoreObject, tmpPtr);

            // Return one if the unblocked thread is higher priority than currently executing thread
            return (tmpPtr->priority < runPtr->priority);
        }

        tmpPtr = tmpPtr->next;
    }

    // Block list did not contain any threads with this semaphore as the blockPtr, should never happen
    assert(0);
}

static void removeDynamicPriorityFromOwner(OS_SemaphoreObjectTypeDef *semaphoreObject) {
    // No priority gained through this semaphore, do nothing
    if ((!semaphoreObject->priorityHasBeenGranted)) {
        return;
    }

    // Something else has granted a higher priority than this semaphore, do nothing
    if (semaphoreObject->owner->priority < semaphoreObject->priorityLevelGranted) {
        semaphoreObject->priorityHasBeenGranted = 0;
        semaphoreObject->priorityLevelGranted = 0;
        return;
    }


    semaphoreObject->priorityHasBeenGranted = 0;
    semaphoreObject->priorityLevelGranted = 0;

    OS_TCBTypeDef *tmpPtr = blockHeadPtr;
    while (tmpPtr != NULL) {
        // Find threads that are being blocked by the currently running thread, but through a different semaphore
        if ((tmpPtr->blockPtr != semaphoreObject) && (tmpPtr->blockPtr->owner == semaphoreObject->owner)) {
            // The thread still owns other semaphores, so restore its priority to match the highest priority task,
            // that is being blocked by one of those semaphores, to conserve priority inheritance of the other semaphores
            semaphoreObject->owner->priority = tmpPtr->priority;
            // Re-insert the owner after modifying its priority to conserve the ordering in the thread list
            OS_ReadyListRemove(semaphoreObject->owner);
            OS_ReadyListInsert(semaphoreObject->owner);
            return;
        }

        tmpPtr = tmpPtr->next;
    }

    // If this point is reached, this is the last semaphore owned by the thread,
    // so we can safely restore its priority to the base priority level
    semaphoreObject->owner->priority = semaphoreObject->owner->basePriority;
    // Re-insert the owner after modifying its priority to conserve the ordering in the thread list
    OS_ReadyListRemove(semaphoreObject->owner);
    OS_ReadyListInsert(semaphoreObject->owner);
}

void OS_Signal(OS_SemaphoreObjectTypeDef *semaphoreObject) {
    uint32_t priority = OS_CriticalEnter();
    uint32_t shouldSuspend = 0;

    semaphoreObject->value += 1;
    if (semaphoreObject->value > 1) {
        semaphoreObject->value = 1;
    }

    // Flag semaphores and signals from ISRs should not have the owner manipulated in any way
    if ((semaphoreObject->owner != NULL) && (semaphoreObject->type != SEMAPHORE_FLAG)) {
        removeDynamicPriorityFromOwner(semaphoreObject);
        semaphoreRemoveOwner(semaphoreObject);
    }

    // If someone is waiting for this semaphore, unblock the highest priority thread on the semaphores block list
    if (semaphoreObject->value < 1) {
        shouldSuspend = unblockThread(semaphoreObject);
    }

    OS_CriticalExit(priority);

    // Currently running thread will suspend if the unblocked task was higher priority
    if (shouldSuspend == 1) {
        OS_Suspend(OS_SUSPEND_UNBLOCK);
    }
}


/* ----------------------------------------- Semaphore acquisition ------------------------------------------------ */
static void grantDynamicPriorityToOwner(OS_SemaphoreObjectTypeDef *semaphoreObject) {
    semaphoreObject->owner->priority = runPtr->priority;
    semaphoreObject->priorityLevelGranted = runPtr->priority;
    semaphoreObject->priorityHasBeenGranted = 1;

    // Re-insert the owner (if not asleep or inactive periodic) after modifying its priority, to conserve the ordering in the thread list
    if (semaphoreObject->owner->sleep == 0) {
        if (semaphoreObject->owner->basePeriod == 0 || !semaphoreObject->owner->hasFullyRan) {
            if (semaphoreObject->owner->blockPtr != NULL) {
                OS_BlockedListRemove(semaphoreObject->owner);
                OS_BlockedListInsert(semaphoreObject->owner);
            } else {
                OS_ReadyListRemove(semaphoreObject->owner);
                OS_ReadyListInsert(semaphoreObject->owner);
            }
        }
    }
}

void OS_Wait(OS_SemaphoreObjectTypeDef *semaphoreObject) {
    uint32_t priority = OS_CriticalEnter();
    semaphoreObject->value -= 1;

    // If no semaphore available, block the thread on the semaphore and suspend the thread
    if (semaphoreObject->value < 0) {
        OS_ReadyListRemove(runPtr);
        OS_BlockedListInsert(runPtr);
        runPtr->blockPtr = semaphoreObject;

        if (semaphoreObject->type != SEMAPHORE_FLAG) {
            // If owner of thread has higher priority than the currently running thread, elevate the owner priority
            if (runPtr->priority <= semaphoreObject->owner->priority) {
                grantDynamicPriorityToOwner(semaphoreObject);
            }
        }

        OS_CriticalExit(priority);
        OS_Suspend(OS_SUSPEND_BLOCK);
    } else {
        semaphoreSetOwner(semaphoreObject, runPtr);
        OS_CriticalExit(priority);
    }
}