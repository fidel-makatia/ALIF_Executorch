#!/usr/bin/env python3
"""
Build, Flash, and Test RTT Hello World for Alif E8 DevKit
"""

import subprocess
import sys
import os
import time

PROJECT_DIR = "/Users/fidelmakatia/alif-e8-mnist-npu/alif_project"
CBUILD = "/Users/fidelmakatia/cmsis-toolbox/bin/cbuild"
ELF_FILE = os.path.join(PROJECT_DIR, "out/blinky/E8-HE/debug/blinky.elf")
FLASH_SCRIPT = os.path.join(PROJECT_DIR, "flash_rtt.jlink")

def run_command(cmd, description, check=True):
    """Run a shell command and print output"""
    print(f"\n{'='*60}")
    print(f"  {description}")
    print(f"{'='*60}")
    print(f"Command: {' '.join(cmd) if isinstance(cmd, list) else cmd}")
    print()

    try:
        result = subprocess.run(
            cmd,
            cwd=PROJECT_DIR,
            capture_output=True,
            text=True,
            check=check
        )
        if result.stdout:
            print(result.stdout)
        if result.returncode != 0 and result.stderr:
            print("STDERR:", result.stderr)
        return result
    except subprocess.CalledProcessError as e:
        print(f"ERROR: Command failed with exit code {e.returncode}")
        if e.stdout:
            print("STDOUT:", e.stdout)
        if e.stderr:
            print("STDERR:", e.stderr)
        if check:
            raise
        return e

def build_project():
    """Build the project using cbuild"""
    print("\n" + "="*60)
    print("  STEP 1: Building Project")
    print("="*60)

    # Try cbuild first
    cmd = [CBUILD, "alif.csolution.yml", "-c", "blinky.debug+E8-HE", "--rebuild"]
    result = run_command(cmd, "Building with cbuild", check=False)

    if result.returncode != 0:
        print("\ncbuild failed. Trying cbuild2cmake approach...")

        # Try cbuild2cmake
        cmd = ["/Users/fidelmakatia/cmsis-toolbox/bin/cbuild2cmake",
               "alif.csolution.yml", "-c", "blinky.debug+E8-HE"]
        result = run_command(cmd, "Converting to CMake", check=False)

        if result.returncode != 0:
            print("ERROR: cbuild2cmake also failed")
            return False

        # Run cmake
        build_dir = os.path.join(PROJECT_DIR, "out/blinky/E8-HE/debug")
        cmd = ["cmake", "-B", build_dir, "-S", build_dir]
        result = run_command(cmd, "Running CMake", check=False)

        if result.returncode != 0:
            print("ERROR: cmake failed")
            return False

        # Run ninja
        cmd = ["ninja", "-C", build_dir]
        result = run_command(cmd, "Running Ninja", check=False)

        if result.returncode != 0:
            print("ERROR: ninja failed")
            return False

    # Verify ELF file exists
    if not os.path.exists(ELF_FILE):
        print(f"\nERROR: ELF file not found at: {ELF_FILE}")
        return False

    print(f"\nSUCCESS: Build complete!")
    print(f"ELF file: {ELF_FILE}")
    return True

def flash_device():
    """Flash the device using JLinkExe"""
    print("\n" + "="*60)
    print("  STEP 2: Flashing Device")
    print("="*60)

    # Check if JLinkExe is available
    try:
        subprocess.run(["which", "JLinkExe"], capture_output=True, check=True)
    except subprocess.CalledProcessError:
        print("ERROR: JLinkExe not found in PATH")
        print("Please install J-Link software or add JLinkExe to your PATH")
        return False

    # Flash the device
    cmd = [
        "JLinkExe",
        "-device", "Cortex-M55",
        "-if", "SWD",
        "-speed", "4000",
        "-autoconnect", "1",
        "-CommandFile", FLASH_SCRIPT
    ]

    result = run_command(cmd, "Flashing with JLinkExe", check=False)

    if result.returncode != 0:
        print("ERROR: Flashing failed")
        return False

    print("\nSUCCESS: Device flashed!")
    return True

def test_rtt():
    """Test RTT output"""
    print("\n" + "="*60)
    print("  STEP 3: Testing RTT Output")
    print("="*60)

    print("\nExpected RTT output:")
    print("  ========================================")
    print("    RTT TEST - Hello World!")
    print("    Alif E8 HE Core")
    print("  ========================================")
    print("  [RTT] Hello World! Count: 1")
    print("  [RTT] Hello World! Count: 2")
    print("  ...")
    print()

    # Check if JLinkRTTClient is available
    try:
        subprocess.run(["which", "JLinkRTTClient"], capture_output=True, check=True)

        print("Starting JLinkRTTClient (will run for 10 seconds)...")
        print("Press Ctrl+C to stop earlier")
        print("-" * 60)

        try:
            subprocess.run(["JLinkRTTClient"], timeout=10)
        except subprocess.TimeoutExpired:
            print("-" * 60)
            print("RTT capture complete")
        except KeyboardInterrupt:
            print("\nRTT capture stopped by user")

    except subprocess.CalledProcessError:
        print("WARNING: JLinkRTTClient not found")
        print("\nTo test RTT manually:")
        print("  1. Run: JLinkRTTClient")
        print("  2. Or connect with JLinkExe:")
        print("     JLinkExe -device Cortex-M55 -if SWD -speed 4000 -autoconnect 1")
        print("     Then type: r (reset) and g (go)")
        return False

    return True

def main():
    """Main execution"""
    print("="*60)
    print("  Alif E8 RTT Hello World - Build, Flash & Test")
    print("="*60)

    # Step 1: Build
    if not build_project():
        print("\nBuild failed. Exiting.")
        sys.exit(1)

    # Step 2: Flash
    if not flash_device():
        print("\nFlashing failed. Exiting.")
        sys.exit(1)

    # Step 3: Test RTT
    test_rtt()

    print("\n" + "="*60)
    print("  Complete!")
    print("="*60)

if __name__ == "__main__":
    main()
