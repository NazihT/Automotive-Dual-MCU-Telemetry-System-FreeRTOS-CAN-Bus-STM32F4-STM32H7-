#include "app_tasks.h"
#include "stm32f4xx_hal.h"
#include "main.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "vehicule_state.h"
#include "sensors.h"
#include "actuators.h"
#include "can_driver.h"

/* -------------------------------------------------------------------------
 * Shared State
 * ------------------------------------------------------------------------- */
static VehicleState_t    g_vehicle_state;
static SemaphoreHandle_t xStateMutex         = NULL;
static SemaphoreHandle_t xEmergencySemaphore = NULL;

/* DMA writes here directly  */
static volatile uint16_t adc_raw_buffer;

/* -------------------------------------------------------------------------
 * Tasks
 * ------------------------------------------------------------------------- */

void vTemperatureTask(void *pvParameters)
{
    uint16_t local_raw_temp = 0;

    for (;;)
    {
        local_raw_temp = SENSOR_GetRawTemp();

        if (xSemaphoreTake(xStateMutex, portMAX_DELAY) == pdTRUE)
        {
            g_vehicle_state.raw_temp = local_raw_temp;
            xSemaphoreGive(xStateMutex);
        }

        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

void vAdcTask(void *pvParameters)
{
    for (;;)
    {
        if (xSemaphoreTake(xStateMutex, portMAX_DELAY) == pdTRUE)
        {
            g_vehicle_state.adc_val = adc_raw_buffer;
            xSemaphoreGive(xStateMutex);
        }

        vTaskDelay(pdMS_TO_TICKS(120));
    }
}

void vStateControlTask(void *pvParameters)
{
    uint8_t fan_speed = 0;
    static uint8_t overheat_count = 0;
    for (;;)
    {
    	if (xSemaphoreTake(xStateMutex, portMAX_DELAY) == pdTRUE)
    	{
    	    uint16_t current_temp = g_vehicle_state.raw_temp;

    	    if (current_temp > 800) {
    	        overheat_count++;
    	        if (overheat_count >= 3)
    	            xSemaphoreGive(xEmergencySemaphore);
    	    } else {
    	        overheat_count = 0;
    	    }

    	    if      (current_temp > 500) fan_speed = 100;
    	    else if (current_temp > 400) fan_speed = 50;
    	    else if (current_temp < 350) fan_speed = 20;

    	    g_vehicle_state.fan_speed = fan_speed;
    	    xSemaphoreGive(xStateMutex);
    	}


        ACTUATOR_SetFanSpeed(fan_speed);

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void vCanTask(void *pvParameters)
{
	VehicleState_t local_state;
    for (;;)
    {
        if (xSemaphoreTake(xStateMutex, portMAX_DELAY) == pdTRUE)
        {
            g_vehicle_state.timestamp = HAL_GetTick();
            local_state=g_vehicle_state;
            xSemaphoreGive(xStateMutex);
        }
        CAN_TransmitVehicleState(&local_state);
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

void vEmergencyTask(void *pvParameters)
{
    for (;;)
    {
        if (xSemaphoreTake(xEmergencySemaphore, portMAX_DELAY) == pdTRUE)
        {
            HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, GPIO_PIN_SET);
            /* TODO: transmit emergency frame to H7, controlled shutdown */
            while (1);
        }
    }
}

/* -------------------------------------------------------------------------
 * System Init
 * ------------------------------------------------------------------------- */

void APP_System_Init(void)
{
    xStateMutex         = xSemaphoreCreateMutex();
    xEmergencySemaphore = xSemaphoreCreateBinary();

    HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_4);
    HAL_ADC_Start_DMA(&hadc1, (uint32_t *)&adc_raw_buffer, 1);
    HAL_CAN_Start(&hcan1);

    xTaskCreate(vTemperatureTask,  "TempTask",      256, NULL, 3, NULL);
    xTaskCreate(vAdcTask,          "AdcTask",       128, NULL, 2, NULL);
    xTaskCreate(vCanTask,          "CanTask",       256, NULL, 5, NULL);
    xTaskCreate(vEmergencyTask,    "EmergencyTask", 128, NULL, 7, NULL);
    xTaskCreate(vStateControlTask, "FanCtrlTask",   128, NULL, 6, NULL);
}
