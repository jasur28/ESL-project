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
#define PERIOD_US (16000000 / PWM_FREQUENCY)  // Total period in microseconds


// Timer for double-click detection
APP_TIMER_DEF(double_click_timer);
static volatile bool awaiting_second_click = false; // Flag for double-click detection
static volatile bool is_blinking_active = false;   // Flag to control LED blinking

// Timer timeout handler
void double_click_timeout_handler(void* p_context)
{
    // Timeout reached, so reset the flag for double-click detection
    awaiting_second_click = false;
}

// Button event handler
void button_event_handler(nrfx_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
    if (pin == BUTTON_PIN)
    {
        if (awaiting_second_click)
        {
            // Double-click detected
            awaiting_second_click = false;   // Reset the flag
            app_timer_stop(double_click_timer); // Stop the timer

            // Toggle blinking state on double-click
            is_blinking_active = !is_blinking_active;
        }
        else
        {
            // Start waiting for a second click
            awaiting_second_click = true;
            
            // Start or restart the timer for double-click interval
            app_timer_start(double_click_timer, APP_TIMER_TICKS(500), NULL); // 500ms interval for double-click
        }
    }
}

void init_gpiote_double_click()
{
    // Initialize GPIOTE module
    if (!nrfx_gpiote_is_init())
    {
        nrfx_gpiote_init();
    }

    // Configure LED pins as output
    nrf_gpio_cfg_output(YELLOW_LED_PIN);
    nrf_gpio_cfg_output(RED_LED_PIN);
    nrf_gpio_cfg_output(GREEN_LED_PIN);
    nrf_gpio_cfg_output(BLUE_LED_PIN);

    // Configure button pin with event handling
    nrfx_gpiote_in_config_t config = NRFX_GPIOTE_CONFIG_IN_SENSE_HITOLO(true); // Sense falling edge (press)
    config.pull = NRF_GPIO_PIN_PULLUP;

    // Initialize button with the event handler
    nrfx_gpiote_in_init(BUTTON_PIN, &config, button_event_handler);
    nrfx_gpiote_in_event_enable(BUTTON_PIN, true);
}

void systick_delay_us(uint32_t delay_us) {
    nrfx_systick_state_t start;
    nrfx_systick_get(&start);
    while (!nrfx_systick_test(&start, delay_us));
}

// Function to simulate PWM for LED dimming
void pwm_dimming_led(int led_pin, int duty_cycle) {
    int timeOn = (duty_cycle * PERIOD_US) / 100;   // On time based on duty cycle
    int timeOff = PERIOD_US - timeOn;              // Off time based on duty cycle

    // Turn LED on for timeOn duration
    nrf_gpio_pin_write(led_pin, 0);
    systick_delay_us(timeOn);

    // Turn LED off for timeOff duration
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
    nrfx_systick_init();       // Initialize Systick timer
    init_gpiote_double_click(); // Initialize GPIOTE for button handling

    const int device_id[LEDS_NUMBER] = {7, 2, 1, 4};
    const int led_pins[LEDS_NUMBER] = {YELLOW_LED_PIN, RED_LED_PIN, GREEN_LED_PIN, BLUE_LED_PIN};
    
    int current_led = 0;
    int next_blink = 0;

    // for smooth blinking
    int duty_cycle = 0;    // Start at 0% brightness     
    int fade_step = 1;     // Step to increase or decrease duty cycle
    led_off();
    
    while (true)
    {
        if (is_blinking_active) 
        {
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
                }

                // Reset blink count when moving to the next LED
                next_blink = 0; 
                nrf_delay_ms(1000);  // Delay between LEDs
            }

            // If all LEDs have blinked, reset to first LED
            if (next_blink == 0)
            {
                current_led = 0;
            }
        }
        else
        {
            // Turn off LEDs when blinking is inactive
            led_off();
        }
    }
}
