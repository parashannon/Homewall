#!/bin/bash

set -e

BIN="./1_23_2024.ino.bin"
USB_PATH="1-1.1"

echo "HomeWall Arduino uploader"
echo "-------------------------"

if [ ! -f "$BIN" ]; then
    echo "ERROR: Could not find $BIN"
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
    exit 1
fi

echo "Found Arduino at: $PORT"

echo
echo "Triggering 1200-baud bootloader reset..."

stty -F "$PORT" 1200

sleep 2

echo
echo "Waiting for Arduino bootloader..."

BOOT_PORT=""

for i in {1..20}; do
    BOOT_PORT=$(find_port || true)

    if [ -n "$BOOT_PORT" ]; then
        break
    fi

    sleep 0.5
done

if [ -z "$BOOT_PORT" ]; then
    echo "ERROR: Bootloader serial port did not appear."
    exit 1
fi

echo "Bootloader found at: $BOOT_PORT"

TTY_NAME=$(basename "$BOOT_PORT")

echo
echo "Uploading:"
echo "  $BIN"
echo "to:"
echo "  $BOOT_PORT"
echo

/usr/bin/bossac \
    --port="$TTY_NAME" \
    -i \
    -e \
    -w \
    -v \
    -R \
    "$BIN"

echo
echo "Upload complete."