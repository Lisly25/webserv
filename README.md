# webserv




#### Info

``` bash 
# This Make command will launch a container in localhost and we can test the proxypassing to this container from our reverse proxy
make proxy-cgi-test

```

### Check the tests/proxy-cgi-test.conf 

+ Change the **server_name** to "localhost" if accessing trough host taht you dont have root or sudo priviledges therefore use virtualmachine to run the server so you can name your own domain to /etc/hosts and map it to localhost"
```bash
# In vm 
echo "mydomainname.com >> /etc/hosts"
```

#### Access the prot that rev proxy is listening map it to non standard over 1024

mydomain.com:4242



#### in browser use the fn + f12 and network tab to see if the requests go trough and with what HTTP status codes are the requests returning 200 = OK

### Configuration file setup

The configuration file is made up of 'contexts' and 'directives'. Contexts are configurable units, and directives the configurable attributes. The main contexts are servers, which can contain within themselves a number of 'location' contexts.

In other words, a configuration file can contain the configurations of multiple servers. In addition, each additional server can have a number of settings per 'location'. In a standard case, a 'location' will be a directory on the server, which can contain for example .html pages that a client can visit.

When defining a location context, the hostname of the website is omitted (since it will already have been defined in a server_name directive within the server context)

```
server {	# Configuration for example_website1.com
	server_name example_website.com
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

#### General formatting rules

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

	This latter format will throw an error.

+ location context must also be formatted in a similar way, the only difference being that the location keyword must be followed by a URI (this string must start with a '/' symbol since we don't handle regular expressions here). Example of correct format:

	```
	location /some/URI {
		...
	}
	```

+ trailing whitespaces (after semicolons, braces, or on empty lines) are not allowed

+ Comments can be created by typing '#' -> can also be used inline, in which case, everything after it is ignored (there is no option to close it off)

#### Server-context directives

These directives are defined within a server context, outside of location contexts. They apply regardless of location

##### listen

Defines which port the server's listening on for requests. Must be a single number in the range 0-65535. Setting it to under 1024 will give a warning about requiring certain permissions.

```
	listen 4242;
```

##### host

The IP-address of the server. Optional directive - the default value is 127.0.0.1 (the loopback address)

```
	host 13.13.13.1;
```

##### server_name

The hostname of the server / the domain name. Or multiple of these, in which case they must be separated by spaces. If the server is hosting 'example.com', this is where 'example.com' is specified.

```
	server_name website1.com website2com;
```

#### server_root

Used to define a default directory for the whole server: all other URIs will be relative to this (error pages, index pages, roots within locations, etc.) This must be an absolute path, and a single string. Only an optional argument: if not present, then the above listed URIs will be interpreted as absolute paths themselves.

If the directive is present, but has no value, it will be interpreted as having no server_root defined.

```
	server_root	/home/user/webpages/;
```

##### client_max_body_size

Can be used to limit the body size of requests the client can send.

+ it is an optional field: if omitted, will be set to 1M

+ can be set to 0, which will remove any limit

+ to specify a distinct value, the unit of measurement must also be included: can either be K or M

```
	client_max_body_size 5M;
```

##### error_page

Allows a pre-defined page to be displayed if a certain type of error occured. In the example below, if the client tried to GET a page that does not exist, the server won't just see the default 404 page of their browser, but an html page fetched from the server:

```
	error_page 404 example.com/errors/404.html;
```

Multiple codes can be defined, but only a single string at the end. However, it doesn't have to be a URL, it can also be a URI, but must include at least one '/'

Note: this might need to be subject to changes

#### Location-context directives

##### allowed_methods

Mandatory directive for all location contexts. Currently supported methods: POST, GET, DELETE. Must list at least one allowed method (capitalized). Trying to use a method within a location where it is not allowed will return error code 405

```
	allowed_methods GET POST;
```

##### root

Mandatory directive. NOTE: might remove this requirement if there's a redirection.
It is used to define where on the server are the files located.

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

##### index

Optional. It can be used to set up how a request ending with '/' (a directory) is handled: files can be defined that will be displayed in such a case. The last file can be a file with an absolute path. The files will be checked in the order they are given

```
	index index_file1.html index_file2.html /files/default_index.html;
```

##### autoindex

Can be used inside non-cgi (for what a cgi is, see redirections) location context: is set to off by default, and as such, is not mandatory. It is used to turn on or off directory listing. If turned on, it will take effect if no files defined by the 'index' directive can be found

#### Redirections

Redirections are handled inside location contexts. Only one type of redirection is allowed in one location

##### alias

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

##### proxy_pass

In this case, requests will be redirected to an entirely different server. This server can be defined as an IP-address + port.

##### cgi_pass

This redirection is used for cgi scripts

+ a location context may also contain redirections:
	
	* `alias` will replace `root` + `location` (results in redirection within the server)
	* `proxy_pass` will define a different server to which we are redirecting
	* `cgi_pass` is used to set the address of a cgi server
	
	A location context may contain only one of these (or none, in which case no redirection is performed)

	An exception will be thrown if the alias is not actually pointing to an existing directory in the filesystem
	(Or, if there is no redirection the `root` + `location` does not point to a real directory)

Proxy_pass passes the requests to other server off loads the work from our server and the server its passed to in this examples a docker containers have their own CGI to handle the code execution and generate the reponse for the user. 


