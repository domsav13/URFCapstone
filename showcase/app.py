from flask import Flask, render_template, request
import subprocess, time

FIFO_PATH = '/tmp/arduino_cmd'
IMU_EXEC  = './read_bno055'

app = Flask(__name__)

# open FIFO once, line-buffered
while True:
    try:
        fifo = open(FIFO_PATH, 'w', buffering=1)
        break
    except OSError:
        time.sleep(0.1)

def get_imu():
    try:
        p = subprocess.Popen([IMU_EXEC],
                             stdout=subprocess.PIPE,
                             stderr=subprocess.DEVNULL,
                             text=True)
        for line in p.stdout:
            if line.startswith('---'):
                data = next(p.stdout).split()
                p.kill()
                return map(float, data[:3])
        p.kill()
    except:
        pass
    return None, None, None

@app.route('/', methods=['GET','POST'])
def index():
    msg = ''
    pan = request.form.get('pan','0')
    tilt= request.form.get('tilt','0')

    if request.method=='POST':
        # toggle commands
        cmd = request.form.get('cmd')
        if cmd in ('CW','CCW','UP','DOWN'):
            fifo.write(cmd + '\n')
            msg=f'Sent: {cmd}'
        # vector move
        elif 'move' in request.form:
            fifo.write(f'({pan},{tilt})\n')
            msg=f'Sent: ({pan}, {tilt})°'

    elif request.method=='GET' and request.args.get('imu'):
        y,r,p=get_imu()
        if y is not None:
            msg=f"Yaw:{y:.2f}° Roll:{r:.2f}° Pitch:{p:.2f}°"
        else:
            msg="IMU error"

    return render_template('index.html',
                           message=msg,
                           pan=pan, tilt=tilt)

if __name__=='__main__':
    app.run(host='0.0.0.0', port=5000, debug=True)
