# syntax=docker/dockerfile:1

#
# Building chan_quectel on Ubuntu 24.04
#
ARG DEBIAN_FRONTEND=noninteractive
ARG CC_TARGET=amd64
ARG CC_TOOLCHAIN=arm-linux-gnueabihf
ARG ARM_GNU_TOOLCHAIN_VER=13.2.rel1

FROM scratch AS dl-task
ADD --chmod=444 http://github.com/go-task/task/releases/latest/download/task_linux_amd64.deb .

FROM ubuntu:24.04 AS task-base
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
    task -d /build/task.d/deb upgrade-and-install-pkgs -- file ca-certificates jq dpkg-dev libssl3
RUN --network=none task -d /build create-build-directory && \
    task -d /build/task.d/deb create-dotenv && \
    task -d /build chown-build-directory -- nobody:nogroup

FROM scratch AS dl-ninja-build
ADD --chmod=444 https://api.github.com/repos/ninja-build/ninja/releases/latest ninja-build-latest.json

FROM base AS ninja-build
RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt,sharing=locked \
    task -d /build/task.d/deb install-pkgs -- build-essential cmake curl libgtest-dev
COPY --from=task ninja-build /build/task.d/ninja-build

USER nobody:nogroup
RUN --mount=type=bind,from=dl-ninja-build,target=/build/cache task -d /build/task.d/ninja-build download2
RUN --network=none task -d /build/task.d/ninja-build configure build install

FROM base AS asterisk-dev
RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt,sharing=locked \
    task -d /build/task.d/deb install-pkgs -- asterisk-dev

FROM base AS asterisk-dev-cc
ARG CC_TARGET

COPY ubuntu.sources /etc/apt/sources.list.d/
COPY ubuntu-arm.sources /etc/apt/sources.list.d/arm.sources
RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt,sharing=locked \
    task -d /build/task.d/deb install-pkgs-arch -- asterisk-dev:${CC_TARGET}

FROM base AS chan-quectel-base
RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt,sharing=locked \
    task -d /build/task.d/deb install-pkgs -- git cmake lintian
COPY --from=ninja-build --link --chown=root:root /build/install /
COPY --from=task chan-quectel /build/task.d/chan-quectel
COPY --from=task ubuntu /build/task.d/ubuntu

FROM chan-quectel-base AS chan-quectel
RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt,sharing=locked \
    task -d /build/task.d/deb install-pkgs -- build-essential libsqlite3-dev libasound2-dev
RUN --network=none --mount=type=bind,from=asterisk-dev,target=/build/cache task -d /build/task.d/ubuntu asterisk-headers

FROM chan-quectel-base AS chan-quectel-cc
ARG CC_TARGET
ARG CC_TOOLCHAIN

COPY ubuntu.sources /etc/apt/sources.list.d/
COPY ubuntu-arm.sources /etc/apt/sources.list.d/arm.sources
COPY dpkg-buildflags-${CC_TARGET}.env /build/buildflags.env
RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt,sharing=locked \
    task -d /build/task.d/deb install-pkgs-arch -- qemu-user-static libc6-dev-${CC_TARGET}-cross \
    gcc-${CC_TOOLCHAIN} g++-${CC_TOOLCHAIN} binutils-${CC_TOOLCHAIN} \
    libsqlite3-dev:${CC_TARGET} libasound2-dev:${CC_TARGET}
RUN --network=none --mount=type=bind,from=asterisk-dev-cc,target=/build/cache task -d /build/task.d/ubuntu asterisk-headers

FROM chan-quectel-base AS chan-quectel-cc-ext
ARG CC_TARGET
ARG CC_TOOLCHAIN

COPY ubuntu.sources /etc/apt/sources.list.d/
COPY ubuntu-arm.sources /etc/apt/sources.list.d/arm.sources
COPY dpkg-buildflags-${CC_TARGET}.env /build/buildflags.env
RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt,sharing=locked \
    task -d /build/task.d/deb install-pkgs-arch -- qemu-user-static libc6-dev-${CC_TARGET}-cross libgcc-13-dev-${CC_TARGET}-cross \
    libsqlite3-dev:${CC_TARGET} libasound2-dev:${CC_TARGET}
RUN --network=none --mount=type=bind,from=asterisk-dev-cc,target=/build/cache task -d /build/task.d/ubuntu asterisk-headers
COPY --from=task arm-gnu-toolchain /build/task.d/arm-gnu-toolchain
COPY --from=arm-gnu-toolchain --link --chown=root:root /build/arm-gnu-toolchain/${CC_TOOLCHAIN} /build/arm-gnu-toolchain/${CC_TOOLCHAIN}

RUN --network=none task -d /build/task.d/arm-gnu-toolchain ext-cross-compiliing-${CC_TOOLCHAIN} fix-ext-crt-startup-${CC_TOOLCHAIN}
