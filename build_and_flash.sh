#!/bin/bash

# Build and Flash RTT Hello World for Alif E8 DevKit
# This script automates the build, flash, and RTT test process

set -e  # Exit on error

echo "========================================="
echo "  Alif E8 RTT Hello World Build & Flash"
echo "========================================="

PROJECT_DIR="/Users/fidelmakatia/alif-e8-mnist-npu/alif_project"
CBUILD="/Users/fidelmakatia/cmsis-toolbox/bin/cbuild"
ELF_FILE="$PROJECT_DIR/out/blinky/E8-HE/debug/blinky.elf"

cd "$PROJECT_DIR"

# Step 1: Build the project
echo ""
echo "[1/4] Building project..."
echo "Running: $CBUILD alif.csolution.yml -c blinky.debug+E8-HE --rebuild"
$CBUILD alif.csolution.yml -c blinky.debug+E8-HE --rebuild

if [ $? -ne 0 ]; then
    echo "ERROR: Build failed with cbuild. Trying cbuild2cmake..."
    /Users/fidelmakatia/cmsis-toolbox/bin/cbuild2cmake alif.csolution.yml -c blinky.debug+E8-HE
    if [ $? -ne 0 ]; then
        echo "ERROR: cbuild2cmake failed"
        exit 1
    fi

    echo "Running cmake..."
    cmake -B out/blinky/E8-HE/debug -S out/blinky/E8-HE/debug
    if [ $? -ne 0 ]; then
        echo "ERROR: cmake failed"
        exit 1
    fi

    echo "Running ninja..."
    ninja -C out/blinky/E8-HE/debug
    if [ $? -ne 0 ]; then
        echo "ERROR: ninja failed"
        exit 1
    fi
fi

# Verify ELF file exists
if [ ! -f "$ELF_FILE" ]; then
    echo "ERROR: ELF file not found at: $ELF_FILE"
    exit 1
fi

echo "Build successful! ELF file: $ELF_FILE"

# Step 2: Flash the device
echo ""
echo "[2/4] Flashing device..."
echo "Using JLinkExe with script: flash_rtt.jlink"

# Check if JLinkExe is available
if ! command -v JLinkExe &> /dev/null; then
    echo "ERROR: JLinkExe not found in PATH"
    echo "Please install J-Link software or add JLinkExe to your PATH"
    exit 1
fi

JLinkExe -device Cortex-M55 -if SWD -speed 4000 -autoconnect 1 -CommandFile flash_rtt.jlink

if [ $? -ne 0 ]; then
    echo "ERROR: Flashing failed"
    exit 1
fi

echo "Flashing successful!"

# Step 3: Test RTT output
echo ""
echo "[3/4] Testing RTT output..."
echo "Starting JLinkRTTClient to capture RTT output..."
echo ""
echo "NOTE: You should see output like:"
echo "  ========================================"
echo "    RTT TEST - Hello World!"
echo "    Alif E8 HE Core"
echo "  ========================================"
echo "  [RTT] Hello World! Count: 1"
echo "  [RTT] Hello World! Count: 2"
echo "  ..."
echo ""
echo "Press Ctrl+C to stop RTT client"
echo ""

# Start JLinkRTTClient in background and capture output
if command -v JLinkRTTClient &> /dev/null; then
    timeout 10 JLinkRTTClient || true
else
    echo "WARNING: JLinkRTTClient not found. Please run it manually:"
    echo "  JLinkRTTClient"
    echo ""
    echo "Or connect with JLinkExe and reset:"
    echo "  JLinkExe -device Cortex-M55 -if SWD -speed 4000 -autoconnect 1"
    echo "  Then type: r (reset) and g (go)"
fi

echo ""
echo "[4/4] Complete!"
echo "========================================="
