# WebServ &nbsp; &nbsp; <img src="assets/apache.png" alt="Apache" width="30"/> &nbsp;<img src="assets/caddy.png" alt="Caddy" width="100"/> &nbsp;<img src="assets/gunicorn.png" alt="Gunicorn" width="50"/> &nbsp;<img src="assets/litespeed.png" alt="LiteSpeed" width="60"/> &nbsp;<img src="assets/nginx.webp" alt="NGINX" width="60"/> &nbsp;<img src="assets/openresty.png" alt="OpenResty" width="100"/>

WebServ is a custom-built web server developed in a team of 3 as a project for the Hive curriculum. While inspired by the configuration syntax of NGINX, WebServ is a unique implementation designed to handle core HTTP functionalities such as request handling, CGI execution, file uploads, proxying, and more.

## Features

- **NGINX-inspired configuration syntax**: Uses a familiar structure while providing flexibility specific to WebServ.
- **Supported HTTP methods**: `GET`, `POST`, `DELETE`, `HEAD`.
- **CGI script execution**: Run server-side scripts like Python or PHP via the `cgi_pass` directive.
- **File uploads**: Handle file uploads with configurable directories.
- **Auto-indexing**: List files in a directory when no index file is present.
- **Proxying**: Forward requests to other services with the `proxy_pass` directive.
- **Custom error pages**: Serve custom HTML pages for specific error codes.
- **Request size limiting**: Limit the size of incoming request bodies.

## Example Configuration

NOTE: for a more detailed description of configuration file usage, look at [this document](https://github.com/Lisly25/webserv/blob/main/docs/How_to_make_configuration_file.md) in the repository.

```nginx
server {
    listen 5252;
    server_name localhost;

    error_page 404 ./errors/404.html;
    client_max_body_size 10M;

    location / {
        allowed_methods GET POST DELETE HEAD;
        root fun_facts;
        index index.html;
    }

    location /upload/ {
        allowed_methods POST;
        cgi_pass /cgi-scripts/upload_handler.py;
        upload_folder uploads;
    }

    location /list/ {
        allowed_methods GET;
        alias uploads;
        autoindex on;
    }

    location /delete/ {
        allowed_methods DELETE;
        cgi_pass /cgi-scripts/delete.py;
    }

    location /proxy-homer/ {
        allowed_methods GET POST;
        proxy_pass localhost:4141;
    }
}
```

## Key Directives
+ listen: Defines the port the server listens on.
+ error_page: Custom error pages for specific status codes.
+ client_max_body_size: Limits the size of request bodies.
+ location: Defines behavior for specific URL paths:
+ allowed_methods: Restricts allowed HTTP methods.
+ root or alias: Specifies the document root or alias for the location.
+ cgi_pass: Executes CGI scripts.
+ proxy_pass: Forwards requests to other servers.
+ return: HTTP redirection

## Getting Started
+ Clone the repository.
```bash
git clone git@github.com:Lisly25/webserv.git
```

+ Compile and run the server.
```bash
make && ./webserv tests/real-deal.conf
```

The configuration syntax was inspired by NGINX, but WebServ is an entirely custom server implementation with its own unique features and behavior :D
