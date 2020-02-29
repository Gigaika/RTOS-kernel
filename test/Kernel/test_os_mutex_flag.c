#include "unity.h"

#include "mrtos_config.h"
#include "os_core.h"
#include "os_threads.h"
#include "os_timers.h"
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