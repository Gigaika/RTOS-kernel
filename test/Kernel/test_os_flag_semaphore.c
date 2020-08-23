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

/* -------------------------------------------- Flag semaphore tests---------------------------------------------- */
void test_FlagSemaphoreBlocks(void) {
    OS_SemaphoreObjectTypeDef testSemaphore;
    OS_InitSemaphore(&testSemaphore, SEMAPHORE_FLAG);
    TEST_ASSERT_EQUAL_INT(0, testSemaphore.value);

    StackElementTypeDef testStack1[20];
    OS_CreateThread(&testFn, testStack1, 20,3, "test thread1");

    runPtr = OS_GetReadyThreadByIdentifier("test thread1");

    EXPECT_BLOCKED();
    OS_Wait(&testSemaphore);
    TEST_ASSERT_EQUAL_PTR(&testSemaphore, OS_GetBlockedThreadByIdentifier("test thread1")->blockPtr);
    TEST_ASSERT_EQUAL_INT(-1, testSemaphore.value);
}

void test_FlagSemaphoreUnBlocks(void) {
    OS_SemaphoreObjectTypeDef testSemaphore;
    OS_InitSemaphore(&testSemaphore, SEMAPHORE_FLAG);

    StackElementTypeDef testStack1[20];
    OS_CreateThread(&testFn, testStack1, 20,3, "test thread1");

    runPtr = OS_GetReadyThreadByIdentifier("test thread1");

    EXPECT_BLOCKED();
    OS_Wait(&testSemaphore);

    runPtr = idlePtr;

    EXPECT_SCHEDULER();
    OS_Signal(&testSemaphore);

    TEST_ASSERT_EQUAL_PTR(NULL, OS_GetReadyThreadByIdentifier("test thread1")->blockPtr);
    TEST_ASSERT_EQUAL_INT(0, testSemaphore.value);
}

void test_FlagSemaphoreSignalNoOneWaiting(void) {
    OS_SemaphoreObjectTypeDef testSemaphore;
    OS_InitSemaphore(&testSemaphore, SEMAPHORE_FLAG);

    OS_Signal(&testSemaphore);

    TEST_ASSERT_EQUAL_INT(1, testSemaphore.value);
}

void test_IndirectPriorityInheritanceWorksWhenTopOwnerBlocked(void) {
    OS_SemaphoreObjectTypeDef testSemaphore0;
    OS_InitSemaphore(&testSemaphore0, SEMAPHORE_FLAG);
    OS_SemaphoreObjectTypeDef testSemaphore1;
    OS_InitSemaphore(&testSemaphore1, SEMAPHORE_MUTEX);
    OS_SemaphoreObjectTypeDef testSemaphore2;
    OS_InitSemaphore(&testSemaphore2, SEMAPHORE_MUTEX);

    StackElementTypeDef testStack1[20];
    OS_CreateThread(&testFn, testStack1, 20, 1, "test thread1");
    StackElementTypeDef testStack2[20];
    OS_CreateThread(&testFn, testStack2, 20, 2, "test thread2");
    StackElementTypeDef testStack3[20];
    OS_CreateThread(&testFn, testStack3, 20, 3, "test thread3");

    // thread 3 gets control of semaphore 1, and blocks on flag semaphore 0
    runPtr = OS_GetReadyThreadByIdentifier("test thread3");
    OS_Wait(&testSemaphore1);
    EXPECT_BLOCKED();
    OS_Wait(&testSemaphore0);

    // thread 2 gets control of semaphore 2, and blocks on semaphore 1
    runPtr = OS_GetReadyThreadByIdentifier("test thread2");
    OS_Wait(&testSemaphore2);
    EXPECT_BLOCKED();
    OS_Wait(&testSemaphore1);

    // thread 1 blocks on semaphore 2
    runPtr = OS_GetReadyThreadByIdentifier("test thread1");
    EXPECT_BLOCKED();
    OS_Wait(&testSemaphore2);
    // thread 1 should have granted dynamic priority to thread 2, and because thread 2 is also blocked,
    // to whatever is blocking thread 2 as well (thread 3)
    TEST_ASSERT_EQUAL_INT(1, OS_GetBlockedThreadByIdentifier("test thread2")->priority);
    TEST_ASSERT_EQUAL_INT(1, OS_GetBlockedThreadByIdentifier("test thread3")->priority);

    OS_Signal(&testSemaphore0);
    // Unblocking thread 3, should not affect its priority
    TEST_ASSERT_EQUAL_INT(1, OS_GetReadyThreadByIdentifier("test thread3")->priority);

    runPtr = OS_GetReadyThreadByIdentifier("test thread3");
    EXPECT_SCHEDULER();
    OS_Signal(&testSemaphore1);
    // When thread 3 releases the semaphore that was blocking thread 2 (and thread 1 indirectly) it should restore the priority
    TEST_ASSERT_EQUAL_INT(3, OS_GetReadyThreadByIdentifier("test thread3")->priority);
}