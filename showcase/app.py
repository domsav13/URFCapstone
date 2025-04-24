from flask import Flask, render_template, request
import time

FIFO_PATH = '/tmp/arduino_cmd'

app = Flask(__name__)

# open FIFO once
while True:
    try:
        fifo = open(FIFO_PATH, 'w', buffering=1)
        break
    except OSError:
        time.sleep(0.1)

@app.route('/', methods=['GET','POST'])
def index():
    message=''
    pan  = request.form.get('pan','0')
    tilt = request.form.get('tilt','0')

    if request.method=='POST':
        cmd = request.form.get('cmd')
        if cmd:
            fifo.write(cmd+'\n')
            message = f"Sent: {cmd}"
        elif request.form.get('move'):
            fifo.write(f"({pan},{tilt})\n")
            message = f"Sent angles: ({pan},{tilt})"
    return render_template('index.html',
                           message=message,
                           pan=pan, tilt=tilt)

if __name__=='__main__':
    app.run(host='0.0.0.0',port=5000,debug=True)
