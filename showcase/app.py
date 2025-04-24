from flask import Flask, render_template, request, Response, stream_with_context
import subprocess
import time

FIFO_PATH = '/tmp/arduino_cmd'
IMU_BIN   = './read_bno055'

app = Flask(__name__)

# Open the command FIFO once
while True:
    try:
        fifo = open(FIFO_PATH, 'w', buffering=1)
        break
    except OSError:
        time.sleep(0.1)

@app.route('/', methods=['GET','POST'])
def index():
    message = ''
    pan   = request.form.get('pan','0')
    tilt  = request.form.get('tilt','0')

    if request.method=='POST':
        if request.form.get('move'):
            fifo.write(f"({pan},{tilt})\n")
            message = f"Sent angles ({pan},{tilt})"
        elif request.form.get('cmd'):
            cmd = request.form['cmd']
            fifo.write(cmd + '\n')
            message = f"Sent: {cmd}"
    return render_template('index.html',
                           message=message,
                           pan=pan, tilt=tilt)

@app.route('/imu')
def imu():
    return render_template('imu.html')

@app.route('/imu_stream')
def imu_stream():
    def generate():
        p = subprocess.Popen([IMU_BIN],
                             stdout=subprocess.PIPE,
                             stderr=subprocess.DEVNULL,
                             text=True)
        for line in p.stdout:
            yield f"data:{line}\n\n"
    return Response(stream_with_context(generate()),
                    mimetype='text/event-stream')

if __name__=='__main__':
    app.run(host='0.0.0.0', port=5000, debug=True)
