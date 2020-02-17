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
 * @brief: If the owner of the semaphore has lower priority than the currently running thread,
 *         then temporarily grant the owner of the semaphore an equal priority as the currently running thread
 * @param semaphoreObject: The semaphore whose owner is should get a temporary priority boost (if needed)
 */
static void grantDynamicPriorityToOwner(OS_SemaphoreObjectTypeDef *semaphoreObject);

/***
 * @brief: If the owner of the semaphore was granted "dynamic" priority for priority inheritance,
 *         then revert the changes, unless it has gained an even lower priority since
 * @param semaphoreObject: The semaphore whose owner is to be reverted to its original priority
 */
static void removeDynamicPriorityFromOwner(OS_SemaphoreObjectTypeDef *semaphoreObject);

/**
 * @brief: Iterates the linked list of threads, and unblocks the lowest priority task that is waiting for the semaphore.
 *         Complies with round robin, so if multiple tasks have same priority, the longest waiting one is chosen.
 * @param semaphore: Pointer to the semaphore that the task to be freed should be waiting for
 * @return: 1 if the unblocked task has higher priority than the currently running task
 */
static int32_t unblockThread(OS_SemaphoreObjectTypeDef *semaphoreObject);

void semaphoreAddOwner(OS_SemaphoreObjectTypeDef *semaphoreObject, OS_TCBTypeDef *newOwner);
void semaphoreRemoveOwner(OS_SemaphoreObjectTypeDef *semaphoreObject);


// TODO: Support multiple instances and multiple owners
void OS_InitSemaphore(OS_SemaphoreObjectTypeDef *semaphoreObject, SemaphoreType type) {
    if (type == SEMAPHORE_MUTEX) {
        semaphoreObject->value = 1;
    } else if (type == SEMAPHORE_MAILBOX) {
        semaphoreObject->value = 0;
    }
    semaphoreObject->owner = NULL;
    semaphoreObject->numKeys = 1;
    semaphoreObject->PrioBefore = 0;
    semaphoreObject->PrioGranted = 0;
}

void semaphoreAddOwner(OS_SemaphoreObjectTypeDef *semaphoreObject, OS_TCBTypeDef *newOwner) {
    semaphoreObject->owner = newOwner;
}

void semaphoreRemoveOwner(OS_SemaphoreObjectTypeDef *semaphoreObject) {
    semaphoreObject->owner = NULL;
}

static int32_t unblockThread(OS_SemaphoreObjectTypeDef *semaphoreObject) {
    OS_TCBTypeDef *tmpPtr = blockHeadPtr;
    OS_TCBTypeDef *highestPtr = NULL;

    // Iterate through the linked list of threads and find a thread blocked by the same semaphore, and unblock it
    while (tmpPtr != NULL) {
        if ((tmpPtr->blockPtr == semaphoreObject) && (highestPtr == NULL)) {
            highestPtr = tmpPtr;
        } else if ((tmpPtr->blockPtr == semaphoreObject) && (tmpPtr->priority < highestPtr->priority)) {
            highestPtr = tmpPtr;
        }

        tmpPtr = tmpPtr->next;
    }

    assert(highestPtr != NULL);

    // Remove the block from the highest priority thread found
    OS_BlockedListRemove(highestPtr);
    OS_ReadyListInsert(highestPtr);
    semaphoreAddOwner(semaphoreObject, highestPtr);

    // Return one if the unblocked thread is higher priority than currently executing thread
    return (highestPtr->priority < runPtr->priority);
}


static void removeDynamicPriorityFromOwner(OS_SemaphoreObjectTypeDef *semaphoreObject) {
    if (semaphoreObject->PrioBefore == semaphoreObject->owner->priority) {
        return;
    } else {
        // Task was granted even higher priority by something else, leave it be
        if (semaphoreObject->owner->priority < semaphoreObject->PrioGranted) {
            return;
        }
        semaphoreObject->owner->priority = semaphoreObject->PrioBefore;
    }
}

void OS_Signal(OS_SemaphoreObjectTypeDef *semaphoreObject) {
    uint32_t priority = OS_CriticalEnter();
    uint32_t shouldSuspend = 0;

    semaphoreObject->value += 1;
    if (semaphoreObject->value > 1) {
        semaphoreObject->value = 1;
    }

    removeDynamicPriorityFromOwner(semaphoreObject);
    semaphoreRemoveOwner(semaphoreObject);
    // If someone is waiting for the semaphore, unblock one of the waiting threads
    if (semaphoreObject->value < 1) {
        // Thread will suspend if the unblocked task is higher priority
        shouldSuspend = unblockThread(semaphoreObject);
    }

    OS_CriticalExit(priority);

    if (shouldSuspend == 1) {
        OS_Suspend();
    }
}

static void grantDynamicPriorityToOwner(OS_SemaphoreObjectTypeDef *semaphoreObject) {
    semaphoreObject->PrioBefore = semaphoreObject->owner->priority;
    // If the owner of the semaphore has lower priority than the task getting blocked, elevate its priority temporarily
    if (semaphoreObject->owner->priority < runPtr->priority) {
        semaphoreObject->owner->priority = runPtr->priority;
        semaphoreObject->PrioGranted = runPtr->priority;
    } else {
        semaphoreObject->PrioGranted = semaphoreObject->owner->priority;
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
        // If not mailbox
        if (semaphoreObject->owner != NULL) {
            grantDynamicPriorityToOwner(semaphoreObject);
        }
        OS_CriticalExit(priority);
        OS_Suspend();
    } else {
        semaphoreObject->owner = runPtr;
        OS_CriticalExit(priority);
    }
}