# Vision Node

The Vision Node performs local tomato leaf image analysis using an ESP32-S3-CAM and OV3660 camera.

## Hardware

- ESP32-S3
- OV3660 camera
- White illumination LED

## AI Model

- Architecture: MobileNetV2
- Input resolution: 128 × 128
- Quantization: Full INT8
- Number of classes: 10

## Functions

- Capture tomato leaf images
- Preprocess images for inference
- Run the quantized model locally
- Generate a disease-class prediction
- Evaluate prediction confidence
- Perform visual crop-stress analysis
- Transmit results to the Main Gateway

## Confidence Handling

Low-confidence predictions should be treated as uncertain and verified through another image or field inspection.

## Setup

The camera pin mapping, board configuration, model header, TensorFlow Lite dependencies, and ESP-NOW settings must match the published source code.

See the project documentation for the model evaluation results and limitations.
