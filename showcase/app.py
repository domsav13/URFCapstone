from flask import Flask, render_template, request
import subprocess
import time

FIFO_PATH = '/tmp/arduino_cmd'
IMU_EXEC  = './read_bno055'

app = Flask(__name__)

# Open the FIFO once, line-buffered
while True:
    try:
        fifo = open(FIFO_PATH, 'w', buffering=1)
        break
    except OSError:
        time.sleep(0.1)

def get_imu_status():
    try:
        p = subprocess.Popen([IMU_EXEC],
                             stdout=subprocess.PIPE,
                             stderr=subprocess.DEVNULL,
                             text=True)
        for line in p.stdout:
            if line.startswith('---'):
                data = next(p.stdout).split()
                p.kill()
                if len(data) >= 3:
                    return map(float, data[:3])
                break
        p.kill()
    except Exception:
        pass
    return None, None, None

@app.route('/', methods=['GET', 'POST'])
def index():
    message = ''
    pan     = request.form.get('pan', '0')
    tilt    = request.form.get('tilt', '0')

    if request.method == 'POST':
        # Direction toggles
        cmd = request.form.get('cmd')
        if cmd in ('CW','CCW','UP','DOWN'):
            try:
                fifo.write(cmd + '\n')
                message = f"Sent: {cmd}"
            except Exception as e:
                message = f"Error: {e}"
        # Angle move
        elif request.form.get('move'):
            try:
                fifo.write(f"({pan},{tilt})\n")
                message = f"Sent angles: ({pan}, {tilt})°"
            except Exception as e:
                message = f"Error: {e}"

    elif request.method == 'GET' and request.args.get('imu'):
        yaw, roll, pitch = get_imu_status()
        if yaw is not None:
            message = f"Yaw: {yaw:.2f}°, Roll: {roll:.2f}°, Pitch: {pitch:.2f}°"
        else:
            message = "Error reading IMU"

    return render_template('index.html',
                           message=message,
                           pan=pan,
                           tilt=tilt)

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000, debug=True)
