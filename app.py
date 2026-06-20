from flask import Flask, render_template, request, jsonify
import subprocess
import sys
import os
import json

app = Flask(__name__)
EXE_NAME = "engine.exe" if sys.platform == "win32" else "./engine"

@app.route('/')
def home():
    return render_template('index.html')

@app.route('/run_sweep', methods=['POST'])
def run_sweep():
    # 1. Run C Engine
    process = subprocess.run([EXE_NAME], text=True, capture_output=True)
    output = process.stdout.strip()
    
    parts = output.split('|')
    processed = parts[0].split(':')[1] if len(parts) > 0 else "0"
    frauds_count = parts[1].split(':')[1] if len(parts) > 1 else "0"
    
    fraud_list = []
    fraud_ids = set()
    
    # 2. Get Frauds
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
                    fraud_ids.add(details[0])

    # 3. Get Safe Transactions by cross-referencing
    safe_list = []
    if os.path.exists("backend_data.csv"):
        with open("backend_data.csv", "r") as f:
            lines = f.readlines()[1:] # Skip header
            for line in lines:
                details = line.strip().split(',')
                if len(details) >= 7: # Strict column check to prevent crash
                    txn_id = details[0]
                    if txn_id not in fraud_ids:
                        amt = details[1]
                        hr = details[2]
                        cat = details[6]
                        safe_list.append({
                            "txn_id": txn_id,
                            "amount": f"₹{amt}",
                            "time": f"{hr}:00" if ":" not in hr else hr,
                            "category": cat
                        })
    
    return jsonify({
        "processed": processed,
        "frauds_count": frauds_count,
        "safe_count": str(len(safe_list)), # NEW: Send exact safe count to frontend
        "alerts": fraud_list,
        "safe": safe_list
    })

@app.route('/api/tree', methods=['GET'])
def get_tree():
    try:
        process = subprocess.run([EXE_NAME, "--tree"], text=True, capture_output=True)
        return jsonify(json.loads(process.stdout.strip()))
    except Exception as e:
        return jsonify({"error": str(e)}), 500

if __name__ == '__main__':
    app.run(debug=True)