

- [ ] Add Signal handlers for SIGKILL and SIGINT clean free and exit
        And check for what signals to ignore and so on

- [ ] Add exceptions to all methods and put hte in the WebErrors namespace so they are isolated there



- [ ] Build locations structures to the WebParser where we hold the data about each location and their-
        passes and the data to that location and make use of it in the server code

- [ ] Check in the WebServer handclient that if the request is to a cgi then fork and execute the cgi binary or can we just proxy pass it a other server offload the work there.