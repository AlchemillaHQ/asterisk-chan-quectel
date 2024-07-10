#!/bin/bash -e

echoerr() { echo "$@" 1>&2; }

if [[ $# -lt 2 ]] ; then
    echoerr $0 envfile command
    exit 1
fi

set -a
source $1
set +a;
"${@:2}"
