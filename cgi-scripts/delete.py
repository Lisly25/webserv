#!/usr/bin/env python3

import os
import sys
import urllib.parse

uploads_dir = "/home/skorbai/webserv/gh_repo/uploads/"

def http_response(status_code, content_type, body, additional_headers=None):
    """Generate an HTTP response with the proper status line, headers, and body."""
    print(f"HTTP/1.1 {status_code}")
    headers = {
        "Content-Type": content_type,
        "Content-Length": str(len(body)),
        "Cache-Control": "no-cache, no-store, must-revalidate",
        "Connection": "close"
    }
    if additional_headers:
        headers.update(additional_headers)
    for key, value in headers.items():
        print(f"{key}: {value}")
    print()
    print(body)
    
    sys.stdout.flush()
    if status_code.startswith("2"):
        exit(0)
    else:
        exit(1)


def handle_delete_request():
    """Handle the DELETE request to remove a file."""
    query_string = os.environ.get('QUERY_STRING', '')
    params = urllib.parse.parse_qs(query_string)
    file_name = params.get('filename', [None])[0]

    if not file_name:
        http_response("400 Bad Request", "text/plain", "Error: File path not provided.")
    file_name = urllib.parse.unquote(file_name)
    target_path = os.path.abspath(os.path.join(uploads_dir, file_name))
    if not target_path.startswith(uploads_dir):
        http_response("403 Forbidden", "text/plain", "Error: Unauthorized file path.")
    if os.path.exists(target_path):
        try:
            os.remove(target_path)
            http_response("200 OK", "text/plain", f"File '{file_name}' deleted successfully.")
        except OSError as e:
            http_response("500 Internal Server Error", "text/plain", f"Error: {e.strerror}. File '{file_name}' not deleted.")
    else:
        http_response("404 Not Found", "text/plain", f"Error: File '{file_name}' does not exist.")


def main():
    """Main function to route DELETE requests and handle file deletion."""
    if os.environ.get('REQUEST_METHOD') == 'DELETE':
        handle_delete_request()
    else:
        http_response("405 Method Not Allowed", "text/plain", "Error: Only DELETE method is supported.")

if __name__ == "__main__":
    main()
