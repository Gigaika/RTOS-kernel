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

static void pendSVStub(int NumCalls) {
    OS_Schedule();
}

void setUp(void) {
    DisableInterrupts_Ignore();
    BSP_SysClockConfig_Ignore();
    BSP_HardwareInit_Ignore();
    OS_CriticalEnter_IgnoreAndReturn(1);
    OS_CriticalExit_Ignore();
    BSP_TriggerPendSV_AddCallback(&pendSVStub);

    StackElementTypeDef idleStack[20];
    OS_Init(&idleFn, idleStack, 20);
}

void tearDown(void) {
    OS_ResetState();
}

void test_PeriodicSignallingWorks(void) {
    StackElementTypeDef testStack1[20];
    OS_CreateThread(&testFn, testStack1, 20, 3, "test thread1");
    StackElementTypeDef testStack2[20];
    OS_CreateThread(&testFn, testStack2, 20, 1, "periodic thread1");

    OS_SemaphoreObjectTypeDef testSemaphore1;
    OS_InitSemaphore(&testSemaphore1, SEMAPHORE_FLAG);
    OS_StartPeriodicSignal(&testSemaphore1, 10*SYS_TICK_PERIOD_MILLIS);

    runPtr = OS_GetReadyThreadByIdentifier("periodic thread1");
    // OS wait would be called in beginning of thread main loop, do it here instead for simplicity
    EXPECT_BLOCKED();
    OS_Wait(&testSemaphore1);

    // Scheduler will run once for each time slice, create expect statements for each timeslice consumed in this test
    for (int i = 0; i < (9 * SYS_TICK_PERIOD_MILLIS) / THREAD_TIME_SLICE_MILLIS; i++) {
        EXPECT_SCHEDULER();
    }

    // Trigger the systick handler until we are 1 systick away from the periodic thread being ready to run
    for (int i = 0; i < 9; i++) {
        SysTickHandler();
        TEST_ASSERT_EQUAL_PTR(OS_GetReadyThreadByIdentifier("test thread1"), runPtr);
    }

    // Periodic thread should now be ready to run, and it should be scheduled regardless of timeslice, as it is higher priority than runPtr
    EXPECT_SCHEDULER();
    EXPECT_SCHEDULER();
    SysTickHandler();
    TEST_ASSERT_EQUAL_PTR(OS_GetReadyThreadByIdentifier("periodic thread1"), runPtr);
}
