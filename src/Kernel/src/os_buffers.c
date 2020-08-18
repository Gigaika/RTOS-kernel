//
// Created by Aleksi on 03/03/2020.
//


#include "os_buffers.h"
#include "os_scheduling.h"
#include "os_core.h"
#include "memory.h"


/* ---------------------------------------- Private function declarations ----------------------------------------- */
static void incrementPointer(OS_BufferTypeDef *bufferObject, uint32_t elements);
static uint32_t getFreeElements(OS_BufferTypeDef *bufferObject);


static void incrementPointer(OS_BufferTypeDef *bufferObject, uint32_t elements) {

}

uint32_t getFreeElements(OS_BufferTypeDef *bufferObject) {
    if (bufferObject->readIndex > bufferObject->writeIndex) {
        return bufferObject->readIndex - bufferObject->writeIndex;
    } else if (bufferObject->readIndex < bufferObject->writeIndex) {
        return (bufferObject->elements-bufferObject->writeIndex) + bufferObject->readIndex;
    } else {
        return bufferObject->spaceRemaining ? bufferObject->elements : 0;
    }
}

void OS_BufferInit(OS_BufferTypeDef *bufferObject, void *dataPtr, uint32_t elements, uint32_t dataSizeBytes) {
    OS_InitSemaphore(&bufferObject->semaphore, SEMAPHORE_MUTEX);
    bufferObject->dataPtr = dataPtr;
    bufferObject->elements = elements;
    bufferObject->dataSizeBytes = dataSizeBytes;
    bufferObject->writeIndex = 0;
    bufferObject->readIndex = 0;
    bufferObject->spaceRemaining = elements;
}

void OS_BufferWrite(OS_BufferTypeDef *bufferObject, void *dataPtr, uint32_t dataSize) {
    OS_Wait(&bufferObject->semaphore);
    uint32_t pri = OS_CriticalEnter();

    dataSize = dataSize > bufferObject->elements ? bufferObject->elements : dataSize;

    // If we have to overwrite unread data
    uint32_t overwrite = 0;
    if (dataSize > bufferObject->spaceRemaining) {
        uint32_t missed = dataSize - bufferObject->spaceRemaining;
        bufferObject->missed += missed;
        bufferObject->spaceRemaining = 0;
        overwrite = 1;
    } else {
        bufferObject->spaceRemaining -= dataSize;
    }

    // If we need to roll over and write in two parts
    if (dataSize > (bufferObject->elements - bufferObject->writeIndex)) {
        // Cast to byte ptr so that pointer arithmetic can be done
        uint8_t *castDestPtr = bufferObject->dataPtr;
        uint8_t *castSrcPtr = dataPtr;

        uint32_t firstWriteSize = bufferObject->elements - bufferObject->writeIndex;
        memcpy(castDestPtr+(bufferObject->writeIndex*bufferObject->dataSizeBytes), castSrcPtr, firstWriteSize*bufferObject->dataSizeBytes);
        bufferObject->writeIndex = 0;

        uint32_t secondWriteSize = dataSize-firstWriteSize;
        memcpy(castDestPtr+(bufferObject->writeIndex*bufferObject->dataSizeBytes), castSrcPtr+(firstWriteSize*bufferObject->dataSizeBytes), secondWriteSize*bufferObject->dataSizeBytes);
        bufferObject->writeIndex += secondWriteSize;
    } else {
        memcpy(bufferObject->dataPtr+(bufferObject->writeIndex*bufferObject->dataSizeBytes), dataPtr, dataSize*bufferObject->dataSizeBytes);
        // If we wrote until the last index
        if (dataSize == (bufferObject->elements - bufferObject->writeIndex)) {
            bufferObject->writeIndex = 0;
        } else {
            bufferObject->writeIndex += dataSize;
        }
    }

    // If we have overwritten, then the oldest data (next to be read) will be at current write index
    if (overwrite) {
        bufferObject->readIndex = bufferObject->writeIndex;
    }

    OS_CriticalExit(pri);
    OS_Signal(&bufferObject->semaphore);
}

void OS_BufferRead(OS_BufferTypeDef *bufferObject, void *dataPtr, uint32_t dataSize) {
    OS_Wait(&bufferObject->semaphore);
    uint32_t pri = OS_CriticalEnter();

    uint32_t unread = (bufferObject->elements - bufferObject->spaceRemaining);
    dataSize = dataSize > unread ? unread : dataSize;
    bufferObject->lastReadSize = dataSize;

    // Cast to byte ptr so that pointer arithmetic can be done
    uint8_t *castDestPtr = dataPtr;
    uint8_t *castSrcPtr = bufferObject->dataPtr;

    // If we need to roll over and read in two parts
    if (dataSize > (bufferObject->elements - bufferObject->readIndex)) {
        uint32_t firstReadSize = bufferObject->elements - bufferObject->readIndex;
        memcpy(castDestPtr, castSrcPtr+(bufferObject->readIndex*bufferObject->dataSizeBytes), firstReadSize*bufferObject->dataSizeBytes);
        bufferObject->readIndex = 0;

        uint32_t secondReadSize = dataSize-firstReadSize;
        memcpy(castDestPtr+(firstReadSize*bufferObject->dataSizeBytes), castSrcPtr+(bufferObject->readIndex*bufferObject->dataSizeBytes), secondReadSize*bufferObject->dataSizeBytes);
        bufferObject->readIndex += secondReadSize;
        bufferObject->spaceRemaining += (firstReadSize+secondReadSize);
    } else {
        memcpy(castDestPtr+(bufferObject->readIndex*bufferObject->dataSizeBytes), castSrcPtr, dataSize*bufferObject->dataSizeBytes);
        // If we read until the last index
        if (dataSize == (bufferObject->elements - bufferObject->writeIndex)) {
            bufferObject->readIndex = 0;
        } else {
            bufferObject->readIndex += dataSize;
        }
        bufferObject->spaceRemaining += (dataSize);
    }

    OS_CriticalExit(pri);
    OS_Signal(&bufferObject->semaphore);
}
