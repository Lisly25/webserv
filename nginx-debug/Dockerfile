FROM nginx:alpine

# Install necessary packages
RUN apk add --no-cache fcgiwrap bash

# Copy Nginx configuration and CGI script
COPY nginx.conf /etc/nginx/nginx.conf
COPY ./fun_facts /fun_facts
COPY ./ubuntu_cgi_tester /ubuntu_cgi_tester

# Ensure the CGI script is executable
RUN chmod +x /ubuntu_cgi_tester

# Expose the port
EXPOSE 8085

# Run fcgiwrap directly and ensure the socket has the correct permissions
CMD chmod 777 /var/run/fcgiwrap.socket && \
    /usr/bin/fcgiwrap -s unix:/var/run/fcgiwrap.socket & \
    nginx -g 'daemon off;'
