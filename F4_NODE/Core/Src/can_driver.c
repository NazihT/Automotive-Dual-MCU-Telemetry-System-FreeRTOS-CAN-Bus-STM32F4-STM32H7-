#include "can_driver.h"
#include "main.h"

HAL_StatusTypeDef CAN_TransmitVehicleState(VehicleState_t *state)
{
    CAN_TxHeaderTypeDef TxHeader;
    uint8_t             txData[8];
    uint32_t            TxMailbox;

    /* Frame header */
    TxHeader.StdId              = 0x100;
    TxHeader.IDE                = CAN_ID_STD;
    TxHeader.RTR                = CAN_RTR_DATA;
    TxHeader.DLC                = 8;
    TxHeader.TransmitGlobalTime = DISABLE;

    /* Pack struct into 8-byte frame
     * [0-1] : raw_temp  (uint16_t)
     * [2-3] : adc_val   (uint16_t)
     * [4]   : fan_speed (uint8_t)
     * [5-7] : timestamp lower 3 bytes (prototype: full uint32 needs 2 frames)
     */
    txData[0] = (state->raw_temp  >> 8) & 0xFF;
    txData[1] = (state->raw_temp      ) & 0xFF;

    txData[2] = (state->adc_val   >> 8) & 0xFF;
    txData[3] = (state->adc_val       ) & 0xFF;

    txData[4] =  state->fan_speed;

    txData[5] = (state->timestamp >> 16) & 0xFF;
    txData[6] = (state->timestamp >>  8) & 0xFF;
    txData[7] = (state->timestamp      ) & 0xFF;

    return HAL_CAN_AddTxMessage(&hcan1, &TxHeader, txData, &TxMailbox);
}
