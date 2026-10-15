# Main Gateway

The Main Gateway is the central decision-making unit of Krishi Saathi.

## Hardware

- ESP32
- 1.8-inch TFT display
- GPIO indicator LED for irrigation simulation

## Responsibilities

- Receive sensor data through ESP-NOW
- Receive crop AI results from the vision node
- Evaluate environmental risks
- Make irrigation decisions
- Correlate soil measurements with visual stress indicators
- Generate local farmer advisories
- Display field status and recommendations

## Local Display

The TFT provides pages for:

- Field status
- Field health
- Crop AI results
- Farmer advisory

## Irrigation Demonstration

The current prototype uses a GPIO indicator LED to simulate an irrigation decision. It is not a production pump controller.

## Setup

Refer to the published sketch for exact pin assignments, packet structures, communication settings, and required libraries.
