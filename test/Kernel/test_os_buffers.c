#include "unity.h"

#include "mrtos_config.h"
#include "os_core.h"
#include "os_threads.h"
#include "os_semaphore.h"
#include "os_scheduling.h"
#include "os_buffers.h"
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

void test_BufferWritesData(void) {
    uint32_t data[10] = {0};
    OS_BufferTypeDef testBuffer;
    OS_BufferInit(&testBuffer, &data, 10, sizeof(uint32_t));

    StackElementTypeDef testStack1[20];
    OS_CreateThread(&testFn, testStack1, 20,3, "test thread1");
    StackElementTypeDef testStack2[20];
    OS_CreateThread(&testFn, testStack2, 20,3, "test thread2");

    runPtr = OS_GetReadyThreadByIdentifier("test thread1");
    uint32_t writeData[5] = {1, 2, 3, 4, 5};
    OS_BufferWrite(&testBuffer, &writeData, 5);

    for (int i = 0; i < 5; i++) {
        TEST_ASSERT_EQUAL_INT(writeData[i], data[i]);
    }

    for (int i = 5; i < 10; i++) {
        TEST_ASSERT_EQUAL_INT(0, data[i]);
    }

    TEST_ASSERT_EQUAL_INT(5, testBuffer.spaceRemaining);
}

void test_BufferWritesDataOverflow(void) {
    uint32_t data[10] = {0};
    OS_BufferTypeDef testBuffer;
    OS_BufferInit(&testBuffer, &data, 10, sizeof(uint32_t));

    StackElementTypeDef testStack1[20];
    OS_CreateThread(&testFn, testStack1, 20,3, "test thread1");
    StackElementTypeDef testStack2[20];
    OS_CreateThread(&testFn, testStack2, 20,3, "test thread2");

    runPtr = OS_GetReadyThreadByIdentifier("test thread1");
    uint32_t writeData1[10] = {100, 200, 300, 400, 5, 6, 7, 8, 9, 10};
    OS_BufferWrite(&testBuffer, &writeData1, 10);
    uint32_t writeData2[2] = {11, 12};
    OS_BufferWrite(&testBuffer, &writeData2, 2);

    for (int i = 0; i < 2; i++) {
        TEST_ASSERT_EQUAL_INT(writeData2[i], data[i]);
    }

    for (int i = 3; i < 10; i++) {
        TEST_ASSERT_EQUAL_INT(writeData1[i], data[i]);
    }

    TEST_ASSERT_EQUAL_INT(0, testBuffer.spaceRemaining);
    TEST_ASSERT_EQUAL_INT(2, testBuffer.missed);
    TEST_ASSERT_EQUAL_INT(2, testBuffer.readIndex);
}