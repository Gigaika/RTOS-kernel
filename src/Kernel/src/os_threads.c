//
// Created by Aleksi on 12/02/2020.
//


#include "os_threads.h"
#include "stddef.h"
#include "memory.h"
#include "assert.h"
#include "os_core.h"


/* ---------------------------------------- Private function declarations ----------------------------------------- */
static void OS_ValidateTCB(uint32_t stackSize);
static void OS_PeriodicListInsert(OS_TCBTypeDef *thread);

/**
 * @brief: Initializes a thread stack with default values to make it function correctly when scheduled for first time
 * @param tcb: Pointer to the TCB element whose stack is to be initialized
 * @param function: Pointer to the thread main function
 */
static void OS_InitializeTCBStack(OS_TCBTypeDef *thread, void (*function)(void *));

/**
 * @brief: Helper function that maps initial values to the TCB element
 */
static void OS_MapInitialThreadValues(OS_TCBTypeDef *thread, StackElementTypeDef *stkPtr, uint32_t stackSize, uint32_t priority, const char *identifier, uint32_t period);

/***
 * @brief: Adds the thread to the linked list of ready threads. The thread will become the runPtr if it has the highest priority so far.
 * @param tcb: Pointer to the TCB element that is to be added
 */
static void OS_AddThread(OS_TCBTypeDef *thread);

/**
 * @brief: Inserts an element to the end of the provided thread linked list
 * @param head: Pointer to the pointer of the first element of the linked list
 * @param tail: Pointer to the pointer of the last element of the linked list
 * @param element: Pointer to the TCB element that should be added
 */
static void OS_ThreadLinkedListInsert(OS_TCBTypeDef **head, OS_TCBTypeDef **tail, OS_TCBTypeDef *element);

/**
 * @brief: Removes an element from the provided thread linked list
 * @param head: Pointer to the pointer of the first element of the linked list
 * @param tail: Pointer to the pointer of the last element of the linked list
 * @param element: Pointer to the TCB element that should be removed
 */
static void OS_ThreadLinkedListRemove(OS_TCBTypeDef **head, OS_TCBTypeDef **tail, OS_TCBTypeDef *element);


/* ---------------------------------------------- Private variables ----------------------------------------------- */
static OS_TCBTypeDef idleThreadAllocation = { 0 };
// Pre-allocate memory for the TCBs
static OS_TCBTypeDef threadAllocations[NUM_USER_THREADS] = { 0 };
// Periodic threads need to be in two lists at times (periodic, and ready/blocked), so make periodic list a pre-allocated array of pointers,
// since it would otherwise mess up the next&prev pointers of the TCB. (Extra null element in list to make iterating easier)
static OS_TCBTypeDef *periodicThreads[NUM_USER_THREADS+1] = { NULL };
// Number of user threads created
static uint32_t threadsCreated = 0;


/* ----------------------------------------------- Global variables ----------------------------------------------- */
OS_TCBTypeDef *idlePtr = NULL;   // Thread to be executed when nothing else is ready
OS_TCBTypeDef *runPtr = NULL;    // Currently executing thread

OS_TCBTypeDef *readyHeadPtr = NULL;
OS_TCBTypeDef *readyTailPtr = NULL;

OS_TCBTypeDef *sleepHeadPtr = NULL;
OS_TCBTypeDef *sleepTailPtr = NULL;

OS_TCBTypeDef *blockHeadPtr = NULL;
OS_TCBTypeDef *blockTailPtr = NULL;

// TODO: shouldn't be modifiable
OS_TCBTypeDef **periodicListPtr = periodicThreads;


/* ------------------------------------------- Test helper functions ---------------------------------------------- */
/**
 * @brief: Resets the internal state of the module. Only compiled for tests.
 */
void OS_ResetThreads(void) {
    memset(&idleThreadAllocation, 0, sizeof(idleThreadAllocation));
    memset(threadAllocations, 0, sizeof(threadAllocations));
    memset(periodicThreads, 0, sizeof(*threadAllocations));
    threadsCreated = 0;
    idlePtr = NULL;
    runPtr = NULL;
    readyHeadPtr = NULL;
    readyTailPtr = NULL;
    sleepHeadPtr = NULL;
    sleepTailPtr = NULL;
    blockHeadPtr = NULL;
    blockTailPtr = NULL;
}

/**
 * @brief: Finds a ready thread with the specified identifier. Returns NULL if none found. Only compiled for tests.
 */
OS_TCBTypeDef *OS_GetReadyThreadByIdentifier(const char *identifier) {
    OS_TCBTypeDef *tmpPtr = readyHeadPtr;
    while (tmpPtr != NULL) {
        if (tmpPtr->identifier == identifier) {
            return tmpPtr;
        }

        tmpPtr = tmpPtr->next;
    }

    return NULL;
}

/**
 * @brief: Finds a sleeping thread with the specified identifier. Returns NULL if none found. Only compiled for tests.
 */
OS_TCBTypeDef *OS_GetSleepingThreadByIdentifier(const char *identifier) {
    OS_TCBTypeDef *tmpPtr = sleepHeadPtr;
    while (tmpPtr != NULL) {
        if (tmpPtr->identifier == identifier) {
            return tmpPtr;
        }

        tmpPtr = tmpPtr->next;
    }

    return NULL;
}

/**
 * @brief: Finds a blocked thread with the specified identifier. Returns NULL if none found. Only compiled for tests.
 */
OS_TCBTypeDef *OS_GetBlockedThreadByIdentifier(const char *identifier) {
    OS_TCBTypeDef *tmpPtr = blockHeadPtr;
    while (tmpPtr != NULL) {
        if (tmpPtr->identifier == identifier) {
            return tmpPtr;
        }

        tmpPtr = tmpPtr->next;
    }

    return NULL;
}


/* ----------------------------------------- Thread creation functions -------------------------------------------- */
static void OS_AddThread(OS_TCBTypeDef *thread) {
    // if runPtr or idlePtr not initialized, OS_Init has not been called before creating user threads like it should have
    assert(runPtr != NULL && idlePtr != NULL);

    OS_ReadyListInsert(thread);
    if (thread->priority < runPtr->priority) {
        runPtr = thread;
    }
}

static void OS_InitializeTCBStack(OS_TCBTypeDef *thread, void (*function)(void *)) {
    // make stkPtr point to last element
    thread->stkPtr = &thread->stkPtr[thread->stackSize-1];

    *thread->stkPtr-- = 0x0100000;            // PSR
    *thread->stkPtr-- = (uint32_t)function;   // Program counter
    *thread->stkPtr-- = 0x14141414;           // Link register
    *thread->stkPtr-- = 0x12121212;           // R12
    *thread->stkPtr-- = 0x03030303;           // R3 ->
    *thread->stkPtr-- = 0x02020202;           // -
    *thread->stkPtr-- = 0x01010101;           // -
    *thread->stkPtr-- = 0x00000000;           // R0
    *thread->stkPtr-- = 0x11111111;           // R11 ->
    *thread->stkPtr-- = 0x10101010;           // -
    *thread->stkPtr-- = 0x09090909;           // -
    *thread->stkPtr-- = 0x08080808;           // -
    *thread->stkPtr-- = 0x07070707;           // -
    *thread->stkPtr-- = 0x06060606;           // -
    *thread->stkPtr-- = 0x05050505;           // -
    *thread->stkPtr = 0x04040404;             // R4
}

static void OS_MapInitialThreadValues(OS_TCBTypeDef *thread, StackElementTypeDef *stkPtr, uint32_t stackSize, uint32_t priority, const char *identifier, uint32_t period) {
    thread->stkPtr = stkPtr;
    thread->next = NULL;
    thread->prev = NULL;
    thread->stackSize = stackSize;
    thread->identifier = identifier;

    if (priority > THREAD_MIN_PRIORITY) {
        priority = THREAD_MIN_PRIORITY;
    } else if (priority < THREAD_MAX_PRIORITY) {
        priority = THREAD_MAX_PRIORITY;
    }

    thread->priority = priority;
    thread->basePriority = priority;
    thread->period = period;
    thread->basePeriod = period;
    thread->hasFullyRan = 1;
}

static void OS_ValidateTCB(uint32_t stackSize) {
    // make sure we are not trying to create too many threads
    assert(threadsCreated < NUM_USER_THREADS);
    // make sure stack can fit at least the initial stack frame
    assert(stackSize > 16);
}

static void OS_PeriodicListInsert(OS_TCBTypeDef *thread) {
    for (uint32_t i = 0; i < threadsCreated; i++) {
        if (periodicThreads[i] == NULL) {
            periodicThreads[i] = thread;
            return;
        }
    }
}

void OS_CreateThread(void (*function)(void *), StackElementTypeDef *stkPtr, uint32_t stackSize, uint32_t priority, const char *identifier) {
    OS_ValidateTCB(stackSize);
    OS_TCBTypeDef *newThread = &threadAllocations[threadsCreated];
    OS_MapInitialThreadValues(newThread, stkPtr, stackSize, priority, identifier, 0);
    OS_InitializeTCBStack(newThread, function);
    OS_AddThread(newThread);
    threadsCreated++;
}

void OS_CreatePeriodicThread(void (*function)(void *), StackElementTypeDef *stkPtr, uint32_t stackSize, uint32_t priority, uint32_t periodMillis, const char *identifier) {
    OS_ValidateTCB(stackSize);
    OS_TCBTypeDef *newThread = &threadAllocations[threadsCreated];
    OS_MapInitialThreadValues(newThread, stkPtr, stackSize, priority, identifier, periodMillis);
    OS_InitializeTCBStack(newThread, function);
    threadsCreated++;
    OS_PeriodicListInsert(newThread);
}

void OS_CreateIdleThread(void (*idleFunction)(void *), StackElementTypeDef *idleStkPtr, uint32_t stackSize) {
    // make sure stack can fit at least the initial stack frame
    assert(stackSize > 16);

    OS_TCBTypeDef *idleThread = &idleThreadAllocation;
    OS_MapInitialThreadValues(idleThread, idleStkPtr, stackSize, THREAD_MIN_PRIORITY + 1, "idle thread", 0);
    OS_InitializeTCBStack(idleThread, idleFunction);
    idlePtr = idleThread;
    // make the run pointer be the idle thread just in case no other threads are added
    runPtr = idleThread;
}


/* ------------------------------------- Thread list manipulation functions --------------------------------------- */
static void OS_ThreadLinkedListInsert(OS_TCBTypeDef **head, OS_TCBTypeDef **tail, OS_TCBTypeDef *element) {
    assert(element != NULL);

    OS_TCBTypeDef *oldLast = *tail;

    if (oldLast == NULL) {  // if this is the first element, modify head pointer as well
        *head = element;
    } else {
        oldLast->next = element;
        element->prev = oldLast;
    }

    *tail = element;
}

static void OS_ThreadLinkedListRemove(OS_TCBTypeDef **head, OS_TCBTypeDef **tail, OS_TCBTypeDef *element) {
    assert(element != NULL);

    OS_TCBTypeDef *previous = element->prev;
    OS_TCBTypeDef *next = element->next;
    element->next = NULL;
    element->prev = NULL;

    // thread was the only element
    if (previous == NULL && next == NULL) {
        *head = NULL;
        *tail = NULL;
        return;
    }

    if (next != NULL) {
        next->prev = previous;
    } else {    // if thread was the last element
        *tail = previous;
    }

    if (previous != NULL) {
        previous->next = next;
    } else {    // if thread was the first element
        *head = next;
    }
}

/* --------------------------- Exported function wrappers for thread list manipulation ---------------------------- */
void OS_ReadyListInsert(OS_TCBTypeDef *thread) {
    uint32_t priority = OS_CriticalEnter();
    OS_ThreadLinkedListInsert(&readyHeadPtr, &readyTailPtr, thread);
    OS_CriticalExit(priority);
}

void OS_ReadyListRemove(OS_TCBTypeDef *thread) {
    uint32_t priority = OS_CriticalEnter();
    OS_ThreadLinkedListRemove(&readyHeadPtr, &readyTailPtr, thread);
    OS_CriticalExit(priority);
}

void OS_SleepListInsert(OS_TCBTypeDef *thread) {
    uint32_t priority = OS_CriticalEnter();
    OS_ThreadLinkedListInsert(&sleepHeadPtr, &sleepTailPtr, thread);
    OS_CriticalExit(priority);
}

void OS_SleepListRemove(OS_TCBTypeDef *thread) {
    uint32_t priority = OS_CriticalEnter();
    OS_ThreadLinkedListRemove(&sleepHeadPtr, &sleepTailPtr, thread);
    OS_CriticalExit(priority);
}

void OS_BlockedListInsert(OS_TCBTypeDef *thread) {
    uint32_t priority = OS_CriticalEnter();
    OS_ThreadLinkedListInsert(&blockHeadPtr, &blockTailPtr, thread);
    OS_CriticalExit(priority);
}

void OS_BlockedListRemove(OS_TCBTypeDef *thread) {
    uint32_t priority = OS_CriticalEnter();
    OS_ThreadLinkedListRemove(&blockHeadPtr, &blockTailPtr, thread);
    OS_CriticalExit(priority);
}