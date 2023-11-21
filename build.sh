#!/bin/sh

# Available args:
#   - ./build.sh clean
#   - ./build.sh test
#   - ./build.sh test release
#   - ./build.sh release

CC=clang

if [[ $1 = "release" ]]; then
    OPT_LEVEL=O3
elif [[ $1 = "test" && $2 = "release" ]]; then
    OPT_LEVEL=O3
else
    OPT_LEVEL=O0
fi

FLAGS="-Wall -Wextra -Werror -std=c11 -g"

OUT_DIR=./target
LIB_NAME=seagreenlib

SRC_DIR=./src
INCLUDE_DIR=./include
TEST_SRC_DIR=./tests

SRC_FILES=$(echo $(find $SRC_DIR -type f -name "*.c") $(find src -type f -name "*.S"))
TEST_SRC_FILES=$(echo $(find $TEST_SRC_DIR -type f -name "*.c"))

if [[ !($1 = "clean" || $1 = "test" || $1 = "release" || $1 = "") ]]; then
    echo "Usage: ./$(basename $0) <clean|test|test release|release>"
    exit 1
fi

if [[ $1 = "clean" ]]; then
    rm -rf $OUT_DIR
    exit 0
fi

function build_objs {
    MACROS=""
    OUT="$OUT_DIR"

    mkdir -p $OUT

    for FILE in $SRC_FILES; do
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

function file_to_test_name {
    TEST_FILE_NAME=$(basename $1)
    echo "${TEST_FILE_NAME%.*}"
}

function build_test {
    TEST_OUT="$OUT_DIR/tests"

    FILES="$OUT_DIR/$LIB_NAME.a $1"
    MACROS="-DCGN_TEST"

    OUT_FILE=$TEST_OUT/$(file_to_test_name $1)

    mkdir -p $TEST_OUT

    $CC -$OPT_LEVEL $MACROS $FLAGS $FILES -o $OUT_FILE -I$INCLUDE_DIR
}

build_objs $1 &&

if [[ $1 = "test" ]]; then
    TEST_NAMES=()

    for FILE in $TEST_SRC_FILES; do
        TEST_NAMES+=$(file_to_test_name $FILE)
        build_test $FILE
    done &&

    for TEST_NAME in $TEST_NAMES; do
        echo "\n----- Running test: $TEST_NAME -----\n"
        time $OUT_DIR/tests/$TEST_NAME
    done
fi
