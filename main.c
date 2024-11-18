#include <stdbool.h>
#include <stdint.h>
#include "nrfx_systick.h"
#include "nrf_gpio.h"
#include "nrf_delay.h"
#include "nrfx_gpiote.h"
#include "app_timer.h"

// Convert port and pin into pin number
#define YELLOW_LED_PIN  NRF_GPIO_PIN_MAP(0,6)
#define RED_LED_PIN  NRF_GPIO_PIN_MAP(0,8)
#define GREEN_LED_PIN  NRF_GPIO_PIN_MAP(1,9)
#define BLUE_LED_PIN  NRF_GPIO_PIN_MAP(0,12)
#define BUTTON_PIN  NRF_GPIO_PIN_MAP(1,6)

#define LEDS_NUMBER 4

// PWM frequency and period calculation
#define PWM_FREQUENCY 1000            // 1 kHz PWM frequency
#define PERIOD_US (6000000 / PWM_FREQUENCY)  // Total period in microseconds

// Timer for double-click detection
APP_TIMER_DEF(double_click_timer);
static volatile bool awaiting_second_click = false; // Flag for double-click detection
static volatile bool is_blinking_active = false;   // Flag to control LED blinking

// Timer timeout handler
void double_click_timeout_handler(void* p_context)
{
    awaiting_second_click = false; // Reset the flag for double-click detection
}

// Button event handler
void button_event_handler(nrfx_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
    if (pin == BUTTON_PIN)
    {
        if (awaiting_second_click)
        {
            // Double-click detected
            awaiting_second_click = false;
            app_timer_stop(double_click_timer);

            // Toggle blinking state on double-click
            is_blinking_active = !is_blinking_active;

            // If blinking is turned off, ensure all LEDs are turned off immediately
            if (!is_blinking_active)
            {
                nrf_gpio_pin_write(YELLOW_LED_PIN, 1);
                nrf_gpio_pin_write(RED_LED_PIN, 1);
                nrf_gpio_pin_write(GREEN_LED_PIN, 1);
                nrf_gpio_pin_write(BLUE_LED_PIN, 1);
            }
        }
        else
        {
            awaiting_second_click = true;
            app_timer_start(double_click_timer, APP_TIMER_TICKS(500), NULL); // 500ms interval for double-click
        }
    }
}

void init_gpiote_double_click()
{
    if (!nrfx_gpiote_is_init())
    {
        nrfx_gpiote_init();
    }

    nrf_gpio_cfg_output(YELLOW_LED_PIN);
    nrf_gpio_cfg_output(RED_LED_PIN);
    nrf_gpio_cfg_output(GREEN_LED_PIN);
    nrf_gpio_cfg_output(BLUE_LED_PIN);

    nrfx_gpiote_in_config_t config = NRFX_GPIOTE_CONFIG_IN_SENSE_HITOLO(true); // Sense falling edge (press)
    config.pull = NRF_GPIO_PIN_PULLUP;

    nrfx_gpiote_in_init(BUTTON_PIN, &config, button_event_handler);
    nrfx_gpiote_in_event_enable(BUTTON_PIN, true);
}

void systick_delay_us(uint32_t delay_us)
{
    nrfx_systick_state_t start;
    nrfx_systick_get(&start);
    while (!nrfx_systick_test(&start, delay_us));
}

// Simulate PWM for LED dimming
void pwm_dimming_led(int led_pin, int duty_cycle)
{
    int timeOn = (duty_cycle * PERIOD_US) / 100;   // On time
    int timeOff = PERIOD_US - timeOn;             // Off time

    nrf_gpio_pin_write(led_pin, 0);
    systick_delay_us(timeOn);

    nrf_gpio_pin_write(led_pin, 1);
    systick_delay_us(timeOff);
}

void led_off(void)
{
    nrf_gpio_pin_write(YELLOW_LED_PIN, 1);
    nrf_gpio_pin_write(RED_LED_PIN, 1);
    nrf_gpio_pin_write(GREEN_LED_PIN, 1);
    nrf_gpio_pin_write(BLUE_LED_PIN, 1);
}

int main(void)
{
    nrfx_systick_init();
    init_gpiote_double_click();

    const int device_id[LEDS_NUMBER] = {7, 2, 1, 4};
    const int led_pins[LEDS_NUMBER] = {YELLOW_LED_PIN, RED_LED_PIN, GREEN_LED_PIN, BLUE_LED_PIN};

    int current_led = 0;
    int next_blink = 0;

    int duty_cycle = 0;
    int fade_step = 1;
    led_off();

    while (true)
    {
        if (is_blinking_active)
        {
            for (int i = current_led; i < LEDS_NUMBER; i++)
            {
                for (int j = next_blink; j < device_id[i]; j++)
                {
                    if (!is_blinking_active) break; // Stop blinking if the flag is toggled off

                    for (duty_cycle = 0; duty_cycle <= 100; duty_cycle += fade_step)
                    {
                        pwm_dimming_led(led_pins[i], duty_cycle);
                        if (!is_blinking_active) break; // Stop smoothly
                    }

                    for (duty_cycle = 100; duty_cycle >= 0; duty_cycle -= fade_step)
                    {
                        pwm_dimming_led(led_pins[i], duty_cycle);
                        if (!is_blinking_active) break;
                    }
                }
                if (!is_blinking_active) break;

                next_blink = 0;
                nrf_delay_ms(1000);
            }
            if (!is_blinking_active) led_off(); // Ensure LEDs are off
        }
    }
}
