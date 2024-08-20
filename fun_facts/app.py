from flask import Flask, request, send_from_directory, jsonify, redirect, render_template_string
import os

app = Flask(__name__)
BASE_DIR = os.path.dirname(os.path.abspath(__file__))
UPLOAD_FOLDER = os.path.join(BASE_DIR, 'docs')
app.config['UPLOAD_FOLDER'] = UPLOAD_FOLDER

@app.route('/', methods=['GET']) #SAME AS location / { index index.html; }
def serve_index():
    return send_from_directory(BASE_DIR, 'index.html')

@app.route('/<path:filename>', methods=['GET']) 
def serve_static(filename):
    return send_from_directory(BASE_DIR, filename)

@app.route('/install-file', methods=['POST']) #SAME AS location /install-file { cgi_pass "path/to/install-file.py"; }
def upload_file():
    if 'user_fact' not in request.files:
        return "No file part", 400
    file = request.files['user_fact']
    if file.filename == '':
        return "No selected file", 400
    if file:
        file_path = os.path.join(app.config['UPLOAD_FOLDER'], file.filename)
        try:
            file.save(file_path)
            return redirect('/')
        except Exception as e:
            return f"An error occurred while saving the file: {str(e)}", 500

@app.route('/append-content', methods=['POST']) # SAME AS location /append-content { fastcgi_pass "path/to/append-content.py"; }
def append_content():
    content = request.form.get('append_content', '')
    if content:
        file_path = os.path.join(app.config['UPLOAD_FOLDER'], 'sample_fact.txt')
        try:
            with open(file_path, 'a') as file:
                file.write(content + '\n')
            return "Content appended successfully", 200
        except Exception as e:
            return f"An error occurred while appending content: {str(e)}", 500
    return "No content to append", 400

@app.route('/docs/<filename>', methods=['DELETE']) #SAME AS location /docs/<filename> { fastcgi_pass "path/to/delete-file.py"; }
def delete_file(filename):
    file_path = os.path.join(app.config['UPLOAD_FOLDER'], filename)
    if os.path.exists(file_path):
        try:
            os.remove(file_path)
            return jsonify({"message": "File deleted successfully"}), 200
        except Exception as e:
            return jsonify({"message": f"Error deleting file: {str(e)}"}), 500
    return jsonify({"message": "File not found"}), 404

@app.route('/docs/<path:filename>', methods=['GET'])
def serve_docs(filename):
    return send_from_directory(app.config['UPLOAD_FOLDER'], filename)

@app.route('/list-files', methods=['GET'])
def list_files():
    try:
        files = os.listdir(app.config['UPLOAD_FOLDER'])
        return jsonify(files)
    except Exception as e:
        return jsonify({"error": f"Error listing files: {str(e)}"}), 500

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=8085, debug=True)
