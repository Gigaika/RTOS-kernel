//
// Created by Aleksi on 12/02/2020.
//

#include "os_threads.h"
#include "stddef.h"
#include "memory.h"
#include "assert.h" // TODO: Implement embedded friendly assert
#include "os_core.h"


/* ---------------------------------------- Private function declarations ----------------------------------------- */
/**
 * @brief: Initializes a thread stack with default values to make it function correctly when scheduled for first time
 * @param tcb: Pointer to the last element of the stack that should be initialized
 * @param function: Pointer to the thread function
 * @return: None
 */
static void OS_InitializeTCBStack(OS_TCBTypeDef *thread, void (*function)(void *));

/**
 * @brief: Helper function to map the values to the TCB struct
 */
static void OS_MapThreadValues(OS_TCBTypeDef *thread, StackElementTypeDef *stkPtr, uint32_t stackSize, uint32_t priority, const char *identifier);

/***
 * @brief: Adds a TCB to the global linked list of threads, the new TCB will become runPtr if it has highest priority
 * @param tcb: Pointer to the TCB that is to be added
 * @return: None
 */
static void OS_AddThread(OS_TCBTypeDef *thread);

/**
 * @brief: Inserts an element to the end of the provided linked list
 * @param head: Pointer to the first element of the linked list
 * @param tail: Pointer to the last element of the linked list
 * @param element: Pointer to the element that should be added
 */
static void OS_ThreadLinkedListInsert(OS_TCBTypeDef **head, OS_TCBTypeDef **tail, OS_TCBTypeDef *element);

/**
 * @brief: Removes an element from the provided linked list
 * @param head: Pointer to the first element of the linked list
 * @param tail: Pointer to the last element of the linked list
 * @param element: Pointer to the element that should be removed
 */
static void OS_ThreadLinkedListRemove(OS_TCBTypeDef **head, OS_TCBTypeDef **tail, OS_TCBTypeDef *element);


/* ---------------------------------------------- Private variables ----------------------------------------------- */
static OS_TCBTypeDef idleThreadAllocation = { 0 };
static OS_TCBTypeDef threadAllocations[NUM_USER_THREADS] = { 0 };         // Pre-allocate memory for the TCBs
static uint32_t threadsCreated = 0;                                              // Number of user threads created

OS_TCBTypeDef *idlePtr = NULL;   // Thread to be executed when nothing else is ready
OS_TCBTypeDef *runPtr = NULL;    // Currently executing thread

OS_TCBTypeDef *readyHeadPtr = NULL;
OS_TCBTypeDef *readyTailPtr = NULL;

OS_TCBTypeDef *sleepHeadPtr = NULL;
OS_TCBTypeDef *sleepTailPtr = NULL;

OS_TCBTypeDef *blockHeadPtr = NULL;
OS_TCBTypeDef *blockTailPtr = NULL;


/* -------------------------------------------- Function definitions ---------------------------------------------- */
/**
 * @brief: Resets the internal state of the module. Only compiled for tests.
 */
void OS_ResetThreads(void) {
    memset(&idleThreadAllocation, 0, sizeof(idleThreadAllocation));
    memset(threadAllocations, 0, sizeof(threadAllocations));
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

static void OS_InitializeTCBStack(OS_TCBTypeDef *thread, void (*function)(void *)) {
    // Make stkPtr point to last element
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

static void OS_AddThread(OS_TCBTypeDef *thread) {
    // runPtr or idlePtr not initialized, OS_Init has not been called before creating user threads
    assert(runPtr != NULL && idlePtr != NULL);

    OS_ReadyListInsert(thread);
    if (thread->priority < runPtr->priority) {
        runPtr = thread;
    }
}

static void OS_MapThreadValues(OS_TCBTypeDef *thread, StackElementTypeDef *stkPtr, uint32_t stackSize, uint32_t priority, const char *identifier) {
    thread->stkPtr = stkPtr;
    thread->next = NULL;
    thread->prev = NULL;
    thread->stackSize = stackSize;
    thread->identifier = identifier;

    // Validate the priority
    if (priority > THREAD_MIN_PRIORITY) {
        priority = THREAD_MIN_PRIORITY;
    } else if (priority < THREAD_MAX_PRIORITY) {
        priority = THREAD_MAX_PRIORITY;
    }

    thread->priority = priority;
    thread->basePriority = priority;
}

void OS_CreateThread(void (*function)(void *), StackElementTypeDef *stkPtr, uint32_t stackSize, uint32_t priority, const char *identifier) {
    // Make sure we are not trying to create too many threads
    assert(threadsCreated <= NUM_USER_THREADS);
    // Make sure stack can fit at least the initial stack frame
    assert(stackSize > 16);

    OS_TCBTypeDef *newThread = &threadAllocations[threadsCreated];
    OS_MapThreadValues(newThread, stkPtr, stackSize, priority, identifier);
    OS_InitializeTCBStack(newThread, function);
    OS_AddThread(newThread);
    threadsCreated++;
}

void OS_CreateIdleThread(void (*idleFunction)(void *), StackElementTypeDef *idleStkPtr, uint32_t stackSize) {
    // Make sure stack can fit at least the initial stack frame
    assert(stackSize > 16);
    OS_TCBTypeDef *idleThread = &idleThreadAllocation;
    OS_MapThreadValues(idleThread, idleStkPtr, stackSize, THREAD_MIN_PRIORITY + 1, "Idle thread");
    OS_InitializeTCBStack(idleThread, idleFunction);
    idlePtr = idleThread;
    // Make the run pointer be the idle thread just in case no other threads are added
    runPtr = idleThread;
}

static void OS_ThreadLinkedListInsert(OS_TCBTypeDef **head, OS_TCBTypeDef **tail, OS_TCBTypeDef *element) {
    OS_TCBTypeDef *oldLast = *tail;

    // If last is null, then this is the first element, set head pointer as well
    if (oldLast == NULL) {
        *head = element;
    } else {
        oldLast->next = element;
        element->prev = oldLast;
    }
    *tail = element;
}

static void OS_ThreadLinkedListRemove(OS_TCBTypeDef **head, OS_TCBTypeDef **tail, OS_TCBTypeDef *element) {
    OS_TCBTypeDef *previous = element->prev;
    OS_TCBTypeDef *next = element->next;
    element->next = NULL;
    element->prev = NULL;

    if (next != NULL) {
        next->prev = previous;
    }

    if (previous != NULL) {
        previous->next = next;
    }

    // This was the only element
    if (previous == NULL && next == NULL) {
        *head = NULL;
        *tail = NULL;
        return;
    }

    // If next is NULL, the previous element is the new last, otherwise just use the next element
    *tail = (next != NULL) ? next : previous;
}

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
    OS_ThreadLinkedListInsert(&sleepHeadPtr, &sleepTailPtr, thread);
    OS_CriticalExit(priority);
}

void OS_BlockedListInsert(OS_TCBTypeDef *thread) {
    uint32_t priority = OS_CriticalEnter();
    OS_ThreadLinkedListInsert(&blockHeadPtr, &blockTailPtr, thread);
    OS_CriticalExit(priority);
}

void OS_BlockedListRemove(OS_TCBTypeDef *thread) {
    uint32_t priority = OS_CriticalEnter();
    OS_ThreadLinkedListInsert(&blockHeadPtr, &blockTailPtr, thread);
    OS_CriticalExit(priority);
}