# Krishi Saathi — AI Documentation

## Overview

Krishi Saathi uses a MobileNetV2-based tomato leaf classification model optimized for deployment on the ESP32-S3.

The model is designed to classify 10 tomato leaf conditions and provide a prediction confidence value.

## Model Configuration

| Parameter | Value |
|---|---|
| Architecture | MobileNetV2 |
| Input resolution | 128 × 128 |
| Number of classes | 10 |
| Quantization | Full INT8 |
| Edge device | ESP32-S3 |
| QAT Keras test accuracy | 92.16% |
| Full-INT8 TFLite test accuracy | 89.42% |

## Training and Evaluation

The model was trained using a tomato leaf image dataset.

The reported test-set accuracies are:

- QAT Keras model: 92.16%
- Fully INT8 TensorFlow Lite model: 89.42%

The fully INT8 result is reported separately because quantization can affect model performance.

## Supported Classes

1. Bacterial Spot
2. Early Blight
3. Healthy
4. Late Blight
5. Leaf Mold
6. Mosaic Virus
7. Septoria Leaf Spot
8. Spider Mites
9. Target Spot
10. Yellow Leaf Curl Virus

## Inference Pipeline

1. Capture an image using the OV3660 camera.
2. Crop and resize the image to the model's input dimensions.
3. Convert the image into the input representation expected by the INT8 model.
4. Run inference on the ESP32-S3.
5. Interpret the output class and confidence.
6. Transmit the result to the Main Gateway.

## Limitations

- Performance depends on image quality, lighting, camera position, and field conditions.
- Test-set accuracy does not guarantee the same accuracy in real agricultural environments.
- Low-confidence predictions require verification.
- Disease classification does not replace professional agricultural diagnosis.

## Future Work

- Expand field-image datasets.
- Evaluate performance under diverse outdoor lighting.
- Improve robustness to real-world leaf backgrounds.
- Validate the model using independent field images.
