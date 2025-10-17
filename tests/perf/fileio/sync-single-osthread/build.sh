#!/bin/bash

if [[ $CC = "" ]]; then
    CC=clang
fi

IS_RELEASE=false

if [[ $OPT_LEVEL = "" ]]; then
    if [[ $2 = "release" ]]; then
        OPT_LEVEL=O3
    else
        OPT_LEVEL=O1
    fi
fi

if [[ $CC_FLAGS = "" ]]; then
    if [[ $2 = "release" ]]; then
        CC_FLAGS="-Wall -Wextra -std=c11 -DNDEBUG"
    else
        CC_FLAGS="-Wall -Wextra -std=c11 -g"
    fi
fi

TARGET_DIR=./target
OUT_FILE=$TARGET_DIR/sync-single-osthread.out

OUTPUT_FILE_DIR=./out
mkdir -p $OUTPUT_FILE_DIR

INCLUDE_DIR=../../../../include
SRC_FILES="main.c"

if [[ !($1 = "clean" || $1 = "run" || $1 = "release" || $1 = "") ]]; then
    echo "Usage: ./$(basename $0) <clean|run|release>"
    exit 1
fi

if [[ $1 = "clean" ]]; then
    rm -rf $TARGET_DIR
    rm -rf $OUTPUT_FILE_DIR
    exit 0
fi

function build_objs {
    if [[ $1 = "release" ]]; then
        MACROS=""
    else
        MACROS="-DCGN_DEBUG"
    fi

    mkdir -p $TARGET_DIR

    (PS4="\000" set -x; $CC -$OPT_LEVEL $MACROS $CC_FLAGS $SRC_FILES -o $OUT_FILE -I$INCLUDE_DIR) || exit 1
}

if [[ $1 = "run" ]]; then
    build_objs ""
    $OUT_FILE
else
    build_objs $1

    if [[ $2 = "run" ]]; then
        $OUT_FILE
    fi
fi
