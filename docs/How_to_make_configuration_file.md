# How to make a configuration file

## The main components of the format

The configuration file is made up of 'contexts' and 'directives'. Contexts are configurable units, and directives the configurable attributes. The main contexts are servers, which can contain within themselves a number of 'location' contexts.

In other words, a configuration file can contain the configurations of multiple servers, which the program will try to host simultaneously. In a standard case, a 'location' will be a directory on the server, which can contain for example .html pages that a client can visit.

When defining a location context, the hostname of the website is omitted (since it will already have been defined in a server_name directive within the server context)

```
server {	# Configuration for example_website1.com
	server_name example_website1.com
	...
	location /dir1/ {		# Settings will take effect when visting example_website1.com/dir1/page.html
		...
	}

	location /dir2/ {		# Settings will take effect when visting example_website1.com/dir2/page.html
		...
	}
}

server {	# Configuration for example_website2.com
	server_name example_website2.com
	...
	location /dir1/ {
		...
	}

	location /dir2/ {
		...
	}
}
```

NOTE: Currently, the program will determine the cwd while running, and will use this as a prefix to paths defined in the config file.
All directives under location context should be written with this in mind

## General formatting rules

+ the file extension must be .conf
+ directives must be followed by ';'
+ unclosed braces '{}' are not allowed
+ Currently, the only accepted format for server contexts is

  	```
	server {
		...
	}
	```

	as opposed to for example

	```
	server
	{
		...
	}
	```

	This latter format will cause an error.

+ location context must also be formatted in a similar way, the only difference being that the location keyword must be followed by a URI (this string must start with a '/' symbol since we don't handle regular expressions here). Example of correct format:

	```
	location /some/URI {
		...
	}
	```

+ trailing whitespaces (after semicolons, braces, or on empty lines) are not allowed

+ Comments can be created by typing '#' -> can also be used inline, in which case, everything after it is ignored (there is no option to close it off)

## Server-context directives

These directives are defined within a server context, outside of location contexts. They apply to all locations within that server

### listen

Defines which port the server's listening on for requests. Must be a single number in the range 0-65535. Setting it to under 1024 will give a warning about requiring certain permissions.
Must be unique to that server - if the same port is defined for two servers, that will display an error about failing to bind a port.

```
	listen 4242;
```

### server_name

The hostname of the server / the domain name. Or multiple of these, in which case they must be separated by spaces. If the server is hosting 'example.com', this is where 'example.com' is specified.
For this to work, the server_name should also be specified in the /etc/hosts file on the server machine

```
	server_name website1.com website2com;
```

### client_max_body_size

Can be used to limit the body size of requests the client can send.

+ it is an optional field: if omitted, will be set to 1M

+ can be set to 0, which will remove any limit

+ to specify a distinct value, the unit of measurement must also be included: can either be K or M

```
	client_max_body_size 5M;
```

### error_page

If an error occurs, the program will respond with an accurate html error code, and a default error page (which also displays the error code).

However, with this directive, it is possible to make custom error pages for different error codes. For the codes that weren't overwritten, the original defaults will be used.

The codes must be valid http error codes (400-599). If the path is incorrect, no error will occur, but the server will roll back to using the default error pages.

```
	error_page 404 ./example-site/errors/404.html;
  error_page 500 ./example-site/errors/Internal_server_error.html;
```

The path to the page should be a single string, including at least one '/'

## Location-context directives

### allowed_methods

Mandatory directive for all location contexts. Currently supported methods: POST, GET, HEAD, DELETE. Must list at least one allowed method (capitalized). Trying to use a method within a location where it is not allowed will trigger an error

```
	allowed_methods GET POST;
```

### root

Mandatory to specify if there's no redirection.
It is used to define where on the server machine the files are located.

For example, a client is trying to visit www.example.com/daily_news/highscore.html

`example.com` is the server name. The location context that will handle this request might look like this:

```
	location /daily_news/ {
		root /websites/website1/pages/;
	}
```

In which case, the server will look in its /websites/website1/pages/daily_news/ directory for a highscore.html file.

In other words, 'root' is attached as a prefix to the location URI to get the full address of a location within a server's filesystem.

This directive must be a single string

### index

Optional. It can be used to set up how a request ending with '/' (a directory) is handled: files can be defined that will be displayed in such a case. The files will be checked in the order they are given, and the best match will be chosen

```
	index index_file1.html /docs/index_file2.html;
```

### autoindex

Can be used inside non-cgi (for what a cgi is, see redirections) location context: is set to off by default, and as such, is not mandatory. It is used to turn on or off directory listing. If turned on, it will take effect if no files defined by the 'index' directive can be found

```
  autoindex on;
```

### upload_folder

Currently, file uploads can only be implemented by using cgi scripts. If a directory is specfied here, on startup, the program will check if the directory exists, and exit with an error if it does not.

Moreover, the directory will be passed onto the child process executing cgi scripts as the value of the environment variable `UPLOAD_FOLDER`.

```
  upload_folder site_uploads;
```

## Redirections

Redirections are handled inside location contexts. Only one type of redirection is allowed in one location

### alias

Instead of using the default `root` + `location` to derive the files' location within the filesystem, this directive can be used to define a different path, which will be used as an absolute path. For example:

```
	location /first_place/ {
		root /website1/docs/;
		alias /website2/utils/redirection/;
		...
	}
```

The path used to search for files will be /website2/utils/redirection/
If this directory does not exist, and exception will be thrown

### proxy_pass

In this case, requests will be redirected to an entirely different server, with our server acting as a reverse proxy. This server can be defined as servername + port.

```
  proxy_pass localhost:4646;
```

### cgi_pass

This redirection is used for cgi scripts. Currently, it works by executing a specific script if the location is matched.

```
location /delete/ {
  allowed_methods DELETE HEAD;
  cgi_pass /cgi-scripts/delete.py;
}
```

### HTTP redirection

In this case, a different website's address, not necessarily running on our server, can be specified, where the client will be redirected with an HTTP 300-type rediection

```
  return https://catoftheday.com;
```
