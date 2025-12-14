# Quick Start - RTT Hello World Test

## 1. Build (Choose ONE method)

### Option A: Direct Build
```bash
cd /Users/fidelmakatia/alif-e8-mnist-npu/alif_project
/Users/fidelmakatia/cmsis-toolbox/bin/cbuild alif.csolution.yml -c blinky.debug+E8-HE --rebuild
```

### Option B: Automated Script
```bash
cd /Users/fidelmakatia/alif-e8-mnist-npu/alif_project
chmod +x build_and_flash.sh
./build_and_flash.sh
```

## 2. Flash

```bash
cd /Users/fidelmakatia/alif-e8-mnist-npu/alif_project
JLinkExe -device Cortex-M55 -if SWD -speed 4000 -autoconnect 1 -CommandFile flash_rtt.jlink
```

## 3. Test RTT

```bash
JLinkRTTClient
```

## Expected Output

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

## LED Indicators

- **White flash** (startup) - Board initialized
- **Green flash** - RTT ready
- **Blue blink** (1 Hz) - Running

## Need Help?

See detailed instructions: `RTT_TEST_INSTRUCTIONS.md`
