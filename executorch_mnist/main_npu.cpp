/**
 * ExecuTorch MNIST on Alif E8 + Ethos-U55 NPU
 * Complete working implementation
 */

#include <cstdint>
#include <cstring>

// Alif/CMSIS headers
extern "C" {
#include "RTE_Components.h"
#include CMSIS_device_header
#include "Driver_IO.h"
#include "board_config.h"
#include "ethosu_driver.h"
#include "se_services_port.h"
#include "services_lib_api.h"
}

// ExecuTorch headers
#include <executorch/runtime/platform/runtime.h>
#include <executorch/runtime/executor/program.h>
#include <executorch/runtime/core/memory_allocator.h>
#include <executorch/runtime/executor/memory_manager.h>
#include <extension/data_loader/buffer_data_loader.h>

// Model data (aligned)
#include "model_pte.h"

using executorch::extension::BufferDataLoader;
using executorch::runtime::Error;
using executorch::runtime::EValue;
using executorch::runtime::HierarchicalAllocator;
using executorch::runtime::MemoryAllocator;
using executorch::runtime::MemoryManager;
using executorch::runtime::Method;
using executorch::runtime::MethodMeta;
using executorch::runtime::Program;
using executorch::runtime::Result;
using executorch::runtime::Span;

// GPIO Driver macros for LED control
#define _GET_DRIVER_REF(ref, peri, chan) \
    extern ARM_DRIVER_##peri Driver_##peri##chan; \
    static ARM_DRIVER_##peri * ref = &Driver_##peri##chan;
#define GET_DRIVER_REF(ref, peri, chan) _GET_DRIVER_REF(ref, peri, chan)

#if defined(BOARD_RGB_LED_INSTANCE) && (BOARD_RGB_LED_INSTANCE == 0)
    GET_DRIVER_REF(gpio_b, GPIO, BOARD_LEDRGB0_B_GPIO_PORT);
    GET_DRIVER_REF(gpio_g, GPIO, BOARD_LEDRGB0_G_GPIO_PORT);
    GET_DRIVER_REF(gpio_r, GPIO, BOARD_LEDRGB0_R_GPIO_PORT);
    #define BOARD_LEDRGB_B_GPIO_PIN BOARD_LEDRGB0_B_GPIO_PIN
    #define BOARD_LEDRGB_G_GPIO_PIN BOARD_LEDRGB0_G_GPIO_PIN
    #define BOARD_LEDRGB_R_GPIO_PIN BOARD_LEDRGB0_R_GPIO_PIN
#else
    GET_DRIVER_REF(gpio_b, GPIO, BOARD_LEDRGB1_B_GPIO_PORT);
    GET_DRIVER_REF(gpio_g, GPIO, BOARD_LEDRGB1_G_GPIO_PORT);
    GET_DRIVER_REF(gpio_r, GPIO, BOARD_LEDRGB1_R_GPIO_PORT);
    #define BOARD_LEDRGB_B_GPIO_PIN BOARD_LEDRGB1_B_GPIO_PIN
    #define BOARD_LEDRGB_G_GPIO_PIN BOARD_LEDRGB1_G_GPIO_PIN
    #define BOARD_LEDRGB_R_GPIO_PIN BOARD_LEDRGB1_R_GPIO_PIN
#endif

// NPU base address for HE core
#ifndef NPU_HE_BASE
#define NPU_HE_BASE 0x400E1000UL
#endif

// Ethos-U55 driver instance
static struct ethosu_driver ethosu_drv;

// Memory pools for ExecuTorch - placed in SRAM0 for better performance
#define METHOD_ALLOCATOR_POOL_SIZE (512 * 1024)  // 512KB for method setup
#define TEMP_ALLOCATOR_POOL_SIZE   (256 * 1024)  // 256KB for inference scratch

// Use SRAM0 for memory pools (4MB available)
static uint8_t __attribute__((section(".bss.noinit"), aligned(16)))
    method_allocator_pool[METHOD_ALLOCATOR_POOL_SIZE];
static uint8_t __attribute__((section(".bss.noinit"), aligned(16)))
    temp_allocator_pool[TEMP_ALLOCATOR_POOL_SIZE];

static void delay_ms(uint32_t ms) {
    volatile uint32_t count;
    for (uint32_t i = 0; i < ms; i++) {
        for (count = 0; count < 40000; count++) {
            __NOP();
        }
    }
}

// LED control helpers
static void led_all_off() {
    gpio_r->SetValue(BOARD_LEDRGB_R_GPIO_PIN, GPIO_PIN_OUTPUT_STATE_LOW);
    gpio_g->SetValue(BOARD_LEDRGB_G_GPIO_PIN, GPIO_PIN_OUTPUT_STATE_LOW);
    gpio_b->SetValue(BOARD_LEDRGB_B_GPIO_PIN, GPIO_PIN_OUTPUT_STATE_LOW);
}

static void led_red_on() {
    gpio_r->SetValue(BOARD_LEDRGB_R_GPIO_PIN, GPIO_PIN_OUTPUT_STATE_HIGH);
}

static void led_green_on() {
    gpio_g->SetValue(BOARD_LEDRGB_G_GPIO_PIN, GPIO_PIN_OUTPUT_STATE_HIGH);
}

static void led_blue_on() {
    gpio_b->SetValue(BOARD_LEDRGB_B_GPIO_PIN, GPIO_PIN_OUTPUT_STATE_HIGH);
}

static void signal_error() {
    // Fast red blink forever
    while (1) {
        led_all_off();
        led_red_on();
        delay_ms(100);
        led_all_off();
        delay_ms(100);
    }
}

static void signal_success() {
    // Green solid
    led_all_off();
    led_green_on();
}


int main(void) {
    // Initialize board pins
    int32_t ret = board_pins_config();
    if (ret != 0) {
        while(1);
    }

    // Initialize LEDs
    gpio_b->Initialize(BOARD_LEDRGB_B_GPIO_PIN, NULL);
    gpio_b->PowerControl(BOARD_LEDRGB_B_GPIO_PIN, ARM_POWER_FULL);
    gpio_b->SetDirection(BOARD_LEDRGB_B_GPIO_PIN, GPIO_PIN_DIRECTION_OUTPUT);

    gpio_r->Initialize(BOARD_LEDRGB_R_GPIO_PIN, NULL);
    gpio_r->PowerControl(BOARD_LEDRGB_R_GPIO_PIN, ARM_POWER_FULL);
    gpio_r->SetDirection(BOARD_LEDRGB_R_GPIO_PIN, GPIO_PIN_DIRECTION_OUTPUT);

    gpio_g->Initialize(BOARD_LEDRGB_G_GPIO_PIN, NULL);
    gpio_g->PowerControl(BOARD_LEDRGB_G_GPIO_PIN, ARM_POWER_FULL);
    gpio_g->SetDirection(BOARD_LEDRGB_G_GPIO_PIN, GPIO_PIN_DIRECTION_OUTPUT);

    led_all_off();

    // Signal start: RED
    led_red_on();
    delay_ms(500);
    led_all_off();
    delay_ms(200);

    // Enable SRAM0 power via Secure Enclave services
    uint32_t mem_error = 0;
    uint32_t ret_services = SERVICES_power_memory_req(
        se_services_s_handle,
        POWER_MEM_SRAM_0_ENABLE,
        &mem_error);

    if (ret_services != 0 || mem_error != 0) {
        // Failed to enable SRAM0 - signal error
        signal_error();
    }

    // Brief YELLOW (R+G) flash = SRAM0 power enabled
    led_red_on();
    led_green_on();
    delay_ms(200);
    led_all_off();
    delay_ms(200);

    // Test SRAM0 access before using it
    // Write a pattern and read it back to verify SRAM0 is accessible
    volatile uint32_t* sram0_test = (volatile uint32_t*)method_allocator_pool;
    *sram0_test = 0xDEADBEEF;
    if (*sram0_test != 0xDEADBEEF) {
        // SRAM0 access failed - flash RED rapidly
        signal_error();
    }

    // SRAM0 OK - brief GREEN flash
    led_green_on();
    delay_ms(200);
    led_all_off();
    delay_ms(200);

    // Initialize NPU
    int npu_result = ethosu_init(&ethosu_drv, (void*)NPU_HE_BASE, NULL, 0, 0, 0);
    if (npu_result != 0) {
        signal_error();
    }

    // Signal NPU OK: BLUE blink
    led_blue_on();
    delay_ms(300);
    led_all_off();
    delay_ms(200);

    // Initialize ExecuTorch runtime
    executorch::runtime::runtime_init();

    // Signal runtime OK: CYAN (G+B) blink
    led_green_on();
    led_blue_on();
    delay_ms(300);
    led_all_off();
    delay_ms(200);

    // Create memory allocators (using built-in MemoryAllocator)
    MemoryAllocator method_allocator(METHOD_ALLOCATOR_POOL_SIZE, method_allocator_pool);
    MemoryAllocator temp_allocator(TEMP_ALLOCATOR_POOL_SIZE, temp_allocator_pool);

    // Load the model
    BufferDataLoader loader(model_pte, model_pte_len);
    Result<Program> program_result = Program::load(&loader);

    if (!program_result.ok()) {
        // Model load failed - signal error
        signal_error();
    }

    Program& program = program_result.get();

    // Signal model loaded: CYAN (G+B)
    led_green_on();
    led_blue_on();
    delay_ms(500);
    led_all_off();
    delay_ms(200);

    // Get method name (should be "forward")
    const char* method_name = "forward";
    Result<MethodMeta> method_meta = program.method_meta(method_name);
    if (!method_meta.ok()) {
        signal_error();
    }

    // Set up planned memory buffers
    size_t num_buffers = method_meta->num_memory_planned_buffers();
    Span<uint8_t>* planned_spans = nullptr;

    if (num_buffers > 0) {
        planned_spans = static_cast<Span<uint8_t>*>(
            method_allocator.allocate(num_buffers * sizeof(Span<uint8_t>), 8));

        for (size_t i = 0; i < num_buffers; i++) {
            size_t buffer_size = method_meta->memory_planned_buffer_size(i).get();
            uint8_t* buffer = static_cast<uint8_t*>(
                method_allocator.allocate(buffer_size, 16));
            if (buffer == nullptr) {
                signal_error();
            }
            planned_spans[i] = Span<uint8_t>(buffer, buffer_size);
        }
    }

    HierarchicalAllocator planned_memory(
        planned_spans ? Span<Span<uint8_t>>(planned_spans, num_buffers)
                      : Span<Span<uint8_t>>());

    // Create memory manager
    MemoryManager memory_manager(&method_allocator, &planned_memory, &temp_allocator);

    // Load the method
    Result<Method> method_result = program.load_method(method_name, &memory_manager);
    if (!method_result.ok()) {
        signal_error();
    }

    Method& method = method_result.get();

    // Signal method loaded: YELLOW (R+G)
    led_red_on();
    led_green_on();
    delay_ms(500);
    led_all_off();
    delay_ms(200);

    // Prepare input - MNIST is 1x1x28x28 = 784 values, int8
    // Fill with a simple test pattern (digit "7" approximation)
    EValue& input_eval = method.mutable_input(0);
    if (input_eval.isTensor()) {
        auto& input_tensor = input_eval.toTensor();
        int8_t* input_data = input_tensor.mutable_data_ptr<int8_t>();
        size_t input_size = input_tensor.numel();

        // Clear to zero (background)
        memset(input_data, 0, input_size);

        // Draw a simple "7" pattern
        // Top horizontal line (row 5, cols 8-20)
        for (int c = 8; c <= 20; c++) {
            input_data[5 * 28 + c] = 127;
        }
        // Diagonal line
        for (int r = 6; r < 24; r++) {
            int c = 20 - (r - 6) / 2;
            if (c >= 0 && c < 28) {
                input_data[r * 28 + c] = 127;
            }
        }
    }

    // Execute inference
    Error exec_status = method.execute();
    if (exec_status != Error::Ok) {
        signal_error();
    }

    // Get output
    EValue output = method.get_output(0);
    int predicted_digit = -1;

    if (output.isTensor()) {
        auto& output_tensor = output.toTensor();
        const int8_t* output_data = output_tensor.const_data_ptr<int8_t>();
        size_t output_size = output_tensor.numel();

        // Find argmax (predicted digit)
        int8_t max_val = output_data[0];
        predicted_digit = 0;
        for (size_t i = 1; i < output_size && i < 10; i++) {
            if (output_data[i] > max_val) {
                max_val = output_data[i];
                predicted_digit = i;
            }
        }
    }

    // Signal inference complete!
    // Blink the predicted digit number of times
    signal_success();
    delay_ms(1000);

    // Show result: blink GREEN for predicted digit
    for (int i = 0; i <= predicted_digit; i++) {
        led_all_off();
        delay_ms(300);
        led_green_on();
        delay_ms(300);
    }

    // Final: solid WHITE (all on) = SUCCESS
    led_all_off();
    delay_ms(500);
    led_red_on();
    led_green_on();
    led_blue_on();

    // Keep blinking to show it's alive
    while (1) {
        delay_ms(2000);
        led_all_off();
        delay_ms(200);
        led_red_on();
        led_green_on();
        led_blue_on();
    }

    return 0;
}
