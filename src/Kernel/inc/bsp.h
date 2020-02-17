//
// Created by aleksi on 04/02/2020.
//

#ifndef MRTOS_BSP_H
#define MRTOS_BSP_H

void BSP_SysClockConfig(uint32_t frequency);
void BSP_HardwareInit(void);
void BSP_TriggerPendSV(void);
void BSP_SysClockConfig(uint32_t frequency);
void BSP_HardwareInit();
void BSP_TriggerPendSV();
void EnableInterrupts();
void DisableInterrupts();
void PendSV_Handler(void);
uint32_t OS_CriticalEnter(void);
void OS_CriticalExit(uint32_t param);

#endif //MRTOS_BSP_H
