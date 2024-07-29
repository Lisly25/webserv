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




