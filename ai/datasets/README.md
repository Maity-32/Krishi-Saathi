# Dataset Documentation

## Purpose

The Krishi Saathi AI model was trained for tomato leaf disease classification.

## Dataset

The training workflow used a tomato leaf image dataset derived from PlantVillage.

## Dataset Processing

The images were processed into a tomato-specific dataset for model training and evaluation.

The model supports 10 classes:

1. Bacterial Spot
2. Early Blight
3. Healthy
4. Late Blight
5. Leaf Mold
6. Mosaic Virus
7. Spider Mites
8. Target Spot
9. Septoria Leaf Spot
10. Yellow Leaf Curl Virus

## Reported Dataset Split

- Training: 70%
- Validation: 15%
- Testing: 15%

The reported split used stratified sampling with random seed 42.

## Model Input

128 × 128 pixels.

## Limitations

A dataset collected under controlled conditions may not represent every field environment.

Additional field images are needed to evaluate robustness to natural lighting, different camera positions, backgrounds, and real-world crop conditions.

## Dataset Access

Refer to the original dataset source and its applicable license and terms before downloading or redistributing the images.
