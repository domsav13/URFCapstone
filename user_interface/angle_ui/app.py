from flask import Flask, request, render_template_string, redirect, url_for, flash
import subprocess
import os

app = Flask(__name__)
app.secret_key = 'your_secret_key_here'  # Replace with a secure key

# HTML template for the UI
HTML = '''
<!doctype html>
<html>
  <head>
    <title>Motor Controller</title>
  </head>
  <body>
    <h1>Set Motor Angle</h1>
    {% with messages = get_flashed_messages() %}
      {% if messages %}
        <ul style="color: red;">
          {% for message in messages %}
            <li>{{ message }}</li>
          {% endfor %}
        </ul>
      {% endif %}
    {% endwith %}
    <form method="post">
      <label for="angle">Angle (0-360):</label>
      <input type="number" id="angle" name="angle" min="0" max="360" required>
      <input type="submit" value="Submit">
    </form>
  </body>
</html>
'''

# Use an absolute path for the binary
MOTOR_CONTROLLER_BINARY = "/home/pi/Desktop/URFCapstone/user_interface/angle_ui/motor_controller"  # adjust as necessary

@app.route('/', methods=['GET', 'POST'])
def index():
    if request.method == 'POST':
        try:
            angle = float(request.form['angle'])
            if angle < 0 or angle > 360:
                flash("Angle must be between 0 and 360.")
                return redirect(url_for('index'))
        except ValueError:
            flash("Invalid input.")
            return redirect(url_for('index'))

        # Execute the C program using the absolute path
        try:
            result = subprocess.run(
                [MOTOR_CONTROLLER_BINARY],
                input=f"{angle}\n",
                text=True,
                capture_output=True,
                check=True,
                timeout=10  # Optional: ensure the process doesn't hang indefinitely
            )
            flash("C program output: " + result.stdout)
        except subprocess.CalledProcessError as e:
            flash("Error executing C program: " + e.stderr)
        except Exception as e:
            flash("An unexpected error occurred: " + str(e))

        return redirect(url_for('index'))

    return render_template_string(HTML)

if __name__ == '__main__':
    app.run(debug=True, host='0.0.0.0')
