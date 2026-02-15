FROM alpine:latest

RUN apk add --no-cache  nginx busybox-suid

COPY frontend/ /usr/share/nginx/html
COPY deploy/nginx.conf /etc/nginx/http.d/default.conf

COPY deploy/start.sh /start.sh
RUN chmod +x /start.sh

EXPOSE 80

CMD ["/start.sh"]
