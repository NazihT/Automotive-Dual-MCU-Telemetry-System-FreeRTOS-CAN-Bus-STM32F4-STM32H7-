#include "actuators.h"
#include "stm32f4xx_hal.h"
#include "main.h"

void ACTUATOR_SetFanSpeed(uint8_t percent)
{
    if (percent > 100) percent = 100;

    uint32_t pulse = (percent * FAN_PWM_MAX_PULSE) / 100;
    __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_4, pulse);
}
