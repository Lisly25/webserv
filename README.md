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

+ the file extension must be .conf
+ directives must be followed by ';'
+ unclosed braces '{}' are not allowed
+ Currently, the only accepted format for server contexts is
  	```
	server {
		...
	}

	as opposed to for example

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

+ port's ("listen" directive) value must be a number 0-65535. Setting it to under 1024 will give a warning

+ server_name field is optional. Several strings can be specified, which have to be separated by spaces

+ client_max_body_size can be set in kilobytes or megabytes (for example 1K or 5M). The field can be omitted, and then the default will be in effect, which is 1m. If it is set to "0" (not 0K or 0M though), body size will be unlimited (though we might want to limit the max max_body_size...? For now, I'll just limit it at LONG_MAX :D)

+ setting up default error pages is optional, but if done, must be done in the server context in the following format:

	```
	error_page <error_code1> <error_code2> <error_code_n> <error_page_address>
	```

	(whitespaces between the elements are not counted)

	The error page address can be a URI or a URL at the moment
	(thus, if the error page address does not contain at least one '/', an error will be thrown during the parsing)

+ inside a location context, the allowed_methods directive can be used to list the allowed methods (only POST, GET, and DELETE are implemented). Example in which DELETE is not allowed:

	```
	location / {
		allowed_methods GET POST
	}
	```

	Trying to use a disallowed method will return code 405. Not listing any method as allowed will throw an exception, as well as specifying a method that is not supported

+ all location contexts must also contain a root directive, no matter which type location it is. Its value must be a single string (it cannot contain whitespaces)

+ `autoindex` can be used inside non-cgi location context: is set to off by default, and as such, is not mandatory. It is used to turn on or off directory listing

+ a location context may also contain redirections:
	
	* `alias` will replace `root` + `location` (results in redirection within the server)
	* `proxy_pass` will define a different server to which we are redirecting
	* `cgi_pass` is used to set the address of a cgi server
	
	A location context may contain only one of these (or none, in which case no redirection is performed)

Proxy_pass passes the requests to other server off loads the work from our server and the server its passed to in this examples a docker containers have their own CGI to handle the code execution and generate the reponse for the user. 

