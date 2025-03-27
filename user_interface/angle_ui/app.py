from flask import Flask, request, render_template_string, redirect, url_for, flash
import subprocess

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

@app.route('/', methods=['GET', 'POST'])
def index():
    if request.method == 'POST':
        try:
            # Retrieve and validate the angle from the form
            angle = float(request.form['angle'])
            if angle < 0 or angle > 360:
                flash("Angle must be between 0 and 360.")
                return redirect(url_for('index'))
        except ValueError:
            flash("Invalid input.")
            return redirect(url_for('index'))

        # Execute the C program by providing the angle as input
        try:
            # Assuming the compiled C binary is named "motor_controller" and is in the current directory.
            process = subprocess.run(
                ["./motor_controller"],
                input=f"{angle}\n",
                text=True,
                capture_output=True,
                check=True
            )
            # Flash the output from the C program (stdout)
            flash("C program output: " + process.stdout)
        except subprocess.CalledProcessError as e:
            flash("Error executing C program: " + e.stderr)
        except Exception as e:
            flash("An unexpected error occurred: " + str(e))

        return redirect(url_for('index'))

    return render_template_string(HTML)

if __name__ == '__main__':
    app.run(debug=True, host='0.0.0.0')
