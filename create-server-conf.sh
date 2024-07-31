#!/bin/bash

generate_server_config() {
    read -p "Enter the server name (e.g., example.com): " servername
    read -p "Enter the port to listen on (use port number > 1024): " port

    locations=""

    while true; do
        read -p "Enter the location path (e.g., / or /proxy): " path
        read -p "Is this location for a proxy pass or CGI? (proxy/cgi): " type

        if [[ $type == "proxy" ]]; then
            proxy_pass="proxy_pass http://127.0.0.1:8080;"
        else
            proxy_pass="cgi_pass tests/ubuntu_cgi_tester;"
        fi

        locations+="    location $path {\n        $proxy_pass\n    }\n"

        read -p "Do you want to add another location? (yes/no): " add_more
        if [[ $add_more != "yes" ]]; then
            break
        fi
    done

    config="server {\n    # Listen on the specified port\n    listen $port;\n\n    # Change /etc/hosts for any servername you want just make it point to localhost\n    # something.something 127.0.0.1\n    server_name $servername;\n\n$locations}"

    echo -e "Generated Server Configuration:\n"
    echo -e "$config"

    read -p "Do you want to save this configuration to a file? (yes/no): " save_config
    if [[ $save_config == "yes" ]]; then
        filename="tests/server-script.conf"
        echo -e "$config" > "$filename"
        echo "Configuration saved to $filename"
    fi
}



generate_server_config

echo -e "\n MAP THE SERVER NAME YOU CHOSE IN THE etc/hosts to point to 127.0.0.1\n\n"

echo 'echo "servername 127.0.0.1 >> /etc/hosts"'
echo "\n\n"

