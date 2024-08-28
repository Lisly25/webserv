import os
import cgi
import urllib.parse

print("Content-Type: text/plain")
print()

if os.environ.get('REQUEST_METHOD') == 'DELETE':
    query_string = os.environ.get('QUERY_STRING', '')

    print(f"Requested file path: {query_string}")

    if query_string:
        if os.path.exists(query_string):
            try:
                os.remove(query_string)
                print(f"File '{query_string}' deleted successfully.")
            except OSError as e:
                print(f"Error: {e.strerror}. File '{query_string}' not deleted.")
        else:
            print(f"Error: File '{query_string}' does not exist.")
    else:
        print("Error: File path not provided.")
else:
    print("Error: Only DELETE method is supported.")

