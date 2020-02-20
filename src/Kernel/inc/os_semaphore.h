//
// Created by aleksi on 02/02/2020.
//

#ifndef MRTOS_OS_SEMAPHORE_H
#define MRTOS_OS_SEMAPHORE_H


#include "stdint.h"


/* --------------------------------------- Type definitions and structures --------------------------------------- */
typedef enum {
    SEMAPHORE_MUTEX,
    SEMAPHORE_FLAG
} SemaphoreType;

// Forward definition as OS_SemaphoreObjectTypeDef depends on OS_TCBTypeDef, and vice versa
typedef struct OS_TCBStruct OS_TCBTypeDef;

typedef struct OS_SemaphoreStruct {
    int32_t value;
    SemaphoreType type;
    OS_TCBTypeDef *owner;
    uint32_t priorityHasBeenGranted;
    uint32_t priorityLevelGranted;
} OS_SemaphoreObjectTypeDef;

/***
 * @brief: Initializes a semaphore
 * @param semaphore: The semaphore to initialize
 * @param initialValue: The initial value for the semaphore
 */
void OS_InitSemaphore(OS_SemaphoreObjectTypeDef *semaphoreObject, SemaphoreType type);

/**
 * @brief: Relinquishes control of a semaphore and unblocks another task waiting for the same semaphore (if exists)
 * @param semaphore: The semaphore to free
 */
void OS_Signal(OS_SemaphoreObjectTypeDef *semaphoreObject);

/**
 * @brief: Acquires control of a semaphore, will block the task until it gets hold of it, if it is already in use
 * @param semaphore: The semaphore to acquire
 */
void OS_Wait(OS_SemaphoreObjectTypeDef *semaphoreObject);

#endif //MRTOS_OS_SEMAPHORE_H
