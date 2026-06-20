# Real-Time FinTech Fraud Monitoring Center (SOC)

A high-performance, real-time fraud detection pipeline designed for FinTech and Security Operations Center (SOC) environments. This platform combines an in-memory machine learning engine built in pure C with an asynchronous Python Flask streaming server to evaluate transaction risk dynamically.

## 🚀 Key Features
* **Hybrid Machine Learning Architecture:** Implements a recursive ID3 Decision Tree classifier from scratch in C, evaluating Shannon Entropy and Information Gain dynamically.
* **Heuristic Risk Scoring:** Processes multi-dimensional vectors (amount, time, IP, location, device) to generate an absolute risk score from $0$ to $100$.
* **Behavioral Anomaly Engine:** Benchmarks live transaction volumes against historical customer metrics (`user_profiles.csv`) to flag spikes exceeding a $3\times$ variance threshold.
* **SOC Dark Dashboard:** A real-time terminal UI that streams live transaction data every 1.5 seconds, dynamically pushing alerts for `HIGH` and `CRITICAL` incidents.
* **Desktop Push Notifications:** Built-in browser notification integration to alert security analysts of urgent threats instantly.

## 📂 Project Structure
```text
Fraud_Detection_PBL/
│
├── app.py                  # Asynchronous Flask API Bridge
├── engine.c                # C ML & Heuristic Scoring Engine
├── backend_data.csv        # Simulated Live Transaction Ledger
├── training_rules.csv      # AI Historical Rules Dataset
├── user_profiles.csv       # Behavioral Profiles Layer
│
└── templates/
    └── index.html          # SOC Real-Time Web Dashboard