//
// Created by aleksi on 01/02/2020.
//

#ifndef MRTOS_MRTOS_CONFIG_H
#define MRTOS_MRTOS_CONFIG_H

#include "stdint.h"

/* ---------------------- Thread configuration --------------------------*/
#define THREAD_MIN_PRIORITY 255
#define THREAD_MAX_PRIORITY 1
#define NUM_USER_THREADS 3
#define THREAD_TIME_SLICE_MILLIS 5

/* ---------------------- System configuration ---------------------------*/
#define SYSCLOCK_FREQUENCY 80
#define SYS_TICK_PERIOD_MILLIS 1
#define NUM_SOFT_TIMERS 16

/* --------------------- Configuration typedefs ----------------------------*/
typedef uint32_t StackElementTypeDef;       // 32-bit wide stack

#endif //MRTOS_MRTOS_CONFIG_H