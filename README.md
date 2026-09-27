# 🌱 Krishi Saathi
### Edge-AI Powered Agricultural Risk Management & Decision Support System
> **Sense → Detect → Assess Risk → Advise**

Krishi Saathi is an edge-first smart farming system that combines IoT sensors, computer vision, on-device AI and sensor fusion to identify agricultural risks and provide actionable field-level advisories.

The system is designed not simply as a crop monitoring device, but as an **agricultural risk-management and decision-support platform** focused on reducing preventable crop losses and inefficient resource use.

---

## 🎯 Problem
Agriculture is affected by multiple interconnected risks:

- Drought and low soil moisture
- Heat stress
- Excessive rainfall
- Waterlogging
- Crop diseases
- Pest infestation
- Nutrient imbalance
- Unnecessary irrigation and input usage
- Limited internet connectivity in agricultural areas

Conventional monitoring systems often provide isolated sensor readings or image-based predictions without considering the complete field condition.

Krishi Saathi addresses this gap by combining **soil, environmental and visual crop information** at the edge.

---

## 💡 Our Solution

Krishi Saathi consists of three major components:

### 1. Sensor Node — ESP32

Collects:

- Nitrogen (N)
- Phosphorus (P)
- Potassium (K)
- Soil pH
- Electrical Conductivity (EC)
- Soil moisture
- Soil temperature
- Air temperature
- Air humidity
- Rain/wetness condition
- Field waterlogging condition

The sensor node communicates with the main gateway using **ESP-NOW**.

---

### 2. Vision Node — ESP32-S3-CAM

The vision node captures tomato leaf images using an OV3660 camera.

A lightweight MobileNetV2-based model is deployed directly on the ESP32-S3 using full INT8 quantization.

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

The system also performs visual stress analysis using:

- Green leaf-color percentage
- Yellow/chlorotic regions
- Brown/necrotic regions
- Plant/color coverage
- Texture variation

---
### 3. Main Edge Gateway — ESP32
The main gateway receives information from the sensor and vision nodes.
It performs:

- Sensor fusion
- Environmental risk analysis
- Irrigation decision logic
- Crop-health assessment
- Visual nutrient-stress correlation
- Farmer advisory generation

A local TFT display provides field status and recommendations without requiring continuous internet connectivity.

---
# 🧠 Edge AI
The disease detection model is based on MobileNetV2 and optimized for constrained edge hardware using Quantization Aware Training (QAT) and full INT8 TensorFlow Lite quantization.

### Model Configuration

| Parameter | Value |
|---|---|
| Architecture | MobileNetV2 |
| Input Size | 128 × 128 |
| Classes | 10 |
| Quantization | Full INT8 |
| Target Device | ESP32-S3 |
| QAT Keras Test Accuracy | 92.16% |
| Full-INT8 TFLite Test Accuracy | 89.42% |

The **89.42% result represents the fully INT8 TensorFlow Lite model intended for edge deployment**.
The system also uses confidence-aware prediction handling. Low-confidence predictions are treated as uncertain and the user is advised to verify or recapture the image instead of receiving an overconfident result.
---
# 🔗 Multi-Modal Intelligence
Krishi Saathi does not depend on a single sensor or a single AI prediction.
It combines:

```text
Soil Data
    +
Environmental Data
    +
Visual Crop Data
    ↓
Sensor + AI Fusion
    ↓
Risk Assessment
    ↓
Actionable Advisory

Ensessment
    ↓
Actionable Advisory

### Example

Low soil nitrogen

Visible leaf yellowing

↓

Possible nutrient stress

The system does not claim that RGB images alone directly measure nutrient concentration. Visual symptoms are correlated with measured NPK, pH and EC values before generating an advisory.
