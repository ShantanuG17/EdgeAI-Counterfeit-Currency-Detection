# EdgeAI Counterfeit Currency Detection

An embedded TinyML-based system for detecting counterfeit Indian currency using multispectral sensing, machine learning and ESP32-S3 edge deployment.

## Overview

Counterfeit currency detection is an important challenge in financial security. This project develops a compact, portable currency authentication system using spectral measurements and an embedded neural network.

The system uses the **AS7265x 18-channel multispectral sensor** to analyse the optical characteristics of Indian banknotes. Measurements are collected from three distinct regions:

- Region 1: Ink
- Region 2: Security thread
- Region 3: Blank paper

The collected spectral data is processed and used to train a Multilayer Perceptron (MLP) classifier. The trained model is quantized and deployed on an ESP32-S3 microcontroller for on-device inference without requiring cloud connectivity.

## Key Features

- 18-channel multispectral sensing (410–940 nm).
- Analysis of three different banknote regions.
- Genuine versus counterfeit currency classification.
- Feature selection and data preprocessing.
- Multilayer Perceptron neural network.
- INT8 post-training quantization.
- Embedded inference on ESP32-S3.
- Cloud-independent operation.

## Hardware Components

| Component | Purpose |
|---|---|
| ESP32-S3 | Embedded processing and ML inference |
| AS7265x | 18-channel multispectral sensing |
| Sensor-integrated UV LED (365 nm) | UV illumination |
| Sensor-integrated IR LED (850 nm) | Infrared illumination |
| Sensor-integrated white LED | Visible illumination |

The AS7265x communicates with the ESP32-S3 through I2C.

## Software and Technologies

- Arduino IDE – Embedded firmware development
- Python – Data processing and machine learning
- Google Colab – Data analysis and experimentation
- Edge Impulse – Model development and evaluation
- TensorFlow Lite Micro – Embedded model deployment
- NumPy, Pandas and Scikit-learn – Data processing and analysis

## System Architecture

The system follows a sensor-to-inference pipeline:

1. **Spectral Data Acquisition:** The AS7265x sensor captures spectral measurements from genuine and counterfeit Indian banknotes.
2. **Dataset Preparation:** Measurements are labelled and organised for machine learning.
3. **Data Preprocessing:** Exploratory analysis, feature selection and normalization are performed.
4. **Model Training:** A two-hidden-layer MLP classifier is developed.
5. **Model Optimization:** The trained model undergoes INT8 post-training quantization.
6. **Embedded Deployment:** The optimized model is deployed on the ESP32-S3 using TensorFlow Lite Micro.
7. **Real-Time Classification:** The device classifies the banknote as GENUINE or FAKE.

## Machine Learning Methodology

### Dataset

A dataset containing 555 Indian banknote samples was collected.

The dataset includes:

- Genuine and counterfeit banknotes.
- ₹100, ₹200 and ₹500 denominations.
- Three measurement regions per banknote.
- Multispectral measurements under UV, visible and IR illumination.

### Feature Selection and Preprocessing

Feature selection was performed using statistical techniques, including:

- Pearson correlation.
- Mutual information.

The original measurement features were reduced to 15 selected features for classification.

The selected features were normalized using StandardScaler.

### Neural Network Architecture

A two-hidden-layer Multilayer Perceptron was developed.

| Layer | Neurons | Activation |
|---|---:|---|
| Input | 15 | — |
| Hidden Layer 1 | 32 | ReLU |
| Hidden Layer 2 | 16 | ReLU |
| Output | 2 | Softmax |

The network contains 1,074 trainable parameters and was trained using the Adam optimizer.

## Model Performance

The final model was evaluated on a test set of 103 banknote samples.

| Metric | Result |
|---|---:|
| Test Accuracy | 97.09% |
| F1-Score | 0.9709 |
| ROC-AUC | 0.9710 |
| False Positive Rate | 2.0% |
| False Negative Rate | 3.85% |

### INT8 Quantization

Post-training INT8 quantization was performed to reduce the model's memory requirements.

| Parameter | Float32 | INT8 |
|---|---:|---:|
| Model size | 3.1 kB | 1.1 kB |
| Test accuracy | 97.09% | 97.09% |

The quantized model retained the same classification accuracy as the original model.

The model inference time was reported to be below 1 millisecond on the ESP32-S3.

## Repository Structure

```text
tinyml-currency-detection/
│
├── README.md
│
├── Arduino/
│  

