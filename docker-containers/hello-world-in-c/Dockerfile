# syntax=docker/dockerfile:1

ARG ALPINE_VERSION=3.22

FROM  alpine:${ALPINE_VERSION} AS build

RUN apk add --no-cache build-base file

WORKDIR /src

COPY hello-world.c Makefile .

RUN make

FROM alpine:${ALPINE_VERSION}

COPY --from=build /src/hello-world /hello-world

CMD ["/hello-world"]

