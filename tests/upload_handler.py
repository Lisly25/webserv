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

def handle_upload():
    """Handle the file upload process, from reading input to saving the file."""
    upload_dir = os.path.join(os.getcwd(), "uploads")
    create_upload_dir(upload_dir)

    content_length = int(os.environ.get('CONTENT_LENGTH', 0))
    raw_headers = sys.stdin.buffer.read(4096)
    
    filename, file_content_start = parse_headers(raw_headers)
    if not filename:
        print("Error: Unable to extract the filename or find the end of headers.", file=sys.stderr)
        return

    file_path = os.path.join(upload_dir, filename)
    initial_data = raw_headers[file_content_start:]
    
    stream_to_file(file_path, initial_data, content_length)

    print(f"File '{filename}' uploaded successfully to {upload_dir}.")

def main():
    """Main function to handle exceptions and initiate the upload process."""
    try:
        print("Content-Type: text/html")
        print()
        handle_upload()
    except Exception:
        error_message = traceback.format_exc()
        print("Error occurred:\n", error_message, file=sys.stderr)

if __name__ == "__main__":
    main()
