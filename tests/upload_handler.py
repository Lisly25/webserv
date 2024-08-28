#!/usr/bin/env python3
import os
import re

print("Content-Type: text/html")
print()  # End of headers

# Directory to save uploaded files
upload_dir = os.getcwd() + "/uploads"
# create the dir if does not exist
if not os.path.exists(upload_dir):
    os.makedirs(upload_dir)
# Get the file content from the environment variable
raw_data = os.environ.get('REQUEST_BODY', '')

# Regular expression to find the filename
filename_pattern = re.compile(r'filename="([^"]+)"')

# Find the filename
filename_match = filename_pattern.search(raw_data)
if filename_match:
    filename = filename_match.group(1)
    # Remove headers and boundaries from the raw data
    file_content = re.split(r'\r\n\r\n', raw_data, 1)[1]  # Splitting to remove headers
    file_content = file_content.rsplit('\r\n', 2)[0]  # Removing the trailing boundary
    file_path = os.path.join(upload_dir, filename)


    # Write the actual file content to the specified directory
    with open(file_path, 'wb') as output_file:
        output_file.write(file_content.encode())

    print(f"File '{filename}' uploaded successfully to {upload_dir}.")
else:
    print("Error: Unable to extract the filename or file content.")

