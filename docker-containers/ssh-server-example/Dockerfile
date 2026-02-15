FROM alpine:latest

RUN apk update                                                   \
 && apk add --no-cache openssh                                   \
 && ssh-keygen -A                                                \
 && echo "PermitRootLogin yes" >> /etc/ssh/sshd_config           \
 && echo "PasswordAuthentication yes" >> /etc/ssh/sshd_config    \
 && adduser -D eris                                              \
 && echo "root:root" | chpasswd                                  \
 && echo "eris:linux" | chpasswd

EXPOSE 22

CMD [ "/usr/sbin/sshd", "-D" ]

