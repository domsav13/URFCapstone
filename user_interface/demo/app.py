from flask import Flask, request, render_template_string, redirect, url_for, flash
import subprocess
import os
import logging

app = Flask(__name__)
app.secret_key = 'your_secret_key_here'  # Replace with a secure key

# Set up logging for debugging.
logging.basicConfig(level=logging.DEBUG)

HTML = '''
<!doctype html>
<html>
  <head>
    <title>Motor Controller</title>
    <style>
      body {
        font-family: Arial, sans-serif;
        margin: 20px;
        background-color: #f5f5f5;
      }
      h1, h2 {
        color: #333;
      }
      form {
        margin-bottom: 20px;
      }
      label {
        font-size: 20px;
      }
      input[type="number"] {
        font-size: 20px;
        padding: 10px;
        width: 250px;
      }
      input[type="submit"] {
        font-size: 20px;
        padding: 10px 20px;
        margin-right: 10px;
        cursor: pointer;
      }
      .button-group {
        margin-bottom: 20px;
      }
      .flash-messages {
        color: red;
        margin-bottom: 20px;
      }
    </style>
  </head>
  <body>
    <h1>Motor Controller Interface</h1>
    <div class="flash-messages">
      {% with messages = get_flashed_messages() %}
        {% if messages %}
          {% for message in messages %}
            <p>{{ message }}</p>
          {% endfor %}
        {% endif %}
      {% endwith %}
    </div>
    <h2>Set Angle</h2>
    <form method="post">
      <input type="hidden" name="action" value="angle">
      <label for="angle">Angle (0-360):</label>
      <input type="number" id="angle" name="angle" min="0" max="360" required>
      <input type="submit" value="Set Angle">
    </form>
    <h2>Continuous Rotation</h2>
    <div class="button-group">
      <form method="post" style="display:inline;">
        <input type="hidden" name="action" value="CW">
        <input type="submit" value="Toggle CW">
      </form>
      <form method="post" style="display:inline;">
        <input type="hidden" name="action" value="CCW">
        <input type="submit" value="Toggle CCW">
      </form>
    </div>
  </body>
</html>
'''

# Determine the absolute path of the current file's directory and build the path to the binary.
basedir = os.path.dirname(os.path.abspath(__file__))
MOTOR_CONTROLLER_BINARY = os.path.join(basedir, 'motor_controller')

@app.route('/', methods=['GET', 'POST'])
def index():
    if request.method == 'POST':
        action = request.form.get('action', '')
        command = ''
        if action == 'angle':
            try:
                angle = float(request.form.get('angle', ''))
                if angle < 0 or angle > 360:
                    flash("Angle must be between 0 and 360.")
                    return redirect(url_for('index'))
                command = f"{angle}"
            except ValueError:
                flash("Invalid angle input.")
                return redirect(url_for('index'))
        elif action in ['CW', 'CCW']:
            command = action
        else:
            flash("Unknown action.")
            return redirect(url_for('index'))
            
        try:
            logging.debug("Executing motor controller with command: %s", command)
            result = subprocess.run(
                [MOTOR_CONTROLLER_BINARY],
                input=f"{command}\n",
                text=True,
                capture_output=True,
                check=True,
                timeout=10  # Prevent hanging indefinitely
            )
            logging.debug("Subprocess stdout: %s", result.stdout)
            logging.debug("Subprocess stderr: %s", result.stderr)
            flash("C program output: " + result.stdout)
        except subprocess.CalledProcessError as e:
            logging.error("Subprocess error: %s", e.stderr)
            flash("Error executing C program: " + e.stderr)
        except Exception as e:
            logging.exception("Unexpected error:")
            flash("An unexpected error occurred: " + str(e))
            
        return redirect(url_for('index'))
    return render_template_string(HTML)

if __name__ == '__main__':
    app.run(debug=True, host='0.0.0.0')
