FROM nginx:alpine

RUN apk add --no-cache fcgiwrap bash

COPY nginx.conf /etc/nginx/nginx.conf

COPY fun_facts /fun_facts
COPY ubuntu_cgi_tester /tests/ubuntu_cgi_tester

RUN chmod +x /tests/ubuntu_cgi_tester

EXPOSE 8080

CMD spawn-fcgi -s /var/run/fcgiwrap.socket /usr/bin/fcgiwrap && nginx -g 'daemon off;'
