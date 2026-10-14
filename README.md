# 🌱 Krishi Saathi

### Edge-AI Powered Agricultural Risk Management and Decision-Support System

**Smart India Hackathon 2026 | Smart Farming**

> Sense → Detect → Assess Risk → Advise

Krishi Saathi is an edge-first agricultural risk-management system that combines IoT sensors, computer vision, on-device AI, and sensor fusion to identify potential crop and environmental risks and generate actionable farming advisories.

Rather than functioning only as a crop-monitoring device, Krishi Saathi aims to help farmers identify risks earlier, make better-informed decisions, reduce preventable losses, and improve the use of agricultural resources.

The current prototype focuses on tomato cultivation.

---

## 1. The Problem

Agriculture faces multiple interconnected risks:

- Drought and low soil moisture
- Heat stress and irregular rainfall
- Waterlogging
- Crop diseases and pest infestations
- Nutrient imbalance
- Unnecessary irrigation and fertilizer application
- Delayed identification of crop-health problems
- Limited internet connectivity in agricultural areas

Individual sensors and image-classification systems provide only part of the picture.

A useful agricultural decision-support system should combine soil measurements, environmental conditions, and visual crop information to provide more meaningful recommendations.

## 2. Our Solution

Krishi Saathi integrates three main components:

1. **Sensor Node:** Collects soil and environmental information.
2. **Vision Node:** Captures crop images and performs on-device AI inference.
3. **Main Edge Gateway:** Combines incoming information and generates local advisories.

The system is designed to perform its core sensing, inference, and decision-making functions locally without depending on continuous internet connectivity.

## 3. System Architecture

### Sensor Node — ESP32

The sensor node integrates:

- Nitrogen (N)
- Phosphorus (P)
- Potassium (K)
- Soil pH
- Electrical Conductivity (EC)
- Soil moisture
- Soil temperature
- Air temperature
- Air humidity
- Rain/wetness sensing
- Field water-level sensing

A local ESP-NOW connection transfers sensor information to the main gateway.

### Vision Node — ESP32-S3-CAM

The vision node uses an OV3660 camera to capture tomato leaf images.

A lightweight MobileNetV2-based model, converted to a fully INT8 TensorFlow Lite representation, is used for on-device classification.

The current model supports 10 tomato leaf classes:

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

The vision module also evaluates visual stress indicators, including leaf-color distribution, yellow/chlorotic regions, brown/necrotic regions, plant-color coverage, and texture characteristics.

Visual stress analysis is treated as an indicator of possible crop stress, not as a direct measurement of chlorophyll or nutrient concentration.

### Main Edge Gateway — ESP32

The main gateway receives sensor and vision results and performs:

- Sensor and visual-information fusion
- Environmental risk assessment
- Irrigation decision-making
- Nutrient-stress correlation
- Crop-health advisory generation
- Local display of results

A TFT display provides field status, AI results, and farmer-oriented recommendations.

## 4. Edge AI

The tomato disease-classification model uses MobileNetV2 and was optimized using Quantization Aware Training (QAT) and full INT8 TensorFlow Lite quantization.

| Parameter | Value |
|---|---|
| Model architecture | MobileNetV2 |
| Input resolution | 128 × 128 |
| Number of classes | 10 |
| Quantization | Full INT8 |
| Target device | ESP32-S3 |
| QAT Keras test accuracy | 92.16% |
| Full-INT8 TFLite test accuracy | 89.42% |

The 89.42% result represents the fully INT8 TensorFlow Lite model used for edge deployment.

The model uses confidence-aware handling. Low-confidence predictions are flagged for verification rather than automatically treated as reliable diagnoses.

These accuracy values are test-set results; real-world performance must also be evaluated under varied field lighting, camera conditions, and crop conditions.

## 5. Multi-Modal Intelligence

Krishi Saathi combines multiple sources of information instead of relying exclusively on one sensor or one AI prediction.

```text
Soil Measurements
        +
Environmental Conditions
        +
Visual Crop Information
        |
        v
Sensor and AI Fusion
        |
        v
Agricultural Risk Assessment
        |
        v
Actionable Farmer Advisory
```

### Example: Possible Nutrient Stress

Low measured soil nitrogen combined with visible leaf yellowing can trigger an advisory to investigate possible nitrogen-related stress.

The system does not assume that yellowing alone proves a specific nutrient deficiency.

### Example: Heat and Drought Risk

High air temperature combined with low soil moisture can indicate elevated heat and drought stress risk.

## 6. Smart Irrigation

The irrigation decision considers soil moisture, rain/wetness, and field waterlogging conditions.

| Field condition | Prototype decision |
|---|---|
| Dry soil, no rain, no waterlogging | Irrigation recommended |
| Rain detected | Wait; irrigation delayed |
| Waterlogging detected | Irrigation blocked |
| Soil moisture sufficient | No irrigation recommended |

The current prototype demonstrates irrigation decisions using a GPIO indicator LED rather than a production irrigation pump.

## 7. Agricultural Risk Management

Krishi Saathi aims to help identify risks that may lead to preventable agricultural losses.

### Environmental risks

- Low soil moisture and drought risk
- Heat stress
- Excessive rain
- Field waterlogging

### Crop-health risks

- Tomato leaf diseases
- Spider mite infestation classification
- Visual nutrient-stress indicators

### Decision support

- Irrigation recommendations
- Low-confidence image verification
- Possible nutrient-stress advisories
- Crop-health inspection recommendations

## 8. Edge-First Operation

The core architecture supports local operation:

- Sensor acquisition
- ESP-NOW communication
- On-device AI inference
- Environmental risk analysis
- Irrigation decisions
- Farmer advisory generation
- TFT display

Cloud services and mobile applications can extend the system with remote monitoring, historical data, analytics, and notifications.

They are intended as enhancements rather than prerequisites for the core local decisions.

## 9. Prototype Status

The following components have been implemented and tested during prototype development:

- [x] ESP32 sensor node
- [x] 7-in-1 RS485 soil sensor integration
- [x] DHT22 environmental sensing
- [x] Rain/wetness sensing
- [x] Field water-level sensing
- [x] ESP-NOW sensor-to-gateway communication
- [x] Main ESP32 gateway
- [x] TFT local display
- [x] Irrigation decision logic
- [x] Environmental risk analysis
- [x] ESP32-S3 camera integration
- [x] OV3660 image capture
- [x] INT8 AI inference
- [x] Confidence-aware prediction handling
- [x] Visual crop-stress analysis
- [x] Soil and visual nutrient-stress fusion
- [x] Local farmer advisory generation

This checklist describes the prototype's reported implementation status. Individual components and their operating conditions should be verified against the published firmware and demonstration evidence.

## 10. Economic Value

Krishi Saathi does not claim a guaranteed increase in yield or farmer income at the prototype stage.

Its intended economic value comes from the potential to:

- Reduce preventable crop losses
- Identify risks earlier
- Avoid unnecessary irrigation
- Support better-informed input decisions
- Improve the timing of crop inspection and intervention

Economic viability should be established through field trials measuring water consumption, input expenditure, intervention time, crop loss, yield preservation, and total cultivation cost.

## 11. Feasibility and Scalability

The system uses modular ESP32-based nodes and a lightweight quantized AI model.

Its modular architecture allows additional sensing and vision nodes to be considered for larger deployments.

The current focus on tomato provides a starting point for validating the complete sensing, AI, fusion, and advisory pipeline before extending it to other crops.

Future expansion will require crop-specific datasets, model validation, and field testing.

## 12. Future Scope

- Additional crop-specific disease models
- Improved nutrient-stress analysis
- More advanced pest detection
- Long-term crop growth monitoring
- Mobile application integration
- SMS alerts
- Historical field analytics
- Cloud-assisted analytics
- Larger agricultural deployments

## 13. Repository Structure

```text
firmware/   Embedded firmware for sensor, gateway, and vision nodes
ai/         Model information, training, and evaluation
hardware/   Hardware connections and component documentation
docs/       Architecture and technical reports
images/     Prototype, architecture, and result images
datasets/   Dataset documentation and access information
```

## 14. Project Information

**Project:** Krishi Saathi  
**Hackathon:** Smart India Hackathon 2026  
**Domain:** Smart Farming / Precision Agriculture  
**Approach:** IoT + Computer Vision + Edge AI + Sensor Fusion

---

Krishi Saathi is a prototype agricultural decision-support system. Its outputs should be verified against field conditions and appropriate agricultural guidance before high-impact farming decisions are made.
