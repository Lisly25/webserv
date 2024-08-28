#!/usr/bin/env python3
import os
import sys

print("Content-Type: text/html")
print()  # End of headers

# Make sure the request method is POST
if os.environ.get('REQUEST_METHOD') == 'POST':
    # Read the content length from the environment
    print("content length: ", os.environ.get('CONTENT_LENGTH', 0))
    content_length = int(os.environ.get('CONTENT_LENGTH', 0))
    if content_length > 0:
        # Read the entire input from stdin
        body = sys.stdin.read(content_length)

        # Parse the file content from the request body (simple parsing)
        boundary = body.split('\r\n')[0]
        parts = body.split(boundary)
        for part in parts:
            if 'Content-Disposition:' in part and 'filename="' in part:
                headers, file_content = part.split('\r\n\r\n', 1)
                file_content = file_content.rsplit('\r\n', 1)[0]  # Remove the trailing boundary

                # Extract the filename from the headers
                filename_line = [line for line in headers.split('\r\n') if 'filename="' in line][0]
                filename = filename_line.split('filename="')[1].split('"')[0]

                # Save the file content to a file
                with open(f'/path/to/save/{filename}', 'wb') as f:
                    f.write(file_content.encode('utf-8'))
                
                print(f"<html><body>File '{filename}' uploaded successfully.</body></html>")
                break
    else:
        print("<html><body>No content to read.</body></html>")
else:
    print("<html><body>Error: Only POST method is supported.</body></html>")

