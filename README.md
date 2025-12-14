# ALIF ExecuTorch - MNIST NPU Inference Demo

[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](https://opensource.org/licenses/Apache-2.0)
[![Platform](https://img.shields.io/badge/Platform-Alif%20Ensemble-green.svg)](https://alifsemi.com/)
[![NPU](https://img.shields.io/badge/NPU-Ethos--U55-orange.svg)](https://www.arm.com/products/silicon-ip-cpu/ethos/ethos-u55)

A production-ready **MNIST handwritten digit recognition** demo running on **Alif Ensemble DevKits** with **ARM Ethos-U55 NPU** acceleration using **Meta's ExecuTorch** runtime.

---

## Table of Contents

- [Overview](#overview)
- [Key Features](#key-features)
- [Hardware Requirements](#hardware-requirements)
- [Software Requirements](#software-requirements)
- [Project Structure](#project-structure)
- [Quick Start](#quick-start)
- [Building the Project](#building-the-project)
- [Flashing the Device](#flashing-the-device)
- [Understanding the Output](#understanding-the-output)
- [Technical Details](#technical-details)
- [Supported Boards](#supported-boards)
- [Troubleshooting](#troubleshooting)
- [References](#references)
- [License](#license)

---

## Overview

This project demonstrates **on-device machine learning inference** on the Alif Ensemble microcontroller family using:

- **ExecuTorch Runtime**: Meta's lightweight, portable ML inference framework optimized for edge devices
- **ARM Ethos-U55 NPU**: Dedicated neural processing unit for efficient ML inference
- **Quantized INT8 Model**: Memory-efficient MNIST classifier (~111KB .pte model)

The demo classifies a hardcoded test digit "7" and displays the prediction result through RGB LED blinks.

### Architecture Diagram

```
+------------------------+     +-------------------+     +------------------+
|   ExecuTorch Runtime   | --> |   Ethos-U55 NPU   | --> |   INT8 Output    |
|   (Program Loader)     |     |   (128 MACs/cyc)  |     |   (Digit 0-9)    |
+------------------------+     +-------------------+     +------------------+
         |                              |
         v                              v
+------------------------+     +-------------------+
|   .pte Model File      |     |   SRAM0 Memory    |
|   (MRAM Storage)       |     |   (Scratch Space) |
+------------------------+     +-------------------+
```

---

## Key Features

| Feature | Description |
|---------|-------------|
| **ExecuTorch Integration** | Full ExecuTorch runtime with ARM delegate support |
| **NPU Acceleration** | Ethos-U55 hardware acceleration for CNN layers |
| **Quantized Model** | INT8 quantized MNIST model for minimal memory footprint |
| **Multi-Board Support** | Works on E7, E8, and E1C DevKits |
| **LED Status Indicators** | Visual feedback for initialization and inference stages |
| **CMSIS-Toolbox Build** | Modern, reproducible build system |
| **J-Link Flashing** | Easy deployment via SEGGER J-Link |

### LED Status Codes

| LED Color | Pattern | Meaning |
|-----------|---------|---------|
| RED | Solid (500ms) | System starting |
| YELLOW (R+G) | Flash | SRAM0 power enabled |
| GREEN | Flash | SRAM0 memory test passed |
| BLUE | Flash | NPU initialized |
| CYAN (G+B) | Flash | ExecuTorch runtime ready |
| CYAN (G+B) | Solid (500ms) | Model loaded successfully |
| YELLOW (R+G) | Solid (500ms) | Method loaded, ready for inference |
| GREEN | N blinks | Predicted digit (N = digit value) |
| WHITE (R+G+B) | Solid | Inference complete, success! |
| RED | Fast blink | Error occurred |

---

## Hardware Requirements

### Supported Development Kits

| Board | Device | Core | NPU |
|-------|--------|------|-----|
| **DevKit-E8** (Recommended) | AE822FA0E5597LS0 | Dual Cortex-M55 | Ethos-U55 |
| DevKit-E7 | AE722F80F55D5LS | Dual Cortex-M55 | Ethos-U55 |
| DevKit-E1C | AE1C1F4051920PH | Cortex-M55 | Ethos-U55 |

### Required Connections

1. **J-Link Debugger**: Connect via the **PRG USB** port (not DBG USB)
2. **Power**: USB provides power, or use external 5V supply

### Optional: UART Debug Output

For debug messages, connect a **1.8V logic level** USB-to-TTL converter:

| USB-TTL Pin | J8 Header Pin | Signal |
|-------------|---------------|--------|
| GND | Pin 1 | Ground |
| RXD | Pin 12 | P1_1 (UART_TX) |
| TXD | Pin 14 | P1_0 (UART_RX) |

> **Warning**: The Alif Ensemble uses **1.8V I/O logic levels**. Using a 3.3V or 5V USB-TTL converter may damage the board.

---

## Software Requirements

### 1. ARM GCC Toolchain

```bash
# macOS (Homebrew)
brew install arm-none-eabi-gcc

# Ubuntu/Debian
sudo apt-get install gcc-arm-none-eabi

# Windows
# Download from: https://developer.arm.com/downloads/-/gnu-rm

# Verify installation
arm-none-eabi-gcc --version
```

**Required Version**: 10.3 or later (C++17 support required)

### 2. CMSIS-Toolbox

Download from [ARM CMSIS-Toolbox Releases](https://github.com/Open-CMSIS-Pack/cmsis-toolbox/releases)

```bash
# Extract and add to PATH
export PATH=$PATH:~/cmsis-toolbox/bin

# Verify installation
cbuild --version
```

**Required Version**: 2.6.0 or later

### 3. SEGGER J-Link

Download from [SEGGER Downloads](https://www.segger.com/downloads/jlink/)

```bash
# macOS (Homebrew)
brew install --cask segger-jlink

# Verify installation
JLinkExe --version
```

### 4. Alif Ensemble CMSIS Pack

The pack will be downloaded automatically during the first build, or install manually:

```bash
cpackget add AlifSemiconductor::Ensemble@2.0.4
cpackget add ARM::CMSIS@6.0.0
```

---

## Project Structure

```
alif_project/
├── executorch_mnist/           # Main MNIST NPU demo
│   ├── main_npu.cpp            # Application entry point
│   ├── model_pte.h             # Embedded model data (~700KB)
│   ├── executorch_mnist.cproject.yml  # Project configuration
│   ├── executorch/             # ExecuTorch headers
│   │   └── include/            # Backend and runtime headers
│   └── executorch_npu/         # NPU-specific components
│       ├── include/            # ExecuTorch include files
│       └── lib/                # Pre-built libraries
│           ├── libexecutorch_core.a          # Core runtime (~203KB)
│           ├── libexecutorch.a               # Main library (~66KB)
│           ├── libexecutorch_delegate_ethos_u.a  # NPU delegate (~20KB)
│           └── libflatccrt.a                 # FlatBuffers runtime (~173KB)
│
├── device/                     # Device configuration
│   └── ensemble/               # Alif Ensemble specific
│       ├── alif-ensemble.clayer.yml
│       └── RTE/                # Runtime Environment config
│
├── blinky/                     # Simple LED blink demo
├── hello/                      # Hello World UART demo
├── hello_rtt/                  # RTT debug demo
│
├── alif.csolution.yml          # Main solution file
├── cdefault.yml                # Default compiler settings
├── flash_executorch_mnist.sh   # Flash script
├── flash_npu.jlink             # J-Link command file
│
└── out/                        # Build output directory
    └── executorch_mnist/
        └── E8-HE/
            └── debug/
                ├── executorch_mnist.elf
                └── executorch_mnist.bin
```

---

## Quick Start

```bash
# 1. Clone the repository
git clone https://github.com/fidel-makatia/ALIF_Executorch.git
cd ALIF_Executorch

# 2. Build the project
cbuild alif.csolution.yml -c executorch_mnist.debug+E8-HE

# 3. Flash to device
./flash_executorch_mnist.sh

# 4. Watch the LEDs!
# - Green blinks indicate the predicted digit
# - White LED = success
```

---

## Building the Project

### Build Commands

```bash
# Build for Alif E8 DevKit (HE core) - Recommended
cbuild alif.csolution.yml -c executorch_mnist.debug+E8-HE

# Build for Alif E7 DevKit (HE core)
cbuild alif.csolution.yml -c executorch_mnist.debug+E7-HE

# Build for Alif E7 DevKit (HP core)
cbuild alif.csolution.yml -c executorch_mnist.debug+E7-HP

# Clean build
rm -rf out/ tmp/
cbuild alif.csolution.yml -c executorch_mnist.debug+E8-HE
```

### Expected Build Output

```
info csolution: valid solution created
Building context: "executorch_mnist.debug+E8-HE"
...
Linking...
Program Size: Code=XXXXX RO-data=XXXXX RW-data=XXXXX ZI-data=XXXXX
"./out/executorch_mnist/E8-HE/debug/executorch_mnist.elf" - 0 Error(s), 0 Warning(s).
Build summary: 1 succeeded, 0 failed
```

### Build Configuration Options

The project supports multiple build configurations:

| Configuration | Target | Core | Use Case |
|---------------|--------|------|----------|
| `debug+E8-HE` | DevKit-E8 | M55 HE | Development (recommended) |
| `debug+E8-HP` | DevKit-E8 | M55 HP | High performance |
| `debug+E7-HE` | DevKit-E7 | M55 HE | E7 development |
| `debug+E7-HP` | DevKit-E7 | M55 HP | E7 high performance |
| `release+E8-HE` | DevKit-E8 | M55 HE | Optimized release |

---

## Flashing the Device

### Method 1: Using the Flash Script (Recommended)

```bash
./flash_executorch_mnist.sh
```

### Method 2: Manual J-Link Commands

```bash
JLinkExe -device Cortex-M55 -if SWD -speed 4000 -autoconnect 1 -CommandFile flash_npu.jlink
```

### J-Link Script Contents

```jlink
loadfile out/executorch_mnist/E8-HE/debug/executorch_mnist.elf
SetPC 0x800007C0
w4 0xE000ED08 0x80000000
g
exit
```

**Key Parameters**:
- `SetPC 0x800007C0`: Sets program counter to MRAM entry point
- `w4 0xE000ED08 0x80000000`: Sets VTOR (Vector Table Offset Register) to MRAM base
- `g`: Starts execution

---

## Understanding the Output

### LED Sequence for Successful Inference

1. **RED** (500ms) - System initialization
2. **YELLOW** (200ms) - SRAM0 power enabled
3. **GREEN** (200ms) - Memory test passed
4. **BLUE** (300ms) - Ethos-U55 NPU initialized
5. **CYAN** (300ms) - ExecuTorch runtime initialized
6. **CYAN** (500ms) - Model loaded
7. **YELLOW** (500ms) - Method loaded
8. **GREEN blinks** - Shows predicted digit (7 blinks for digit "7")
9. **WHITE** (solid) - Inference complete!

### Interpreting Results

The demo runs inference on a hardcoded test image of digit "7". After successful inference:
- The GREEN LED blinks **N+1 times** where N is the predicted digit
- For digit "7", you'll see **8 green blinks**
- Then the LED turns solid WHITE

---

## Technical Details

### ExecuTorch Runtime

ExecuTorch is Meta's on-device inference framework designed for:
- **Minimal footprint**: Core runtime under 50KB
- **Portable**: Runs on MCUs to mobile devices
- **Delegate support**: Hardware acceleration via backends

### Memory Layout

| Region | Address | Size | Usage |
|--------|---------|------|-------|
| MRAM | 0x80000000 | 5.5 MB | Code + Model storage |
| SRAM0 | 0x02000000 | 4 MB | Runtime scratch space |
| ITCM | 0x00000000 | 256 KB | Fast code (TCM) |
| DTCM | 0x20000000 | 256 KB | Fast data (TCM) |

### Memory Allocation

```cpp
// Method allocator: 512KB for model loading
#define METHOD_ALLOCATOR_POOL_SIZE (512 * 1024)

// Temp allocator: 256KB for inference scratch
#define TEMP_ALLOCATOR_POOL_SIZE   (256 * 1024)
```

### Ethos-U55 NPU Specifications

| Specification | Value |
|---------------|-------|
| Architecture | Ethos-U55 |
| MAC Units | 128 MACs/cycle |
| Memory Interface | AXI |
| Supported Ops | Conv2D, DepthwiseConv, Pooling, FC, etc. |
| Quantization | INT8, INT16 |

### Model Details

| Property | Value |
|----------|-------|
| Model Type | MNIST Classifier |
| Input Shape | 1x1x28x28 (INT8) |
| Output Shape | 1x10 (INT8 logits) |
| Format | ExecuTorch .pte (FlatBuffer) |
| Delegate | Ethos-U ARM Delegate |
| Size | ~111 KB (quantized) |

---

## Supported Boards

### DevKit-E8 (AE822FA0E5597LS0)

| Feature | Specification |
|---------|---------------|
| Cores | Dual Cortex-M55 (HE + HP) |
| NPU | Ethos-U55 (128 MACs) |
| MRAM | 5.5 MB |
| SRAM | 9.75 MB total |
| Package | BGA194 |

### DevKit-E7 (AE722F80F55D5LS)

| Feature | Specification |
|---------|---------------|
| Cores | Dual Cortex-M55 (HE + HP) |
| NPU | Ethos-U55 (128 MACs) |
| MRAM | 2 MB |
| SRAM | 4.5 MB total |
| Package | BGA |

### DevKit-E1C (AE1C1F4051920PH)

| Feature | Specification |
|---------|---------------|
| Cores | Cortex-M55 (HE) |
| NPU | Ethos-U55 |
| MRAM | 1 MB |
| SRAM | 2 MB |
| Package | QFN |

---

## Troubleshooting

### Build Issues

**Error: "CMSIS pack not found"**
```bash
# Install required packs
cpackget add AlifSemiconductor::Ensemble@2.0.4
cpackget add ARM::CMSIS@6.0.0
```

**Error: "arm-none-eabi-gcc not found"**
```bash
# Verify toolchain is in PATH
which arm-none-eabi-gcc
# If not found, add to PATH or reinstall
```

**Error: "C++17 features not supported"**
```bash
# Ensure GCC version is 10.3 or later
arm-none-eabi-gcc --version
```

### Flashing Issues

**Error: "J-Link cannot connect"**
- Ensure you're connected to the **PRG USB** port (not DBG USB)
- Press the RESET button on the board
- Close any other J-Link sessions
- Try reducing SWD speed: `-speed 1000`

**Error: "ELF file not found"**
- Build the project first: `cbuild alif.csolution.yml -c executorch_mnist.debug+E8-HE`
- Check the output path in `flash_executorch_mnist.sh`

### Runtime Issues

**LED shows fast RED blink (error)**
- SRAM0 power not enabled: Check Secure Enclave services
- NPU initialization failed: Verify NPU base address
- Model load failed: Check model data integrity
- Memory allocation failed: Increase allocator pool sizes

**No LED activity after flash**
- Verify VTOR is set correctly: `w4 0xE000ED08 0x80000000`
- Check entry point address: `SetPC 0x800007C0`
- Try power cycling the board

**Wrong prediction**
- The demo uses a hardcoded "7" test pattern
- Verify model file integrity
- Check input data quantization

---

## References

### Documentation

- [ExecuTorch Documentation](https://pytorch.org/executorch/)
- [Alif Semiconductor](https://alifsemi.com/)
- [ARM Ethos-U55 Documentation](https://developer.arm.com/Processors/Ethos-U55)
- [CMSIS-Toolbox User Guide](https://github.com/Open-CMSIS-Pack/cmsis-toolbox)

### Related Projects

- [Alif VSCode Template](https://github.com/alifsemi/alif_vscode-template)
- [ExecuTorch ARM Examples](https://github.com/pytorch/executorch/tree/main/examples/arm)

### Tools

- [SEGGER J-Link](https://www.segger.com/products/debug-probes/j-link/)
- [ARM GCC Toolchain](https://developer.arm.com/downloads/-/gnu-rm)

---

## License

This project is licensed under the Apache License 2.0 - see the [LICENSE](LICENSE) file for details.

### Third-Party Licenses

- **ExecuTorch**: BSD License (Meta)
- **Ethos-U Driver**: Apache 2.0 (ARM)
- **CMSIS**: Apache 2.0 (ARM)
- **Alif Ensemble Pack**: Alif Semiconductor License

---

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

1. Fork the repository
2. Create your feature branch (`git checkout -b feature/AmazingFeature`)
3. Commit your changes (`git commit -m 'Add some AmazingFeature'`)
4. Push to the branch (`git push origin feature/AmazingFeature`)
5. Open a Pull Request

---

## Author

**Fidel Makatia**

- GitHub: [@fidel-makatia](https://github.com/fidel-makatia)

---

## Acknowledgments

- Alif Semiconductor for the Ensemble DevKit and documentation
- Meta AI for ExecuTorch framework
- ARM for Ethos-U NPU and CMSIS ecosystem
- SEGGER for J-Link debugging tools
