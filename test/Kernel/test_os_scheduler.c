#include "unity.h"

#include "mrtos_config.h"
#include "os_core.h"
#include "os_threads.h"
#include "os_semaphore.h"
#include "os_scheduling.h"
#include "mock_bsp.h"

#define EXPECT_SCHEDULER() BSP_TriggerPendSV_Expect()

static void idleFn(void *ptr) {}
static void testFn(void *ptr) {}

static void pendSVStub(int NumCalls) {
    OS_Schedule();
}

void setUp(void) {
    // TODO: Edit test cases to accommodate the fact that runptr no longer gets updated before first schedule/ctx switch
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

void test_SleepWorks(void) {
    BSP_TriggerPendSV_AddCallback(&pendSVStub);

    StackElementTypeDef testStack1[20];
    OS_CreateThread(&testFn, testStack1, 20,1, "test thread1");
    StackElementTypeDef testStack2[20];
    OS_CreateThread(&testFn, testStack2, 20, 3, "test thread2");

    runPtr = OS_GetReadyThreadByIdentifier("test thread1");
    EXPECT_SCHEDULER();
    OS_Sleep(10*SYS_TICK_PERIOD_MILLIS);
    OS_Schedule();
    TEST_ASSERT_EQUAL_STRING("test thread2", runPtr->identifier);

    // Scheduler will run once for each time slice, create expect statements for each timeslice consumed in this test
    for (int i = 0; i < (9 * SYS_TICK_PERIOD_MILLIS) / THREAD_TIME_SLICE_MILLIS; i++) {
        EXPECT_SCHEDULER();
    }

    // Trigger the systick handler until we are 1 systick away from the sleeping thread being ready to run
    for (int i = 0; i < 9; i++) {
        SysTick_Handler();
        TEST_ASSERT_EQUAL_STRING("test thread2", runPtr->identifier);
    }

    // Sleeping thread should now be ready to run, and it should be scheduled regardless of timeslice, as it is higher priority than runPtr
    EXPECT_SCHEDULER();
    SysTick_Handler();
    TEST_ASSERT_EQUAL_STRING("test thread1", runPtr->identifier);
}

void test_SleepEndRunsScheduler(void) {
    BSP_TriggerPendSV_AddCallback(&pendSVStub);

    StackElementTypeDef testStack1[20];
    OS_CreateThread(&testFn, testStack1, 20,1, "test thread1");
    StackElementTypeDef testStack2[20];
    OS_CreateThread(&testFn, testStack2, 20, 3, "test thread2");

    runPtr = OS_GetReadyThreadByIdentifier("test thread1");
    EXPECT_SCHEDULER();
    OS_Sleep(7*SYS_TICK_PERIOD_MILLIS);
    OS_Schedule();
    TEST_ASSERT_EQUAL_STRING("test thread2", runPtr->identifier);

    // Scheduler will run once for each time slice, create expect statements for each timeslice consumed in this test
    for (int i = 0; i < (7 * SYS_TICK_PERIOD_MILLIS) / THREAD_TIME_SLICE_MILLIS; i++) {
        EXPECT_SCHEDULER();
    }

    // Trigger the systick handler until we are 1 systick away from the sleeping thread being ready to run
    for (int i = 0; i < 6; i++) {
        SysTick_Handler();
        TEST_ASSERT_EQUAL_STRING("test thread2", runPtr->identifier);
    }

    // Sleeping thread should now be ready to run, and it should be scheduled regardless of timeslice, as it is higher priority than runPtr
    EXPECT_SCHEDULER();
    SysTick_Handler();
    TEST_ASSERT_EQUAL_STRING("test thread1", runPtr->identifier);
}

void test_periodicThreadGetsScheduled(void) {
    BSP_TriggerPendSV_AddCallback(&pendSVStub);

    StackElementTypeDef testStack1[20];
    OS_CreateThread(&testFn, testStack1, 20, 3, "test thread1");
    StackElementTypeDef testStack2[20];
    OS_CreatePeriodicThread(&testFn, testStack2, 20, 1, 10*SYS_TICK_PERIOD_MILLIS, "periodic thread1");


    runPtr = OS_GetReadyThreadByIdentifier("test thread1");

    // Scheduler will run once for each time slice, create expect statements for each timeslice consumed in this test
    for (int i = 0; i < (9 * SYS_TICK_PERIOD_MILLIS) / THREAD_TIME_SLICE_MILLIS; i++) {
        EXPECT_SCHEDULER();
    }

    // Trigger the systick handler until we are 1 systick away from the periodic thread being ready to run
    for (int i = 0; i < 9; i++) {
        SysTick_Handler();
        TEST_ASSERT_EQUAL_PTR(OS_GetReadyThreadByIdentifier("test thread1"), runPtr);
    }

    // Periodic thread should now be ready to run, and it should be scheduled regardless of timeslice, as it is higher priority than runPtr
    EXPECT_SCHEDULER();
    SysTick_Handler();
    TEST_ASSERT_EQUAL_PTR(OS_GetReadyThreadByIdentifier("periodic thread1"), runPtr);
}

void test_periodicThreadRunsTooLong(void) {
    BSP_TriggerPendSV_AddCallback(&pendSVStub);

    StackElementTypeDef testStack1[20];
    OS_CreateThread(&testFn, testStack1, 20, 3, "test thread1");
    StackElementTypeDef testStack2[20];
    OS_CreatePeriodicThread(&testFn, testStack2, 20, 1, 10*SYS_TICK_PERIOD_MILLIS, "periodic thread1");


    runPtr = OS_GetReadyThreadByIdentifier("test thread1");

    // Scheduler will run once for each time slice, create expect statements for each timeslice consumed in this test
    for (int i = 0; i < (9 * SYS_TICK_PERIOD_MILLIS) / THREAD_TIME_SLICE_MILLIS; i++) {
        EXPECT_SCHEDULER();
    }

    // Trigger the systick handler until we are 1 systick away from the periodic thread being ready to run
    for (int i = 0; i < 9; i++) {
        SysTick_Handler();
        TEST_ASSERT_EQUAL_PTR(OS_GetReadyThreadByIdentifier("test thread1"), runPtr);
    }

    // Periodic thread should now be ready to run, and it should be scheduled regardless of timeslice, as it is higher priority than runPtr
    EXPECT_SCHEDULER();
    SysTick_Handler();
    TEST_ASSERT_EQUAL_PTR(OS_GetReadyThreadByIdentifier("periodic thread1"), runPtr);
}