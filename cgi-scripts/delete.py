#!/usr/bin/env python3

import os
import urllib.parse

uploads_dir = os.path.join(os.getcwd(), "uploads")

print("Content-Type: text/plain")
print()

if os.environ.get('REQUEST_METHOD') == 'DELETE':
    query_string = os.environ.get('QUERY_STRING', '')
    params = urllib.parse.parse_qs(query_string)
    file_name = params.get('filename', [None])[0]

    if file_name:
        file_name = urllib.parse.unquote(file_name)
        target_path = os.path.abspath(os.path.join(uploads_dir, file_name))

        if os.path.commonprefix([target_path, uploads_dir]) == uploads_dir:
            if os.path.exists(target_path):
                try:
                    os.remove(target_path)
                    print(f"File '{file_name}' deleted successfully.")
                except OSError as e:
                    print(f"Error: {e.strerror}. File '{file_name}' not deleted.")
            else:
                print(f"Error: File '{file_name}' does not exist.")
        else:
            print("Error: Unauthorized file path.")
    else:
        print("Error: File path not provided.")
else:
    print("Error: Only DELETE method is supported.")
