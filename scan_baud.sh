#!/bin/bash
# Interactive baud rate scanner using screen

SERIAL_PORT="/dev/cu.usbserial-BG01OWHR"
JLINK_SCRIPT="/Users/fidelmakatia/alif-e8-mnist-npu/alif_project/flash_mnist.jlink"

# Baud rates to try (focused on likely candidates)
BAUD_RATES=(
    57600
    115200
    128000
    230400
    250000
    256000
    460800
    500000
    576000
    921600
    1000000
)

cleanup() {
    pkill -9 screen 2>/dev/null
    lsof "$SERIAL_PORT" 2>/dev/null | awk 'NR>1 {print $2}' | xargs kill -9 2>/dev/null
    sleep 0.5
}

echo "========================================"
echo "  Interactive Baud Rate Scanner"
echo "========================================"
echo ""
echo "This will open screen at each baud rate."
echo "Look for readable text like 'MNIST' or 'UART'"
echo ""
echo "Controls inside screen:"
echo "  - Press board RESET to see startup message"
echo "  - Press Ctrl+A then K then Y to exit screen"
echo ""

# Reflash first
echo "Reflashing board..."
JLinkExe -device Cortex-M55 -if SWD -speed 4000 -autoconnect 1 -CommandFile "$JLINK_SCRIPT" 2>&1 | grep -E "(O.K.|Error)"
echo ""

for baud in "${BAUD_RATES[@]}"; do
    cleanup

    echo "========================================"
    echo "  Testing: $baud baud"
    echo "========================================"
    echo ""
    echo "Opening screen... (Ctrl+A, K, Y to exit)"
    echo "Press RESET on board to see output!"
    echo ""

    # Run screen - user will interact with it
    screen "$SERIAL_PORT" "$baud"

    echo ""
    echo "Was the text readable at $baud baud? (y/n/q)"
    read -r answer

    case "$answer" in
        y|Y)
            echo ""
            echo "========================================"
            echo "  SUCCESS! Correct baud rate: $baud"
            echo "========================================"
            echo ""
            echo "To connect in the future, use:"
            echo "  screen $SERIAL_PORT $baud"
            echo ""
            cleanup
            exit 0
            ;;
        q|Q)
            echo "Quitting..."
            cleanup
            exit 0
            ;;
        *)
            echo "Trying next baud rate..."
            # Reflash to restart output
            echo "Reflashing board..."
            JLinkExe -device Cortex-M55 -if SWD -speed 4000 -autoconnect 1 -CommandFile "$JLINK_SCRIPT" 2>&1 | grep "O.K."
            ;;
    esac
done

echo ""
echo "========================================"
echo "  No matching baud rate found"
echo "========================================"
cleanup
