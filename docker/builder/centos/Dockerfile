# syntax=docker/dockerfile:1

#
# Basic building pipeline
#

ARG BASEIMG
ARG BRANCH=master
ARG PRESET=rpm

FROM scratch AS asterisk-headers

FROM ${BASEIMG} AS build-from-repo
ARG BRANCH
ARG ASTERISK_VERSION_NUM
ARG PRESET
ARG CMAKE_VERBOSE

USER nobody:nobody
RUN task -d /build/task.d/chan-quectel clone
RUN --network=none scl enable devtoolset-11 'task -d /build/task.d/chan-quectel configure'
RUN --network=none --mount=type=bind,from=asterisk-headers,target=/build/cache scl enable devtoolset-11 'task -d /build/task.d/chan-quectel build test'

FROM ${BASEIMG} AS build-from-local
ARG ASTERISK_VERSION_NUM
ARG PRESET
ARG CMAKE_VERBOSE

COPY --from=chan-quectel-src --chown=nobody:nobody / /build/chan-quectel
USER nobody:nobody
RUN --network=none scl enable devtoolset-11 'task -d /build/task.d/chan-quectel configure'
RUN --network=none --mount=type=bind,from=asterisk-headers,target=/build/cache scl enable devtoolset-11 'task -d /build/task.d/chan-quectel build test'