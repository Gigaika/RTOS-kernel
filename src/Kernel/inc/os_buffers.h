//
// Created by Aleksi on 03/03/2020.
//

#ifndef SIMPLERTOS_OS_BUFFERS_H
#define SIMPLERTOS_OS_BUFFERS_H

#include "os_semaphore.h"
#include "stdint.h"

typedef struct {
    uint32_t elements;
    uint32_t dataSizeBytes;
    uint32_t spaceRemaining;
    uint32_t missed;
    uint32_t lastReadSize;
    OS_SemaphoreObjectTypeDef semaphore;
    void *dataPtr;
    uint32_t readIndex;
    uint32_t writeIndex;
} OS_BufferTypeDef;

void OS_BufferInit(OS_BufferTypeDef *bufferObject, void *dataPtr, uint32_t elements, uint32_t dataSizeBytes);
void OS_BufferRead(OS_BufferTypeDef *bufferObject, void *dataPtr, uint32_t dataSize);
void OS_BufferWrite(OS_BufferTypeDef *bufferObject, void *dataPtr, uint32_t dataSize);

#endif //SIMPLERTOS_OS_BUFFERS_H
