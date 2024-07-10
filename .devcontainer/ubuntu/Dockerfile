# syntax=docker/dockerfile:1

#
# dev-container Ubuntu LTS

ARG DEBIAN_FRONTEND=noninteractive

FROM scratch AS dl-task
ADD --chmod=444 http://github.com/go-task/task/releases/latest/download/task_linux_amd64.deb .

FROM mcr.microsoft.com/devcontainers/base:noble
ARG DEBIAN_FRONTEND
RUN --network=none --mount=type=bind,from=dl-task,target=/mnt/download dpkg --install /mnt/download/task_linux_amd64.deb
RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt,sharing=locked <<-EOF
#!/bin/bash -e

apt-get -qq update -y
apt-get -qq upgrade -y --no-install-recommends
apt-get -qq install -y --no-install-recommends file cmake libsqlite3-dev libasound2-dev asterisk-dev
EOF

