#include "unity.h"

#include "mrtos_config.h"
#include "os_core.h"
#include "os_threads.h"
#include "os_timers.h"
#include "os_semaphore.h"
#include "os_scheduling.h"
#include "mock_bsp.h"

static void idleFn(void *ptr) {}
static void testFn(void *ptr) {}

#define EXPECT_SUSPEND() BSP_TriggerPendSV_Expect()
#define EXPECT_SCHEDULER() EXPECT_SUSPEND()
#define EXPECT_BLOCKED() EXPECT_SUSPEND()


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

void test_MultiSemaphorePriorityInheritanceWorks(void) {
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
    // since thread 3 is still blocking a higher priority thread through another semaphore, priority should not be restored
    TEST_ASSERT_EQUAL_INT(1, OS_GetReadyThreadByIdentifier("test thread3")->priority);

    // thread 3 releases semaphore 1
    runPtr = OS_GetReadyThreadByIdentifier("test thread3");
    EXPECT_SCHEDULER();
    OS_Signal(&testSemaphore1);
    // semaphore 1 was the last semaphore thread 3 owned, its priority should now be restored
    TEST_ASSERT_EQUAL_INT(3, OS_GetReadyThreadByIdentifier("test thread3")->priority);
}
