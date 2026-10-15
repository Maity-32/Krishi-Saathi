# Hardware Documentation

Krishi Saathi uses a modular architecture with separate sensing, vision, and gateway nodes.

## 1. Sensor Node

Main controller: ESP32

Components:

- 7-in-1 RS485 soil sensor
- DHT22
- Rain/wetness sensor
- Field water-level sensor

Communication: ESP-NOW

## 2. Vision Node

Main controller: ESP32-S3

Components:

- OV3660 camera
- White illumination LED

Functions:

- Image capture
- On-device disease classification
- Visual crop-stress analysis
- ESP-NOW result transmission

## 3. Main Gateway

Main controller: ESP32

Components:

- TFT display
- Indicator LED for irrigation simulation

Functions:

- Sensor data reception
- Vision-result reception
- Sensor fusion
- Environmental risk analysis
- Irrigation decisions
- Farmer advisory display

## Electrical Documentation

Exact pin assignments, power requirements, wiring diagrams, and communication settings should be documented using the final working hardware configuration.
