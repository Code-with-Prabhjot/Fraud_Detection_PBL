from flask import Flask, render_template, request, jsonify
import subprocess
import sys
import os
import json
import sqlite3
import random

app = Flask(__name__)
EXE_NAME = "engine.exe" if sys.platform == "win32" else "./engine"
DB_NAME = "soc_database.db"

def init_db():
    with sqlite3.connect(DB_NAME) as conn:
        c = conn.cursor()
        c.execute('''CREATE TABLE IF NOT EXISTS transactions (
                        txn_id TEXT PRIMARY KEY, amount INTEGER, hour INTEGER,
                        device TEXT, location TEXT, ip TEXT, category TEXT,
                        beh_anom TEXT, new_ben TEXT, rap_txn TEXT, 
                        wknd TEXT, new_acc TEXT, fail_log TEXT,
                        status TEXT DEFAULT 'PENDING', reason TEXT DEFAULT ''
                     )''')
        
        # Pre-seed 100 diverse rows for the demo so the database is populated instantly
        demo_data = [
            ("TXN-1001", 12500, 14, "Recognized", "Local", "Safe", "Retail", "No", "No", "No", "No", "No", "No"),
            ("TXN-1002", 85400, 2, "Unrecognized", "Foreign", "Blacklisted", "Crypto", "Yes", "Yes", "No", "No", "No", "Yes"),
            ("TXN-1003", 3400, 9, "Recognized", "Local", "Safe", "Retail", "No", "No", "No", "No", "No", "No"),
            ("TXN-1004", 67000, 15, "Unrecognized", "Local", "Safe", "Retail", "No", "No", "No", "No", "No", "No"),
            ("TXN-1005", 92000, 3, "Recognized", "Local", "Blacklisted", "Crypto", "No", "No", "Yes", "Yes", "No", "No"),
            ("TXN-1006", 1500, 11, "Unrecognized", "Local", "Safe", "Retail", "No", "No", "No", "No", "No", "No"),
            ("TXN-1007", 54000, 10, "Unrecognized", "Foreign", "Blacklisted", "Retail", "Yes", "No", "No", "No", "No", "Yes"),
            ("TXN-1008", 125000, 4, "Unrecognized", "Foreign", "Safe", "Crypto", "Yes", "Yes", "Yes", "Yes", "Yes", "No"),
            ("TXN-1009", 850000, 1, "Unrecognized", "Foreign", "Blacklisted", "Crypto", "Yes", "Yes", "Yes", "No", "Yes", "Yes"),
            ("TXN-1010", 250, 18, "Recognized", "Local", "Safe", "Groceries", "No", "No", "No", "No", "No", "No")
        ]
        
        # Multiply and slightly mutate the 10 base templates to reach 100 rows quickly
        extended_data = []
        for i in range(100):
            base = list(demo_data[i % 10])
            base[0] = f"TXN-2{i:03d}" # Unique IDs
            base[1] = base[1] + random.randint(-50, 500) # Slightly vary amount
            extended_data.append(tuple(base))
            
        c.executemany("""INSERT OR IGNORE INTO transactions 
                         (txn_id, amount, hour, device, location, ip, category, beh_anom, new_ben, rap_txn, wknd, new_acc, fail_log, status) 
                         VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,'PENDING')""", extended_data)
        conn.commit()

init_db()

@app.route('/')
def home():
    return render_template('index.html')

@app.route('/payment')
def payment_portal():
    return render_template('payment.html')

@app.route('/api/pay', methods=['POST'])
def make_payment():
    data = request.json
    txn_id = f"TXN-{random.randint(90000, 99999)}"
    
    with sqlite3.connect(DB_NAME) as conn:
        c = conn.cursor()
        c.execute("""INSERT INTO transactions 
                     (txn_id, amount, hour, device, location, ip, category, beh_anom, new_ben, rap_txn, wknd, new_acc, fail_log, status) 
                     VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,'PENDING')""", 
                  (txn_id, data['amount'], data['hour'], data['device'], data['location'], data['ip'], data['category'], 
                   data['beh_anom'], data['new_ben'], data['rap_txn'], data['wknd'], data['new_acc'], data['fail_log']))
        conn.commit()
    
    return jsonify({"success": True, "txn_id": txn_id})

@app.route('/run_sweep', methods=['POST'])
def run_sweep():
    # 1. Fetch PENDING transactions from the queue
    with sqlite3.connect(DB_NAME) as conn:
        c = conn.cursor()
        c.execute("SELECT txn_id, amount, hour, device, location, ip, category, beh_anom, new_ben, rap_txn, wknd, new_acc, fail_log FROM transactions WHERE status='PENDING'")
        pending_txns = c.fetchall()

    # 2. Process ONLY if there are new pending transactions in the queue
    if pending_txns:
        # Load the CSV Cache
        with open("backend_data.csv", "w") as f:
            f.write("Txn_ID,Amount,Hour,Device,Location,IP,Category,Behavioral_Anomaly,New_Beneficiary,Rapid_Transactions,Weekend,New_Account,Failed_Logins\n")
            for r in pending_txns:
                f.write(f"{r[0]},{r[1]},{r[2]},{r[3]},{r[4]},{r[5]},{r[6]},{r[7]},{r[8]},{r[9]},{r[10]},{r[11]},{r[12]}\n")

        # Execute C-Kernel
        subprocess.run([EXE_NAME], text=True, capture_output=True)
        
        # Read the logic results and Archive them into SQL Database
        if os.path.exists("sweep_results.log"):
            with open("sweep_results.log", "r") as f:
                lines = f.readlines()
            with sqlite3.connect(DB_NAME) as conn:
                c = conn.cursor()
                for line in lines:
                    details = line.strip().split('|')
                    if len(details) >= 6:
                        c.execute("UPDATE transactions SET status=?, reason=? WHERE txn_id=?", (details[4], details[5], details[0]))
                conn.commit()

        # Wipe the Temporary CSV Cache clean for the next batch
        with open("backend_data.csv", "w") as f:
            f.write("Txn_ID,Amount,Hour,Device,Location,IP,Category,Behavioral_Anomaly,New_Beneficiary,Rapid_Transactions,Weekend,New_Account,Failed_Logins\n")

    # 3. NEW: Fetch the ENTIRE HISTORY from SQL to display on the Dashboard
    fraud_list = []
    safe_list = []
    with sqlite3.connect(DB_NAME) as conn:
        c = conn.cursor()
        # Grabs everything that is already processed, ordering by exact insertion time
        c.execute("SELECT txn_id, amount, hour, category, reason, status FROM transactions WHERE status != 'PENDING' ORDER BY rowid DESC")
        all_txns = c.fetchall()
        
        for row in all_txns:
            obj = {"txn_id": row[0], "amount": f"₹{row[1]}", "time": f"{row[2]}:00", "category": row[3], "reason": row[4]}
            if row[5] == "FRAUD":
                fraud_list.append(obj)
            else:
                safe_list.append(obj)

    return jsonify({
        "processed": str(len(all_txns)),
        "frauds_count": str(len(fraud_list)),
        "safe_count": str(len(safe_list)),
        "alerts": fraud_list[:150],  # Show up to 150 on UI
        "safe": safe_list[:150]      
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