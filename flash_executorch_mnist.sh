#!/bin/bash

# Flash ExecutorTorch MNIST NPU Demo for Alif E8 DevKit
# This script flashes the pre-built binary to the HE core

set -e  # Exit on error

echo "=========================================="
echo "  Flash ExecuTorch MNIST NPU Demo"
echo "  Alif E8 DevKit - HE Core"
echo "=========================================="

PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"
ELF_FILE="$PROJECT_DIR/out/executorch_mnist/E8-HE/debug/executorch_mnist.elf"
BIN_FILE="$PROJECT_DIR/out/executorch_mnist/E8-HE/debug/executorch_mnist.bin"

cd "$PROJECT_DIR"

# Check if files exist
if [ ! -f "$ELF_FILE" ]; then
    echo "ERROR: ELF file not found at: $ELF_FILE"
    echo "Please build the project first with:"
    echo "  cbuild alif.csolution.yml -c executorch_mnist.debug+E8-HE"
    exit 1
fi

echo "ELF file: $ELF_FILE"
echo "Size: $(ls -lh "$ELF_FILE" | awk '{print $5}')"
echo ""

# Check if JLinkExe is available
if ! command -v JLinkExe &> /dev/null; then
    echo "ERROR: JLinkExe not found in PATH"
    echo "Please install J-Link software or add JLinkExe to your PATH"
    echo ""
    echo "On macOS: brew install --cask segger-jlink"
    echo "Or download from: https://www.segger.com/downloads/jlink/"
    exit 1
fi

# Create JLink command script matching working blinky example
# Key: NO reset commands - just load, set PC/VTOR, and go
JLINK_SCRIPT="$PROJECT_DIR/tmp_flash_mnist.jlink"
cat > "$JLINK_SCRIPT" << EOF
loadfile $ELF_FILE
SetPC 0x800007C0
w4 0xE000ED08 0x80000000
g
exit
EOF

echo "[1/2] Flashing device via J-Link..."
echo "Device: Cortex-M55 (Generic)"
echo "Interface: SWD"
echo ""

JLinkExe -device Cortex-M55 -if SWD -speed 4000 -autoconnect 1 -CommandFile "$JLINK_SCRIPT"
FLASH_RESULT=$?

# Cleanup temp script
rm -f "$JLINK_SCRIPT"

if [ $FLASH_RESULT -ne 0 ]; then
    echo ""
    echo "ERROR: Flashing failed!"
    echo "Make sure:"
    echo "  1. J-Link is connected to the board"
    echo "  2. Board is powered on"
    echo "  3. SWD pins are connected correctly"
    exit 1
fi

echo ""
echo "[2/2] Flash complete!"
echo ""
echo "=========================================="
echo "  Next Steps:"
echo "=========================================="
echo ""
echo "1. Connect UART to see output:"
echo "   - UART4 TX: Pin P3_1 (or check board schematic)"
echo "   - Baud rate: 115200"
echo "   - Use: screen /dev/tty.usbserial-* 115200"
echo ""
echo "2. Or use J-Link RTT for debug output:"
echo "   JLinkRTTClient"
echo ""
echo "Expected output:"
echo "  ================================================"
echo "    ExecuTorch MNIST on Alif E8 + Ethos-U55 NPU"
echo "  ================================================"
echo "  [INIT] ExecuTorch runtime initialized"
echo "  [NPU] Initializing Ethos-U55..."
echo "  [LOAD] Loading MNIST model..."
echo "  [TEST] Running inference on sample digit '7'..."
echo "  PREDICTED DIGIT: 7"
echo ""
echo "=========================================="
