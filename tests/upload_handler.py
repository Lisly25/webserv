#!/usr/bin/env python3
import cgi
import os

print("Content-Type: text/html")
print()  # End of headers

# Directory to save uploaded files
upload_dir = "/home/uahmed/HIVE/webserv/uploads"

# Create an instance of FieldStorage to parse the form data
file_content = os.environ.get('REQUEST_BODY', '')

# Check if a file is uploaded
if file_content:
    filename = 'uploaded_file.txt'
    file_path = os.path.join(upload_dir, filename)
    # Write the file to the specified directory
    with open(file_path, 'wb') as output_file:
        output_file.write(file_content.encode())
    print(f"File '{filename}' uploaded successfully to {upload_dir}.")
else:
    print("Error: No file part in the request.")

