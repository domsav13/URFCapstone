from flask import Flask, render_template_string, request, jsonify
import subprocess
import signal
import time

app = Flask(__name__)

# Store process info
process = None

HTML_TEMPLATE = """
<!DOCTYPE html>
<html>
<head>
    <title>Stepper Motor Control</title>
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; background-color: #f4f4f9; }
        h1 { color: #333; }
        input[type="number"] { width: 100%; padding: 10px; margin-bottom: 10px; }
        button { 
            padding: 15px 30px;
            background-color: #4CAF50; 
            color: white; 
            border: none; 
            cursor: pointer; 
            font-size: 18px;
            transition: 0.3s;
        }
        button:active {
            background-color: #45a049;
            transform: scale(0.98);
        }
    </style>
    <script>
        let running = false;

        function startMotor() {
            if (!running) {
                let stepDelay = document.getElementById('step_delay').value;
                running = true;

                fetch('/start_motor', {
                    method: 'POST',
                    headers: {'Content-Type': 'application/json'},
                    body: JSON.stringify({'step_delay': stepDelay})
                });
            }
        }

        function stopMotor() {
            if (running) {
                running = false;
                fetch('/stop_motor', {method: 'POST'});
            }
        }
    </script>
</head>
<body>
    <h1>Stepper Motor Control</h1>
    
    <label>Step Delay (Microseconds):</label>
    <input type="number" id="step_delay" min="1" value="1000">

    <br><br>

    <button onmousedown="startMotor()" onmouseup="stopMotor()">Press and Hold to Move</button>

</body>
</html>
"""

@app.route('/')
def index():
    return render_template_string(HTML_TEMPLATE)

@app.route('/start_motor', methods=['POST'])
def start_motor():
    global process

    if process is None:
        data = request.get_json()
        step_delay = str(data.get('step_delay', '1000'))

        if int(step_delay) < 1:
            return jsonify({'status': 'Error: Step delay must be >= 1 microsecond'})

        # Start motor process
        process = subprocess.Popen(['./motor_button', step_delay])
        return jsonify({'status': f'Motor started with step delay {step_delay} microseconds.'})
    
    return jsonify({'status': 'Motor already running'})

@app.route('/stop_motor', methods=['POST'])
def stop_motor():
    global process

    if process:
        process.send_signal(signal.SIGTERM)
        process.wait()
        process = None
        return jsonify({'status': 'Motor stopped'})
    
    return jsonify({'status': 'Motor not running'})

if __name__ == '__main__':
    app.run(debug=True, host='0.0.0.0', port=5000)
