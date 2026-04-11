#ifndef CAN_DRIVER_H
#define CAN_DRIVER_H

#include "stm32f4xx_hal.h"
#include "vehicule_state.h"

HAL_StatusTypeDef CAN_TransmitVehicleState(VehicleState_t *state);

#endif /* CAN_DRIVER_H */
