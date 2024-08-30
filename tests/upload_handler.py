#!/usr/bin/env python3
import os
import sys
import re
import traceback

def create_upload_dir(directory):
    if not os.path.exists(directory):
        os.makedirs(directory)


def read_raw_data(content_length):
    return sys.stdin.buffer.read(content_length)


def parse_headers(raw_data_str):
    filename_pattern = re.compile(r'filename="([^"]+)"')
    filename_match = filename_pattern.search(raw_data_str)
    if filename_match:
        filename = filename_match.group(1)
        header_end = raw_data_str.find("\r\n\r\n")
        if header_end != -1:
            return filename, header_end + 4
    return None, None


def extract_file_content(raw_data, file_content_start):
    return raw_data[file_content_start:]


def remove_trailing_boundary(raw_data_str, file_content, file_content_start):
    boundary_index = raw_data_str.find("--", file_content_start)
    if boundary_index != -1:
        return file_content[:boundary_index - file_content_start - 2]
    return file_content


def save_file(file_content, filepath):
    with open(filepath, 'wb') as output_file:
        output_file.write(file_content)


def main():
    try:
        print("Content-Type: text/html")
        print()

        upload_dir = os.path.join(os.getcwd(), "uploads")
        create_upload_dir(upload_dir)

        content_length = int(os.environ.get('CONTENT_LENGTH', 0))
        raw_data = read_raw_data(content_length)

        raw_data_str = raw_data.decode('utf-8', errors='ignore')

        filename, file_content_start = parse_headers(raw_data_str)
        if filename and file_content_start:
            file_content = extract_file_content(raw_data, file_content_start)
            file_content = remove_trailing_boundary(raw_data_str, file_content, file_content_start)

            file_path = os.path.join(upload_dir, filename)
            save_file(file_content, file_path)

            print(f"File '{filename}' uploaded successfully to {upload_dir}.")
        else:
            print("Error: Unable to extract the filename or find the end of headers.", file=sys.stderr)

    except Exception:
        error_message = traceback.format_exc()
        print("Error occurred:\n", error_message, file=sys.stderr)

if __name__ == "__main__":
    main()
