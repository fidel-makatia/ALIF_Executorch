/**
 * @file executorch_runner.cpp
 * @brief Real ExecuTorch runtime wrapper for Alif E8
 */

#include "executorch_runner.h"

#include <executorch/runtime/executor/program.h>
#include <executorch/runtime/executor/method.h>
#include <executorch/runtime/platform/runtime.h>
#include <executorch/runtime/core/memory_allocator.h>
#include <executorch/runtime/executor/memory_manager.h>
#include <executorch/extension/data_loader/buffer_data_loader.h>

#include <cstring>
#include <cstdio>

using namespace executorch::runtime;
using namespace executorch::extension;

/* Memory pools */
static uint8_t method_allocator_pool[512 * 1024] __attribute__((aligned(16)));
static uint8_t planned_memory_pool[512 * 1024] __attribute__((aligned(16)));

/* Global state */
static bool g_initialized = false;
static Program* g_program = nullptr;
static Method* g_method = nullptr;

/* Static storage for objects */
static uint8_t program_storage[sizeof(Program)] __attribute__((aligned(8)));
static uint8_t method_storage[sizeof(Method)] __attribute__((aligned(8)));
static uint8_t loader_storage[sizeof(BufferDataLoader)] __attribute__((aligned(8)));
static uint8_t memory_manager_storage[sizeof(MemoryManager)] __attribute__((aligned(8)));
static uint8_t method_allocator_storage[sizeof(MemoryAllocator)] __attribute__((aligned(8)));
static uint8_t planned_allocator_storage[sizeof(HierarchicalAllocator)] __attribute__((aligned(8)));
static uint8_t span_storage[sizeof(Span<uint8_t>)] __attribute__((aligned(8)));

extern "C" {

int executorch_init(const uint8_t* model_data, size_t model_size)
{
    if (g_initialized) {
        return 0;
    }

    if (model_data == nullptr || model_size == 0) {
        printf("[ET] Error: Invalid model data\r\n");
        return -1;
    }

    printf("[ET] Initializing ExecuTorch runtime...\r\n");

    /* Initialize runtime */
    runtime_init();

    /* Create data loader */
    BufferDataLoader* loader = new (loader_storage) BufferDataLoader(model_data, model_size);

    /* Load program */
    Result<Program> program_result = Program::load(loader);
    if (!program_result.ok()) {
        printf("[ET] Failed to load program: %d\r\n", (int)program_result.error());
        return -2;
    }
    g_program = new (program_storage) Program(std::move(program_result.get()));
    printf("[ET] Program loaded successfully\r\n");

    /* Setup memory allocator */
    MemoryAllocator* method_allocator = new (method_allocator_storage) 
        MemoryAllocator(sizeof(method_allocator_pool), method_allocator_pool);

    /* Setup planned memory */
    Span<uint8_t>* planned_span = new (span_storage) 
        Span<uint8_t>(planned_memory_pool, sizeof(planned_memory_pool));
    
    HierarchicalAllocator* planned_allocator = new (planned_allocator_storage)
        HierarchicalAllocator({planned_span, 1});

    /* Create memory manager */
    MemoryManager* memory_manager = new (memory_manager_storage)
        MemoryManager(method_allocator, planned_allocator);

    /* Load method */
    Result<Method> method_result = g_program->load_method("forward", memory_manager);
    if (!method_result.ok()) {
        printf("[ET] Failed to load method: %d\r\n", (int)method_result.error());
        return -3;
    }
    g_method = new (method_storage) Method(std::move(method_result.get()));

    g_initialized = true;
    printf("[ET] ExecuTorch initialized successfully\r\n");
    printf("[ET] Memory: method=%uKB, planned=%uKB\r\n",
           (unsigned)(sizeof(method_allocator_pool)/1024),
           (unsigned)(sizeof(planned_memory_pool)/1024));

    return 0;
}

int executorch_run_inference(const int8_t* input_data, size_t input_size,
                              int8_t* output_data, size_t output_size)
{
    if (!g_initialized || g_method == nullptr) {
        printf("[ET] Error: Not initialized\r\n");
        return -1;
    }

    /* Get input tensor and copy data */
    EValue& input_val = g_method->mutable_input(0);
    if (!input_val.isTensor()) {
        printf("[ET] Error: Input is not a tensor\r\n");
        return -2;
    }
    
    exec_aten::Tensor input_tensor = input_val.toTensor();
    void* input_ptr = input_tensor.mutable_data_ptr();
    if (input_ptr != nullptr) {
        memcpy(input_ptr, input_data, input_size);
    }

    /* Execute */
    Error status = g_method->execute();
    if (status != Error::Ok) {
        printf("[ET] Execute failed: %d\r\n", (int)status);
        return -3;
    }

    /* Get output */
    EValue output_val = g_method->get_output(0);
    if (!output_val.isTensor()) {
        printf("[ET] Error: Output is not a tensor\r\n");
        return -4;
    }
    
    exec_aten::Tensor output_tensor = output_val.toTensor();
    const void* output_ptr = output_tensor.const_data_ptr();
    if (output_ptr != nullptr) {
        size_t copy_size = output_size < output_tensor.nbytes() ? output_size : output_tensor.nbytes();
        memcpy(output_data, output_ptr, copy_size);
    }

    return 0;
}

void executorch_deinit(void)
{
    g_method = nullptr;
    g_program = nullptr;
    g_initialized = false;
    printf("[ET] Deinitialized\r\n");
}

} /* extern "C" */
