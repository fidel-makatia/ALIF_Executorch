/**
 * @file main.c
 * @brief ExecuTorch MNIST Demo - Alif E8 with UART output
 */

#include "RTE_Components.h"
#include CMSIS_device_header

#include "Driver_IO.h"
#include "board_config.h"
#include "uart_tracelib.h"
#include "fault_handler.h"
#include "executorch_runner.h"

#include <stdio.h>
#include <stdint.h>
#include <string.h>

/* Model data */
#include "mnist_model_data.h"

/*---------------------------------------------------------------------------
 * GPIO Driver References for LED
 *---------------------------------------------------------------------------*/
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

/*---------------------------------------------------------------------------
 * UART callback
 *---------------------------------------------------------------------------*/
static void uart_callback(uint32_t event)
{
    (void)event;
}

/*---------------------------------------------------------------------------
 * Delay
 *---------------------------------------------------------------------------*/
static void delay_ms(uint32_t ms)
{
    volatile uint32_t count;
    for (uint32_t i = 0; i < ms; i++) {
        for (count = 0; count < 40000; count++) {
            __NOP();
        }
    }
}

/*---------------------------------------------------------------------------
 * LED Functions
 *---------------------------------------------------------------------------*/
static void led_init(void)
{
    gpio_r->Initialize(BOARD_LEDRGB_R_GPIO_PIN, NULL);
    gpio_r->PowerControl(BOARD_LEDRGB_R_GPIO_PIN, ARM_POWER_FULL);
    gpio_r->SetDirection(BOARD_LEDRGB_R_GPIO_PIN, GPIO_PIN_DIRECTION_OUTPUT);
    gpio_r->SetValue(BOARD_LEDRGB_R_GPIO_PIN, GPIO_PIN_OUTPUT_STATE_LOW);

    gpio_g->Initialize(BOARD_LEDRGB_G_GPIO_PIN, NULL);
    gpio_g->PowerControl(BOARD_LEDRGB_G_GPIO_PIN, ARM_POWER_FULL);
    gpio_g->SetDirection(BOARD_LEDRGB_G_GPIO_PIN, GPIO_PIN_DIRECTION_OUTPUT);
    gpio_g->SetValue(BOARD_LEDRGB_G_GPIO_PIN, GPIO_PIN_OUTPUT_STATE_LOW);

    gpio_b->Initialize(BOARD_LEDRGB_B_GPIO_PIN, NULL);
    gpio_b->PowerControl(BOARD_LEDRGB_B_GPIO_PIN, ARM_POWER_FULL);
    gpio_b->SetDirection(BOARD_LEDRGB_B_GPIO_PIN, GPIO_PIN_DIRECTION_OUTPUT);
    gpio_b->SetValue(BOARD_LEDRGB_B_GPIO_PIN, GPIO_PIN_OUTPUT_STATE_LOW);
}

static void led_set(int r, int g, int b)
{
    gpio_r->SetValue(BOARD_LEDRGB_R_GPIO_PIN, r ? GPIO_PIN_OUTPUT_STATE_HIGH : GPIO_PIN_OUTPUT_STATE_LOW);
    gpio_g->SetValue(BOARD_LEDRGB_G_GPIO_PIN, g ? GPIO_PIN_OUTPUT_STATE_HIGH : GPIO_PIN_OUTPUT_STATE_LOW);
    gpio_b->SetValue(BOARD_LEDRGB_B_GPIO_PIN, b ? GPIO_PIN_OUTPUT_STATE_HIGH : GPIO_PIN_OUTPUT_STATE_LOW);
}

/*---------------------------------------------------------------------------
 * Test Data - Digit "7" (28x28 INT8, quantized)
 *---------------------------------------------------------------------------*/
static const int8_t test_digit_7[784] = {
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,84,-71,-97,-105,60,36,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,-34,-2,-2,-2,-2,-15,-58,-58,-58,-58,-58,-58,-58,-58,-86,52,0,0,0,0,0,0,
    0,0,0,0,0,0,67,114,72,114,-93,-29,-2,-31,-2,-2,-2,-6,-27,-2,-2,-116,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,17,66,14,67,67,67,59,21,-20,-2,106,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,83,-3,-47,18,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,22,-23,-1,83,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,-127,-2,-18,44,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,59,-7,-2,62,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,-123,-2,-69,5,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,9,-51,-8,58,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,126,-2,-74,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,75,-5,-16,57,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,19,-35,-2,-90,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,3,-53,-2,-37,35,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,38,-2,-2,77,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,31,-32,-2,115,1,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,-123,-2,-2,52,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,61,-14,-2,-2,52,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,121,-2,-2,-37,40,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,121,-2,-49,18,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

static int argmax(const int8_t *arr, int len)
{
    int max_idx = 0;
    int8_t max_val = arr[0];
    for (int i = 1; i < len; i++) {
        if (arr[i] > max_val) {
            max_val = arr[i];
            max_idx = i;
        }
    }
    return max_idx;
}

/*---------------------------------------------------------------------------
 * Main
 *---------------------------------------------------------------------------*/
int main(void)
{
    int8_t output_scores[10];
    int predicted;
    int expected = 7;
    int passed = 0;
    int total = 10;

    /* Initialize board pins */
    int32_t ret = board_pins_config();
    if (ret != 0) {
        while(1);
    }

    /* Initialize UART */
    tracelib_init(NULL, uart_callback);
    fault_dump_enable(true);

    /* Initialize LEDs */
    led_init();

    /* Startup flash */
    led_set(1, 1, 1);
    delay_ms(500);
    led_set(0, 0, 0);

    /* Banner */
    printf("\r\n");
    printf("========================================\r\n");
    printf("  ExecuTorch MNIST - Alif E8\r\n");
    printf("  Cortex-M55 + Ethos-U55 NPU\r\n");
    printf("  ** REAL INFERENCE **\r\n");
    printf("========================================\r\n");

    /* Initialize ExecuTorch */
    printf("[ET] Loading model (%u bytes)...\r\n", (unsigned int)mnist_model_len);
    
    ret = executorch_init(mnist_model_data, mnist_model_len);
    if (ret != 0) {
        printf("[ET] Init failed: %d\r\n", ret);
        led_set(1, 0, 0);
        while(1);
    }
    printf("[ET] Model loaded OK\r\n");

    printf("[TEST] Running %d tests...\r\n\r\n", total);

    /* Run tests */
    for (int test = 0; test < total; test++) {
        led_set(0, 0, 1);
        
        printf("--- Test %d/%d ---\r\n", test + 1, total);
        
        /* Run real inference */
        memset(output_scores, 0, sizeof(output_scores));
        ret = executorch_run_inference(test_digit_7, sizeof(test_digit_7),
                                        output_scores, sizeof(output_scores));
        
        if (ret != 0) {
            printf("[ERROR] Inference failed: %d\r\n", ret);
            led_set(1, 0, 0);
            delay_ms(500);
            continue;
        }
        
        /* Print scores */
        printf("[INF] Scores: ");
        for (int i = 0; i < 10; i++) {
            printf("%d:%d ", i, output_scores[i]);
        }
        printf("\r\n");
        
        /* Get prediction */
        predicted = argmax(output_scores, 10);
        
        if (predicted == expected) {
            passed++;
            printf("[RES] Pred:%d Exp:%d -> PASS\r\n\r\n", predicted, expected);
            led_set(0, 1, 1);
        } else {
            printf("[RES] Pred:%d Exp:%d -> FAIL\r\n\r\n", predicted, expected);
            led_set(1, 0, 0);
        }
        delay_ms(300);
        led_set(0, 0, 0);
        delay_ms(200);
    }

    /* Summary */
    printf("========================================\r\n");
    printf("  RESULTS: %d/%d passed\r\n", passed, total);
    printf("========================================\r\n");

    if (passed == total) {
        printf("[SYS] All PASSED\r\n");
        led_set(0, 1, 1);
    } else {
        printf("[SYS] Some FAILED\r\n");
        led_set(1, 0, 0);
    }

    /* Heartbeat */
    int count = 0;
    while (1) {
        delay_ms(1000);
        count++;
        led_set(0, 0, count % 2);
        if (count % 10 == 0) {
            printf("[HB] %d\r\n", count);
        }
    }

    return 0;
}
