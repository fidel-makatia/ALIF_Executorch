# ExecuTorch for Alif E8 with Ethos-U55 NPU

## Package Contents

### Libraries (lib/)
- `libexecutorch_core.a` - Core ExecuTorch runtime (199KB)
- `libexecutorch.a` - ExecuTorch main library (65KB)  
- `libexecutorch_delegate_ethos_u.a` - Ethos-U NPU delegate (20KB)
- `libflatccrt.a` - FlatBuffer runtime (169KB)

**Total: ~460KB**

### Model
- `mnist_ethosu_arm_delegate_ethos-u55-128.pte` - MNIST model (113KB)
  - 100% NPU accelerated
  - Inference time: ~0.31ms per image

### Headers (include/)
- ExecuTorch runtime headers
- Kernel headers
- Extension headers

## Build Configuration

Target: ARM Cortex-M55 with Ethos-U55 NPU
Architecture: armv8.1-m.main
Float ABI: hard
C++ Standard: C++17

## Linker Flags
```
-lexecutorch_delegate_ethos_u
-lexecutorch
-lexecutorch_core
-lflatccrt
```

## Required Defines
```c
#define ET_MINIMAL 1
```

## Integration Notes

1. The model is fully delegated to Ethos-U55 NPU
2. No CPU portable kernels are needed
3. You need to provide Ethos-U driver for your platform
4. Memory allocation pools should be configured based on your memory map

## Memory Requirements

- Model size: 113KB (can be in flash/MRAM)
- Runtime memory: ~200KB recommended for inference buffers
- Stack: 8KB minimum recommended

## Example Usage

See `arm_executor_runner.cpp` for reference implementation.
