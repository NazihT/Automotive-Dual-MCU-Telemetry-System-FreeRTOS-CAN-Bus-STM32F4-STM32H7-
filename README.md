# Dual-Node CAN Vehicle Monitoring System

Two STM32 nodes communicating over CAN bus. The F4 reads sensors, runs control logic, and broadcasts system state. The H7 receives, decodes, and displays it in real time.

This is a work in progress. Current state is documented per node. Planned features are listed explicitly.

---

## System Architecture

```
┌─────────────────────────────────────┐         ┌─────────────────────────────────────┐
│           STM32F4 Node              │         │           STM32H7 Node              │
│                                     │         │                                     │
│  AM2320 ──► I2C ──► TempTask        │         │                                     │
│                         │           │   CAN   │                                     │
│  Potentiometer ► DMA ►AdcTask       ├────────►│  CAN RX ──► Unpack ──► SPI Display  │
│                         │           │500 kbps │                                     │
│              StateControlTask       │  0x100  │                                     │
│                  │ (fan PWM)        │         │                                     │
│              CanTask ───────────────┤         │                                     │
│                                     │         │                                     │
│  [Emergency: GPIO_PIN_14 + halt]    │         │                                     │
└─────────────────────────────────────┘         └─────────────────────────────────────┘
```

---

## Shared Data Structure

Both nodes operate on the same `VehicleState_t` struct:

```c
typedef struct {
    uint16_t raw_temp;    // Raw I2C reading from AM2320
    uint16_t adc_val;     // Potentiometer value via DMA
    uint8_t  fan_speed;   // Computed fan duty cycle (0-100)
    uint32_t timestamp;   // HAL_GetTick() at transmission time
} VehicleState_t;
```

Transmitted as a raw CAN frame at ID `0x100`, 500 kbps.

---

## Nodes

| Node | MCU | Role | README |
|------|-----|------|--------|
| F4 Node | STM32F407 | Sensor acquisition, control logic, CAN TX | [F4 Node](./F4_Node/README.md) |
| H7 Node | STM32H7xx | CAN RX, display, future UDS responder | [H7 Node](./H7_Node/README.md) |

---

## Roadmap

- [ ] CAN ACK/NACK error handling and bus-off recovery
- [ ] UDS (ISO 14229) diagnostic layer on H7
- [ ] Emergency CAN frame from F4 on overheat (replacing current GPIO halt)
- [ ] H7 sends commands back to F4 (bidirectional)
- [ ] Watchdog integration on both nodes

---

## Hardware

| Item | Detail |
|------|--------|
| F4 MCU | STM32F407VGT6 |
| H7 MCU | STM32H7xx |
| CAN Transceivers | MCP2551 or equivalent |
| Bus Speed | 500 kbps |
| Sensor | AM2320 (I2C temperature/humidity) |
| Fake Accelerometer | Potentiometer on ADC1 |
| Fan Control | PWM via TIM4 CH4 |

---

## Build

Both nodes built with STM32CubeIDE. FreeRTOS included as middleware. Open each node's `.ioc` file to inspect peripheral configuration and pin assignments.
