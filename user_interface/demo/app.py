from flask import Flask, request, render_template_string, redirect, url_for, flash
import os

app = Flask(__name__)
app.secret_key = 'your_secret_key_here'  # Replace with a secure key

FIFO_PATH = "/tmp/arduino_cmd"

HTML = '''
<!doctype html>
<html lang="en">
  <head>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>Motor Controller</title>
    <style>
      body {
        font-family: 'Helvetica Neue', Helvetica, Arial, sans-serif;
        margin: 0;
        padding: 0;
        background-color: #f8f9fa;
      }
      .container {
        max-width: 600px;
        margin: 20px auto;
        padding: 20px;
        background: #ffffff;
        border-radius: 8px;
        box-shadow: 0 2px 4px rgba(0,0,0,0.1);
      }
      h1, h2 {
        text-align: center;
        color: #343a40;
      }
      form {
        margin: 15px 0;
      }
      label {
        font-size: 16px;
        margin-bottom: 5px;
        display: block;
      }
      input[type="number"] {
        width: 100%;
        padding: 12px;
        margin-bottom: 10px;
        border: 1px solid #ced4da;
        border-radius: 4px;
        font-size: 16px;
      }
      input[type="submit"] {
        width: 100%;
        padding: 12px;
        background-color: #007bff;
        border: none;
        color: #fff;
        font-size: 16px;
        border-radius: 4px;
        margin-bottom: 10px;
      }
      input[type="submit"]:hover {
        background-color: #0056b3;
      }
      .flash-messages {
        color: #dc3545;
        text-align: center;
        margin-bottom: 10px;
      }
      @media (min-width: 600px) {
        input[type="submit"] {
          width: auto;
          display: inline-block;
          margin: 5px;
        }
      }
    </style>
  </head>
  <body>
    <div class="container">
      <h1>Motor Controller</h1>
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
      <form method="post">
        <input type="hidden" name="action" value="CW">
        <input type="submit" value="Toggle CW">
      </form>
      <form method="post">
        <input type="hidden" name="action" value="CCW">
        <input type="submit" value="Toggle CCW">
      </form>
    </div>
  </body>
</html>
'''

def send_command_fifo(command):
    """Write a command to the FIFO so the daemon can send it to the Arduino."""
    try:
        with open(FIFO_PATH, "w") as fifo:
            fifo.write(command + "\n")
    except Exception as e:
        raise Exception(f"Error writing to FIFO: {e}")

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
            send_command_fifo(command)
            flash("Command sent: " + command)
        except Exception as e:
            flash(str(e))
            
        return redirect(url_for('index'))
    return render_template_string(HTML)

if __name__ == '__main__':
    app.run(debug=True, host='0.0.0.0')
