from flask import Flask, request, jsonify, render_template
from flask_pymongo import PyMongo
from flask_cors import CORS
from datetime import datetime
from flask_socketio import SocketIO, emit
import json
from bson import ObjectId

app = Flask(__name__)
CORS(app)
socketio = SocketIO(app)

# Configuración de la conexión a MongoDB
app.config["MONGO_URI"] = "mongodb://localhost:27017/temperature_humidity_db"
mongo = PyMongo(app)
db = mongo.db

class JSONEncoder(json.JSONEncoder):
    def default(self, o):
        if isinstance(o, ObjectId):
            return str(o)
        if isinstance(o, datetime):
            return o.isoformat()
        return super(JSONEncoder, self).default(o)

app.json_encoder = JSONEncoder

@app.route('/endpoint', methods=['POST'])
def receive_data():
    data = request.json
    temperature = data.get('temperature')
    humidity = data.get('humidity')

    if temperature is not None and humidity is not None:
        entry = {
            'temperature': temperature,
            'humidity': humidity,
            'timestamp': datetime.utcnow()
        }
        db.sensor_data.insert_one(entry)
        entry['_id'] = str(entry['_id'])  # Convertir ObjectId a cadena
        entry['timestamp'] = entry['timestamp'].isoformat()  # Convertir datetime a cadena ISO 8601
        socketio.emit('new_data', entry)  # Emitir datos nuevos a todos los clientes
        return jsonify({'message': 'Data received successfully!'}), 200
    else:
        return jsonify({'message': 'Invalid data!'}), 400

@app.route('/data', methods=['GET'])
def get_data():
    data = list(db.sensor_data.find().sort('timestamp', -1).limit(100))
    for entry in data:
        entry['_id'] = str(entry['_id'])  # Convertir ObjectId a cadena
        entry['timestamp'] = entry['timestamp'].isoformat()  # Convertir datetime a cadena ISO 8601
    return jsonify(data), 200

@app.route('/')
def index():
    return render_template('index.html')

if __name__ == '__main__':
    socketio.run(app, debug=True, host='0.0.0.0', port=5000)
