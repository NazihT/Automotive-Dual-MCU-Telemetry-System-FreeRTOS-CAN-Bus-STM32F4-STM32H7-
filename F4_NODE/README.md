# F4 Node — Sensor Acquisition & CAN Transmitter
 
STM32F407 running FreeRTOS. Reads an AM2320 temperature sensor over I2C and a potentiometer over DMA-driven ADC. Packs state into a shared struct, runs fan control logic, and broadcasts over CAN at 500 kbps.
 
---
 
## Responsibilities
 
- Periodic sensor acquisition (temperature + ADC)
- Threshold-based fan speed control via PWM
- Overheat detection with emergency escalation
- CAN transmission of full vehicle state at 200ms intervals
 
---
 
## Task Architecture
 
| Task | Priority | Period | Role |
|------|----------|--------|------|
| `vEmergencyTask` | 7 (highest) | Blocking on semaphore | Halts system on confirmed overheat |
| `vStateControlTask` | 6 | 100ms | Reads temp, computes fan speed, triggers emergency |
| `vCanTask` | 5 | 200ms | Snapshots state, stamps timestamp, transmits CAN frame |
| `vTemperatureTask` | 3 | 200ms | Reads AM2320 over I2C, updates shared state |
| `vAdcTask` | 2 | 120ms | Copies DMA ADC buffer into shared state |
 
---
 
## Shared State & Synchronization
 
All tasks share a single `VehicleState_t` struct protected by a mutex:
 
```c
typedef struct {
    uint16_t raw_temp;    // Raw AM2320 reading
    uint16_t adc_val;     // Potentiometer (DMA-fed, continuous)
    uint8_t  fan_speed;   // Active fan duty 0-100
    uint32_t timestamp;   // HAL_GetTick() snapshot at TX time
} VehicleState_t;
```
 
`xStateMutex` — standard FreeRTOS mutex. Every task takes it before touching `g_vehicle_state` and gives it immediately after. Timeout is `portMAX_DELAY` everywhere.
 
`xEmergencySemaphore` — binary semaphore. `vStateControlTask` gives it after 3 consecutive overheat readings. `vEmergencyTask` blocks on it indefinitely at the highest priority.
 
---
 
## Fan Control Logic
 
```
raw_temp > 500  →  fan_speed = 100%
raw_temp > 400  →  fan_speed = 50%
raw_temp < 350  →  fan_speed = 20%
```
 
PWM output on TIM4 Channel 4. `ACTUATOR_SetFanSpeed()` maps 0-100 to timer compare register.
 
---
 
## Overheat / Emergency Path
 
`vStateControlTask` increments `overheat_count` on every reading above 800. Three consecutive overheats give `xEmergencySemaphore`. `vEmergencyTask` wakes, asserts `GPIOD PIN_14`, and enters an infinite loop — full halt.
 
> **TODO:** Replace GPIO halt with a CAN emergency frame to the H7 node for coordinated shutdown.
 
---
 
## ADC — DMA Configuration
 
ADC1 runs in continuous DMA mode. The DMA writes directly into `adc_raw_buffer` in the background:
 
```c
static volatile uint16_t adc_raw_buffer;
HAL_ADC_Start_DMA(&hadc1, (uint32_t *)&adc_raw_buffer, 1);
```
 
`vAdcTask` copies `adc_raw_buffer` into the shared struct under mutex. No polling. No blocking ADC reads.
 
---
 
## CAN Transmission
 
- **ID:** `0x100`
- **Speed:** 500 kbps
- **Period:** 200ms
- **Payload:** Raw `VehicleState_t` struct (8 bytes)
 
`vCanTask` takes the mutex, stamps the timestamp, snapshots the full struct into a local copy, releases the mutex, then transmits. CAN TX never holds the mutex.
 
---
 
## Hardware
 
| Peripheral | Detail |
|------------|--------|
| MCU | STM32F407VGT6 |
| Sensor | AM2320 — I2C temperature/humidity |
| Fake accelerometer | Potentiometer on ADC1 |
| Fan PWM | TIM4 Channel 4 |
| CAN | hcan1, MCP2551 transceiver |
| Emergency GPIO | GPIOD PIN_14 |
 
---
 
## Project Structure
 
```
F4_Node/
├── Core/
│   ├── Src/
│   │   ├── main.c
│   │   ├── app_tasks.c       # All FreeRTOS tasks + APP_System_Init()
│   │   ├── sensors.c         # AM2320 I2C driver
│   │   ├── actuators.c       # Fan PWM control
│   │   ├── can_driver.c      # CAN TX — packs and sends VehicleState_t
│   │   └── vehicule_state.c  # Struct definition
│   └── Inc/
│       ├── app_tasks.h
│       ├── sensors.h
│       ├── actuators.h
│       ├── can_driver.h
│       └── vehicule_state.h
├── F4_Node.ioc               # CubeMX config — peripherals, pins, clocks
└── README.md
```
 
---
 
## Known Limitations / TODO
 
- Emergency path currently halts in `while(1)` — no CAN notification to H7
- `portMAX_DELAY` on all mutex takes — no timeout recovery path yet
- Fan control thresholds are raw ADC values, not converted to actual °C
- No watchdog yet
