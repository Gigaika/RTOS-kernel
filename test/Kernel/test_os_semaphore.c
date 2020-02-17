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

void test_mutexSemaphoreDecrements(void) {
    OS_SemaphoreObjectTypeDef testSemaphore;
    OS_InitSemaphore(&testSemaphore, SEMAPHORE_MUTEX);

    StackElementTypeDef testStack1[20];
    OS_CreateThread(&testFn, testStack1, 20,3, "test thread1");

    runPtr = readyTailPtr;

    OS_Wait(&testSemaphore);
    TEST_ASSERT_EQUAL_STRING(testSemaphore.owner->identifier, "test thread1");
    TEST_ASSERT_EQUAL_INT(testSemaphore.value, 0);
}

void test_mutexSemaphoreIncrements(void) {
    OS_SemaphoreObjectTypeDef testSemaphore;
    OS_InitSemaphore(&testSemaphore, SEMAPHORE_MUTEX);

    StackElementTypeDef testStack1[20];
    OS_CreateThread(&testFn, testStack1, 20,3, "test thread1");

    runPtr = readyTailPtr;

    OS_Wait(&testSemaphore);
    OS_Signal(&testSemaphore);
    TEST_ASSERT_EQUAL_PTR(testSemaphore.owner, NULL);
    TEST_ASSERT_EQUAL_INT(testSemaphore.value, 1);
}

void test_mutexSemaphoreGetsBlocked(void) {
    OS_SemaphoreObjectTypeDef testSemaphore;
    OS_InitSemaphore(&testSemaphore, SEMAPHORE_MUTEX);

    StackElementTypeDef testStack1[20];
    OS_CreateThread(&testFn, testStack1, 20,3, "test thread1");

    StackElementTypeDef testStack2[20];
    OS_CreateThread(&testFn, testStack2, 20,3, "test thread2");

    runPtr = readyHeadPtr;
    OS_Wait(&testSemaphore);

    runPtr = readyTailPtr;
    BSP_TriggerPendSV_Expect();
    // Should call OS_Suspend->BSP_TriggerPendSV as thread gets blocked
    OS_Wait(&testSemaphore);

    TEST_ASSERT_EQUAL_STRING(blockTailPtr->identifier, "test thread2");
    TEST_ASSERT_EQUAL_PTR(blockTailPtr->blockPtr, &testSemaphore);
}

void test_mutexSemaphoreGetsUnblockedAndScheduled(void) {
    OS_SemaphoreObjectTypeDef testSemaphore;
    OS_InitSemaphore(&testSemaphore, SEMAPHORE_MUTEX);

    StackElementTypeDef testStack1[20];
    OS_CreateThread(&testFn, testStack1, 20,3, "test thread1");

    StackElementTypeDef testStack2[20];
    OS_CreateThread(&testFn, testStack2, 20,1, "test thread2");

    // runPtr = thread1
    runPtr = readyHeadPtr;
    OS_Wait(&testSemaphore);

    // runPtr = thread2
    runPtr = readyTailPtr;
    BSP_TriggerPendSV_Expect();
    // Should call OS_Suspend->BSP_TriggerPendSV as thread2 gets blocked
    OS_Wait(&testSemaphore);

    // runPtr = thread1
    runPtr = readyHeadPtr;
    BSP_TriggerPendSV_Expect();
    // Should call OS_Suspend->BSP_TriggerPendSV to let higher priority blocked thread2 run instantly
    OS_Signal(&testSemaphore);

    TEST_ASSERT_EQUAL_STRING(testSemaphore.owner->identifier, "test thread2");
    TEST_ASSERT_EQUAL_INT(testSemaphore.value, 0);
}