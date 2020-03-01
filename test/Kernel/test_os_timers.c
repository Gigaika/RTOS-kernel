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

uint32_t called = 0;

static void pendSVStub(int NumCalls) {
    called++;
    OS_Schedule();
}

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

void periodicThreadGetsScheduled(void) {
    BSP_TriggerPendSV_AddCallback(&pendSVStub);

    StackElementTypeDef testStack1[20];
    OS_CreateThread(&testFn, testStack1, 20, 3, "test thread1");
    StackElementTypeDef testStack1[20];
    OS_CreatePeriodicThread(&testFn, testStack1, 20, 1, "test thread2", 10*SYS_TICK_PERIOD_MILLIS);

    OS_SemaphoreObjectTypeDef testSemaphore1;
    OS_InitSemaphore(&testSemaphore1, SEMAPHORE_FLAG);

    runPtr = OS_GetReadyThreadByIdentifier("test thread2");
    // OS wait would be called in beginning of thread main loop, do it here instead for simplicity
    OS_Wait(&testSemaphore1);

    for (i = 0; i < 10; i++) {
        SysTickHandler();
        EXPECT_EQUAL_PTR(runPtr, OS_GetReadyThreadByIdentifier("test thread1"));
    }

    EXPECT_EQUAL_PTR(runPtr, OS_GetReadyThreadByIdentifier("test thread2"));
}