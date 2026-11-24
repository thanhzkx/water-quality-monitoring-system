

<div align="center">



\# 💧 Water Quality Monitoring System



\*\*Real-time Water Quality Monitoring with IoT \& Edge AI\*\*



!\[ESP32](https://img.shields.io/badge/ESP32-E7352C?style=for-the-badge\&logo=espressif\&logoColor=white)

!\[C++](https://img.shields.io/badge/C++-00599C?style=for-the-badge\&logo=cplusplus\&logoColor=white)

!\[Python](https://img.shields.io/badge/Python-3776AB?style=for-the-badge\&logo=python\&logoColor=white)

!\[TensorFlow](https://img.shields.io/badge/TensorFlow-FF6F00?style=for-the-badge\&logo=tensorflow\&logoColor=white)

!\[MQTT](https://img.shields.io/badge/MQTT-660066?style=for-the-badge\&logo=mqtt\&logoColor=white)



!\[Node-RED](https://img.shields.io/badge/Node--RED-8F0000?style=for-the-badge\&logo=nodered\&logoColor=white)

!\[MySQL](https://img.shields.io/badge/MySQL-4479A1?style=for-the-badge\&logo=mysql\&logoColor=white)

!\[React](https://img.shields.io/badge/React-20232A?style=for-the-badge\&logo=react\&logoColor=61DAFB)

!\[Node.js](https://img.shields.io/badge/Node.js-339933?style=for-the-badge\&logo=nodedotjs\&logoColor=white)

!\[Docker](https://img.shields.io/badge/Docker-2496ED?style=for-the-badge\&logo=docker\&logoColor=white)



!\[Status](https://img.shields.io/badge/Status-In\_Development-orange)

!\[Platform](https://img.shields.io/badge/Platform-ESP32-blue)

!\[AI](https://img.shields.io/badge/Edge\_AI-INT8-green)



\[Overview](#-overview) •

\[Features](#-features) •

\[Architecture](#-system-architecture) •

\[Edge AI](#-edge-ai) •

\[Structure](#-repository-structure) •

\[Roadmap](#-roadmap)



</div>



\---



\## 📖 Overview



An end-to-end IoT water quality monitoring system 

designed to collect, process, and analyze sensor 

data in real time.



The system integrates ESP32-based embedded devices,

MQTT communication, automated data processing,

and on-device anomaly detection using a quantized

Autoencoder model.



The project explores the integration of Embedded

Systems, Internet of Things, Edge AI, and modern

web technologies into a unified monitoring platform.



\## ✨ Features



\- 🌡️ Real-time temperature monitoring

\- 🧪 pH level measurement

\- 💧 Turbidity monitoring

\- 🤖 On-device AI anomaly detection

\- 📡 MQTT-based communication

\- 📊 Real-time and historical data visualization

\- 🗄️ Centralized sensor data storage

\- 🔄 Automated data preprocessing

\- 📱 Multi-device monitoring

\- 💬 AI-assisted water quality analysis



\## 🏗️ System Architecture



```mermaid

flowchart TD

&#x20;   subgraph Hardware

&#x20;       A\[Temperature Sensor]

&#x20;       B\[pH Sensor]

&#x20;       C\[Turbidity Sensor]

&#x20;   end



&#x20;   subgraph Edge\_Device

&#x20;       D\[ESP32]

&#x20;       E\[INT8 Autoencoder]

&#x20;       D --> E

&#x20;   end



&#x20;   subgraph Infrastructure

&#x20;       F\[MQTT Broker]

&#x20;       G\[Node-RED]

&#x20;       H\[(MySQL)]

&#x20;   end



&#x20;   subgraph Application

&#x20;       I\[Express API]

&#x20;       J\[React Dashboard]

&#x20;       K\[AI Assistant]

&#x20;       L\[Node-RED Dashboard]

&#x20;   end



&#x20;   A --> D

&#x20;   B --> D

&#x20;   C --> D

&#x20;   E --> F

&#x20;   F --> G

&#x20;   G --> H

&#x20;   G --> L

&#x20;   H --> I

&#x20;   I --> J

&#x20;   I --> K

```



\## 🛠️ Technology Stack



| Layer | Technology |

| :--- | :--- |

| Hardware | ESP32, DS18B20, pH, Turbidity |

| Firmware | C/C++, ESP-IDF |

| Edge AI | TensorFlow Lite Micro |

| Machine Learning | Python, TensorFlow, Keras |

| Communication | MQTT, Mosquitto |

| Data Processing | Node-RED |

| Database | MySQL |

| Backend | Node.js, Express |

| Frontend | React, Vite |

| AI Assistant | Groq API |

| Infrastructure | Docker Compose |



\## 🔌 Hardware



| Component | Description |

| :--- | :--- |

| ESP32 | Main microcontroller |

| DS18B20 | Water temperature sensor |

| pH Sensor | Water acidity measurement |

| Turbidity Sensor | Water clarity measurement |



\## 🧠 Edge AI



The anomaly detection model is deployed directly

on the ESP32 using TensorFlow Lite Micro.



| Parameter | Specification |

| :--- | :--- |

| Model | Autoencoder |

| Input Features | Temperature, pH, Turbidity |

| Window Size | 10 samples |

| Input Shape | 10 × 3 |

| Quantization | INT8 |

| Inference Engine | TensorFlow Lite Micro |

| Target Device | ESP32 |



\### Anomaly Detection Pipeline



```mermaid

flowchart LR

&#x20;   A\[Sensor Data] --> B\[Sliding Window]

&#x20;   B --> C\[Normalization]

&#x20;   C --> D\[INT8 Autoencoder]

&#x20;   D --> E\[Reconstruction Error]

&#x20;   E --> F{Threshold}

&#x20;   F -->|Within| G\[Normal]

&#x20;   F -->|Above| H\[Anomaly]

```



\## 🔄 Data Processing



```mermaid

flowchart LR

&#x20;   A\[Raw Data] --> B\[NULL Filtering]

&#x20;   B --> C\[Deduplication]

&#x20;   C --> D\[Resampling]

&#x20;   D --> E\[Rolling IQR]

&#x20;   E --> F\[Interpolation]

&#x20;   F --> G\[Clean Data]

```



\## 📁 Repository Structure



```text

water-quality-monitoring-system/

│

├── firmware/          # Embedded firmware

├── ai/                # Machine learning and Edge AI

├── backend/           # Backend services

├── frontend/          # Web dashboard

├── infrastructure/    # Infrastructure configuration

├── database/          # Database schema

├── data/              # Datasets

├── docs/              # Documentation

│

├── .gitignore

└── README.md

```



\## 🚀 Getting Started



\### Clone the repository



```bash

git clone https://github.com/thanhzkx/water-quality-monitoring-system.git

cd water-quality-monitoring-system

```



\### Prerequisites



\- ESP-IDF

\- Python 3

\- Node.js

\- Docker

\- Docker Compose



Deployment instructions are currently under development.



\## 🔒 Security



Production credentials, API keys, private data,

and database backups must not be committed to

this repository.



Security hardening and deployment configuration

are ongoing development tasks.



\## 🗺️ Roadmap



\- \[x] Sensor data acquisition

\- \[x] MQTT communication

\- \[x] Database integration

\- \[x] Data preprocessing

\- \[x] Autoencoder training

\- \[x] INT8 model conversion

\- \[x] Edge AI integration

\- \[x] Monitoring dashboard

\- \[ ] Security hardening

\- \[ ] Embedded performance benchmarking

\- \[ ] Automated testing

\- \[ ] Complete deployment documentation



\## 👨‍💻 Author



\*\*Nguyen Minh Thanh\*\*



&#x20;

Ho Chi Minh City University of Technology (HCMUT)



\[!\[GitHub](https://img.shields.io/badge/GitHub-thanhzkx-181717?style=for-the-badge\&logo=github)](https://github.com/thanhzkx)



\---



<div align="center">



\*\*Built with Embedded Systems, IoT \& Edge AI\*\*



⭐ Star this repository if you find it interesting!



</div>



