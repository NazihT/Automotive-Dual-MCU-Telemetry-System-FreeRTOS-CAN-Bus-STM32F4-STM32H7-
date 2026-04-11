#include "sensors.h"
#include "stm32f4xx_hal.h"
#include "main.h"

uint16_t SENSOR_GetRawTemp(void)
{
    uint8_t dummy  = 0x00;
    uint8_t data[8];
    uint8_t cmd[3] = {0x03, 0x00, 0x04};

    /* Wake up the sensor */
    HAL_I2C_Master_Transmit(&hi2c1, 0x5C << 1, &dummy, 0, 100);
    //HAL_Delay(1);

    /* Send read command */
    if (HAL_I2C_Master_Transmit(&hi2c1, 0x5C << 1, cmd, 3, 100) != HAL_OK)
        return 0xFFFF;

    HAL_Delay(1);

    /* Read 8 bytes */
    if (HAL_I2C_Master_Receive(&hi2c1, 0x5C << 1, data, 8, 100) != HAL_OK)
        return 0xAAAA;

    /* Parse temperature from bytes 4-5 */
    uint16_t temp_raw = (data[4] << 8) | data[5];

    return temp_raw;
}
