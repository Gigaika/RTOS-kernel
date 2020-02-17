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

void test_threadsCanBeCreated(void) {
    StackElementTypeDef testStack[20];
    OS_CreateThread(&testFn, testStack, 20,3, "test thread");
    TEST_ASSERT_EQUAL_STRING(readyTailPtr->identifier, "test thread");
}

void test_multipleThreadsCanBeCreated(void) {
    StackElementTypeDef testStack1[20];
    OS_CreateThread(&testFn, testStack1, 20,3, "test thread1");
    StackElementTypeDef testStack2[20];
    OS_CreateThread(&testFn, testStack2, 20,3, "test thread2");
    TEST_ASSERT_EQUAL_STRING(readyHeadPtr->identifier, "test thread1");
    TEST_ASSERT_EQUAL_STRING(readyTailPtr->identifier, "test thread2");
}

void test_threadStackIsCorrectlyInitialized(void) {
    StackElementTypeDef testStack[20] = {0};
    OS_CreateThread(&testFn, testStack,20, 3, "test thread");
    TEST_ASSERT_EQUAL_PTR(runPtr->stkPtr, &testStack[4]);
    TEST_ASSERT_EQUAL_INT(testStack[19], 0x0100000);
    TEST_ASSERT_EQUAL_INT(testStack[4], 0x04040404);
}
