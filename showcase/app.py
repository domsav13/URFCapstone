from flask import Flask, render_template, request
import os
import subprocess
import time

FIFO_PATH = '/tmp/arduino_cmd'
IMU_EXEC  = './read_bno055'

app = Flask(__name__)

# ——— Open the FIFO once, line-buffered ———
# Block until the daemon has opened the read-end
while True:
    try:
        fifo = open(FIFO_PATH, 'w', buffering=1)  # line-buffered
        break
    except OSError:
        time.sleep(0.1)

def get_imu_status():
    """
    Run read_bno055, skip to the '---' header, then parse the next line.
    """
    try:
        p = subprocess.Popen([IMU_EXEC],
                             stdout=subprocess.PIPE,
                             stderr=subprocess.DEVNULL,
                             text=True)
        for line in p.stdout:
            if line.startswith('---'):
                data = next(p.stdout).split()
                if len(data) >= 3:
                    yaw, roll, pitch = map(float, data[:3])
                    p.kill()
                    return yaw, roll, pitch
        p.kill()
    except Exception:
        pass
    return None, None, None

@app.route('/', methods=['GET', 'POST'])
def index():
    message = None
    pan  = request.form.get('pan', '')
    tilt = request.form.get('tilt', '')

    if request.method == 'POST' and 'move' in request.form:
        cmd = f"({pan},{tilt})\n"
        try:
            fifo.write(cmd)   # newline triggers flush
            message = f"Command sent: {cmd.strip()}"
        except Exception as e:
            message = f"Error sending command: {e}"

    elif request.method == 'GET' and request.args.get('imu'):
        yaw, roll, pitch = get_imu_status()
        if yaw is not None:
            message = f"Yaw: {yaw:.2f}°, Roll: {roll:.2f}°, Pitch: {pitch:.2f}°"
        else:
            message = "Error reading IMU"

    return render_template('index.html', message=message, pan=pan, tilt=tilt)

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000, debug=True)
