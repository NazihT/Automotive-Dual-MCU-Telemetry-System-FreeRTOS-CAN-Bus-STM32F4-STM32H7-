#ifndef INC_VEHICLE_STATE_H_
#define INC_VEHICLE_STATE_H_

#include <stdint.h>

typedef struct {
    uint16_t raw_temp;
    uint16_t adc_val;
    uint8_t  fan_speed;
    uint32_t timestamp;
} VehicleState_t;

#endif /* INC_VEHICLE_STATE_H_ */
