# Sensor Node

The Sensor Node collects soil and environmental data for Krishi Saathi.

## Hardware

- ESP32
- 7-in-1 RS485 soil sensor
- DHT22
- Rain/wetness sensor
- Field water-level sensor

## Communication

Sensor information is transmitted to the Main Gateway using ESP-NOW.

## Responsibilities

- Acquire sensor measurements
- Convert readings into the expected data format
- Transmit sensor information to the gateway

## Setup

1. Install the ESP32 board package in Arduino IDE.
2. Install the libraries required by the published sketch.
3. Connect the sensors according to the hardware documentation.
4. Select the appropriate ESP32 board and port.
5. Compile and upload the firmware.

Refer to the source code for the exact pin assignments, sensor configuration, and communication settings.
