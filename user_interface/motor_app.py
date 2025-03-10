from flask import Flask, render_template_string, request, jsonify
import subprocess
import os
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
        button { padding: 10px 20px; background-color: #4CAF50; color: white; border: none; cursor: pointer; }
        button:hover { background-color: #45a049; }
        pre { background-color: #eee; padding: 10px; border: 1px solid #ccc; overflow: auto; }
    </style>
</head>
<body>
    <h1>Stepper Motor Control</h1>
    <form method="POST">
        <label>Angle (Degrees):</label>
        <input type="number" step="0.1" name="angle" required><br>

        <label>Step Delay (Microseconds):</label>
        <input type="number" min="1" name="step_delay" required><br>

        <button type="submit">Rotate</button>
    </form>
    {% if output %}
        <h2>Output:</h2>
        <pre>{{ output }}</pre>
    {% endif %}
</body>
</html>
"""

@app.route('/', methods=['GET', 'POST'])
def index():
    output = None
    if request.method == 'POST':
        try:
            angle = request.form['angle']
            step_delay = request.form['step_delay']

            if int(step_delay) < 1:
                return jsonify({'status': 'Error: Step delay must be >= 1 microsecond'})

            # Run C program with angle and step delay
            result = subprocess.run(
                ['./motor_control', angle, step_delay], 
                text=True, capture_output=True, check=True
            )
            output = result.stdout
        except subprocess.CalledProcessError as e:
            output = f"Error: {e.stderr}"
        except Exception as e:
            output = f"Error: {str(e)}"
    
    return render_template_string(HTML_TEMPLATE, output=output)

if __name__ == '__main__':
    app.run(debug=True, host='0.0.0.0', port=5000)
