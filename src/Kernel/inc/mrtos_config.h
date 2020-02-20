//
// Created by aleksi on 01/02/2020.
//

#ifndef MRTOS_MRTOS_CONFIG_H
#define MRTOS_MRTOS_CONFIG_H

#include "stdint.h"

//TODO: create double for tests or use another approach for the configuration

/* ---------------------- Thread configuration --------------------------*/
#define THREAD_MIN_PRIORITY 255         // any x < 255
#define THREAD_MAX_PRIORITY 0           // any x >= 0
#define NUM_USER_THREADS 10
#define THREAD_TIME_SLICE_MILLIS 5

/* ---------------------- System configuration ---------------------------*/
#define SYSCLOCK_FREQUENCY 80
#define SYS_TICK_PERIOD_MILLIS 1
#define NUM_SOFT_TIMERS 8

/* --------------------- Configuration typedefs ----------------------------*/
typedef uint32_t StackElementTypeDef;       // 32-bit wide stack

#endif //MRTOS_MRTOS_CONFIG_H
