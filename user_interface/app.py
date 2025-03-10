from flask import Flask, render_template_string, request
import subprocess

app = Flask(__name__)

HTML_TEMPLATE = """
<!DOCTYPE html>
<html>
<head>
    <title>C Program Executor</title>
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; background-color: #f4f4f9; }
        h1 { color: #333; }
        button { padding: 10px 20px; background-color: #4CAF50; color: white; border: none; cursor: pointer; }
        button:hover { background-color: #45a049; }
        pre { background-color: #eee; padding: 10px; border: 1px solid #ccc; overflow: auto; }
    </style>
</head>
<body>
    <h1>C Program Executor</h1>
    <form method="POST">
        <button type="submit">Run C Program</button>
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
            # Compile the C program
            subprocess.run(["gcc", "test.c", "-o", "test"], check=True)
            
            # Execute the compiled C program and capture output
            result = subprocess.run(["./test"], text=True, capture_output=True)
            output = result.stdout
        except subprocess.CalledProcessError as e:
            output = f"Error: {e.stderr}"

    return render_template_string(HTML_TEMPLATE, output=output)

if __name__ == '__main__':
    app.run(debug=True, host='0.0.0.0', port=5000)
