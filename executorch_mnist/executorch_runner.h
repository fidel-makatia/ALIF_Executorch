/**
 * @file executorch_runner.h
 * @brief ExecuTorch C API for Alif E8
 */

#ifndef EXECUTORCH_RUNNER_H
#define EXECUTORCH_RUNNER_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize ExecuTorch with a model
 * @param model_data Pointer to .pte model data
 * @param model_size Size of model in bytes
 * @return 0 on success, negative on error
 */
int executorch_init(const uint8_t* model_data, size_t model_size);

/**
 * Run inference
 * @param input_data Input tensor data (int8_t for quantized)
 * @param input_size Size of input in bytes
 * @param output_data Output buffer for scores
 * @param output_size Size of output buffer
 * @return 0 on success, negative on error
 */
int executorch_run_inference(const int8_t* input_data, size_t input_size,
                              int8_t* output_data, size_t output_size);

/**
 * Cleanup ExecuTorch
 */
void executorch_deinit(void);

#ifdef __cplusplus
}
#endif

#endif /* EXECUTORCH_RUNNER_H */
