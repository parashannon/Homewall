#!/bin/bash

set -e

BIN="./1_23_2024.ino.bin"
USB_PATH="1-1.1"
FQBN="arduino:samd:nano_33_iot"

echo "HomeWall Arduino uploader"
echo "-------------------------"

if [ ! -f "$BIN" ]; then
    echo "ERROR: Could not find firmware file:"
    echo "  $BIN"
    exit 1
fi

find_port() {
    for dev in /dev/ttyACM*; do
        [ -e "$dev" ] || continue

        tty=$(basename "$dev")
        path=$(readlink -f "/sys/class/tty/$tty/device")

        if echo "$path" | grep -q "/$USB_PATH/"; then
            echo "$dev"
            return 0
        fi
    done

    return 1
}

echo "Looking for Arduino on physical USB path $USB_PATH..."

PORT=$(find_port || true)

if [ -z "$PORT" ]; then
    echo "ERROR: Could not find Arduino on USB path $USB_PATH"
    echo
    echo "Current ACM devices:"
    for dev in /dev/ttyACM*; do
        [ -e "$dev" ] || continue
        tty=$(basename "$dev")
        echo "$dev -> $(readlink -f "/sys/class/tty/$tty/device")"
    done
    exit 1
fi

echo "Found HomeWall Arduino at:"
echo "  $PORT"
echo
echo "Firmware:"
echo "  $BIN"
echo
echo "Starting upload..."
echo

arduino-cli upload \
    --fqbn "$FQBN" \
    --port "$PORT" \
    --input-file "$BIN" \
    --verbose

echo
echo "-------------------------"
echo "Upload complete."