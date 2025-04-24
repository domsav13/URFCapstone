# app.py
from flask import Flask, render_template, request
import os

FIFO_PATH = '/tmp/arduino_cmd'

app = Flask(__name__)

@app.route('/', methods=['GET', 'POST'])
def index():
    message = None
    pan = tilt = ''
    if request.method == 'POST':
        # Read form inputs
        pan = request.form.get('pan', '0')
        tilt = request.form.get('tilt', '0')
        # Format command as (pan,tilt)
        cmd = f"({pan},{tilt})"
        try:
            # Write to FIFO for motor_daemon.c to pick up
            with open(FIFO_PATH, 'w') as fifo:
                fifo.write(cmd + '\n')
            message = f"Command sent: {cmd}"
        except Exception as e:
            message = f"Error sending command: {e}"
    return render_template('index.html', message=message, pan=pan, tilt=tilt)

if __name__ == '__main__':
    # Listen on all interfaces on port 5000
    app.run(host='0.0.0.0', port=5000, debug=True)
