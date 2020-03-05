//
// Created by Aleksi on 14/02/2020.
//

#include <assert.h>
#include "stdint.h"
#include "stdio.h"
#include "bsp.h"

void BSP_SysClockConfig(uint32_t frequency) {

}

void BSP_HardwareInit() {

}


void BSP_TriggerPendSV() {
}

void EnableInterrupts() {

}

void DisableInterrupts() {

}

void PendSV_Handler(void) {
    printf("heh");
}

uint32_t OS_CriticalEnter(void) {
    return 1;
}

void OS_CriticalExit(uint32_t param) {

}
