#include "main.h"
#include "can_driver.h"
#include "vehicule_state.h"

void FDCAN_Config(void)
{
	  FDCAN_FilterTypeDef sFilterConfig;
	  sFilterConfig.IdType = FDCAN_STANDARD_ID;
	  sFilterConfig.FilterIndex = 0;
	  sFilterConfig.FilterType = FDCAN_FILTER_MASK;
	  sFilterConfig.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
	  sFilterConfig.FilterID1 = 0x100; // Match the ID 0x100 you used in the test transmit
	  sFilterConfig.FilterID2 = 0x7FF; // Exact match mask
	  if (HAL_FDCAN_ConfigFilter(&hfdcan1, &sFilterConfig) != HAL_OK) {
	      Error_Handler();
	  }

	  // 2. Global Filter: Reject everything else so we don't get spam
	  HAL_FDCAN_ConfigGlobalFilter(&hfdcan1, FDCAN_REJECT, FDCAN_REJECT, FDCAN_FILTER_REJECT, FDCAN_FILTER_REJECT);

	  // 3. START THE PERIPHERAL (This moves it from Init to Normal mode)
	  if (HAL_FDCAN_Start(&hfdcan1) != HAL_OK) {
	      Error_Handler();
	  }

	  // 4. Activate Notifications (If you want your ISR/Callback to fire)
	  if (HAL_FDCAN_ActivateNotification(&hfdcan1, FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0) != HAL_OK) {
	      Error_Handler();
	  }



}

void FDCAN_Test_Transmit(void)
{
    FDCAN_TxHeaderTypeDef TxHeader;
    uint8_t TxData[8];

    // Hard coded dummy data
    VehicleState_t test_state;
    test_state.raw_temp = 450;    // Halfway hot
    test_state.adc_val = 2048;    // 50% sensor range
    test_state.fan_speed = 50;    // 50% duty
    test_state.timestamp = 0xABCD12; // This will get truncated to fit 8 bytes

    // Prepare Header
    TxHeader.Identifier = 0x100;          // Match your F4's ID
    TxHeader.IdType = FDCAN_STANDARD_ID;
    TxHeader.TxFrameType = FDCAN_DATA_FRAME;
    TxHeader.DataLength = FDCAN_DLC_BYTES_8;
    TxHeader.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
    TxHeader.BitRateSwitch = FDCAN_BRS_OFF;
    TxHeader.FDFormat = FDCAN_CLASSIC_CAN;
    TxHeader.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
    TxHeader.MessageMarker = 0;

    // Manually pack to handle the 8-byte limit and avoid padding issues
    TxData[0] = (uint8_t)(test_state.raw_temp & 0xFF);
    TxData[1] = (uint8_t)(test_state.raw_temp >> 8);
    TxData[2] = (uint8_t)(test_state.adc_val & 0xFF);
    TxData[3] = (uint8_t)(test_state.adc_val >> 8);
    TxData[4] = test_state.fan_speed;
    TxData[5] = (uint8_t)(test_state.timestamp & 0xFF);
    TxData[6] = (uint8_t)(test_state.timestamp >> 8);
    TxData[7] = (uint8_t)(test_state.timestamp >> 16); // Only 3 bytes of timestamp fit!

    if (HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &TxHeader, TxData) != HAL_OK)
    {
        Error_Handler();
    }
}

/* Global variable to hold the latest state */
volatile VehicleState_t g_latest_vehicle_state;

/* The "ISR" Callback */
void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs)
{
    if ((RxFifo0ITs & FDCAN_IT_RX_FIFO0_NEW_MESSAGE) != RESET)
    {
        FDCAN_RxHeaderTypeDef RxHeader;
        uint8_t RxData[8];

        /* 1. Pull message from the Hardware FIFO into RxData */
        if (HAL_FDCAN_GetRxMessage(hfdcan, FDCAN_RX_FIFO0, &RxHeader, RxData) == HAL_OK)
        {
            /* 2. Check if it's the ID we care about (0x100, 0x101, or 0x102) */
            if (RxHeader.Identifier == 0x100 )
            {
                /* 3. Unpack bytes back into the global struct */
                g_latest_vehicle_state.raw_temp  = (uint16_t)(RxData[1] << 8 | RxData[0]);
                g_latest_vehicle_state.adc_val   = (uint16_t)(RxData[3] << 8 | RxData[2]);
                g_latest_vehicle_state.fan_speed = RxData[4];

                // Reconstruct the 24-bit (3 byte) timestamp
                g_latest_vehicle_state.timestamp = (uint32_t)(RxData[7] << 16 | RxData[6] << 8 | RxData[5]);

                /* Optional: Toggle an LED so you know the interrupt hit */
                BSP_LED_Toggle(LED_YELLOW);
            }
        }
    }
}
