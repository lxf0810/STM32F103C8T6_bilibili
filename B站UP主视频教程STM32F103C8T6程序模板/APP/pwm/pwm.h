#ifndef __pwm_H
#define __pwm_H

#include "stm32f10x.h"
#include "bit_operation.h"

#define TIMER TIM3 

void Timer3_Init(unsigned int pre,unsigned int arr);
void TIM3_PWM_Init(unsigned int pre,unsigned int arr);

#endif

