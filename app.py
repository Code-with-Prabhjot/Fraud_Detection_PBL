from flask import Flask, render_template, request, jsonify
import subprocess
import sys
import os

app = Flask(__name__)
EXE_NAME = "engine.exe" if sys.platform == "win32" else "./engine"

@app.route('/')
def home():
    return render_template('index.html')

@app.route('/run_sweep', methods=['POST'])
def run_sweep():
    # 1. Run the C Extraction Engine
    process = subprocess.run([EXE_NAME], text=True, capture_output=True)
    output = process.stdout.strip()
    
    parts = output.split('|')
    processed = parts[0].split(':')[1] if len(parts) > 0 else "0"
    frauds_count = parts[1].split(':')[1] if len(parts) > 1 else "0"
    
    # 2. Read the exact frauds from the log file
    fraud_list = []
    if os.path.exists("fraud_alerts.log"):
        with open("fraud_alerts.log", "r") as f:
            for line in f:
                details = line.strip().split('|')
                if len(details) == 4:
                    fraud_list.append({
                        "txn_id": details[0],
                        "amount": f"₹{details[1]}",
                        "time": details[2],
                        "category": details[3]
                    })
    
    return jsonify({
        "processed": processed,
        "frauds_count": frauds_count,
        "alerts": fraud_list
    })

if __name__ == '__main__':
    app.run(debug=True)