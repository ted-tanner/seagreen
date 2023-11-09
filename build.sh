#!/bin/sh

# Available args:
#   - ./build.sh clean
#   - ./build.sh test
#   - ./build.sh release

CC=clang

if [[ $1 = "release" ]]; then
    OPT_LEVEL=O3
else
    OPT_LEVEL=O0
fi

FLAGS="-Wall -Wextra -Werror -std=c11 -g"

OUT_DIR=target
LIB_NAME=seagreenlib

SRC_DIR=src
INCLUDE_DIR=include
TEST_SRC_DIR=test

SRC_FILES=$(echo $(find $SRC_DIR -type f -name "*.c") $(find src -type f -name "*.S"))
TEST_SRC_FILES=$(echo $(find $TEST_SRC_DIR -type f -name "*.c"))

if [[ $# -gt 1 || !($1 = "clean" || $1 = "test" || $1 = "release" || $1 = "") ]]; then
    echo "Usage: ./$(basename $0) <clean|test|release>"
    exit 1
fi

if [[ $1 = "clean" ]]; then
    rm -rf $OUT_DIR
    exit 0
fi

function build_objs {
    if [[ $1 = "test" ]]; then
        FILES="$SRC_FILES $TEST_SRC_FILES"
        MACROS="-DCGN_TEST"
        OUT="$OUT_DIR/test"
    else
        FILES="$SRC_FILES"
        MACROS=""
        OUT="$OUT_DIR"
    fi

    mkdir -p $OUT

    for FILE in $FILES; do
        FNAME=$(basename $FILE)
        FILE_NO_EXT="${FNAME%.*}"
        OBJ=" $OUT/$FILE_NO_EXT.o"
        (PS4="\000" set -x; $CC -$OPT_LEVEL $MACROS $FLAGS -c $FILE -o $OBJ -I$INCLUDE_DIR)

        if [[ $? -ne 0 ]]; then
            exit 1
        fi
    done

    ar rcs $OUT/$LIB_NAME.a $OUT/*.o
}

function build_testexec {
    OUT="$OUT_DIR/test"

    FILES="$OUT/$LIB_NAME.a $TEST_SRC_DIR/testsuitemain.c"
    MACROS="-DCGN_TEST"

    mkdir -p $OUT

    CC -$OPT_LEVEL $MACROS $FLAGS $FILES -o $OUT/testexec -I$INCLUDE_DIR
}

build_objs $1

if [[ $1 = "test" ]]; then
    build_testexec
    ./target/test/testexec
fi
