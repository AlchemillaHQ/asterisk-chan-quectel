#!/bin/bash -e

echoerr() { echo "$@" 1>&2; }

readonly INSIDE_WORK_TREE=$(git rev-parse --is-inside-work-tree)

if [ "${INSIDE_WORK_TREE}" = 'true' ]; then
    if [ -n "$(git status --porcelain ${GIT_PATHSPEC})" ]; then
        if [ -z "${PRESET}" ]; then
	        echoerr There are uncommited changes
	        exit 1
        else
            echoerr There are uncommited changes
            SOURCE_DATE_EPOCH=$(date -u '+%s')
        fi
    else
        SOURCE_DATE_EPOCH=$(git log -n 1 '--pretty=format:%at')
    fi
else
    SOURCE_DATE_EPOCH=$(date -u '+%s')
fi

readonly SOURCE_DATE=$(date --date=@${SOURCE_DATE_EPOCH} '+%F %T')
echoerr SOURCE_DATE_EPOCH=${SOURCE_DATE_EPOCH} '#' ${SOURCE_DATE}
echo SOURCE_DATE_EPOCH=${SOURCE_DATE_EPOCH}
