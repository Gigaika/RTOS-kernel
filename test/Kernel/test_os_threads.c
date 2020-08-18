#include "unity.h"

#include "mrtos_config.h"
#include "os_core.h"
#include "os_threads.h"
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


/* ------------------------------------- Verify that helper functions work --------------------------------------- */
void test_ThreadFinderFunctionWorks(void) {
    StackElementTypeDef testStack[20];
    OS_CreateThread(&testFn, testStack, 20,3, "test thread");

    OS_TCBTypeDef *found = OS_GetReadyThreadByIdentifier("test thread");
    OS_TCBTypeDef *notFound = OS_GetReadyThreadByIdentifier("test thread not");

    TEST_ASSERT_EQUAL_STRING("test thread", found->identifier);
    TEST_ASSERT_EQUAL_PTR(NULL, notFound);
}

void test_IdleThreadExistsAfterInit(void) {
    TEST_ASSERT_TRUE(idlePtr != NULL);
    TEST_ASSERT_TRUE(runPtr != NULL);
}


/* -------------------------------------------- Thread creation tests---------------------------------------------- */
void test_ThreadsCanBeCreated(void) {
    StackElementTypeDef testStack[20];
    OS_CreateThread(&testFn, testStack, 20,3, "test thread");

    TEST_ASSERT_EQUAL_STRING("test thread", readyHeadPtr->identifier);
    TEST_ASSERT_EQUAL_STRING("test thread", readyTailPtr->identifier);
}

void test_MultipleThreadsCanBeCreated(void) {
    StackElementTypeDef testStack1[20];
    OS_CreateThread(&testFn, testStack1, 20,3, "test thread1");

    StackElementTypeDef testStack2[20];
    OS_CreateThread(&testFn, testStack2, 20,3, "test thread2");

    TEST_ASSERT_EQUAL_STRING("test thread1", readyHeadPtr->identifier);
    TEST_ASSERT_EQUAL_STRING("test thread2", readyTailPtr->identifier);
    TEST_ASSERT_EQUAL_PTR(readyTailPtr, readyHeadPtr->next);
    TEST_ASSERT_EQUAL_PTR(NULL, readyHeadPtr->prev);
    TEST_ASSERT_EQUAL_PTR(readyHeadPtr, readyTailPtr->prev);
    TEST_ASSERT_EQUAL_PTR(NULL, readyTailPtr->next);
}

void test_ThreadStackIsCorrectlyInitialized(void) {
    StackElementTypeDef testStack[20] = {0};
    OS_CreateThread(&testFn, testStack,20, 3, "test thread");

    OS_TCBTypeDef *thread1 = OS_GetReadyThreadByIdentifier("test thread");

    TEST_ASSERT_EQUAL_PTR(&testStack[4], thread1->stkPtr);
    TEST_ASSERT_EQUAL_INT(0x0100000, testStack[19]);
    TEST_ASSERT_EQUAL_INT(0x04040404, testStack[4]);
}

void test_IdleThreadStackIsCorrectlyInitialized(void) {
    TEST_ASSERT_EQUAL_INT(0x00000000, *runPtr->stkPtr);
}

/* ------------------------------------------ Thread list remove tests--------------------------------------------- */
void test_ThreadListRemoveFirstItemWorks(void) {
    StackElementTypeDef testStack1[20];
    OS_CreateThread(&testFn, testStack1, 20,3, "test thread1");
    StackElementTypeDef testStack2[20];
    OS_CreateThread(&testFn, testStack2, 20,3, "test thread2");
    StackElementTypeDef testStack3[20];
    OS_CreateThread(&testFn, testStack3, 20,3, "test thread3");

    OS_TCBTypeDef *toBeRemoved = OS_GetReadyThreadByIdentifier("test thread1");
    OS_ReadyListRemove(toBeRemoved);

    TEST_ASSERT_EQUAL_STRING("test thread2", readyHeadPtr->identifier);
    TEST_ASSERT_EQUAL_STRING("test thread3", readyTailPtr->identifier);
    TEST_ASSERT_EQUAL_PTR(NULL, readyHeadPtr->prev);
    TEST_ASSERT_EQUAL_PTR(readyTailPtr, readyHeadPtr->next);
    TEST_ASSERT_EQUAL_PTR(readyHeadPtr, readyTailPtr->prev);

    TEST_ASSERT_EQUAL_PTR(NULL, toBeRemoved->next);
    TEST_ASSERT_EQUAL_PTR(NULL, toBeRemoved->prev);
}

void test_ThreadListRemoveMiddleItemWorks(void) {
    StackElementTypeDef testStack1[20];
    OS_CreateThread(&testFn, testStack1, 20,3, "test thread1");
    StackElementTypeDef testStack2[20];
    OS_CreateThread(&testFn, testStack2, 20,3, "test thread2");
    StackElementTypeDef testStack3[20];
    OS_CreateThread(&testFn, testStack3, 20,3, "test thread3");
    StackElementTypeDef testStack4[20];
    OS_CreateThread(&testFn, testStack4, 20,3, "test thread4");
    StackElementTypeDef testStack5[20];
    OS_CreateThread(&testFn, testStack5, 20,3, "test thread5");

    OS_TCBTypeDef *toBeRemoved = OS_GetReadyThreadByIdentifier("test thread3");
    OS_TCBTypeDef *toBeRemovedPrev = toBeRemoved->prev;
    OS_TCBTypeDef *toBeRemovedNext = toBeRemoved->next;
    OS_ReadyListRemove(toBeRemoved);

    TEST_ASSERT_EQUAL_STRING("test thread1", readyHeadPtr->identifier);
    TEST_ASSERT_EQUAL_STRING("test thread5", readyTailPtr->identifier);
    TEST_ASSERT_EQUAL_STRING("test thread4", toBeRemovedPrev->next->identifier);
    TEST_ASSERT_EQUAL_STRING("test thread2", toBeRemovedNext->prev->identifier);

    TEST_ASSERT_EQUAL_PTR(NULL, toBeRemoved->next);
    TEST_ASSERT_EQUAL_PTR(NULL, toBeRemoved->prev);
}

void test_ThreadListRemoveLastItemWorks(void) {
    StackElementTypeDef testStack1[20];
    OS_CreateThread(&testFn, testStack1, 20,3, "test thread1");
    StackElementTypeDef testStack2[20];
    OS_CreateThread(&testFn, testStack2, 20,3, "test thread2");
    StackElementTypeDef testStack3[20];
    OS_CreateThread(&testFn, testStack3, 20,3, "test thread3");

    OS_TCBTypeDef *toBeRemoved = OS_GetReadyThreadByIdentifier("test thread3");
    OS_ReadyListRemove(toBeRemoved);

    TEST_ASSERT_EQUAL_STRING("test thread1", readyHeadPtr->identifier);
    TEST_ASSERT_EQUAL_STRING("test thread2", readyTailPtr->identifier);
    TEST_ASSERT_EQUAL_PTR(NULL, readyTailPtr->next);

    TEST_ASSERT_EQUAL_PTR(NULL, toBeRemoved->next);
    TEST_ASSERT_EQUAL_PTR(NULL, toBeRemoved->prev);
}

void test_ThreadListRemoveOnlyItemWorks(void) {
    StackElementTypeDef testStack1[20];
    OS_CreateThread(&testFn, testStack1, 20,3, "test thread1");

    OS_TCBTypeDef *toBeRemoved = OS_GetReadyThreadByIdentifier("test thread1");
    OS_ReadyListRemove(toBeRemoved);

    TEST_ASSERT_EQUAL_PTR(NULL, readyHeadPtr);
    TEST_ASSERT_EQUAL_PTR(NULL, readyTailPtr);
    TEST_ASSERT_EQUAL_PTR(NULL, toBeRemoved->next);
    TEST_ASSERT_EQUAL_PTR(NULL, toBeRemoved->prev);
}

/* ------------------------------------------ Thread list insert tests--------------------------------------------- */
void test_ThreadListInsertFirstItemWorks(void) {
    StackElementTypeDef testStack1[20];
    OS_CreateThread(&testFn, testStack1, 20,3, "test thread1");

    OS_TCBTypeDef *toBeInserted = OS_GetReadyThreadByIdentifier("test thread1");
    OS_ReadyListRemove(toBeInserted);

    OS_BlockedListInsert(toBeInserted);
    TEST_ASSERT_EQUAL_STRING("test thread1", blockHeadPtr->identifier);
    TEST_ASSERT_EQUAL_STRING("test thread1", blockTailPtr->identifier);
    TEST_ASSERT_EQUAL_PTR(NULL, toBeInserted->next);
    TEST_ASSERT_EQUAL_PTR(NULL, toBeInserted->prev);
}

void test_ThreadListInsertItemsWorks(void) {
    StackElementTypeDef testStack1[20];
    OS_CreateThread(&testFn, testStack1, 20,3, "test thread1");
    StackElementTypeDef testStack2[20];
    OS_CreateThread(&testFn, testStack2, 20,3, "test thread2");

    OS_TCBTypeDef *toBeInserted1 = OS_GetReadyThreadByIdentifier("test thread1");
    OS_ReadyListRemove(toBeInserted1);
    OS_TCBTypeDef *toBeInserted2 = OS_GetReadyThreadByIdentifier("test thread2");
    OS_ReadyListRemove(toBeInserted2);

    OS_BlockedListInsert(toBeInserted1);
    OS_BlockedListInsert(toBeInserted2);
    TEST_ASSERT_EQUAL_STRING("test thread1", blockHeadPtr->identifier);
    TEST_ASSERT_EQUAL_STRING("test thread2", blockTailPtr->identifier);

    TEST_ASSERT_EQUAL_PTR(blockTailPtr, blockHeadPtr->next);
    TEST_ASSERT_EQUAL_PTR(NULL, blockHeadPtr->prev);
    TEST_ASSERT_EQUAL_PTR(blockHeadPtr, blockTailPtr->prev);
    TEST_ASSERT_EQUAL_PTR(NULL, blockTailPtr->next);

}

void test_ThreadListOrderedByPriority(void) {
    StackElementTypeDef testStack1[20];
    OS_CreateThread(&testFn, testStack1, 20,3, "test thread1");
    StackElementTypeDef testStack2[20];
    OS_CreateThread(&testFn, testStack2, 20,2, "test thread2");

    OS_TCBTypeDef *toBeInserted1 = OS_GetReadyThreadByIdentifier("test thread1");
    OS_ReadyListRemove(toBeInserted1);
    OS_TCBTypeDef *toBeInserted2 = OS_GetReadyThreadByIdentifier("test thread2");
    OS_ReadyListRemove(toBeInserted2);

    OS_BlockedListInsert(toBeInserted1);
    OS_BlockedListInsert(toBeInserted2);
    TEST_ASSERT_EQUAL_STRING("test thread2", blockHeadPtr->identifier);
    TEST_ASSERT_EQUAL_STRING("test thread1", blockTailPtr->identifier);

    TEST_ASSERT_EQUAL_PTR(blockTailPtr, blockHeadPtr->next);
    TEST_ASSERT_EQUAL_PTR(NULL, blockHeadPtr->prev);
    TEST_ASSERT_EQUAL_PTR(blockHeadPtr, blockTailPtr->prev);
    TEST_ASSERT_EQUAL_PTR(NULL, blockTailPtr->next);
}

/* ------------------------------------------ Periodic thread tests--------------------------------------------- */
void test_PeriodicThreadCreateWorks(void) {
    StackElementTypeDef testStack1[20];
    OS_CreatePeriodicThread(&testFn, testStack1, 20,3, 500, "periodic thread1");

    // Periodic thread should not start in the ready list
    TEST_ASSERT_EQUAL_PTR(NULL, readyHeadPtr);
    TEST_ASSERT_TRUE(*getPeriodicListPtr() != NULL);
    TEST_ASSERT_EQUAL_STRING("periodic thread1", (*getPeriodicListPtr())->identifier);
}