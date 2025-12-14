# RTT Hello World Test - Alif E8 DevKit

## Overview
This guide provides step-by-step instructions to build, flash, and test the RTT Hello World application on the Alif E8 DevKit.

## Project Files

### Main Application
- **Location**: `/Users/fidelmakatia/alif-e8-mnist-npu/alif_project/blinky/main.c`
- **Description**: RTT Hello World test with LED indicators
- **Features**:
  - Initializes SEGGER RTT for debug output
  - Sends "Hello World" banner on startup
  - Sends periodic counter messages every second
  - LED indicators:
    - White flash on startup (all LEDs)
    - Green flash after RTT init
    - Blue LED blinks every second

### Project Configuration
- **Location**: `/Users/fidelmakatia/alif-e8-mnist-npu/alif_project/blinky/blinky.cproject.yml`
- **RTT Library Files**:
  - `../libs/SEGGER_RTT_V796h/RTT/SEGGER_RTT.c`
  - `../libs/SEGGER_RTT_V796h/RTT/SEGGER_RTT_printf.c`
  - `../libs/SEGGER_RTT_V796h/Syscalls/SEGGER_RTT_Syscalls_GCC.c`
- **Include Paths**:
  - `../libs/SEGGER_RTT_V796h/RTT`
  - `../libs/SEGGER_RTT_V796h/Config`

## Build Instructions

### Method 1: Using cbuild (Recommended)

```bash
cd /Users/fidelmakatia/alif-e8-mnist-npu/alif_project
/Users/fidelmakatia/cmsis-toolbox/bin/cbuild alif.csolution.yml -c blinky.debug+E8-HE --rebuild
```

### Method 2: Using cbuild2cmake + CMake + Ninja

If cbuild fails, try this alternative:

```bash
cd /Users/fidelmakatia/alif-e8-mnist-npu/alif_project

# Step 1: Convert to CMake
/Users/fidelmakatia/cmsis-toolbox/bin/cbuild2cmake alif.csolution.yml -c blinky.debug+E8-HE

# Step 2: Run CMake
cmake -B out/blinky/E8-HE/debug -S out/blinky/E8-HE/debug

# Step 3: Build with Ninja
ninja -C out/blinky/E8-HE/debug
```

### Verify Build Success

Check that the ELF file was created:
```bash
ls -lh /Users/fidelmakatia/alif-e8-mnist-npu/alif_project/out/blinky/E8-HE/debug/blinky.elf
```

## Flash Instructions

### Automatic Flashing (Using JLink Script)

A JLink command file has been created at: `/Users/fidelmakatia/alif-e8-mnist-npu/alif_project/flash_rtt.jlink`

**Flash Script Contents:**
```
r                   # Reset
h                   # Halt
loadfile /Users/fidelmakatia/alif-e8-mnist-npu/alif_project/out/blinky/E8-HE/debug/blinky.elf
SetPC 0x800007C0    # Set program counter
w4 0xE000ED08 0x80000000  # Set vector table offset
g                   # Go (run)
exit                # Exit JLinkExe
```

**Execute Flashing:**
```bash
cd /Users/fidelmakatia/alif-e8-mnist-npu/alif_project
JLinkExe -device Cortex-M55 -if SWD -speed 4000 -autoconnect 1 -CommandFile flash_rtt.jlink
```

### Manual Flashing (Interactive JLinkExe)

```bash
JLinkExe -device Cortex-M55 -if SWD -speed 4000 -autoconnect 1
```

Then enter these commands in JLinkExe:
```
r
h
loadfile /Users/fidelmakatia/alif-e8-mnist-npu/alif_project/out/blinky/E8-HE/debug/blinky.elf
SetPC 0x800007C0
w4 0xE000ED08 0x80000000
g
exit
```

## RTT Output Testing

### Expected Output

When the application runs successfully, you should see:

```
========================================
  RTT TEST - Hello World!
  Alif E8 HE Core
========================================
[RTT] Hello World! Count: 1
[RTT] Hello World! Count: 2
[RTT] Hello World! Count: 3
...
```

### Method 1: Using JLinkRTTClient (Easiest)

```bash
JLinkRTTClient
```

This will automatically connect and display RTT output in real-time.

### Method 2: Using JLinkExe with RTT Viewer

1. Start JLinkExe in one terminal:
```bash
JLinkExe -device Cortex-M55 -if SWD -speed 4000 -autoconnect 1
```

2. In JLinkExe, reset the device:
```
r
g
```

3. In another terminal, start JLinkRTTClient:
```bash
JLinkRTTClient
```

### Method 3: Using J-Link RTT Viewer (GUI)

If you have the J-Link software installed with GUI tools:
1. Open "J-Link RTT Viewer"
2. Select Device: Cortex-M55
3. Select Interface: SWD
4. Click "OK"
5. Reset the device if needed

## LED Indicators

The application provides visual feedback through the RGB LED:

| LED State | Meaning |
|-----------|---------|
| White flash (500ms) on startup | Application started, board initialized |
| Green flash (500ms) | RTT initialized successfully |
| Blue blinking (1 Hz) | Application running, sending RTT messages |

## Troubleshooting

### Build Fails
- Verify CMSIS toolbox is installed at `/Users/fidelmakatia/cmsis-toolbox/`
- Check that RTT library files exist in `libs/SEGGER_RTT_V796h/`
- Ensure DFP (Device Family Pack) is installed: AlifSemiconductor::Ensemble@2.0.4

### Flash Fails
- Verify J-Link is connected to the E8 DevKit
- Check USB connection
- Try power cycling the board
- Verify J-Link software is installed

### No RTT Output
- Verify the device was flashed successfully
- Reset the device: In JLinkExe, type `r` then `g`
- Check that JLinkRTTClient is connecting to the right device
- Verify RTT buffer address matches (SEGGER_RTT_Init should auto-detect)

### Device Not Responding
- Power cycle the board
- Check JTAG/SWD connections
- Verify correct device is selected (Cortex-M55)
- Try reducing SWD speed: `-speed 1000` instead of `-speed 4000`

## Automation Scripts

### Bash Script
Execute all steps automatically:
```bash
chmod +x /Users/fidelmakatia/alif-e8-mnist-npu/alif_project/build_and_flash.sh
/Users/fidelmakatia/alif-e8-mnist-npu/alif_project/build_and_flash.sh
```

### Python Script
Alternative automation:
```bash
chmod +x /Users/fidelmakatia/alif-e8-mnist-npu/alif_project/build_flash_test.py
python3 /Users/fidelmakatia/alif-e8-mnist-npu/alif_project/build_flash_test.py
```

## Files Created

The following helper files have been created:

1. **flash_rtt.jlink** - JLink script for flashing
2. **connect_rtt.jlink** - JLink script for RTT connection
3. **build_and_flash.sh** - Bash automation script
4. **build_flash_test.py** - Python automation script
5. **RTT_TEST_INSTRUCTIONS.md** - This documentation

## Quick Start Command Summary

```bash
# Navigate to project
cd /Users/fidelmakatia/alif-e8-mnist-npu/alif_project

# Build
/Users/fidelmakatia/cmsis-toolbox/bin/cbuild alif.csolution.yml -c blinky.debug+E8-HE --rebuild

# Flash
JLinkExe -device Cortex-M55 -if SWD -speed 4000 -autoconnect 1 -CommandFile flash_rtt.jlink

# View RTT Output
JLinkRTTClient
```

## Additional Notes

- The ELF file location: `/Users/fidelmakatia/alif-e8-mnist-npu/alif_project/out/blinky/E8-HE/debug/blinky.elf`
- Target Device: AE822FA0E5597LS0 (Alif E8) - Cortex-M55 HE Core
- Debug Interface: SWD at 4000 kHz
- RTT Buffer: Auto-detected by SEGGER_RTT_Init()
- Message Rate: 1 message per second

## References

- SEGGER RTT Documentation: https://www.segger.com/products/debug-probes/j-link/technology/about-real-time-transfer/
- Alif Semiconductor DevKit: https://alifsemi.com/
- CMSIS-Toolbox: https://github.com/Open-CMSIS-Pack/cmsis-toolbox
