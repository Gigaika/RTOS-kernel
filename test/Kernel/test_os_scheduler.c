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

void test_IdleScenarioWorks(void) {
    OS_Schedule();
    TEST_ASSERT_EQUAL_STRING("idle thread", runPtr->identifier);
}

void test_IdleThreadToUserThreadWorks(void) {
    StackElementTypeDef testStack1[20];
    OS_CreateThread(&testFn, testStack1, 20,1, "test thread1");

    runPtr = OS_GetReadyThreadByIdentifier("test thread1");
    OS_ReadyListRemove(runPtr);
    OS_BlockedListInsert(runPtr);

    OS_Schedule();
    TEST_ASSERT_EQUAL_STRING("idle thread", runPtr->identifier);
}

void test_UserThreadToIdleThreadWorks(void) {
    StackElementTypeDef testStack1[20];
    OS_CreateThread(&testFn, testStack1, 20,1, "test thread1");

    runPtr = idlePtr;
    OS_Schedule();
    TEST_ASSERT_EQUAL_STRING("test thread1", runPtr->identifier);
}

void test_NextThreadWithSamePrioritySelected(void) {
    StackElementTypeDef testStack1[20];
    OS_CreateThread(&testFn, testStack1, 20,1, "test thread1");
    StackElementTypeDef testStack2[20];
    OS_CreateThread(&testFn, testStack2, 20, 1, "test thread2");

    runPtr = OS_GetReadyThreadByIdentifier("test thread1");
    OS_Schedule();
    TEST_ASSERT_EQUAL_STRING("test thread2", runPtr->identifier);
}

void test_HighestPrioritySelected(void) {
    StackElementTypeDef testStack1[20];
    OS_CreateThread(&testFn, testStack1, 20,1, "test thread1");
    StackElementTypeDef testStack2[20];
    OS_CreateThread(&testFn, testStack2, 20, 2, "test thread2");
    StackElementTypeDef testStack3[20];
    OS_CreateThread(&testFn, testStack3, 20, 3, "test thread3");

    runPtr = OS_GetReadyThreadByIdentifier("test thread2");
    OS_Schedule();
    TEST_ASSERT_EQUAL_STRING("test thread1", runPtr->identifier);
}

void test_FirstOfSamePrioritySelected(void) {
    StackElementTypeDef testStack1[20];
    OS_CreateThread(&testFn, testStack1, 20,4, "test thread1");
    StackElementTypeDef testStack2[20];
    OS_CreateThread(&testFn, testStack2, 20, 3, "test thread2");
    StackElementTypeDef testStack3[20];
    OS_CreateThread(&testFn, testStack3, 20, 2, "test thread3");
    StackElementTypeDef testStack4[20];
    OS_CreateThread(&testFn, testStack4, 20, 2, "test thread4");

    runPtr = OS_GetReadyThreadByIdentifier("test thread1");
    OS_Schedule();
    TEST_ASSERT_EQUAL_STRING("test thread3", runPtr->identifier);
}

void test_CurrentRunPointerIsHighest(void) {
    StackElementTypeDef testStack1[20];
    OS_CreateThread(&testFn, testStack1, 20,1, "test thread1");
    StackElementTypeDef testStack2[20];
    OS_CreateThread(&testFn, testStack2, 20, 3, "test thread2");
    StackElementTypeDef testStack3[20];
    OS_CreateThread(&testFn, testStack3, 20, 2, "test thread3");
    StackElementTypeDef testStack4[20];
    OS_CreateThread(&testFn, testStack4, 20, 2, "test thread4");

    runPtr = OS_GetReadyThreadByIdentifier("test thread1");
    OS_Schedule();
    TEST_ASSERT_EQUAL_STRING("test thread1", runPtr->identifier);
}