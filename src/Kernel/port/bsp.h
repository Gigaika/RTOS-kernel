//
// Created by aleksi on 04/02/2020.
//

#ifndef MRTOS_BSP_H
#define MRTOS_BSP_H

#include "stdint.h"

void BSP_SysClockConfig(void);
void BSP_HardwareInit(void);
void BSP_TriggerPendSV(void);
void EnableInterrupts(void);
void DisableInterrupts(void);

/**
 * @brief: Enters a critical section (disables interrupts), written in assembly
 * @return: The value of PRIMASK register before disabling interrupts
 */
uint32_t OS_CriticalEnter(void);

/**
 * @brief: Exits a critical section (enables interrupts), written in assembly
 * @param priority: The priority level that the PRIMASK register should be set to
 */
void OS_CriticalExit(uint32_t prio);

#endif //MRTOS_BSP_H
