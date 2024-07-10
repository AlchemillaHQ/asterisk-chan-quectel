#!/bin/bash -e

DEF_AST_DIR=/usr/include

echoerr() { echo "$@" 1>&2; }

make-ast-archive() {
    local AST_DIR=${1:-${DEF_AST_DIR}}
    if ! [[ -r "${AST_DIR}/asterisk.h" && -d "${AST_DIR}/asterisk" ]]; then
        echoerr Wrong headers directory - ${AST_DIR}
        return 1
    fi
    echoerr Archiving headers from ${AST_DIR}
    tar -I 'gzip --best' --numeric-owner --owner=0 --group=0 --mode='og-w' -cf asterisk-headers.tar.gz -C ${AST_DIR} asterisk.h asterisk/
}

make-ast-archive $1
