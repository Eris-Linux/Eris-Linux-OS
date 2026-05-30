# syntax=docker/dockerfile:1

ARG ALPINE_VERSION=3.22

FROM --platform=$TARGETPLATFORM alpine:${ALPINE_VERSION} AS build

RUN apk add --no-cache build-base file

WORKDIR /src
COPY hello-world.c .

RUN gcc -Wall -Wextra -Os -o /hello-world hello-world.c \
 && strip /hello-world \
 && file /hello-world

FROM --platform=$TARGETPLATFORM alpine:${ALPINE_VERSION}

COPY --from=build /hello-world /hello-world

CMD ["/hello-world"]

