# syntax=docker/dockerfile:1

#
# Building chan_quectel on Debian 10 (Buster)
#

ARG DEBIAN_FRONTEND=noninteractive
ARG CC_TARGET=armhf
ARG CC_TOOLCHAIN=arm-linux-gnueabihf
ARG ARM_GNU_TOOLCHAIN_VER=13.2.rel1
ARG ASTVER=16.30.0

FROM scratch AS dl-task
ADD --chmod=444 http://github.com/go-task/task/releases/latest/download/task_linux_amd64.deb .

FROM debian:buster-slim AS task-base
ARG DEBIAN_FRONTEND
RUN --network=none mkdir -p /build/task.d
RUN --network=none --mount=type=bind,from=dl-task,target=/build/download dpkg --install /build/download/task_linux_amd64.deb
COPY --from=task --chown=nobody:nogroup Taskfile.dist.yaml /build/Taskfile.yaml
COPY --from=task deb /build/task.d/deb

FROM scratch AS dl-arm-gnu-toolchain
ARG ARM_GNU_TOOLCHAIN_VER
ADD --chmod=444 http://armkeil.blob.core.windows.net/developer/Files/downloads/gnu/${ARM_GNU_TOOLCHAIN_VER}/binrel/arm-gnu-toolchain-${ARM_GNU_TOOLCHAIN_VER}-x86_64-arm-none-linux-gnueabihf.tar.xz .
ADD --chmod=444 http://armkeil.blob.core.windows.net/developer/Files/downloads/gnu/${ARM_GNU_TOOLCHAIN_VER}/binrel/arm-gnu-toolchain-${ARM_GNU_TOOLCHAIN_VER}-x86_64-aarch64-none-linux-gnu.tar.xz .

FROM task-base AS arm-gnu-toolchain
ARG ARM_GNU_TOOLCHAIN_VER

RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt,sharing=locked \
    task -d /build/task.d/deb install-pkgs -- xz-utils
COPY --from=task arm-gnu-toolchain /build/task.d/arm-gnu-toolchain
RUN --network=none task -d /build/task.d/arm-gnu-toolchain create-directory

USER nobody:nogroup
RUN --network=none --mount=type=bind,from=dl-arm-gnu-toolchain,target=/build/download task -d /build/task.d/arm-gnu-toolchain prepare

FROM task-base AS base

RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt,sharing=locked \
    task -d /build/task.d/deb upgrade-and-install-pkgs -- file dpkg-dev ca-certificates jq libssl1.1
RUN --network=none task -d /build create-build-directory && \
    task -d /build/task.d/deb create-dotenv && \
    task -d /build chown-build-directory -- nobody:nogroup

FROM scratch as dl-cmake
ADD --chmod=444 https://api.github.com/repos/Kitware/CMake/releases/latest cmake-latest.json

FROM base as cmake
RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt,sharing=locked \
    task -d /build/task.d/deb install-pkgs -- build-essential libssl-dev curl
COPY --from=task cmake /build/task.d/cmake

USER nobody:nogroup
RUN --mount=type=bind,from=dl-cmake,target=/build/cache task -d /build/task.d/cmake download
RUN --network=none task -d /build/task.d/cmake bootstrap build install

FROM scratch AS dl-asterisk
ARG ASTVER
ADD --chmod=444 http://downloads.asterisk.org/pub/telephony/asterisk/old-releases/asterisk-${ASTVER}.tar.gz .

FROM base as asterisk
ARG ASTVER

RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt,sharing=locked \
    task -d /build/task.d/deb install-pkgs -- wget build-essential libsqlite3-dev libssl-dev libedit-dev uuid-dev libxml2-dev
COPY --from=task asterisk /build/task.d/asterisk

USER nobody:nogroup
RUN --network=none --mount=type=bind,from=dl-asterisk,target=/build/download \
    task -d /build/task.d/asterisk old-extract-sources
RUN --mount=type=cache,target=/build/cache,sharing=locked,uid=65534,gid=65534 task -d /build/task.d/asterisk old-configure-16
RUN --network=none task -d /build/task.d/asterisk build install-headers

FROM scratch AS dl-googletest
ADD --chmod=444 https://api.github.com/repos/google/googletest/releases/latest googletest-latest.json

FROM base AS googletest
RUN --mount=type=cache,target=/var/cache/yum,sharing=locked \
    task -d /build/task.d/deb install-pkgs -- build-essential curl
COPY --from=task googletest /build/task.d/googletest
COPY --from=cmake --link --chown=root:root /build/install/ /

USER nobody:nogroup
RUN --mount=type=bind,from=dl-googletest,target=/build/cache task -d /build/task.d/googletest download
RUN --network=none task -d /build/task.d/googletest configure build install

FROM scratch AS dl-ninja-build
ADD --chmod=444 https://api.github.com/repos/ninja-build/ninja/releases/latest ninja-build-latest.json

FROM base AS ninja-build
RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt,sharing=locked \
    task -d /build/task.d/deb install-pkgs -- build-essential curl
COPY --from=cmake --link --chown=root:root /build/install/ /
COPY --from=task ninja-build /build/task.d/ninja-build
COPY --from=googletest --link --chown=root:root /build/install/ /

USER nobody:nogroup
RUN --mount=type=bind,from=dl-ninja-build,target=/build/cache task -d /build/task.d/ninja-build download2
RUN --network=none task -d /build/task.d/ninja-build configure build install

FROM base as chan-quectel-base
RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt,sharing=locked \
    task -d /build/task.d/deb install-pkgs -- git lintian

COPY --from=task chan-quectel /build/task.d/chan-quectel
COPY --from=cmake --link --chown=root:root /build/install/ /
COPY --from=ninja-build --link --chown=root:root /build/install /

FROM chan-quectel-base AS chan-quectel
RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt,sharing=locked \
    task -d /build/task.d/deb install-pkgs -- build-essential libsqlite3-dev libasound2-dev
#COPY --from=asterisk --chown=root:root /build/install/ /
COPY --from=asterisk --chown=root:root /build/install/usr/include /build/asterisk-headers/

FROM chan-quectel-base AS chan-quectel-cc
ARG CC_TARGET
ARG CC_TOOLCHAIN

RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt,sharing=locked \
    task -d /build/task.d/deb install-pkgs-arch -- qemu-user-static libc6-dev-${CC_TARGET}-cross \
    gcc-${CC_TOOLCHAIN} g++-${CC_TOOLCHAIN} binutils-${CC_TOOLCHAIN} \
    libsqlite3-dev:${CC_TARGET} libasound2-dev:${CC_TARGET}

FROM --platform=linux/arm/v6 balenalib/rpi-debian:buster-build AS rpi-debian
ARG DEBIAN_FRONTEND
RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt,sharing=locked <<-EOF
#!/bin/bash -e

apt-get -qq update -y
apt-get -qq upgrade -y --no-install-recommends
apt-get -qq install -y --no-install-recommends libsqlite3-dev libasound2-dev
EOF

FROM rpi-debian AS rpi-debian-asterisk-dev
ARG DEBIAN_FRONTEND
RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt,sharing=locked <<-EOF
#!/bin/bash -e

apt-get -qq update -y
    apt-get -qq install -y --no-install-recommends asterisk-dev
EOF

FROM chan-quectel-base AS chan-quectel-rpi-base
ARG CC_TARGET
ARG CC_TOOLCHAIN

RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt,sharing=locked \
    task -d /build/task.d/deb install-pkgs-arch -- qemu-user-static \
    libsqlite3-dev:${CC_TARGET} libasound2-dev:${CC_TARGET}
COPY --from=task rpi /build/task.d/rpi
RUN --network=none --mount=type=bind,from=rpi-debian-asterisk-dev,target=/build/cache task -d /build/task.d/rpi asterisk-headers
RUN --network=none --mount=type=bind,from=rpi-debian,target=/build/cache task -d /build/task.d/rpi sysroot

FROM chan-quectel-rpi-base AS chan-quectel-rpi
ARG CC_TARGET
ARG CC_TOOLCHAIN

RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt,sharing=locked \
    task -d /build/task.d/deb install-pkgs -- gcc-${CC_TOOLCHAIN} g++-${CC_TOOLCHAIN} binutils-${CC_TOOLCHAIN}

RUN --network=none task -d /build/task.d/rpi fix-crt-startup-${CC_TOOLCHAIN}    

FROM chan-quectel-base AS chan-quectel-cc-ext
ARG CC_TARGET
ARG CC_TOOLCHAIN

RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt,sharing=locked \
    task -d /build/task.d/deb install-pkgs-arch -- qemu-user-static libc6-dev-${CC_TARGET}-cross libgcc-8-dev-${CC_TARGET}-cross \
    libsqlite3-dev:${CC_TARGET} libasound2-dev:${CC_TARGET}
COPY --from=task arm-gnu-toolchain /build/task.d/arm-gnu-toolchain    
COPY --from=arm-gnu-toolchain --link --chown=root:root /build/arm-gnu-toolchain/${CC_TOOLCHAIN} /build/arm-gnu-toolchain/${CC_TOOLCHAIN}

RUN --network=none task -d /build/task.d/arm-gnu-toolchain ext-cross-compiliing-${CC_TOOLCHAIN} fix-ext-crt-startup-${CC_TOOLCHAIN}

FROM chan-quectel-rpi-base AS chan-quectel-rpi-ext
ARG CC_TARGET
ARG CC_TOOLCHAIN
COPY --from=arm-gnu-toolchain --link --chown=root:root /build/arm-gnu-toolchain/${CC_TOOLCHAIN} /build/arm-gnu-toolchain/${CC_TOOLCHAIN}

RUN --network=none task -d /build/task.d/rpi fix-ext-crt-startup-${CC_TOOLCHAIN}
