/**
 * @file main.c
 * @brief RTT Hello World Test for Alif E8
 */

#include "RTE_Components.h"
#include CMSIS_device_header

#include "Driver_IO.h"
#include "board_config.h"
#include "SEGGER_RTT.h"

#include <stdio.h>

/* GPIO Driver References for LED */
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

/* Delay */
static void delay_ms(uint32_t ms)
{
    volatile uint32_t count;
    for (uint32_t i = 0; i < ms; i++) {
        for (count = 0; count < 40000; count++) {
            __NOP();
        }
    }
}

/* LED Functions */
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

static void led_red(int on)
{
    gpio_r->SetValue(BOARD_LEDRGB_R_GPIO_PIN,
                     on ? GPIO_PIN_OUTPUT_STATE_HIGH : GPIO_PIN_OUTPUT_STATE_LOW);
}

static void led_green(int on)
{
    gpio_g->SetValue(BOARD_LEDRGB_G_GPIO_PIN,
                     on ? GPIO_PIN_OUTPUT_STATE_HIGH : GPIO_PIN_OUTPUT_STATE_LOW);
}

static void led_blue(int on)
{
    gpio_b->SetValue(BOARD_LEDRGB_B_GPIO_PIN,
                     on ? GPIO_PIN_OUTPUT_STATE_HIGH : GPIO_PIN_OUTPUT_STATE_LOW);
}

/* Main */
int main(void)
{
    int count = 0;

    /* Initialize board pins */
    int32_t ret = board_pins_config();
    if (ret != 0) {
        while(1);
    }

    /* Initialize LEDs */
    led_init();

    /* Initialize RTT */
    SEGGER_RTT_Init();

    /* Flash all LEDs to show we're alive */
    led_red(1); led_green(1); led_blue(1);
    delay_ms(500);
    led_red(0); led_green(0); led_blue(0);
    delay_ms(200);

    /* Send initial message via RTT */
    SEGGER_RTT_WriteString(0, "\r\n\r\n");
    SEGGER_RTT_WriteString(0, "========================================\r\n");
    SEGGER_RTT_WriteString(0, "  RTT TEST - Hello World!\r\n");
    SEGGER_RTT_WriteString(0, "  Alif E8 HE Core\r\n");
    SEGGER_RTT_WriteString(0, "========================================\r\n");

    /* Green LED = RTT initialized OK */
    led_green(1);
    delay_ms(500);
    led_green(0);

    /* Main loop - send periodic messages via RTT */
    while (1) {
        count++;

        /* Toggle blue LED to show we're running */
        led_blue(count % 2);

        /* Send count via RTT */
        SEGGER_RTT_printf(0, "[RTT] Hello World! Count: %d\r\n", count);

        delay_ms(1000);
    }

    return 0;
}
