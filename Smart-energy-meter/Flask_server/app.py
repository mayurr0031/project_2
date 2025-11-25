from flask import Flask, render_template, jsonify, request
from flask_cors import CORS
import mysql.connector
from mysql.connector import Error
from datetime import datetime, timedelta
import json

app = Flask(__name__)
CORS(app)

# ===================== CONFIGURATION =====================
DB_CONFIG = {
    'host': 'localhost',
    'user': 'root',
    'password': 'Govind#15',
    'database': 'energy_meter',
    'auth_plugin': 'mysql_native_password'
}

# ===================== GLOBAL RELAY STATE =====================
relay_states = {
    'relay1': False,
    'relay2': False,
    'last_updated': None
}

# ===================== DATABASE FUNCTIONS =====================
def get_db_connection():
    """Create and return database connection"""
    try:
        connection = mysql.connector.connect(**DB_CONFIG)
        return connection
    except Error as e:
        print(f"❌ Error connecting to MySQL: {e}")
        return None

def init_database():
    """Initialize database and create tables if they don't exist"""
    try:
        connection = mysql.connector.connect(
            host=DB_CONFIG['host'],
            user=DB_CONFIG['user'],
            password=DB_CONFIG['password'],
            auth_plugin=DB_CONFIG['auth_plugin']
        )
        cursor = connection.cursor()
        
        # Create database if not exists
        cursor.execute("CREATE DATABASE IF NOT EXISTS energy_meter")
        cursor.execute("USE energy_meter")
        
        # Create readings table
        cursor.execute("""
            CREATE TABLE IF NOT EXISTS readings (
                id INT AUTO_INCREMENT PRIMARY KEY,
                timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
                voltage FLOAT,
                current1 FLOAT,
                current2 FLOAT,
                current3 FLOAT,
                total_current FLOAT,
                power1 FLOAT,
                power2 FLOAT,
                total_power FLOAT,
                relay1_state BOOLEAN,
                relay2_state BOOLEAN,
                INDEX idx_timestamp (timestamp)
            )
        """)
        
        # Create daily summary table
        cursor.execute("""
            CREATE TABLE IF NOT EXISTS daily_summary (
                id INT AUTO_INCREMENT PRIMARY KEY,
                date DATE UNIQUE,
                total_energy_kwh FLOAT,
                avg_voltage FLOAT,
                avg_current FLOAT,
                max_power FLOAT,
                min_power FLOAT
            )
        """)
        
        connection.commit()
        cursor.close()
        connection.close()
        print("✅ Database initialized successfully")
    except Error as e:
        print(f"❌ Error initializing database: {e}")

# ===================== API ENDPOINTS =====================

@app.route('/')
def index():
    """Serve main dashboard page"""
    return render_template('index.html')

@app.route('/api/data', methods=['POST'])
def receive_data():
    """Receive sensor data from ESP32 (WITHOUT relay states)"""
    try:
        data = request.get_json()
        print(f"📊 Sensor data received: Power={data.get('total_power', 0):.2f}W, "
              f"Voltage={data.get('voltage', 0):.1f}V")
        
        connection = get_db_connection()
        if not connection:
            return jsonify({'status': 'error', 'message': 'Database connection failed'}), 500
        
        cursor = connection.cursor()
        
        # Use current server relay states for database storage
        query = """
            INSERT INTO readings 
            (voltage, current1, current2, current3, total_current, 
             power1, power2, total_power, relay1_state, relay2_state)
            VALUES (%s, %s, %s, %s, %s, %s, %s, %s, %s, %s)
        """
        
        values = (
            float(data.get('voltage', 0)),
            float(data.get('current1', 0)),
            float(data.get('current2', 0)),
            float(data.get('current3', 0)),
            float(data.get('total_current', 0)),
            float(data.get('power1', 0)),
            float(data.get('power2', 0)),
            float(data.get('total_power', 0)),
            relay_states['relay1'],
            relay_states['relay2']
        )
        
        cursor.execute(query, values)
        connection.commit()
        
        cursor.close()
        connection.close()
        
        return jsonify({'status': 'success', 'message': 'Data saved'}), 200
        
    except Exception as e:
        print(f"❌ Error saving data: {e}")
        return jsonify({'status': 'error', 'message': str(e)}), 500

@app.route('/api/latest', methods=['GET'])
def get_latest():
    """Get the most recent reading"""
    try:
        connection = get_db_connection()
        if not connection:
            return jsonify({'status': 'error', 'message': 'Database connection failed'}), 500
        
        cursor = connection.cursor(dictionary=True)
        cursor.execute("""
            SELECT * FROM readings 
            ORDER BY timestamp DESC 
            LIMIT 1
        """)
        
        result = cursor.fetchone()
        cursor.close()
        connection.close()
        
        if result:
            result['timestamp'] = result['timestamp'].isoformat()
            # Override with current server relay states
            result['relay1_state'] = relay_states['relay1']
            result['relay2_state'] = relay_states['relay2']
            return jsonify(result), 200
        else:
            return jsonify({'status': 'no data', 'message': 'No readings available'}), 404
            
    except Exception as e:
        print(f"❌ Error fetching latest data: {e}")
        return jsonify({'status': 'error', 'message': str(e)}), 500

@app.route('/api/history', methods=['GET'])
def get_history():
    """Get historical data for specified time period"""
    try:
        hours = int(request.args.get('hours', 24))
        
        connection = get_db_connection()
        if not connection:
            return jsonify({'status': 'error'}), 500
        
        cursor = connection.cursor(dictionary=True)
        
        query = """
            SELECT * FROM readings 
            WHERE timestamp >= NOW() - INTERVAL %s HOUR
            ORDER BY timestamp ASC
        """
        
        cursor.execute(query, (hours,))
        results = cursor.fetchall()
        
        cursor.close()
        connection.close()
        
        # Convert datetime to string
        for row in results:
            row['timestamp'] = row['timestamp'].isoformat()
        
        print(f"📈 Retrieved {len(results)} historical records")
        return jsonify(results), 200
        
    except Exception as e:
        print(f"❌ Error fetching history: {e}")
        return jsonify({'status': 'error', 'message': str(e)}), 500

@app.route('/api/stats', methods=['GET'])
def get_stats():
    """Get statistics for specified period"""
    try:
        period = request.args.get('period', 'day')
        
        connection = get_db_connection()
        if not connection:
            return jsonify({'status': 'error'}), 500
        
        cursor = connection.cursor(dictionary=True)
        
        intervals = {'day': 1, 'week': 7, 'month': 30}
        days = intervals.get(period, 1)
        
        query = """
            SELECT 
                COUNT(*) as total_readings,
                AVG(voltage) as avg_voltage,
                AVG(total_current) as avg_current,
                AVG(total_power) as avg_power,
                MAX(total_power) as max_power,
                MIN(total_power) as min_power
            FROM readings
            WHERE timestamp >= NOW() - INTERVAL %s DAY
        """
        
        cursor.execute(query, (days,))
        result = cursor.fetchone()
        
        # Calculate total energy (approximate)
        cursor.execute("""
            SELECT 
                SUM(total_power) * 10 / 3600000 as total_energy_kwh
            FROM readings
            WHERE timestamp >= NOW() - INTERVAL %s DAY
        """, (days,))
        
        energy_result = cursor.fetchone()
        if energy_result and energy_result['total_energy_kwh']:
            result['total_energy_kwh'] = energy_result['total_energy_kwh']
        else:
            result['total_energy_kwh'] = 0
        
        cursor.close()
        connection.close()
        
        return jsonify(result), 200
        
    except Exception as e:
        print(f"❌ Error fetching stats: {e}")
        return jsonify({'status': 'error', 'message': str(e)}), 500

# ===================== RELAY CONTROL ENDPOINTS =====================

@app.route('/api/relay/state', methods=['POST'])
def update_relay_state():
    """ESP32 posts relay state updates (from IR remote or web commands)"""
    global relay_states
    try:
        data = request.get_json()
        relay1 = data.get('relay1')
        relay2 = data.get('relay2')
        
        if relay1 is not None:
            relay_states['relay1'] = bool(relay1)
        if relay2 is not None:
            relay_states['relay2'] = bool(relay2)
        
        relay_states['last_updated'] = datetime.now().isoformat()
        
        print(f"🔌 Relay states updated: R1={relay_states['relay1']}, R2={relay_states['relay2']}")
        
        return jsonify({
            'status': 'success',
            'relay1': relay_states['relay1'],
            'relay2': relay_states['relay2'],
            'timestamp': relay_states['last_updated']
        }), 200
        
    except Exception as e:
        print(f"❌ Error updating relay state: {e}")
        return jsonify({'status': 'error', 'message': str(e)}), 500

@app.route('/api/relay/state', methods=['GET'])
def get_relay_state():
    """ESP32 gets current relay states from server"""
    return jsonify({
        'relay1': relay_states['relay1'],
        'relay2': relay_states['relay2'],
        'timestamp': relay_states['last_updated']
    }), 200

@app.route('/api/relay/control', methods=['POST'])
def control_relay():
    """Web dashboard sends relay control commands"""
    global relay_states
    try:
        data = request.get_json()
        relay = data.get('relay')
        state = data.get('state')
        
        if relay not in [1, 2]:
            return jsonify({'status': 'error', 'message': 'Invalid relay number'}), 400
        
        # Update server state
        relay_key = f'relay{relay}'
        relay_states[relay_key] = state
        relay_states['last_updated'] = datetime.now().isoformat()
        
        print(f"🌐 Web command: Relay {relay} → {'ON' if state else 'OFF'}")
        print(f"   Server states: R1={relay_states['relay1']}, R2={relay_states['relay2']}")
        
        return jsonify({
            'status': 'success',
            'relay': relay,
            'state': state,
            'message': 'Command stored'
        }), 200
        
    except Exception as e:
        print(f"❌ Error controlling relay: {e}")
        return jsonify({'status': 'error', 'message': str(e)}), 500

# ===================== MAIN =====================
if __name__ == '__main__':
    print("\n" + "="*60)
    print("⚡ SMART ENERGY METER - Flask Server")
    print("="*60 + "\n")
    
    # Initialize database
    init_database()
    
    print("\n📡 Server Configuration:")
    print(f"   • Database: {DB_CONFIG['database']}")
    print(f"   • Host: {DB_CONFIG['host']}")
    print(f"   • User: {DB_CONFIG['user']}")
    print(f"   • Server: http://0.0.0.0:5000")
    
    print("\n📋 Features Enabled:")
    print("   ✅ Sensor data logging (separate from relay state)")
    print("   ✅ Real-time dashboard")
    print("   ✅ Relay state synchronization")
    print("   ✅ IR Remote + Web control")
    print("   ✅ Historical data & statistics")
    
    print("\n🔌 Relay Control Flow:")
    print("   • ESP32 → Server: POST /api/relay/state (IR changes)")
    print("   • Server → ESP32: GET /api/relay/state (poll every 3s)")
    print("   • Web → Server: POST /api/relay/control")
    
    print("\n🌐 Access Dashboard:")
    print("   • Local:   http://localhost:5000")
    print("   • Network: http://192.168.31.222:5000")
    
    print("\n" + "="*60)
    print("✅ Server starting...\n")
    
    # Run Flask server
    app.run(host='0.0.0.0', port=5000, debug=True)