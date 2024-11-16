#include <stdbool.h>
#include <stdint.h>
#include "nrfx_systick.h"
#include "nrf_gpio.h"
#include "nrf_delay.h"
#include "app_timer.h"
#include "nrfx_gpiote.h"

// GPIO and LED definitions
#define YELLOW_LED_PIN  NRF_GPIO_PIN_MAP(0,6)
#define RED_LED_PIN     NRF_GPIO_PIN_MAP(0,8)
#define GREEN_LED_PIN   NRF_GPIO_PIN_MAP(1,9)
#define BLUE_LED_PIN    NRF_GPIO_PIN_MAP(0,12)
#define SW_BUTTON_PIN   NRF_GPIO_PIN_MAP(1,6)

#define LEDS_NUMBER 4
#define PWM_FREQUENCY 1000 // 1 kHz PWM frequency
#define PERIOD_US (16000000 / PWM_FREQUENCY) // Total period in microseconds

#define DOUBLE_CLICK_INTERVAL_MS 500 // Max interval for detecting a double-click (ms)

// Function prototypes
void gpio_init(void);
void led_off(void);
void systick_delay_us(uint32_t delay_us);
void pwm_dimming_led(int led_pin, int duty_cycle);
void gpiote_event_handler(nrfx_gpiote_pin_t pin, nrf_gpiote_polarity_t action);
void double_click_timeout_handler(void *context);

// Variables
APP_TIMER_DEF(double_click_timer);
volatile bool is_double_click = false;
volatile uint8_t click_count = 0;

int main(void)
{
    gpio_init();
    nrfx_systick_init();
    app_timer_init();

    // Initialize GPIOTE
    if (!nrfx_gpiote_is_init()) {
        nrfx_gpiote_init();
    }

    // Configure GPIOTE for button
    nrfx_gpiote_in_config_t button_config = NRFX_GPIOTE_CONFIG_IN_SENSE_HITOLO(true);
    button_config.pull = NRF_GPIO_PIN_PULLUP;
    nrfx_gpiote_in_init(SW_BUTTON_PIN, &button_config, gpiote_event_handler);
    nrfx_gpiote_in_event_enable(SW_BUTTON_PIN, true);

    // Initialize APP_TIMER for double-click detection
    app_timer_create(&double_click_timer, APP_TIMER_MODE_SINGLE_SHOT, double_click_timeout_handler);

    // LED control logic
    const int device_id[LEDS_NUMBER] = {7, 2, 1, 4};
    const int led_pins[LEDS_NUMBER] = {YELLOW_LED_PIN, RED_LED_PIN, GREEN_LED_PIN, BLUE_LED_PIN};

    int current_led = 0;
    int next_blink = 0;

    int duty_cycle = 0;    // Start at 0% brightness
    int fade_step = 1;     // Step to increase or decrease duty cycle

    led_off();

    while (true)
    {
        if (is_double_click)
        {
            is_double_click = false; // Clear the flag after processing

            for (int i = current_led; i < LEDS_NUMBER; i++)
            {
                for (int j = next_blink; j < device_id[i]; j++)
                {
                    // Fade in: increase duty cycle from 0% to 100%
                    for (duty_cycle = 0; duty_cycle <= 100; duty_cycle += fade_step)
                    {
                        pwm_dimming_led(led_pins[i], duty_cycle);
                    }

                    // Fade out: decrease duty cycle from 100% to 0%
                    for (duty_cycle = 100; duty_cycle >= 0; duty_cycle -= fade_step)
                    {
                        pwm_dimming_led(led_pins[i], duty_cycle);
                    }

                    if (!is_double_click)
                    {
                        current_led = i;
                        next_blink = j + 1;
                        if (next_blink >= device_id[i])
                        {
                            next_blink = 0;
                            current_led = (i + 1) % LEDS_NUMBER;
                        }
                        break;
                    }
                }

                if (!is_double_click)
                    break;

                next_blink = 0;
                nrf_delay_ms(1000);
            }

            if ((current_led == (device_id[3] - 1)) && next_blink == 0 && is_double_click)
            {
                current_led = 0;
                next_blink = 0;
            }
        }
    }
}

// GPIOTE event handler for button
void gpiote_event_handler(nrfx_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
    if (pin == SW_BUTTON_PIN )
    {
        click_count++;
        if (click_count == 1)
        {
            app_timer_start(double_click_timer, APP_TIMER_TICKS(DOUBLE_CLICK_INTERVAL_MS), NULL);
        }
        else if (click_count == 2)
        {
            is_double_click = true;
            app_timer_stop(double_click_timer);
            click_count = 0; // Reset click count
        }
    }
}

// Double-click timeout handler
void double_click_timeout_handler(void *context)
{
    click_count = 0; // Reset click count if timeout occurs
}

// Other helper functions...

void systick_delay_us(uint32_t delay_us)
{
    nrfx_systick_state_t start;
    nrfx_systick_get(&start);
    while (!nrfx_systick_test(&start, delay_us));
}

void pwm_dimming_led(int led_pin, int duty_cycle)
{
    int timeOn = (duty_cycle * PERIOD_US) / 100;
    int timeOff = PERIOD_US - timeOn;

    nrf_gpio_pin_write(led_pin, 0);
    systick_delay_us(timeOn);

    nrf_gpio_pin_write(led_pin, 1);
    systick_delay_us(timeOff);
}

void gpio_init(void)
{
    nrf_gpio_cfg_output(YELLOW_LED_PIN);
    nrf_gpio_cfg_output(RED_LED_PIN);
    nrf_gpio_cfg_output(GREEN_LED_PIN);
    nrf_gpio_cfg_output(BLUE_LED_PIN);
    nrf_gpio_cfg_input(SW_BUTTON_PIN, NRF_GPIO_PIN_PULLUP);
}

void led_off(void)
{
    nrf_gpio_pin_write(YELLOW_LED_PIN, 1);
    nrf_gpio_pin_write(RED_LED_PIN, 1);
    nrf_gpio_pin_write(GREEN_LED_PIN, 1);
    nrf_gpio_pin_write(BLUE_LED_PIN, 1);
}
