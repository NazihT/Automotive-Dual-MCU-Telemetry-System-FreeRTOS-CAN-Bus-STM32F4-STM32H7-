#ifndef ACTUATORS_H
#define ACTUATORS_H

#include <stdint.h>

/* ARR = 639 configured for 25kHz PWM */
#define FAN_PWM_MAX_PULSE   639U

void ACTUATOR_SetFanSpeed(uint8_t percent);

#endif /* ACTUATORS_H */
