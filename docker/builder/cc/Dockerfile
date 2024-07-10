# syntax=docker/dockerfile:1

#
# Basic building pipeline (CC)
#

ARG BASEIMG
ARG BRANCH=master
ARG PRESET

FROM scratch AS asterisk-headers

FROM ${BASEIMG} AS build-from-repo
ARG BRANCH
ARG ASTERISK_VERSION_NUM
ARG PRESET
ARG CC_TARGET
ARG CC_TOOLCHAIN
ARG CMAKE_VERBOSE

USER nobody:nogroup
RUN task -d /build/task.d/chan-quectel clone
RUN --network=none task -d /build/task.d/chan-quectel configure
RUN --network=none --mount=type=bind,from=asterisk-headers,target=/build/cache task -d /build/task.d/chan-quectel build test

FROM ${BASEIMG} AS build-from-local
ARG ASTERISK_VERSION_NUM
ARG PRESET
ARG CC_TARGET
ARG CC_TOOLCHAIN
ARG CMAKE_VERBOSE

COPY --from=chan-quectel-src --chown=nobody:nogroup / /build/chan-quectel
USER nobody:nogroup
RUN --network=none task -d /build/task.d/chan-quectel configure
RUN --network=none --mount=type=bind,from=asterisk-headers,target=/build/cache task -d /build/task.d/chan-quectel build test
