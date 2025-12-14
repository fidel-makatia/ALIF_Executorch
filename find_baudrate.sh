#!/bin/bash
# Script to find the correct baud rate for USB-TTL serial connection

SERIAL_PORT="/dev/cu.usbserial-BG01OWHR"

# Common baud rates to try
BAUD_RATES=(
    300
    600
    1200
    2400
    4800
    9600
    14400
    19200
    28800
    38400
    57600
    76800
    115200
    230400
    250000
    460800
    500000
    576000
    921600
    1000000
    1500000
    2000000
)

# Kill any existing processes using the port
cleanup() {
    lsof "$SERIAL_PORT" 2>/dev/null | awk 'NR>1 {print $2}' | xargs kill -9 2>/dev/null
    sleep 0.5
}

echo "========================================"
echo "  Baud Rate Finder for Alif E8 UART"
echo "========================================"
echo ""
echo "Serial port: $SERIAL_PORT"
echo ""
echo "Instructions:"
echo "  - Watch each baud rate test for 3 seconds"
echo "  - Look for readable ASCII text like 'MNIST', 'LED', 'UART'"
echo "  - Press Ctrl+C to stop when you find the correct rate"
echo ""
echo "Press ENTER to start..."
read

for baud in "${BAUD_RATES[@]}"; do
    cleanup

    echo ""
    echo "========================================"
    echo "  Testing: $baud baud"
    echo "========================================"

    # Configure serial port
    stty -f "$SERIAL_PORT" $baud cs8 -cstopb -parenb raw 2>/dev/null

    if [ $? -ne 0 ]; then
        echo "  [SKIP] Could not set baud rate $baud"
        continue
    fi

    echo "  Reading for 3 seconds... (look for readable text)"
    echo "  ----------------------------------------"

    # Read serial data for 3 seconds with timeout
    timeout 3 cat "$SERIAL_PORT" 2>/dev/null &
    CAT_PID=$!

    sleep 3
    kill $CAT_PID 2>/dev/null

    echo ""
    echo "  ----------------------------------------"
    echo ""
    echo "  Was the text readable? (y/n/q to quit)"
    read -t 5 answer

    if [ "$answer" = "y" ] || [ "$answer" = "Y" ]; then
        echo ""
        echo "========================================"
        echo "  SUCCESS! Correct baud rate: $baud"
        echo "========================================"
        echo ""
        echo "Use this command to connect:"
        echo "  screen $SERIAL_PORT $baud"
        echo ""
        cleanup
        exit 0
    elif [ "$answer" = "q" ] || [ "$answer" = "Q" ]; then
        echo "Quitting..."
        cleanup
        exit 0
    fi
done

echo ""
echo "========================================"
echo "  No matching baud rate found"
echo "========================================"
echo ""
echo "Possible issues:"
echo "  1. Wiring might be incorrect"
echo "  2. Voltage level mismatch (must be 1.8V)"
echo "  3. Board might not be running"
echo ""

cleanup
