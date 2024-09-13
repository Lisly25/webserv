#!/usr/bin/env python3
import os
import sys
import re
import traceback

def create_upload_dir(directory):
    """Create the upload directory if it doesn't exist."""
    if not os.path.exists(directory):
        os.makedirs(directory)

def stream_to_file(filepath, initial_data, content_length):
    """Stream the file data to disk, handling large files by reading in chunks."""
    try:
        with open(filepath, 'wb') as output_file:
            output_file.write(initial_data)
            remaining = content_length - len(initial_data)
            while remaining > 0:
                chunk_size = min(4096, remaining)
                chunk = sys.stdin.buffer.read(chunk_size)
                if not chunk:
                    break
                output_file.write(chunk)
                remaining -= len(chunk)
    
    except IOError as e:
        print(f"Error writing file: {e}", file=sys.stderr)
        raise


def parse_headers(raw_data):
    """Extract the filename from the headers and determine where the file content starts."""
    header_end = raw_data.find(b"\r\n\r\n")
    if header_end != -1:
        headers = raw_data[:header_end].decode('utf-8', errors='replace')
        filename_match = re.search(r'filename="([^"]+)"', headers)
        if filename_match:
            filename = filename_match.group(1)
            return filename, header_end + 4
    return None, None


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
    exit(0)


def handle_upload():
    """Handle the file upload process, from reading input to saving the file."""
    upload_dir = os.path.join(os.getcwd(), os.environ.get('UPLOAD_FOLDER', 'uploads'))
    create_upload_dir(upload_dir)

    content_length = int(os.environ.get('CONTENT_LENGTH', 0))
    raw_headers = b""
    while b"\r\n\r\n" not in raw_headers:
        raw_headers += sys.stdin.buffer.read(1024)
    
    filename, file_content_start = parse_headers(raw_headers)
    
    if not filename:
        http_response(
            "400 Bad Request", 
            "text/html", 
            "<html><body>Error: Unable to extract the filename or find the end of headers.</body></html>"
        )
        exit(1)

    filename = os.path.basename(filename)
    file_path = os.path.join(upload_dir, filename)
    initial_data = raw_headers[file_content_start:]
    stream_to_file(file_path, initial_data, content_length - len(initial_data))
    http_response(
        "200 OK", 
        "text/html", 
        f"<html><body>File '{filename}' uploaded successfully to {upload_dir}.</body></html>",
        additional_headers={"X-Custom-Header": "FileUpload"}
    )
    exit(0)


def main():
    """Main function to handle exceptions and initiate the upload process."""
    try:
        handle_upload()
    except Exception:
        error_message = traceback.format_exc()
        http_response(
            "500 Internal Server Error", 
            "text/html", 
            f"<html><body>Error occurred:<pre>{error_message}</pre></body></html>"
        )
        exit(1)


if __name__ == "__main__":
    main()
