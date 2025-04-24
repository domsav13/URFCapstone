# app.py
from flask import Flask, render_template, request
import os
import subprocess

FIFO_PATH = '/tmp/arduino_cmd'
IMU_EXEC = './read_bno055'  # Ensure read_bno055 binary is in the same directory or provide full path

app = Flask(__name__)

def get_imu_status():
    """
    Run the read_bno055 executable, parse the first Euler line, and return (yaw, roll, pitch).
    """
    try:
        p = subprocess.Popen([IMU_EXEC], stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        yaw = roll = pitch = None
        # Skip until the dashed header, then read next data line
        for line in p.stdout:
            if line.startswith('---'):
                data_line = next(p.stdout)
                parts = data_line.split()
                if len(parts) >= 3:
                    yaw = float(parts[0])
                    roll = float(parts[1])
                    pitch = float(parts[2])
                break
        p.kill()
        return yaw, roll, pitch
    except Exception:
        return None, None, None

@app.route('/', methods=['GET', 'POST'])
def index():
    message = None
    pan = request.form.get('pan', '')
    tilt = request.form.get('tilt', '')

    if request.method == 'POST' and 'move' in request.form:
        # Send pan/tilt command
        cmd = f"({pan},{tilt})"
        try:
            with open(FIFO_PATH, 'w') as fifo:
                fifo.write(cmd + '\n')
            message = f"Command sent: {cmd}"
        except Exception as e:
            message = f"Error sending command: {e}"

    elif request.method == 'GET' and request.args.get('imu'):
        # Fetch one IMU reading
        yaw, roll, pitch = get_imu_status()
        if yaw is not None:
            message = f"Yaw: {yaw:.2f}°, Roll: {roll:.2f}°, Pitch: {pitch:.2f}°"
        else:
            message = "Error reading IMU"

    return render_template('index.html', message=message, pan=pan, tilt=tilt)

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000, debug=True)
