# syntax=docker/dockerfile:1

ARG ALPINE_VERSION=3.22

FROM alpine:${ALPINE_VERSION} AS build

RUN apk add --no-cache build-base file curl-dev libstdc++ pkgconf qt6-qtbase-dev

WORKDIR /src

COPY liberis/liberis.h /src/liberis/include/
COPY liberis/liberis.c /src/liberis/src/
RUN gcc -Wall -Wextra -Os -lcurl \
  -I/src/liberis/include \
  -c /src/liberis/src/liberis.c \
  -o /src/liberis.o \
  && ar rcs /src/liberis.a /src/liberis.o

COPY *.cpp *.h *.qss Makefile .
RUN make

FROM alpine:${ALPINE_VERSION}

RUN apk add --no-cache libcurl libstdc++ qt6-qtbase-x11 fontconfig ttf-dejavu

COPY --from=build /src/graphic-api-test /graphic-api-test

CMD ["/graphic-api-test"]
