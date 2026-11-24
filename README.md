<h1 align="center">💧 Water Quality Monitoring System</h1><p align="center"><strong>Real-time water quality monitoring with IoT and Edge AI</strong></p><p align="center">
  <img src="https://img.shields.io/badge/ESP32-E7352C?style=for-the-badge&amp;logo=espressif&amp;logoColor=white" alt="ESP32">
  <img src="https://img.shields.io/badge/C%2B%2B-00599C?style=for-the-badge&amp;logo=cplusplus&amp;logoColor=white" alt="C++">
  <img src="https://img.shields.io/badge/Python-3776AB?style=for-the-badge&amp;logo=python&amp;logoColor=white" alt="Python">
  <img src="https://img.shields.io/badge/TensorFlow-FF6F00?style=for-the-badge&amp;logo=tensorflow&amp;logoColor=white" alt="TensorFlow">
  <img src="https://img.shields.io/badge/MQTT-660066?style=for-the-badge&amp;logo=mqtt&amp;logoColor=white" alt="MQTT">
</p><p align="center">
  <img src="https://img.shields.io/badge/Node--RED-8F0000?style=for-the-badge&amp;logo=nodered&amp;logoColor=white" alt="Node-RED">
  <img src="https://img.shields.io/badge/MySQL-4479A1?style=for-the-badge&amp;logo=mysql&amp;logoColor=white" alt="MySQL">
  <img src="https://img.shields.io/badge/React-20232A?style=for-the-badge&amp;logo=react&amp;logoColor=61DAFB" alt="React">
  <img src="https://img.shields.io/badge/Node.js-339933?style=for-the-badge&amp;logo=nodedotjs&amp;logoColor=white" alt="Node.js">
  <img src="https://img.shields.io/badge/Docker-2496ED?style=for-the-badge&amp;logo=docker&amp;logoColor=white" alt="Docker">
</p><p align="center">
  <img src="https://img.shields.io/badge/status-in_development-orange" alt="Status">
  <img src="https://img.shields.io/badge/Edge_AI-INT8-green" alt="Edge AI">
</p><p align="center">
  <a href="#overview">Overview</a> · <a href="#features">Features</a> · <a href="#system-architecture">Architecture</a> · <a href="#edge-ai">Edge AI</a> · <a href="#getting-started">Getting Started</a> · <a href="#roadmap">Roadmap</a>
</p>Overview
An end-to-end IoT water quality monitoring project combining embedded sensor acquisition, MQTT communication, automated data preprocessing, and on-device anomaly detection. An ESP32 processes temperature, pH, and turbidity measurements using an INT8 Autoencoder, while server-side services support storage, visualization, and AI-assisted analysis.
Features
- Real-time temperature, pH, and turbidity monitoring
- On-device anomaly detection with TensorFlow Lite Micro
- MQTT-based sensor telemetry
- Multi-device monitoring and historical visualization
- Automated sensor data cleaning and preprocessing
- MySQL storage and Express API
- Web dashboard and AI-assisted analysis
System Architecture
```mermaid
flowchart TD
    TEMP["Temperature Sensor"] --> ESP["ESP32"]
    PH["pH Sensor"] --> ESP
    TUR["Turbidity Sensor"] --> ESP
    ESP --> AI["INT8 Autoencoder"]
    AI --> MQTT["Mosquitto MQTT Broker"]
    MQTT --> NR["Node-RED"]
    NR --> DB[("MySQL Database")]
    NR --> DASH["Node-RED Dashboard"]
    DB <--> API["Express REST API"]
    API <--> WEB["React Web Dashboard"]
    API <--> GROQ["Groq API"]
    classDef hardware fill:#1d4ed8,color:#fff,stroke:#60a5fa
    classDef edge fill:#15803d,color:#fff,stroke:#4ade80
    classDef infra fill:#7e22ce,color:#fff,stroke:#c084fc
    classDef app fill:#0e7490,color:#fff,stroke:#22d3ee
    class TEMP,PH,TUR hardware
    class ESP,AI edge
    class MQTT,NR,DB infra
    class API,WEB,DASH,GROQ app
```
Technology Stack
Layer	Technologies
Firmware	ESP32, ESP-IDF, C/C++
Edge AI	TensorFlow Lite Micro, INT8 quantization
Model training	Python, TensorFlow, Keras
Communication	MQTT, Mosquitto
Data processing	Node-RED, JavaScript
Database	MySQL
Backend	Node.js, Express
Frontend	React, Vite
AI assistant	Groq API
Infrastructure	Docker Compose


Hardware
Component	Purpose
ESP32	Sensor acquisition, inference and communication
DS18B20	Water temperature measurement
pH sensor	Acidity measurement
Turbidity sensor	Water clarity measurement


Edge AI
The firmware integrates an Autoencoder to detect abnormal patterns in sequential sensor measurements using reconstruction error.
Parameter	Specification
Model	Autoencoder
Features	Temperature, pH, turbidity
Window size	10 samples
Input shape	10 × 3
Quantization	INT8
Inference engine	TensorFlow Lite Micro
Target	ESP32


```mermaid
flowchart TD
    INPUT["Sensor Measurements"] --> WINDOW["Sliding Window"]
    WINDOW --> SCALE["Standardization"]
    SCALE --> MODEL["INT8 Autoencoder"]
    MODEL --> ERROR["Weighted Reconstruction Error"]
    ERROR --> CHECK{"Above Threshold?"}
    CHECK -->|No| NORMAL["Normal"]
    CHECK -->|Yes| ANOMALY["Anomaly"]
```
Evaluation focuses on precision, recall, F1-score, false positive rate, model size, and inference latency. On-device performance requires separate benchmarking.
Data Processing
```mermaid
flowchart TD
    RAW["Raw Sensor Data"] --> FILTER["NULL Filtering"]
    FILTER --> DEDUP["Deduplication"]
    DEDUP --> RESAMPLE["Resampling"]
    RESAMPLE --> IQR["Rolling IQR"]
    IQR --> INTERP["Interpolation"]
    INTERP --> CLEAN["Clean Dataset"]
```
Repository Structure
The project is organized around the following components:
water-quality-monitoring-system/
├── firmware/          # Embedded firmware
├── ai/                # Model training and Edge AI
├── backend/           # Backend API
├── frontend/          # Web dashboard build
├── infrastructure/    # Deployment configurations
├── database/          # Database schema
├── data/              # Approved datasets
├── docs/              # Technical documentation
├── .gitignore
└── README.md
Getting Started
Clone the repository
git clone https://github.com/thanhzkx/water-quality-monitoring-system.git
cd water-quality-monitoring-system
Prerequisites
- ESP-IDF for firmware development
- Python 3 for model training
- Node.js and npm for backend development
- Docker and Docker Compose for infrastructure
Full deployment instructions are under development. The recovered frontend build is not a substitute for the original editable React source. Configure local credentials and review service settings before deployment.
Security
Do not commit production credentials, API keys, private datasets, database backups, or runtime files. Security hardening remains in progress, including MQTT access control, TLS, parameterized SQL queries, input validation, and device-level authorization.

This project is a monitoring prototype. Sensor measurements and anomaly predictions alone cannot establish that water is safe to drink.
Roadmap
- [x] Sensor data acquisition firmware
- [x] MQTT integration
- [x] Database integration and preprocessing logic
- [x] Autoencoder training and INT8 conversion
- [x] Edge AI firmware integration
- [x] Monitoring dashboard implementation
- [ ] Verify reproducible deployment
- [ ] Restore or reconstruct editable frontend source
- [ ] Security hardening
- [ ] On-device performance benchmarking
- [ ] Automated testing
- [ ] Complete technical documentation
Author
Nguyen Minh Thanh —  Ho Chi Minh City University of Technology (HCMUT).

GitHub — thanhzkx
<p align="center">Built with Embedded Systems, IoT and Edge AI.</p>