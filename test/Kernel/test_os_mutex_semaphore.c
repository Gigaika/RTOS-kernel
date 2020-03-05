#include "unity.h"

#include "mrtos_config.h"
#include "os_core.h"
#include "os_threads.h"
#include "os_semaphore.h"
#include "os_scheduling.h"
#include "mock_bsp.h"

#define EXPECT_SCHEDULER() BSP_TriggerPendSV_Expect()
#define EXPECT_BLOCKED() BSP_TriggerPendSV_Expect()

static void idleFn(void *ptr) {}
static void testFn(void *ptr) {}

void setUp(void) {
    DisableInterrupts_Ignore();
    BSP_SysClockConfig_Ignore();
    BSP_HardwareInit_Ignore();
    OS_CriticalEnter_IgnoreAndReturn(1);
    OS_CriticalExit_Ignore();

    StackElementTypeDef idleStack[20];
    OS_Init(&idleFn, idleStack, 20);
}

void tearDown(void) {
    OS_ResetState();
}

/* -------------------------------------------- Mutex semaphore tests---------------------------------------------- */
void test_MutexSemaphoreDecrements(void) {
    OS_SemaphoreObjectTypeDef testSemaphore;
    OS_InitSemaphore(&testSemaphore, SEMAPHORE_MUTEX);
    TEST_ASSERT_EQUAL_INT(1, testSemaphore.value);

    StackElementTypeDef testStack1[20];
    OS_CreateThread(&testFn, testStack1, 20,3, "test thread1");

    runPtr = OS_GetReadyThreadByIdentifier("test thread1");

    OS_Wait(&testSemaphore);
    TEST_ASSERT_EQUAL_STRING("test thread1", testSemaphore.owner->identifier);
    TEST_ASSERT_EQUAL_INT(0, testSemaphore.value);
}

void test_MutexSemaphoreIncrements(void) {
    OS_SemaphoreObjectTypeDef testSemaphore;
    OS_InitSemaphore(&testSemaphore, SEMAPHORE_MUTEX);

    StackElementTypeDef testStack1[20];
    OS_CreateThread(&testFn, testStack1, 20,3, "test thread1");

    runPtr = OS_GetReadyThreadByIdentifier("test thread1");

    OS_Wait(&testSemaphore);
    OS_Signal(&testSemaphore);
    TEST_ASSERT_EQUAL_PTR(NULL, testSemaphore.owner);
    TEST_ASSERT_EQUAL_INT(1, testSemaphore.value);
}

void test_MutexSemaphoreBlocks(void) {
    OS_SemaphoreObjectTypeDef testSemaphore;
    OS_InitSemaphore(&testSemaphore, SEMAPHORE_MUTEX);

    StackElementTypeDef testStack1[20];
    OS_CreateThread(&testFn, testStack1, 20,3, "test thread1");
    StackElementTypeDef testStack2[20];
    OS_CreateThread(&testFn, testStack2, 20,3, "test thread2");

    runPtr = OS_GetReadyThreadByIdentifier("test thread1");
    OS_Wait(&testSemaphore);

    runPtr = OS_GetReadyThreadByIdentifier("test thread2");
    EXPECT_BLOCKED();
    OS_Wait(&testSemaphore);

    TEST_ASSERT_EQUAL_STRING("test thread1", testSemaphore.owner->identifier);
    TEST_ASSERT_EQUAL_INT(-1, testSemaphore.value);
    TEST_ASSERT_EQUAL_STRING("test thread2", blockTailPtr->identifier);
    TEST_ASSERT_EQUAL_PTR(&testSemaphore, blockTailPtr->blockPtr);
}

void test_MutexSemaphoreUnblocks(void) {
    OS_SemaphoreObjectTypeDef testSemaphore;
    OS_InitSemaphore(&testSemaphore, SEMAPHORE_MUTEX);

    StackElementTypeDef testStack1[20];
    OS_CreateThread(&testFn, testStack1, 20,3, "test thread1");
    StackElementTypeDef testStack2[20];
    OS_CreateThread(&testFn, testStack2, 20,3, "test thread2");

    runPtr = OS_GetReadyThreadByIdentifier("test thread1");
    OS_Wait(&testSemaphore);

    runPtr = OS_GetReadyThreadByIdentifier("test thread2");
    EXPECT_BLOCKED();
    OS_Wait(&testSemaphore);

    runPtr = OS_GetReadyThreadByIdentifier("test thread1");
    OS_Signal(&testSemaphore);

    TEST_ASSERT_EQUAL_PTR(NULL, blockTailPtr);
    TEST_ASSERT_EQUAL_STRING("test thread2", readyTailPtr->identifier);
    TEST_ASSERT_EQUAL_STRING("test thread2", testSemaphore.owner->identifier);
    TEST_ASSERT_EQUAL_PTR(NULL, OS_GetReadyThreadByIdentifier("test thread2")->blockPtr);
    TEST_ASSERT_EQUAL_INT(0, testSemaphore.value);
}

void test_MutexSemaphoreUnblocksHighestPriority(void) {
    OS_SemaphoreObjectTypeDef testSemaphore;
    OS_InitSemaphore(&testSemaphore, SEMAPHORE_MUTEX);

    StackElementTypeDef testStack1[20];
    OS_CreateThread(&testFn, testStack1, 20,1, "test thread1");
    StackElementTypeDef testStack2[20];
    OS_CreateThread(&testFn, testStack2, 20,2, "test thread2");
    StackElementTypeDef testStack3[20];
    OS_CreateThread(&testFn, testStack3, 20,3, "test thread3");

    runPtr = OS_GetReadyThreadByIdentifier("test thread1");
    OS_Wait(&testSemaphore);

    runPtr = OS_GetReadyThreadByIdentifier("test thread2");
    EXPECT_BLOCKED();
    OS_Wait(&testSemaphore);

    runPtr = OS_GetReadyThreadByIdentifier("test thread3");
    EXPECT_BLOCKED();
    OS_Wait(&testSemaphore);

    runPtr = OS_GetReadyThreadByIdentifier("test thread1");
    OS_Signal(&testSemaphore);

    TEST_ASSERT_EQUAL_STRING("test thread2", testSemaphore.owner->identifier);
    TEST_ASSERT_EQUAL_INT(-1, testSemaphore.value);

    TEST_ASSERT_EQUAL_STRING("test thread2", readyTailPtr->identifier);
    TEST_ASSERT_EQUAL_STRING("test thread3", blockHeadPtr->identifier);
}

void test_MutexSemaphoreUnblocksLongestWaiting(void) {
    OS_SemaphoreObjectTypeDef testSemaphore;
    OS_InitSemaphore(&testSemaphore, SEMAPHORE_MUTEX);

    StackElementTypeDef testStack1[20];
    OS_CreateThread(&testFn, testStack1, 20,1, "test thread1");
    StackElementTypeDef testStack2[20];
    OS_CreateThread(&testFn, testStack2, 20,2, "test thread2");
    StackElementTypeDef testStack3[20];
    OS_CreateThread(&testFn, testStack3, 20,2, "test thread3");

    runPtr = OS_GetReadyThreadByIdentifier("test thread1");
    OS_Wait(&testSemaphore);

    runPtr = OS_GetReadyThreadByIdentifier("test thread2");
    EXPECT_BLOCKED();
    OS_Wait(&testSemaphore);

    runPtr = OS_GetReadyThreadByIdentifier("test thread3");
    EXPECT_BLOCKED();
    OS_Wait(&testSemaphore);

    runPtr = OS_GetReadyThreadByIdentifier("test thread1");
    OS_Signal(&testSemaphore);

    TEST_ASSERT_EQUAL_STRING("test thread2", testSemaphore.owner->identifier);
    TEST_ASSERT_EQUAL_INT(-1, testSemaphore.value);

    TEST_ASSERT_EQUAL_STRING("test thread2", readyTailPtr->identifier);
    TEST_ASSERT_EQUAL_STRING("test thread3", blockHeadPtr->identifier);
}

/* --------------------------------------- Mutex priority inheritance tests---------------------------------------- */
void test_PriorityInheritanceWorks(void) {
    OS_SemaphoreObjectTypeDef testSemaphore;
    OS_InitSemaphore(&testSemaphore, SEMAPHORE_MUTEX);

    StackElementTypeDef testStack1[20];
    OS_CreateThread(&testFn, testStack1, 20,1, "test thread1");
    StackElementTypeDef testStack2[20];
    OS_CreateThread(&testFn, testStack2, 20, 2, "test thread2");

    // Give ownership of semaphore to thread 2
    runPtr = OS_GetReadyThreadByIdentifier("test thread2");
    OS_Wait(&testSemaphore);

    // Block thread 1 on the semaphore
    runPtr = OS_GetReadyThreadByIdentifier("test thread1");
    EXPECT_BLOCKED();
    OS_Wait(&testSemaphore);

    // Dynamic priority should be granted by the semaphore
    TEST_ASSERT_EQUAL_INT(1, testSemaphore.owner->priority);

    runPtr = OS_GetReadyThreadByIdentifier("test thread2");
    // Signalling should run scheduler afterwards, as we are unblocking a higher priority thread
    EXPECT_SCHEDULER();
    OS_Signal(&testSemaphore);

    // Dynamic priority was removed
    TEST_ASSERT_EQUAL_INT(2, OS_GetReadyThreadByIdentifier("test thread2")->priority);
}

void test_ChainedPriorityInheritanceWorks(void) {
    OS_SemaphoreObjectTypeDef testSemaphore;
    OS_InitSemaphore(&testSemaphore, SEMAPHORE_MUTEX);

    StackElementTypeDef testStack1[20];
    OS_CreateThread(&testFn, testStack1, 20,1, "test thread1");
    StackElementTypeDef testStack2[20];
    OS_CreateThread(&testFn, testStack2, 20, 2, "test thread2");
    StackElementTypeDef testStack3[20];
    OS_CreateThread(&testFn, testStack3, 20, 3, "test thread3");

    // Give ownership of semaphore to thread 3
    runPtr = OS_GetReadyThreadByIdentifier("test thread3");
    OS_Wait(&testSemaphore);

    // Block thread 2 on the semaphore
    runPtr = OS_GetReadyThreadByIdentifier("test thread2");
    EXPECT_BLOCKED();
    OS_Wait(&testSemaphore);
    TEST_ASSERT_EQUAL_INT(2, testSemaphore.owner->priority);

    // Block thread 1 on the semaphore
    runPtr = OS_GetReadyThreadByIdentifier("test thread1");
    EXPECT_BLOCKED();
    OS_Wait(&testSemaphore);
    TEST_ASSERT_EQUAL_INT(1, testSemaphore.owner->priority);

    runPtr = OS_GetReadyThreadByIdentifier("test thread3");
    EXPECT_SCHEDULER();
    OS_Signal(&testSemaphore);

    TEST_ASSERT_EQUAL_INT(3, OS_GetReadyThreadByIdentifier("test thread3")->priority);
}

void test_PriorityInheritanceModifiesThreadList(void) {
    OS_SemaphoreObjectTypeDef testSemaphore;
    OS_InitSemaphore(&testSemaphore, SEMAPHORE_MUTEX);

    StackElementTypeDef testStack1[20];
    OS_CreateThread(&testFn, testStack1, 20,1, "test thread1");
    StackElementTypeDef testStack2[20];
    OS_CreateThread(&testFn, testStack2, 20, 2, "test thread2");
    StackElementTypeDef testStack3[20];
    OS_CreateThread(&testFn, testStack3, 20, 3, "test thread3");

    // Give ownership of semaphore to thread 3
    runPtr = OS_GetReadyThreadByIdentifier("test thread3");
    OS_Wait(&testSemaphore);

    // Block thread 2 on the semaphore
    runPtr = OS_GetReadyThreadByIdentifier("test thread1");
    EXPECT_BLOCKED();
    OS_Wait(&testSemaphore);

    // Thread 3 should now have the highest priority through priority inheritance, and should be first in the sorted thread list
    TEST_ASSERT_EQUAL_STRING("test thread3", readyHeadPtr->identifier);

    runPtr = OS_GetReadyThreadByIdentifier("test thread3");
    EXPECT_SCHEDULER();
    OS_Signal(&testSemaphore);

    // The dynamic priority should have been removed from thread 3, so it should be last in the sorted thread list again
    TEST_ASSERT_EQUAL_STRING("test thread3", readyTailPtr->identifier);
}

void test_MultiSemaphorePriorityInheritanceWorks_HighestLast(void) {
    OS_SemaphoreObjectTypeDef testSemaphore1;
    OS_InitSemaphore(&testSemaphore1, SEMAPHORE_MUTEX);
    OS_SemaphoreObjectTypeDef testSemaphore2;
    OS_InitSemaphore(&testSemaphore2, SEMAPHORE_MUTEX);

    StackElementTypeDef testStack1[20];
    OS_CreateThread(&testFn, testStack1, 20,1, "test thread1");
    StackElementTypeDef testStack2[20];
    OS_CreateThread(&testFn, testStack2, 20, 2, "test thread2");
    StackElementTypeDef testStack3[20];
    OS_CreateThread(&testFn, testStack3, 20, 3, "test thread3");

    // thread 3 gets control of both semaphores
    runPtr = OS_GetReadyThreadByIdentifier("test thread3");
    OS_Wait(&testSemaphore1);
    OS_Wait(&testSemaphore2);

    // thread 2 blocks on semaphore 2
    runPtr = OS_GetReadyThreadByIdentifier("test thread2");
    EXPECT_BLOCKED();
    OS_Wait(&testSemaphore2);
    TEST_ASSERT_EQUAL_INT(2, OS_GetReadyThreadByIdentifier("test thread3")->priority);

    // thread 1 blocks on semaphore 1
    runPtr = OS_GetReadyThreadByIdentifier("test thread1");
    EXPECT_BLOCKED();
    OS_Wait(&testSemaphore1);
    TEST_ASSERT_EQUAL_INT(1, OS_GetReadyThreadByIdentifier("test thread3")->priority);

    // thread 3 releases semaphore 2
    runPtr = OS_GetReadyThreadByIdentifier("test thread3");
    OS_Signal(&testSemaphore2);
    // since thread 3 is still blocking an even higher priority thread through another semaphore, priority should be left as is
    TEST_ASSERT_EQUAL_INT(1, OS_GetReadyThreadByIdentifier("test thread3")->priority);

    // thread 3 releases semaphore 1
    runPtr = OS_GetReadyThreadByIdentifier("test thread3");
    EXPECT_SCHEDULER();
    OS_Signal(&testSemaphore1);
    // semaphore 1 was the last semaphore thread 3 owned, its priority should now be restored fully
    TEST_ASSERT_EQUAL_INT(3, OS_GetReadyThreadByIdentifier("test thread3")->priority);
}

void test_MultiSemaphorePriorityInheritanceWorks_HighestFirst(void) {
    OS_SemaphoreObjectTypeDef testSemaphore1;
    OS_InitSemaphore(&testSemaphore1, SEMAPHORE_MUTEX);
    OS_SemaphoreObjectTypeDef testSemaphore2;
    OS_InitSemaphore(&testSemaphore2, SEMAPHORE_MUTEX);

    StackElementTypeDef testStack1[20];
    OS_CreateThread(&testFn, testStack1, 20,1, "test thread1");
    StackElementTypeDef testStack2[20];
    OS_CreateThread(&testFn, testStack2, 20, 2, "test thread2");
    StackElementTypeDef testStack3[20];
    OS_CreateThread(&testFn, testStack3, 20, 3, "test thread3");

    // thread 3 gets control of both semaphores
    runPtr = OS_GetReadyThreadByIdentifier("test thread3");
    OS_Wait(&testSemaphore1);
    OS_Wait(&testSemaphore2);

    // thread 2 blocks on semaphore 2
    runPtr = OS_GetReadyThreadByIdentifier("test thread2");
    EXPECT_BLOCKED();
    OS_Wait(&testSemaphore2);
    TEST_ASSERT_EQUAL_INT(2, OS_GetReadyThreadByIdentifier("test thread3")->priority);

    // thread 1 blocks on semaphore 1
    runPtr = OS_GetReadyThreadByIdentifier("test thread1");
    EXPECT_BLOCKED();
    OS_Wait(&testSemaphore1);
    TEST_ASSERT_EQUAL_INT(1, OS_GetReadyThreadByIdentifier("test thread3")->priority);

    // thread 3 releases semaphore 1
    runPtr = OS_GetReadyThreadByIdentifier("test thread3");
    EXPECT_SCHEDULER();
    OS_Signal(&testSemaphore1);
    // since we released the semaphore through which thread 3 was blocking the highest priority thread,
    // we should move on to the next highest priority that was granted to thread 3
    TEST_ASSERT_EQUAL_INT(2, OS_GetReadyThreadByIdentifier("test thread3")->priority);

    // thread 3 releases semaphore 2
    runPtr = OS_GetReadyThreadByIdentifier("test thread3");
    EXPECT_SCHEDULER();
    OS_Signal(&testSemaphore2);
    // semaphore 2 was the last semaphore thread 3 owned, its priority should now be fully restored
    TEST_ASSERT_EQUAL_INT(3, OS_GetReadyThreadByIdentifier("test thread3")->priority);
}

void test_SemaphoreDynamicPriorityChangesWhenGranterGetsPriority(void) {
    OS_SemaphoreObjectTypeDef testSemaphore1;
    OS_InitSemaphore(&testSemaphore1, SEMAPHORE_MUTEX);
    OS_SemaphoreObjectTypeDef testSemaphore2;
    OS_InitSemaphore(&testSemaphore2, SEMAPHORE_MUTEX);

    StackElementTypeDef testStack1[20];
    OS_CreateThread(&testFn, testStack1, 20, 1, "test thread1");
    StackElementTypeDef testStack2[20];
    OS_CreateThread(&testFn, testStack2, 20, 2, "test thread2");
    StackElementTypeDef testStack22[20];
    OS_CreateThread(&testFn, testStack22, 20, 2, "test thread2-2");
    StackElementTypeDef testStack3[20];
    OS_CreateThread(&testFn, testStack3, 20, 3, "test thread3");

    // thread 3 gets control of semaphore 1
    runPtr = OS_GetReadyThreadByIdentifier("test thread3");
    OS_Wait(&testSemaphore1);

    // thread 2 gets control of semaphore 2, and blocks on semaphore 1
    runPtr = OS_GetReadyThreadByIdentifier("test thread2");
    OS_Wait(&testSemaphore2);
    EXPECT_BLOCKED();
    OS_Wait(&testSemaphore1);

    // thread 1 blocks on semaphore 2, thread 3 is now only ready thread, and is essentially blocking a high priority thread
    runPtr = OS_GetReadyThreadByIdentifier("test thread1");
    EXPECT_BLOCKED();
    OS_Wait(&testSemaphore2);
    // thread 1 should have granted dynamic priority to thread 2, and as thread 2 is also blocked, to the owner of whatever is blocking thread 2 (thread 3)
    TEST_ASSERT_EQUAL_INT(1, OS_GetReadyThreadByIdentifier("test thread3")->priority);
    // the position in the thread list should have also been modified
    TEST_ASSERT_EQUAL_STRING("test thread 3", readyHeadPtr->identifier);
}