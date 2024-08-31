#!/bin/bash

# Determine the directory containing this script
SCRIPT_DIR=$(dirname "$(readlink -f "$0")")
TESTS_DIR="$SCRIPT_DIR"

SERVER_URL="http://localhost:5252"

UPLOAD_DIR="$SCRIPT_DIR/../uploads"

upload_file() {
    local file="$1"
    local filename=$(basename "$file")
    echo "Uploading $filename..."
    curl -X POST -F "file=@$file" "$SERVER_URL/upload/"
}

delete_file() {
    local filename="$1"
    echo "Sending Delete for $filename..."
    curl -X DELETE "$SERVER_URL/delete/?$filename"
}

main ()
{
    # Upload the files
    find "$TESTS_DIR" -type f ! -name 'POST-EXAMPLES.tar.gz' | while read -r file; do
        upload_file "$file" &
    done

    wait 

    sleep 3

    read -p "Ready to remove the files posted? Press Enter to continue..."

    # Remove the files posted
    for file in "$UPLOAD_DIR"/*; do
        if [ -f "$file" ] && [ "$file" != "$UPLOAD_DIR/POST-EXAMPLES.tar.gz" ]; then
            filename=$(basename "$file")
            delete_file "$filename"
        fi
    done
}

main