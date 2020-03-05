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