#!/bin/bash

# Default compiler
if [[ $CC = "" ]]; then
    CC=clang
fi

IS_RELEASE=false
TARGET_ARCH=""
USE_QEMU=false

# Parse arguments for target architecture
if [[ $# -gt 0 ]]; then
        case "${!#}" in
            "x86_64-linux-gnu"|"aarch64-linux-gnu"|"x86_64-windows-gnu"|"aarch64-windows-gnu"|"x86_64-macos"|"aarch64-macos")
                TARGET_ARCH="${!#}"
                USE_QEMU=true
                # Remove the target arch from arguments
                set -- "${@:1:$(($#-1))}"
                ;;
            "all-targets")
                TARGET_ARCH="all-targets"
                USE_QEMU=true
                # Remove the target arch from arguments
                set -- "${@:1:$(($#-1))}"
                ;;
            "riscv64-linux-gnu"|"riscv64"|"riscv"|"mips"|"mips64"|"arm"|"armv7"|"i386"|"i686")
                echo "Error: Unsupported target architecture '${!#}'"
                echo "Supported architectures: x86_64-linux-gnu, aarch64-linux-gnu, x86_64-windows-gnu, aarch64-windows-gnu, x86_64-macos, aarch64-macos"
                echo "Usage: ./$(basename $0) <clean|test|test release|release> [architecture|all-targets]"
                exit 1
                ;;
        esac
fi


# Set up cross-compilation if target architecture is specified
if [[ $TARGET_ARCH != "" && $TARGET_ARCH != "all-targets" ]]; then
    case $TARGET_ARCH in
        "x86_64-linux-gnu")
            CC="zig cc -target x86_64-linux-gnu"
            QEMU_TARGET=qemu-x86_64
            CC_FLAGS="-Wall -std=c11 -g -fvisibility=hidden -fPIC"
            ;;
        "aarch64-linux-gnu")
            CC="zig cc -target aarch64-linux-gnu"
            QEMU_TARGET=qemu-aarch64
            CC_FLAGS="-Wall -std=c11 -g -fvisibility=hidden -fPIC"
            ;;
        "x86_64-windows-gnu")
            CC="zig cc -target x86_64-windows-gnu"
            QEMU_TARGET=""
            CC_FLAGS="-Wall -std=c11 -g -fvisibility=hidden -fPIC -D_WIN64"
            ;;
        "aarch64-windows-gnu")
            CC="zig cc -target aarch64-windows-gnu"
            QEMU_TARGET=""
            CC_FLAGS="-Wall -std=c11 -g -fvisibility=hidden -fPIC -D_WIN64"
            ;;
        "x86_64-macos")
            CC="zig cc -target x86_64-macos"
            QEMU_TARGET=""
            CC_FLAGS="-Wall -std=c11 -g -fvisibility=hidden -fPIC"
            ;;
        "aarch64-macos")
            CC="zig cc -target aarch64-macos"
            QEMU_TARGET=""
            CC_FLAGS="-Wall -std=c11 -g -fvisibility=hidden -fPIC"
            ;;
    esac
    
    # Check if cross-compiler is available
    if ! command -v $CC >/dev/null 2>&1; then
        echo "Error: Cross-compiler '$CC' not found"
        echo ""
        echo "To install the required cross-compiler:"
        echo "  brew install zig"
        echo ""
        echo "Zig provides cross-compilation support for all target architectures."
        exit 1
    fi
    
    # Check if QEMU user-mode emulator is available (only for Linux targets)
    if [[ -n "$QEMU_TARGET" && ! -z "$QEMU_TARGET" ]]; then
        if ! command -v $QEMU_TARGET >/dev/null 2>&1; then
            echo "Warning: QEMU user-mode emulator '$QEMU_TARGET' not found"
            echo "Cross-compilation will work, but tests cannot be executed without QEMU."
            echo ""
            echo "To install QEMU user-mode emulators:"
            echo "  brew install qemu"
            echo "  # Note: macOS Homebrew QEMU may not include user-mode emulators"
            echo "  # You may need to compile QEMU from source with user-mode support"
            echo ""
            echo "For more information, visit: https://www.qemu.org/download/"
            echo ""
        fi
    fi
    
    export CC
    export TARGET_ARCH
elif [[ $TARGET_ARCH = "" ]]; then
    # Reset to native compiler when no target architecture is specified
    CC=clang
    unset TARGET_ARCH
fi

if [[ $OPT_LEVEL = "" ]]; then
    if [[ $1 = "release" ]]; then
        OPT_LEVEL=O3
    elif [[ $1 = "test" && $2 = "release" ]]; then
        OPT_LEVEL=O3
    else
        OPT_LEVEL=O0
    fi
fi

if [[ $CC_FLAGS = "" ]]; then
    if [[ $1 = "release" ]]; then
        CC_FLAGS="-Wall -std=c11 -DNDEBUG -fvisibility=hidden"
    else
        CC_FLAGS="-Wall -std=c11 -g -fvisibility=hidden"
    fi
fi


TARGET_DIR=./target
LIB_NAME=libseagreen

SRC_DIR=./src
INCLUDE_DIR=./include
TEST_SRC_DIR=./tests

SRC_FILES=$(echo $(find $SRC_DIR -type f -name "*.c") $(find src -type f -name "*.S"))

TEST_SRC_FILES=$(echo $(find $TEST_SRC_DIR -maxdepth 1 -type f -name "*.c" -print0 | sort -z -V | xargs -0))

if [[ !($1 = "clean" || $1 = "test" || $1 = "release" || $1 = "") ]]; then
        echo "Usage: ./$(basename $0) <clean|test|test release|release> [architecture|all-targets]"
    echo "  Target architectures enable cross-compilation and QEMU execution"
    exit 1
fi

if [[ $1 = "clean" ]]; then
    rm -rf $TARGET_DIR
    exit 0
fi

# Clean target directory when cross-compiling to ensure proper architecture
if [[ $TARGET_ARCH != "" ]]; then
    echo "Cross-compiling for $TARGET_ARCH - cleaning target directory"
    rm -rf $TARGET_DIR
fi

function build_objs {
    if [[ $1 = "test" ]]; then
        MACROS="-DCGN_DEBUG"
    else
        MACROS=""
    fi

    OUT="$TARGET_DIR"
    mkdir -p $OUT

    for FILE in $SRC_FILES; do
        FNAME=$(basename $FILE)
        FILE_NO_EXT="${FNAME%.*}"
        OBJ=" $OUT/$FILE_NO_EXT.o"
        (PS4="\000" set -x; $CC -$OPT_LEVEL $MACROS $CC_FLAGS -fPIC -c $FILE -o $OBJ -I$INCLUDE_DIR) || exit 1

        if [[ $? -ne 0 ]]; then
            exit 1
        fi
    done

    # Use Zig's ar for cross-compilation, system ar for native
    if [[ $TARGET_ARCH != "" ]]; then
        zig ar rcs $OUT/$LIB_NAME.a $OUT/*.o
    else
        ar rcs $OUT/$LIB_NAME.a $OUT/*.o
        ranlib $OUT/$LIB_NAME.a
    fi
}

function file_to_test_name {
    TEST_FILE_NAME=$(basename $1)
    echo "${TEST_FILE_NAME%.*}"
}

function test_number_to_file {
    local test_num="$1"
    # Find the test file that starts with the given number
    local test_file=$(find $TEST_SRC_DIR -maxdepth 1 -name "${test_num}-*.c" | head -1)
    if [[ -n "$test_file" ]]; then
        echo "$test_file"
    else
        echo ""
    fi
}

function build_test {
    TEST_OUT="$TARGET_DIR/tests"

    FILES="$TARGET_DIR/$LIB_NAME.a $1"
    MACROS="-DCGN_DEBUG"

    OUT_FILE=$TEST_OUT/$(file_to_test_name $1)

    mkdir -p $TEST_OUT

    # Add pthread library for Linux cross-compilation
    if [[ $TARGET_ARCH != "" ]]; then
        (PS4="\000" set -x; $CC -$OPT_LEVEL $MACROS $CC_FLAGS $1 -o $OUT_FILE -I$INCLUDE_DIR -L$TARGET_DIR -lseagreen -lpthread)
    else
        (PS4="\000" set -x; $CC -$OPT_LEVEL $MACROS $CC_FLAGS $1 -o $OUT_FILE -I$INCLUDE_DIR -L$TARGET_DIR -lseagreen)
    fi
}

function run_with_qemu {
    local test_file="$1"
    local test_name="$2"

    case $TARGET_ARCH in
        "x86_64-linux-gnu")
            if command -v qemu-x86_64 >/dev/null 2>&1; then
                qemu-x86_64 $test_file
            else
                echo "QEMU user-mode emulator not found. Cross-compiled binary created at: $test_file"
                echo "To run this binary, you need to install QEMU user-mode emulators."
                echo "On macOS, you can try: brew install qemu-user-static or compile QEMU from source with user-mode support."
                return 1
            fi
            ;;
        "aarch64-linux-gnu")
            if command -v qemu-aarch64 >/dev/null 2>&1; then
                qemu-aarch64 $test_file
            else
                echo "QEMU user-mode emulator not found. Cross-compiled binary created at: $test_file"
                echo "To run this binary, you need to install QEMU user-mode emulators."
                echo "On macOS, you can try: brew install qemu-user-static or compile QEMU from source with user-mode support."
                return 1
            fi
            ;;
        "x86_64-windows-gnu"|"aarch64-windows-gnu")
            echo "Windows cross-compiled binary created at: $test_file"
            echo "To run this binary, you need a Windows environment or Wine."
            echo "On macOS, you can try: brew install wine-stable"
            return 1
            ;;
        "x86_64-macos"|"aarch64-macos")
            echo "macOS cross-compiled binary created at: $test_file"
            echo "To run this binary, you need a macOS environment."
            echo "Note: Cross-compiled macOS binaries may not run on different macOS versions."
            return 1
            ;;
        *)
            echo "Unknown target architecture: $TARGET_ARCH"
            return 1
            ;;
    esac
}


# Handle all-targets case before building objects
if [[ $TARGET_ARCH = "all-targets" && $1 = "test" ]]; then
    echo "Running tests for all supported cross-compilation targets..."
    echo ""
    
        ARCHES=("x86_64-linux-gnu" "aarch64-linux-gnu" "x86_64-windows-gnu" "aarch64-windows-gnu" "x86_64-macos" "aarch64-macos")
    OVERALL_SUCCESS=0
    OVERALL_FAILED=0
    
    for ARCH in "${ARCHES[@]}"; do
        echo "=========================================="
        echo "Testing architecture: $ARCH"
        echo "=========================================="
        
            # Set up this architecture
            case $ARCH in
                "x86_64-linux-gnu")
                    CC="zig cc -target x86_64-linux-gnu"
                    QEMU_TARGET=qemu-x86_64
                    CC_FLAGS="-Wall -std=c11 -g -fvisibility=hidden -fPIC"
                    ;;
                "aarch64-linux-gnu")
                    CC="zig cc -target aarch64-linux-gnu"
                    QEMU_TARGET=qemu-aarch64
                    CC_FLAGS="-Wall -std=c11 -g -fvisibility=hidden -fPIC"
                    ;;
                "x86_64-windows-gnu")
                    CC="zig cc -target x86_64-windows-gnu"
                    QEMU_TARGET=""
                    CC_FLAGS="-Wall -std=c11 -g -fvisibility=hidden -fPIC -D_WIN64"
                    ;;
                "aarch64-windows-gnu")
                    CC="zig cc -target aarch64-windows-gnu"
                    QEMU_TARGET=""
                    CC_FLAGS="-Wall -std=c11 -g -fvisibility=hidden -fPIC -D_WIN64"
                    ;;
                "x86_64-macos")
                    CC="zig cc -target x86_64-macos"
                    QEMU_TARGET=""
                    CC_FLAGS="-Wall -std=c11 -g -fvisibility=hidden -fPIC"
                    ;;
                "aarch64-macos")
                    CC="zig cc -target aarch64-macos"
                    QEMU_TARGET=""
                    CC_FLAGS="-Wall -std=c11 -g -fvisibility=hidden -fPIC"
                    ;;
            esac
        
        # Check if cross-compiler is available
        if ! command -v $CC >/dev/null 2>&1; then
            echo "Error: Cross-compiler '$CC' not found - skipping $ARCH"
            echo ""
            continue
        fi
        
            # Check if QEMU user-mode emulator is available (only for Linux targets)
            if [[ -n "$QEMU_TARGET" && ! -z "$QEMU_TARGET" ]]; then
                if ! command -v $QEMU_TARGET >/dev/null 2>&1; then
                    echo "Warning: QEMU user-mode emulator '$QEMU_TARGET' not found for $ARCH"
                    echo "Cross-compilation will work, but tests cannot be executed without QEMU."
                    echo ""
                fi
            fi
        
        export CC
        export TARGET_ARCH="$ARCH"
        
        # Clean target directory for this architecture
        echo "Cross-compiling for $ARCH - cleaning target directory"
        rm -rf $TARGET_DIR
        
        # Build objects and library
        build_objs $1 &&
        
        # Build tests
        TEST_NAMES=()
        if [[ $# -gt 1 ]]; then
            TEST_FILE=""
            TEST_ARG=""
            
            # Determine which argument is the test specification
            if [[ $2 = "release" && $# -gt 2 ]]; then
                # Case: test release <test>
                TEST_ARG="$3"
            else
                # Case: test <test>
                TEST_ARG="$2"
            fi
            
            # First, try to interpret as a test number
            if [[ "$TEST_ARG" =~ ^[0-9]+$ ]]; then
                TEST_FILE=$(test_number_to_file "$TEST_ARG")
                if [[ -z "$TEST_FILE" ]]; then
                    echo "Error: Test number '$TEST_ARG' not found"
                    echo "Available test numbers:"
                    for FILE in $TEST_SRC_FILES; do
                        test_name=$(file_to_test_name $FILE)
                        test_num=$(echo $test_name | cut -d'-' -f1)
                        echo "  $test_num - $test_name"
                    done
                    exit 1
                fi
            # If not a number, try as filename
            elif [[ -f "$TEST_ARG" || -f "./tests/$TEST_ARG" ]]; then
                TEST_FILE="$TEST_ARG"
                if [[ ! -f "$TEST_FILE" ]]; then
                    TEST_FILE="./tests/$TEST_ARG"
                fi
            else
                echo "Error: Test '$TEST_ARG' not found"
                echo "Provide either a test number (e.g., '1') or filename (e.g., '1-basic-foo-bar.c')"
                exit 1
            fi
            
            TEST_NAMES+=" $(file_to_test_name $TEST_FILE)"
            build_test $TEST_FILE
        else
            # Build all test files
            for FILE in $TEST_SRC_FILES; do
                TEST_NAMES+=" $(file_to_test_name $FILE)"
                build_test $FILE
            done
        fi &&
        
        # Run tests
        SUCCESS_COUNT=0
        TEST_COUNT=0
        
        for TEST_NAME in $TEST_NAMES; do
            echo "" &&
            echo "----- Running test: $TEST_NAME -----"
            
            echo "Running with QEMU for $ARCH"
            if run_with_qemu "$TARGET_DIR/tests/$TEST_NAME" "$TEST_NAME"; then
                SUCCESS_COUNT=$((SUCCESS_COUNT + 1))
            fi
            TEST_COUNT=$((TEST_COUNT + 1))
        done
        
        echo ""
        echo "------------------"
        echo "PASSED: $SUCCESS_COUNT test(s)"
        echo "FAILED: $((TEST_COUNT - SUCCESS_COUNT)) test(s)"
        echo "------------------"
        echo ""
        
        OVERALL_SUCCESS=$((OVERALL_SUCCESS + SUCCESS_COUNT))
        OVERALL_FAILED=$((OVERALL_FAILED + (TEST_COUNT - SUCCESS_COUNT)))
    done
    
    echo "=========================================="
    echo "OVERALL RESULTS FOR ALL ARCHITECTURES:"
    echo "=========================================="
    echo "PASSED: $OVERALL_SUCCESS test(s)"
    echo "FAILED: $OVERALL_FAILED test(s)"
    echo "=========================================="
    
    if [[ $OVERALL_FAILED -gt 0 ]]; then
        exit 1
    fi
else
    # Single architecture or native compilation
    build_objs $1 &&

    if [[ $1 = "test" ]]; then
        # Single architecture or native compilation
        TEST_NAMES=()
        
        # Check if a specific test was provided (by number or filename)
        # Handle both "test <test>" and "test release <test>" cases
        if [[ $# -gt 1 ]]; then
            TEST_FILE=""
            TEST_ARG=""
            
            # Determine which argument is the test specification
            if [[ $2 = "release" && $# -gt 2 ]]; then
                # Case: test release <test>
                TEST_ARG="$3"
            else
                # Case: test <test>
                TEST_ARG="$2"
            fi
            
            # First, try to interpret as a test number
            if [[ "$TEST_ARG" =~ ^[0-9]+$ ]]; then
                TEST_FILE=$(test_number_to_file "$TEST_ARG")
                if [[ -z "$TEST_FILE" ]]; then
                    echo "Error: Test number '$TEST_ARG' not found"
                    echo "Available test numbers:"
                    for FILE in $TEST_SRC_FILES; do
                        test_name=$(file_to_test_name $FILE)
                        test_num=$(echo $test_name | cut -d'-' -f1)
                        echo "  $test_num - $test_name"
                    done
                    exit 1
                fi
            # If not a number, try as filename
            elif [[ -f "$TEST_ARG" || -f "./tests/$TEST_ARG" ]]; then
                TEST_FILE="$TEST_ARG"
                if [[ ! -f "$TEST_FILE" ]]; then
                    TEST_FILE="./tests/$TEST_ARG"
                fi
            else
                echo "Error: Test '$TEST_ARG' not found"
                echo "Provide either a test number (e.g., '1') or filename (e.g., '1-basic-foo-bar.c')"
                exit 1
            fi
            
            TEST_NAMES+=" $(file_to_test_name $TEST_FILE)"
            build_test $TEST_FILE
        else
            # Build all test files
            for FILE in $TEST_SRC_FILES; do
                TEST_NAMES+=" $(file_to_test_name $FILE)"
                build_test $FILE
            done
        fi &&

        SUCCESS_COUNT=0 &&
        TEST_COUNT=0 &&

        for TEST_NAME in $TEST_NAMES; do
            echo "" &&
            echo "----- Running test: $TEST_NAME -----"
            
            if [[ $USE_QEMU = true ]]; then
                echo "Running with QEMU for $TARGET_ARCH"
                time run_with_qemu "$TARGET_DIR/tests/$TEST_NAME" "$TEST_NAME"
            else
                time $TARGET_DIR/tests/$TEST_NAME
            fi

            if [[ $? -eq 0 ]]; then
                SUCCESS_COUNT=$((SUCCESS_COUNT + 1))
            fi

            TEST_COUNT=$((TEST_COUNT + 1))
        done &&

        echo "" &&
        echo "------------------" &&
        echo "PASSED: $SUCCESS_COUNT test(s)" &&
        echo "FAILED: $((TEST_COUNT-SUCCESS_COUNT)) test(s)" &&
        echo "------------------"
    fi
fi
