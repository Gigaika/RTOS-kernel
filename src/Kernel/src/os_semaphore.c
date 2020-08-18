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

static void reInsertToList(OS_TCBTypeDef *ptr);


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
    return 0;
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
        // Find highest priority thread being blocked by this owner thread, but through another semaphore
        if ((tmpPtr->blockPtr != semaphoreObject) && (tmpPtr->blockPtr->owner == semaphoreObject->owner)) {
            // If the next highest priority is equal to current one, nothing needs to be done
            if (semaphoreObject->owner->priority < tmpPtr->priority) {
                // Restore the owners priority to the next highest priority that has been granted to it through any other semaphore
                semaphoreObject->owner->priority = tmpPtr->priority;
                // Re-insert the owner after modifying its priority to conserve the ordering in the thread list
                reInsertToList(semaphoreObject->owner);
            }
            return;
        }

        tmpPtr = tmpPtr->next;
    }

    // If this point is reached, this is the last semaphore owned by the thread,
    // so we can safely restore its priority to the base priority level
    semaphoreObject->owner->priority = semaphoreObject->owner->basePriority;
    // Re-insert the owner after modifying its priority to conserve the ordering in the thread list
    reInsertToList(semaphoreObject->owner);
}

void OS_Signal(OS_SemaphoreObjectTypeDef *semaphoreObject) {
    uint32_t priority = OS_CriticalEnter();
    uint32_t shouldSuspend = 0;

    semaphoreObject->value += 1;
    if (semaphoreObject->value > 1) {
        semaphoreObject->value = 1;
    }

    // Flag semaphores should not have the owner manipulated in any way
    if (semaphoreObject->type != SEMAPHORE_FLAG) {
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
static void reInsertToList(OS_TCBTypeDef *ptr) {
    if (ptr->sleep == 0) {
        if (ptr->basePeriod == 0 || !ptr->hasFullyRan) {
            if (ptr->blockPtr != NULL) {
                OS_BlockedListRemove(ptr);
                OS_BlockedListInsert(ptr);
            } else {
                OS_ReadyListRemove(ptr);
                OS_ReadyListInsert(ptr);
            }
        }
    }
}

static void grantDynamicPriorityToOwner(OS_SemaphoreObjectTypeDef *semaphoreObject) {
    semaphoreObject->owner->priority = runPtr->priority;
    semaphoreObject->priorityLevelGranted = runPtr->priority;
    semaphoreObject->priorityHasBeenGranted = 1;

    // Re-insert the owner to conserve the ordering in the thread list
    reInsertToList(semaphoreObject->owner);

    // If the blocking thread is also blocked, we need to grant everything that is blocking it priority as well,
    // to make sure lower priority threads are not indirectly blocking higher priority ones
    OS_SemaphoreObjectTypeDef *tmpObject = semaphoreObject->owner->blockPtr;
    while (tmpObject != NULL) {
        OS_TCBTypeDef *tmpOwner = tmpObject->owner;

        // Flag semaphore as there is no owner, stop here
        if (tmpOwner == NULL) {
            return;
        }

        // If the blocking thread has lower priority than the thread being indirectly blocked by it, rise its priority
        if (semaphoreObject->priorityLevelGranted < tmpOwner->priority) {
            tmpOwner->priority = semaphoreObject->owner->priority;
            // Modify the semaphore object properties to ensure priority will get restored correctly when the thread relinquishes control
            tmpObject->priorityLevelGranted = tmpOwner->priority;
            tmpObject->priorityHasBeenGranted = 1;
            // Re-insert the owner to conserve the ordering in the thread list
            reInsertToList(tmpOwner);
        }

        tmpObject = tmpOwner->blockPtr;
    }
}

void OS_Wait(OS_SemaphoreObjectTypeDef *semaphoreObject) {
    uint32_t priority = OS_CriticalEnter();
    semaphoreObject->value -= 1;

    // If no semaphore available, block the thread on the semaphore and suspend the thread
    if (semaphoreObject->value < 0) {
        if (semaphoreObject->owner != NULL) {
            if (semaphoreObject->owner->blockPtr != NULL) {
                if (semaphoreObject->owner->blockPtr->owner == runPtr) {
                    // TODO: Maybe add hook, timeouts to prevent deadlocks, and log the incident when logging implemented
                    assert(0);  // Deadlock
                }
            }
        }

        OS_ReadyListRemove(runPtr);
        OS_BlockedListInsert(runPtr);
        runPtr->blockPtr = semaphoreObject;

        // Only mutex semaphores implement priority inheritance
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