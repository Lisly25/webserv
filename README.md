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
+ Currently, the only accepted format for contexts is
	server {
		...
	}

	as opposed to

	server
	{
		...
	}

	This latter format will throw an error (but arguably shouldn't - will get back to this)

+ lines where the first non/whitespace character is '#' are treated as comments - inline comments are not (yet?) implemented


Proxy_pass passes the requests to other server off loads the work from our server and the server its passed to in this examples a docker containers have their own CGI to handle the code execution and generate the reponse for the user. 

