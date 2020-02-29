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
 * @brief: Makes the semaphore owners priority match the currently running threads priority,
 *         until it gives up control of the semaphore
 * @param semaphoreObject: The semaphore whose owners priority will be elevated
 */
static void grantDynamicPriorityToOwner(OS_SemaphoreObjectTypeDef *semaphoreObject);

/***
 * @brief: Reverts the threads priority level to either the base priority, or to the next highest priority level,
 *         if another semaphore has also elevated its priority
 * @param semaphoreObject: The semaphore whose owners priority will be restored
 */
static void removeDynamicPriorityFromOwner(OS_SemaphoreObjectTypeDef *semaphoreObject);

/**
 * @brief: Iterates the linked list of threads, and unblocks the lowest priority task that is waiting for the semaphore.
 *         Round robin, so if multiple threads have the same priority, the longest waiting one is chosen.
 * @param semaphore: Pointer to the semaphore that the task to be freed should be waiting for
 * @return: 1 if the unblocked task has higher priority than the currently running task
 */
static int32_t unblockThread(OS_SemaphoreObjectTypeDef *semaphoreObject);

static void semaphoreSetOwner(OS_SemaphoreObjectTypeDef *semaphoreObject, OS_TCBTypeDef *newOwner);

static void semaphoreRemoveOwner(OS_SemaphoreObjectTypeDef *semaphoreObject);


/* -------------------------------------------- Function definitions ---------------------------------------------- */
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

static int32_t unblockThread(OS_SemaphoreObjectTypeDef *semaphoreObject) {
    OS_TCBTypeDef *tmpPtr = blockHeadPtr;
    OS_TCBTypeDef *highestPtr = NULL;

    // Iterate through the list of blocked threads, and find the highest priority thread blocked by the same semaphore
    while (tmpPtr != NULL) {
        if (tmpPtr->blockPtr == semaphoreObject) {
            if (highestPtr == NULL) {
                highestPtr = tmpPtr;
            } else if (tmpPtr->priority < highestPtr->priority) {
                highestPtr = tmpPtr;
            }
        }

        tmpPtr = tmpPtr->next;
    }

    assert(highestPtr != NULL);

    // Remove the block from the highest priority thread found
    highestPtr->blockPtr = NULL;
    OS_BlockedListRemove(highestPtr);
    OS_ReadyListInsert(highestPtr);
    semaphoreSetOwner(semaphoreObject, highestPtr);

    // Return one if the unblocked thread is higher priority than currently executing thread
    return (highestPtr->priority < runPtr->priority);
}


static void removeDynamicPriorityFromOwner(OS_SemaphoreObjectTypeDef *semaphoreObject) {
    // No priority gained through this semaphore, do nothing
    if ((!semaphoreObject->priorityHasBeenGranted)) {
        return;
    }

    // Someone else has granted a higher priority than this semaphore, do nothing
    if (semaphoreObject->owner->priority < semaphoreObject->priorityLevelGranted) {
        semaphoreObject->priorityHasBeenGranted = 0;
        semaphoreObject->priorityLevelGranted = 0;
        return;
    }

    OS_TCBTypeDef *tmpPtr = blockHeadPtr;
    OS_TCBTypeDef *highestPtr = NULL;
    while (tmpPtr != NULL) {
        // Find threads that are being blocked by the currently running thread, but through a different semaphore
        if ((tmpPtr->blockPtr != semaphoreObject) && (tmpPtr->blockPtr->owner == semaphoreObject->owner)) {
            if (highestPtr == NULL) {
                highestPtr = tmpPtr;
            } else if (tmpPtr->priority < highestPtr->priority) {
                highestPtr = tmpPtr;
            }
        }
        tmpPtr = tmpPtr->next;
    }

    if (highestPtr == NULL) {
        // This is the last semaphore owned by this thread, so we can safely restore priority to the base priority level
        semaphoreObject->owner->priority = semaphoreObject->owner->basePriority;
    } else {
        // The thread still owns other semaphores, restore priority to match the highest priority task being blocked
        // by one of them, to conserve priority inheritance handled by the other semaphores
        semaphoreObject->owner->priority = highestPtr->priority;
    }

    semaphoreObject->priorityHasBeenGranted = 0;
    semaphoreObject->priorityLevelGranted = 0;
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
        OS_Suspend();
    }
}

static void grantDynamicPriorityToOwner(OS_SemaphoreObjectTypeDef *semaphoreObject) {
    semaphoreObject->owner->priority = runPtr->priority;
    semaphoreObject->priorityLevelGranted = runPtr->priority;
    semaphoreObject->priorityHasBeenGranted = 1;
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
        OS_Suspend();
    } else {
        semaphoreSetOwner(semaphoreObject, runPtr);
        OS_CriticalExit(priority);
    }
}