# syntax=docker/dockerfile:1

ARG ASTVER=18

FROM scratch AS dl-task
ADD --chmod=444 http://github.com/go-task/task/releases/latest/download/task_linux_amd64.rpm .

FROM centos:7.8.2003 as task-base
RUN --network=none mkdir -p /build/task.d
RUN yum-config-manager --save \
  --setopt=exit_on_lock=true \
  --setopt=install_weak_deps=false \
  --setopt=keepcache=true \
  --setopt=debuglevel=0 \
  --setopt=errorlevel=0 \
  --setopt=logfile=/dev/null
RUN --network=none \
  --mount=type=bind,from=dl-task,target=/build/download \
  --mount=type=cache,target=/var/cache/yum,sharing=locked \
  yum install -y -q /build/download/task_linux_amd64.rpm
COPY --from=task --chown=nobody:nobody Taskfile.dist.yaml /build/Taskfile.yaml
COPY --from=task yum /build/task.d/yum

FROM task-base as base

RUN --mount=type=cache,target=/var/cache/yum,sharing=locked \
  task -d /build/task.d/yum install-pkgs -- rpm-build centos-release-scl scl-utils scl-utils-build file openssl sqlite-devel
RUN --mount=type=cache,target=/var/cache/yum,sharing=locked task -d /build/task.d/yum install-pkgs -- devtoolset-11
ADD --chmod=755 http://github.com/jqlang/jq/releases/latest/download/jq-linux-amd64 /usr/bin/jq
RUN --network=none task -d /build create-build-directory && \
  task -d /build/task.d/yum create-dotenv && \
  task -d /build chown-build-directory -- nobody:nobody

FROM scratch as dl-cmake
ADD --chmod=444 https://api.github.com/repos/Kitware/CMake/releases/latest cmake-latest.json

FROM base as cmake
RUN --mount=type=cache,target=/var/cache/yum,sharing=locked \
  task -d /build/task.d/yum install-pkgs -- openssl-devel curl
COPY --from=task cmake /build/task.d/cmake  

USER nobody:nobody
RUN --mount=type=bind,from=dl-cmake,target=/build/cache task -d /build/task.d/cmake download
RUN --network=none scl enable devtoolset-11 'task -d /build/task.d/cmake bootstrap build install'

FROM scratch AS dl-googletest
ADD --chmod=444 https://api.github.com/repos/google/googletest/releases/latest googletest-latest.json

FROM base AS googletest
RUN --mount=type=cache,target=/var/cache/yum,sharing=locked \
  task -d /build/task.d/yum install-pkgs -- curl
COPY --from=task googletest /build/task.d/googletest
COPY --from=cmake --link --chown=root:root /build/install/ /

USER nobody:nobody
RUN --mount=type=bind,from=dl-googletest,target=/build/cache task -d /build/task.d/googletest download
RUN --network=none scl enable devtoolset-11 'task -d /build/task.d/googletest configure build install'

FROM scratch AS dl-ninja-build
ADD --chmod=444 https://api.github.com/repos/ninja-build/ninja/releases/latest ninja-build-latest.json

FROM base AS ninja-build
RUN --mount=type=cache,target=/var/cache/yum,sharing=locked \
  task -d /build/task.d/yum install-pkgs -- curl
COPY --from=task ninja-build /build/task.d/ninja-build  
COPY --from=cmake --link --chown=root:root /build/install/ /
COPY --from=googletest --link --chown=root:root /build/install/ /

USER nobody:nobody
RUN --mount=type=bind,from=dl-ninja-build,target=/build/cache task -d /build/task.d/ninja-build download2
RUN --network=none scl enable devtoolset-11 'task -d /build/task.d/ninja-build configure build install'

FROM scratch AS dl-asterisk
ARG ASTVER
ADD --chmod=444 http://downloads.asterisk.org/pub/telephony/asterisk/asterisk-${ASTVER}-current.tar.gz .

FROM base as asterisk
ARG ASTVER

RUN --mount=type=cache,target=/var/cache/yum,sharing=locked \
  task -d /build/task.d/yum install-pkgs -- \
  patch wget bzip2 \
  openssl-devel libedit-devel libuuid-devel libxml2-devel alsa-lib-devel
COPY --from=task asterisk /build/task.d/asterisk

USER nobody:nobody
RUN --network=none --mount=type=bind,from=dl-asterisk,target=/build/download task -d /build/task.d/asterisk extract-sources
RUN --mount=type=cache,target=/build/cache,sharing=locked,uid=99,gid=99 scl enable devtoolset-11 'task -d /build/task.d/asterisk configure-${ASTVER}'
RUN --network=none scl enable devtoolset-11 'task -d /build/task.d/asterisk build install-headers'

FROM base as chan-quectel
LABEL description="Building chan_quectel on CentOS 7.8 which is a base OS for FreePBX."

RUN --mount=type=cache,target=/var/cache/yum,sharing=locked \
  task -d /build/task.d/yum install-pkgs -- git alsa-lib-devel

COPY --from=task chan-quectel /build/task.d/chan-quectel
COPY --from=ninja-build --link --chown=root:root /build/install /
COPY --from=cmake --link --chown=root:root /build/install /
#COPY --from=asterisk --link --chown=root:root /build/install /
COPY --from=asterisk --chown=root:root /build/install/usr/include /build/asterisk-headers/
