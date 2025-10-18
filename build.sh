#!/bin/bash

set -euo pipefail

# ==============================
# QEMU VM CONFIG (override via env)
# ==============================
: "${VM_SSH_USER:=tester}"
: "${VM_SSH_KEY:=$HOME/.ssh/id_rsa}"
# Prefer ed25519 if the configured key is missing
if [[ ! -f "$VM_SSH_KEY" && -f "$HOME/.ssh/id_ed25519" ]]; then
    VM_SSH_KEY="$HOME/.ssh/id_ed25519"
fi

: "${QEMU_X86_LINUX_IMG:=./vm/x86_64-linux.qcow2}"
: "${QEMU_AARCH64_LINUX_IMG:=./vm/aarch64-linux.qcow2}"
: "${QEMU_X86_WINDOWS_IMG:=./vm/x86_64-windows.qcow2}"
: "${QEMU_AARCH64_WINDOWS_IMG:=./vm/aarch64-windows.qcow2}"

: "${QEMU_X86_LINUX_SSH_PORT:=2222}"
: "${QEMU_AARCH64_LINUX_SSH_PORT:=2223}"
: "${QEMU_X86_WINDOWS_SSH_PORT:=2224}"
: "${QEMU_AARCH64_WINDOWS_SSH_PORT:=2225}"

# Optional VM automation knobs
: "${VM_SSH_WAIT_SECS:=180}"          # total seconds to wait for SSH
: "${VM_SSH_POLL_SECS:=5}"            # seconds between SSH probes
: "${VM_SEED_ISO:=}"                  # path to a NoCloud seed ISO (CIDATA)

QEMU_X86_SYS=qemu-system-x86_64
QEMU_ARM_SYS=qemu-system-aarch64

ssh_cmd_base=(ssh -q -o LogLevel=ERROR -o BatchMode=yes -o PreferredAuthentications=publickey -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null -o ConnectTimeout=5 -i "$VM_SSH_KEY")
scp_cmd_base=(scp -q -o LogLevel=ERROR -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null -i "$VM_SSH_KEY")

# ==============================
# Existing options
# ==============================

# Default compiler
if [[ ${CC:-} = "" ]]; then
    CC=clang
fi

IS_RELEASE=false
TARGET_ARCH=""
USE_QEMU=false

# Parse arguments for target architecture
if [[ $# -gt 0 ]]; then
    case "${!#}" in
        "x86_64-linux-gnu"|"aarch64-linux-gnu"|"x86_64-windows-gnu"|"aarch64-windows-gnu")
            TARGET_ARCH="${!#}"
            USE_QEMU=true
            set -- "${@:1:$(($#-1))}"
            ;;
        "all-targets")
            TARGET_ARCH="all-targets"
            USE_QEMU=true
            set -- "${@:1:$(($#-1))}"
            ;;
        "riscv64-linux-gnu"|"riscv64"|"riscv"|"mips"|"mips64"|"arm"|"armv7"|"i386"|"i686")
            echo "Error: Unsupported target architecture '${!#}'"
            echo "Supported: x86_64-linux-gnu, aarch64-linux-gnu, x86_64-windows-gnu, aarch64-windows-gnu"
            echo "Usage: ./$(basename $0) <clean|test|test release|release> [architecture|all-targets]"
            exit 1
            ;;
    esac
fi

# Set up cross-compilation if target architecture is specified
if [[ ${TARGET_ARCH:-} != "" && $TARGET_ARCH != "all-targets" ]]; then
    case $TARGET_ARCH in
        "x86_64-linux-gnu")
            CC="zig cc -target x86_64-linux-gnu"
            CC_FLAGS="-Wall -std=c11 -g -fvisibility=hidden -fPIC"
            ;;
        "aarch64-linux-gnu")
            CC="zig cc -target aarch64-linux-gnu"
            CC_FLAGS="-Wall -std=c11 -g -fvisibility=hidden -fPIC"
            ;;
        "x86_64-windows-gnu")
            CC="zig cc -target x86_64-windows-gnu"
            CC_FLAGS="-Wall -std=c11 -g -fvisibility=hidden -fPIC -D_WIN64"
            ;;
        "aarch64-windows-gnu")
            CC="zig cc -target aarch64-windows-gnu"
            CC_FLAGS="-Wall -std=c11 -g -fvisibility=hidden -fPIC -D_WIN64"
            ;;
    esac

    # Check cross-compiler
    if ! command -v ${CC%% *} >/dev/null 2>&1; then
        echo "Error: Cross-compiler '$CC' not found"
        echo "Install Zig (provides cross targets):"
        echo "  brew install zig"
        exit 1
    fi

    export CC
    export TARGET_ARCH
elif [[ ${TARGET_ARCH:-} = "" ]]; then
    # Native
    CC=clang
    unset TARGET_ARCH
fi

if [[ ${OPT_LEVEL:-} = "" ]]; then
    if [[ ${1:-} = "release" ]]; then
        OPT_LEVEL=O3
    elif [[ ${1:-} = "test" && ${2:-} = "release" ]]; then
        OPT_LEVEL=O3
    else
        OPT_LEVEL=O0
    fi
fi

if [[ ${CC_FLAGS:-} = "" ]]; then
    if [[ ${1:-} = "release" ]]; then
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

if [[ !(${1:-} = "clean" || ${1:-} = "test" || ${1:-} = "release" || ${1:-} = "") ]]; then
    echo "Usage: ./$(basename $0) <clean|test|test release|release> [architecture|all-targets]"
    echo "  Target architectures enable cross-compilation and QEMU execution"
    exit 1
fi

if [[ ${1:-} = "clean" ]]; then
    rm -rf $TARGET_DIR
    exit 0
fi

# Clean target directory when cross-compiling to ensure proper architecture
if [[ ${TARGET_ARCH:-} != "" ]]; then
    echo "Cross-compiling for $TARGET_ARCH - cleaning target directory"
    rm -rf $TARGET_DIR
fi

# ==============================
# Build helpers
# ==============================

function build_objs {
    local mode="$1"
    local MACROS=""
    if [[ $mode = "test" ]]; then
        MACROS="-DCGN_DEBUG"
    fi

    local OUT="$TARGET_DIR"
    mkdir -p "$OUT"

    for FILE in $SRC_FILES; do
        FNAME=$(basename "$FILE")
        FILE_NO_EXT="${FNAME%.*}"
        OBJ="$OUT/$FILE_NO_EXT.o"
        (PS4=$'\000' set -x; $CC -$OPT_LEVEL $MACROS $CC_FLAGS -fPIC -c "$FILE" -o "$OBJ" -I"$INCLUDE_DIR") || exit 1
    done

    if [[ ${TARGET_ARCH:-} != "" ]]; then
        zig ar rcs "$OUT/$LIB_NAME.a" $OUT/*.o
    else
        ar rcs "$OUT/$LIB_NAME.a" $OUT/*.o
        ranlib "$OUT/$LIB_NAME.a"
    fi
}

function file_to_test_name {
    local TEST_FILE_NAME
    TEST_FILE_NAME=$(basename "$1")
    echo "${TEST_FILE_NAME%.*}"
}

function test_number_to_file {
    local test_num="$1"
    local test_file
    test_file=$(find "$TEST_SRC_DIR" -maxdepth 1 -name "${test_num}-*.c" | head -1 || true)
    if [[ -n "$test_file" ]]; then
        echo "$test_file"
    else
        echo ""
    fi
}

function build_test {
    local src="$1"
    local TEST_OUT="$TARGET_DIR/tests"
    local MACROS="-DCGN_DEBUG"
    local OUT_BASENAME
    OUT_BASENAME="$(file_to_test_name "$src")"

    mkdir -p "$TEST_OUT"

    local out="$TEST_OUT/$OUT_BASENAME"
    # On Windows targets, produce .exe
    if [[ ${TARGET_ARCH:-} = "x86_64-windows-gnu" || ${TARGET_ARCH:-} = "aarch64-windows-gnu" ]]; then
        out="${out}.exe"
    fi

    if [[ ${TARGET_ARCH:-} = "" ]]; then
        (PS4=$'\000' set -x; $CC -$OPT_LEVEL $MACROS $CC_FLAGS "$src" -o "$out" -I"$INCLUDE_DIR" -L"$TARGET_DIR" -lseagreen)
    else
        # Link pthread on Linux; Windows targets will ignore/auto-satisfy as needed by Zig toolchain
        if [[ $TARGET_ARCH = *linux* ]]; then
            (PS4=$'\000' set -x; $CC -$OPT_LEVEL $MACROS $CC_FLAGS "$src" -o "$out" -I"$INCLUDE_DIR" -L"$TARGET_DIR" -lseagreen -lpthread)
        else
            (PS4=$'\000' set -x; $CC -$OPT_LEVEL $MACROS $CC_FLAGS "$src" -o "$out" -I"$INCLUDE_DIR" -L"$TARGET_DIR" -lseagreen)
        fi
    fi
}

# ==============================
# QEMU VM helpers
# ==============================

function ensure_qemu {
    local bin="$1"
    if ! command -v "$bin" >/dev/null 2>&1; then
        echo "Error: '$bin' not found."
        echo "Install QEMU on macOS:"
        echo "  brew install qemu"
        echo ""
        echo "Then create/provide a VM image for this target (see messages below)."
        exit 1
    fi
}

function ensure_vm_image {
    local img="$1" desc="$2"
    if [[ ! -f "$img" ]]; then
        echo "Error: $desc image not found at:"
        echo "  $img"
        echo ""
        echo "How to provide one (summary):"
        echo "  • Linux: create a small Debian/Ubuntu cloud image (qcow2), enable SSH,"
        echo "    add user '$VM_SSH_USER' with your public key, and ensure sshd listens on port 22."
        echo "  • Windows: install Windows (x86_64 or ARM) with OpenSSH Server enabled,"
        echo "    create user '$VM_SSH_USER' with your public key."
        echo ""
        echo "Place the resulting qcow2 at the path above or override via env var."
        exit 1
    fi
}

function wait_for_ssh {
    local port="$1"
    local deadline=$(( $(date +%s) + VM_SSH_WAIT_SECS ))
    while true; do
        if "${ssh_cmd_base[@]}" -p "$port" "$VM_SSH_USER@127.0.0.1" true </dev/null >/dev/null 2>&1; then
            return 0
        fi
        if (( $(date +%s) >= deadline )); then
            echo "Error: SSH did not become ready on localhost:$port after ${VM_SSH_WAIT_SECS}s"
            return 1
        fi
        sleep "$VM_SSH_POLL_SECS"
    done
}

function shutdown_vm {
    local port="$1"
    # Try to poweroff gracefully; ignore failure (e.g., Windows without sudo)
    "${ssh_cmd_base[@]}" -p "$port" "$VM_SSH_USER@127.0.0.1" "sudo poweroff || shutdown /s /t 0 || true" >/dev/null 2>&1 || true
}

function run_in_linux_vm {
    local arch="$1"           # x86_64 | aarch64
    local img="$2"
    local ssh_port="$3"
    local host_test_bin="$4"  # path on host
    local guest_bin="/home/$VM_SSH_USER/testbin/$(basename "$host_test_bin")"

    local qemu_bin
    local accel_args=()
    local machine_args=()
    if [[ $arch = "aarch64" ]]; then
        qemu_bin="$QEMU_ARM_SYS"
        accel_args=(-accel hvf)
        machine_args=(-machine virt)
    else
        qemu_bin="$QEMU_X86_SYS"  # x86_64 guest runs under TCG on Apple Silicon
        accel_args=()             # no HVF on x86 guest here
        machine_args=(-machine q35)
    fi

    ensure_qemu "$qemu_bin"
    ensure_vm_image "$img" "Linux ($arch)"

    # Boot VM headless with user networking + port forward for SSH
    "$qemu_bin" "${accel_args[@]+"${accel_args[@]}"}" -display none -nographic -m 2048 -smp 2 \
        "${machine_args[@]}" \
        -netdev user,id=net0,hostfwd=tcp::"$ssh_port"-:22 \
        -device virtio-net-pci,netdev=net0 \
        -drive file="$img",if=virtio \
        ${VM_SEED_ISO:+-drive if=virtio,format=raw,file="$VM_SEED_ISO"} \
        -serial mon:stdio \
        >/dev/null 2>&1 &
    local vm_pid=$!

    # Cleanup on exit
    trap "kill $vm_pid 2>/dev/null || true" EXIT

    echo "Waiting for SSH on port $ssh_port..."
    wait_for_ssh "$ssh_port"

    # Prepare dir, copy test binary, run it (stream both stdout and stderr)
    "${ssh_cmd_base[@]}" -p "$ssh_port" "$VM_SSH_USER@127.0.0.1" "mkdir -p ~/testbin && chmod 700 ~/testbin"
    "${scp_cmd_base[@]}" -P "$ssh_port" "$host_test_bin" "$VM_SSH_USER@127.0.0.1:$guest_bin"
    # make executable, then run with line-buffering if stdbuf exists
    "${ssh_cmd_base[@]}" -p "$ssh_port" "$VM_SSH_USER@127.0.0.1" "chmod +x \"$guest_bin\""
    "${ssh_cmd_base[@]}" -p "$ssh_port" "$VM_SSH_USER@127.0.0.1" \
        "if command -v stdbuf >/dev/null 2>&1; then stdbuf -oL -eL \"$guest_bin\"; else \"$guest_bin\"; fi" 2>&1
    local rc=$?

    shutdown_vm "$ssh_port"
    wait $vm_pid || true
    trap - EXIT

    return $rc
}

function run_in_windows_vm {
    local arch="$1"           # x86_64 | aarch64
    local img="$2"
    local ssh_port="$3"
    local host_test_bin="$4"  # .exe on host
    local guest_bin="C:/Users/$VM_SSH_USER/testbin/$(basename "$host_test_bin")"

    local qemu_bin
    local accel_args=()
    local machine_args=()
    if [[ $arch = "aarch64" ]]; then
        qemu_bin="$QEMU_ARM_SYS"
        accel_args=(-accel hvf)
        machine_args=(-machine virt)
    else
        qemu_bin="$QEMU_X86_SYS"
        accel_args=()
        machine_args=(-machine q35)
    fi

    ensure_qemu "$qemu_bin"
    ensure_vm_image "$img" "Windows ($arch)"

    "$qemu_bin" "${accel_args[@]+"${accel_args[@]}"}" -nographic -m 4096 -smp 2 \
        "${machine_args[@]}" \
        -netdev user,id=net0,hostfwd=tcp::"$ssh_port"-:22 \
        -device virtio-net-pci,netdev=net0 \
        -drive file="$img",if=virtio \
        -serial mon:stdio \
        >/dev/null 2>&1 &
    local vm_pid=$!

    trap "kill $vm_pid 2>/dev/null || true" EXIT

    echo "Waiting for SSH on port $ssh_port (Windows)..."
    wait_for_ssh "$ssh_port"

    # Windows 11 + OpenSSH: scp works; run via 'cmd /c' so PATH/console behavior is predictable
    "${ssh_cmd_base[@]}" -p "$ssh_port" "$VM_SSH_USER@127.0.0.1" 'powershell -NoProfile -Command "New-Item -ItemType Directory -Force $env:USERPROFILE\testbin | Out-Null"'
    "${scp_cmd_base[@]}" -P "$ssh_port" "$host_test_bin" "$VM_SSH_USER@127.0.0.1:$guest_bin"
    "${ssh_cmd_base[@]}" -p "$ssh_port" "$VM_SSH_USER@127.0.0.1" "cmd /c \"$guest_bin\""
    local rc=$?

    shutdown_vm "$ssh_port"
    wait $vm_pid || true
    trap - EXIT

    return $rc
}

function run_with_qemu {
    local test_file="$1"
    local test_name="$2"

    case "$TARGET_ARCH" in
        "x86_64-linux-gnu")
            run_in_linux_vm x86_64 "$QEMU_X86_LINUX_IMG" "$QEMU_X86_LINUX_SSH_PORT" "$test_file"
            ;;
        "aarch64-linux-gnu")
            run_in_linux_vm aarch64 "$QEMU_AARCH64_LINUX_IMG" "$QEMU_AARCH64_LINUX_SSH_PORT" "$test_file"
            ;;
        "x86_64-windows-gnu")
            # Ensure .exe
            if [[ "$test_file" != *.exe ]]; then
                test_file="${test_file}.exe"
            fi
            run_in_windows_vm x86_64 "$QEMU_X86_WINDOWS_IMG" "$QEMU_X86_WINDOWS_SSH_PORT" "$test_file"
            ;;
        "aarch64-windows-gnu")
            if [[ "$test_file" != *.exe ]]; then
                test_file="${test_file}.exe"
            fi
            run_in_windows_vm aarch64 "$QEMU_AARCH64_WINDOWS_IMG" "$QEMU_AARCH64_WINDOWS_SSH_PORT" "$test_file"
            ;;
        *)
            echo "Unknown target architecture: $TARGET_ARCH"
            return 1
            ;;
    esac
}

# ==============================
# All-targets + single-target flows
# ==============================

if [[ ${TARGET_ARCH:-} = "all-targets" && ${1:-} = "test" ]]; then
    echo "Running tests for all supported cross-compilation targets..."
    echo ""

    ARCHES=("x86_64-linux-gnu" "aarch64-linux-gnu" "x86_64-windows-gnu" "aarch64-windows-gnu")
    OVERALL_SUCCESS=0
    OVERALL_FAILED=0

    for ARCH in "${ARCHES[@]}"; do
        echo "=========================================="
        echo "Testing architecture: $ARCH"
        echo "=========================================="

        case $ARCH in
            "x86_64-linux-gnu")
                CC="zig cc -target x86_64-linux-gnu"
                CC_FLAGS="-Wall -std=c11 -g -fvisibility=hidden -fPIC"
                ;;
            "aarch64-linux-gnu")
                CC="zig cc -target aarch64-linux-gnu"
                CC_FLAGS="-Wall -std=c11 -g -fvisibility=hidden -fPIC"
                ;;
            "x86_64-windows-gnu")
                CC="zig cc -target x86_64-windows-gnu"
                CC_FLAGS="-Wall -std=c11 -g -fvisibility=hidden -fPIC -D_WIN64"
                ;;
            "aarch64-windows-gnu")
                CC="zig cc -target aarch64-windows-gnu"
                CC_FLAGS="-Wall -std=c11 -g -fvisibility=hidden -fPIC -D_WIN64"
                ;;
        esac

        if ! command -v ${CC%% *} >/dev/null 2>&1; then
            echo "Error: Cross-compiler '${CC%% *}' not found - skipping $ARCH"
            echo ""
            continue
        fi

        export CC
        export TARGET_ARCH="$ARCH"

        echo "Cross-compiling for $ARCH - cleaning target directory"
        rm -rf "$TARGET_DIR"

        # Build
        build_objs "$1"

        # Collect tests to build
        TEST_NAMES=()
        if [[ $# -gt 1 ]]; then
            TEST_FILE=""
            TEST_ARG=""
            if [[ ${2:-} = "release" && $# -gt 2 ]]; then
                TEST_ARG="$3"
            else
                TEST_ARG="$2"
            fi
            if [[ "$TEST_ARG" =~ ^[0-9]+$ ]]; then
                TEST_FILE=$(test_number_to_file "$TEST_ARG")
                if [[ -z "$TEST_FILE" ]]; then
                    echo "Error: Test number '$TEST_ARG' not found"
                    echo "Available test numbers:"
                    for FILE in $TEST_SRC_FILES; do
                        test_name=$(file_to_test_name "$FILE")
                        test_num=$(echo "$test_name" | cut -d'-' -f1)
                        echo "  $test_num - $test_name"
                    done
                    exit 1
                fi
            elif [[ -f "$TEST_ARG" || -f "./tests/$TEST_ARG" ]]; then
                TEST_FILE="$TEST_ARG"
                [[ -f "$TEST_FILE" ]] || TEST_FILE="./tests/$TEST_ARG"
            else
                echo "Error: Test '$TEST_ARG' not found"
                echo "Provide test number (e.g., '1') or filename (e.g., '1-basic-foo-bar.c')"
                exit 1
            fi
            TEST_NAMES+=" $(file_to_test_name "$TEST_FILE")"
            build_test "$TEST_FILE"
        else
            for FILE in $TEST_SRC_FILES; do
                TEST_NAMES+=" $(file_to_test_name "$FILE")"
                build_test "$FILE"
            done
        fi

        # Run
        SUCCESS_COUNT=0
        TEST_COUNT=0
        for TEST_NAME in $TEST_NAMES; do
            echo ""
            echo "----- Running test: $TEST_NAME -----"
            echo "Running inside QEMU VM for $ARCH"
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
    build_objs "${1:-}" &&

    if [[ ${1:-} = "test" ]]; then
        TEST_NAMES=()

        if [[ $# -gt 1 ]]; then
            TEST_FILE=""
            TEST_ARG=""
            if [[ ${2:-} = "release" && $# -gt 2 ]]; then
                TEST_ARG="$3"
            elif [[ ${2:-} = "release" && $# -eq 2 ]]; then
                TEST_ARG=""
            else
                TEST_ARG="$2"
            fi

            if [[ -z "$TEST_ARG" ]]; then
                for FILE in $TEST_SRC_FILES; do
                    TEST_NAMES+=" $(file_to_test_name "$FILE")"
                    build_test "$FILE"
                done
            else
                if [[ "$TEST_ARG" =~ ^[0-9]+$ ]]; then
                    TEST_FILE=$(test_number_to_file "$TEST_ARG")
                    if [[ -z "$TEST_FILE" ]]; then
                        echo "Error: Test number '$TEST_ARG' not found"
                        echo "Available test numbers:"
                        for FILE in $TEST_SRC_FILES; do
                            test_name=$(file_to_test_name "$FILE")
                            test_num=$(echo "$test_name" | cut -d'-' -f1)
                            echo "  $test_num - $test_name"
                        done
                        exit 1
                    fi
                elif [[ -f "$TEST_ARG" || -f "./tests/$TEST_ARG" ]]; then
                    TEST_FILE="$TEST_ARG"
                    [[ -f "$TEST_FILE" ]] || TEST_FILE="./tests/$TEST_ARG"
                else
                    echo "Error: Test '$TEST_ARG' not found"
                    echo "Provide either a test number (e.g., '1') or filename (e.g., '1-basic-foo-bar.c')"
                    exit 1
                fi

                TEST_NAMES+=" $(file_to_test_name "$TEST_FILE")"
                build_test "$TEST_FILE"
            fi
        else
            for FILE in $TEST_SRC_FILES; do
                TEST_NAMES+=" $(file_to_test_name "$FILE")"
                build_test "$FILE"
            done
        fi

        SUCCESS_COUNT=0
        TEST_COUNT=0

        for TEST_NAME in $TEST_NAMES; do
            echo ""
            echo "----- Running test: $TEST_NAME -----"

            if [[ ${USE_QEMU:-false} = true && ${TARGET_ARCH:-} != "" ]]; then
                echo "Running inside QEMU VM for $TARGET_ARCH"
                set +e
                time run_with_qemu "$TARGET_DIR/tests/$TEST_NAME" "$TEST_NAME"
                rc=$?
                set -e
            else
                set +e
                time "$TARGET_DIR/tests/$TEST_NAME"
                rc=$?
                set -e
            fi

            if [[ $rc -eq 0 ]]; then
                SUCCESS_COUNT=$((SUCCESS_COUNT + 1))
            fi

            TEST_COUNT=$((TEST_COUNT + 1))
        done

        echo ""
        echo "------------------"
        echo "PASSED: $SUCCESS_COUNT test(s)"
        echo "FAILED: $((TEST_COUNT-SUCCESS_COUNT)) test(s)"
        echo "------------------"
    fi
fi
