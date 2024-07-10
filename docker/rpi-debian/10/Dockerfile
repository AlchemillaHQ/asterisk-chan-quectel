# syntax=docker/dockerfile:1

#
# Building chan_quectel on Raspbian 10 (Buster)
#

ARG DEBIAN_FRONTEND=noninteractive

FROM scratch AS dl-task
ADD --chmod=444 http://github.com/go-task/task/releases/latest/download/task_linux_arm.deb .

FROM balenalib/rpi-debian:buster-build AS task-base
ARG DEBIAN_FRONTEND
RUN --network=none mkdir -p /build/task.d
RUN --network=none --mount=type=bind,from=dl-task,target=/build/download dpkg --install /build/download/task_linux_arm.deb
COPY --from=task --chown=nobody:nogroup Taskfile.dist.yaml /build/Taskfile.yaml
COPY --from=task deb /build/task.d/deb

FROM task-base AS base
RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt,sharing=locked -- \
    task -d /build/task.d/deb upgrade-and-install-pkgs -- file ca-certificates jq dpkg-dev libsqlite3-dev libssl1.1
RUN --network=none task -d /build create-build-directory && \
    task -d /build/task.d/deb create-dotenv && \
    task -d /build chown-build-directory -- nobody:nogroup

FROM scratch as dl-cmake
ADD --chmod=444 https://api.github.com/repos/Kitware/CMake/releases/latest cmake-latest.json

FROM base as cmake
RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt,sharing=locked \
    task -d /build/task.d/deb install-pkgs -- libssl-dev curl
COPY --from=task cmake /build/task.d/cmake    

USER nobody:nogroup
RUN --mount=type=bind,from=dl-cmake,target=/build/cache task -d /build/task.d/cmake download
RUN --network=none task -d /build/task.d/cmake bootstrap build install

FROM scratch AS dl-ninja-build
ADD --chmod=444 https://api.github.com/repos/ninja-build/ninja/releases/latest ninja-build-latest.json

FROM base AS ninja-build
RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt,sharing=locked \
    task -d /build/task.d/deb install-pkgs -- curl libgtest-dev
COPY --from=cmake --link --chown=root:root /build/install/ /
COPY --from=task ninja-build /build/task.d/ninja-build

USER nobody:nogroup
RUN --mount=type=bind,from=dl-ninja-build,target=/build/cache task -d /build/task.d/ninja-build download2
RUN --network=none task -d /build/task.d/ninja-build configure build install

FROM base as chan-quectel
RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt,sharing=locked \
    task -d /build/task.d/deb install-pkgs -- libasound2-dev asterisk-dev
COPY --from=cmake --link --chown=root:root /build/install/ /
COPY --from=ninja-build --link --chown=root:root /build/install /
