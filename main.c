#include <stdbool.h>
#include <stdint.h>
#include "nrfx_systick.h"
#include "nrf_gpio.h"
#include "nrf_delay.h"

// Convert port and pin into pin number
#define YELLOW_LED_PIN  NRF_GPIO_PIN_MAP(0,6)
#define RED_LED_PIN  NRF_GPIO_PIN_MAP(0,8)
#define GREEN_LED_PIN  NRF_GPIO_PIN_MAP(1,9)
#define BLUE_LED_PIN  NRF_GPIO_PIN_MAP(0,12)
#define SW_BUTTON_PIN  NRF_GPIO_PIN_MAP(1,6)

#define LEDS_NUMBER 4

// PWM frequency and period calculation
#define PWM_FREQUENCY 1000            // 1 kHz PWM frequency
#define PERIOD_US (16000000 / PWM_FREQUENCY)  // Total period in microseconds


// Configure GPIO pin as input and output
void gpio_init(void);

void led_off(void);

void led_on(int led_pin);

void delay_700_ms(void);

bool button_pressed(void);

void systick_delay_us(uint32_t delay_us);

void pwm_dimming_led(int led_pin,int duty_cycle);

int main(void)
{
    gpio_init();
    nrfx_systick_init();

    // Real device id is 7200, but for workshops used 7214
    const int device_id[LEDS_NUMBER]={7,2,1,4};
    const int led_pins[LEDS_NUMBER]={YELLOW_LED_PIN,RED_LED_PIN,GREEN_LED_PIN,BLUE_LED_PIN};
    
    int current_led=0;
    int next_blink=0;

    //for smooth blinking
    int duty_cycle = 0;    // Start at 0% brightnes     
    int fade_step = 1;  // Step to increase or decrease duty cycle
    
    led_off();
    while (true)
    {
        if (button_pressed()) 
        {
            for (int i = current_led; i < LEDS_NUMBER; i++) 
            {
                for (int j = next_blink; j < device_id[i]; j++) 
                {
                    // Fade in: increase duty cycle from 0% to 100%
                    for (duty_cycle = 0; duty_cycle <= 100; duty_cycle += fade_step) 
                    {
                        pwm_dimming_led(led_pins[i],duty_cycle);
                    }

                    // Fade out: decrease duty cycle from 100% to 0%
                    for (duty_cycle = 100; duty_cycle >= 0; duty_cycle -= fade_step)
                    {
                        pwm_dimming_led(led_pins[i],duty_cycle);
                    }
                    //led_on(led_pins[i] );
                    //delay_700_ms;
                    //led_off();
                    //delay_700_ms;

                    if (!button_pressed()) 
                    { 
                        current_led = i; // Saves the position of led in the sequence
                        next_blink = j + 1; // Save the next blink position for current_led
                        
                        // If sequence completes for current_led reset n
                        if (next_blink >= device_id[i]) 
                        { 
                            next_blink = 0;
                            current_led = (i + 1) % LEDS_NUMBER; // Next LED
                        }
                        break;
                    }
                }
                if (!button_pressed()) // Exit the loop if button is released
                    break;

                next_blink = 0; // Reset blink count when moving to the next LED
                nrf_delay_ms(1000);
            }

            if ((current_led == (device_id[3]-1)) && next_blink == 0 && button_pressed()) 
            {
                current_led = 0;
                next_blink = 0;
            }
        }
    }
}

void systick_delay_us(uint32_t delay_us) {
    nrfx_systick_state_t start;
    nrfx_systick_get(&start);
    while (!nrfx_systick_test(&start, delay_us));
}

// Function to simulate PWM for LED dimming
void pwm_dimming_led(int led_pin,int duty_cycle) {
    int timeOn = (duty_cycle * PERIOD_US) / 100;   // On time based on duty cycle
    int timeOff = PERIOD_US - timeOn;              // Off time based on duty cycle

    // Turn LED on for timeOn duration
    nrf_gpio_pin_write(led_pin, 0);
    systick_delay_us(timeOn);

    // Turn LED off for timeOff duration
    nrf_gpio_pin_write(led_pin, 1);
    systick_delay_us(timeOff);
}

// Configure GPIO pin as input and output
void gpio_init(void)
{
    nrf_gpio_cfg_output(YELLOW_LED_PIN);
    nrf_gpio_cfg_output(RED_LED_PIN);
    nrf_gpio_cfg_output(GREEN_LED_PIN);
    nrf_gpio_cfg_output(BLUE_LED_PIN);
    nrf_gpio_cfg_input(SW_BUTTON_PIN,NRF_GPIO_PIN_PULLUP);
}

void led_off(void)
{
    nrf_gpio_pin_write(YELLOW_LED_PIN,1);
    nrf_gpio_pin_write(RED_LED_PIN,1);
    nrf_gpio_pin_write(GREEN_LED_PIN,1);
    nrf_gpio_pin_write(BLUE_LED_PIN,1);
}

void led_on(int led_pin)
{
    nrf_gpio_pin_write(led_pin,0);
}

void delay_700_ms(void)
{
    nrf_delay_ms(700);
}

bool button_pressed(void)
{
    return !nrf_gpio_pin_read(SW_BUTTON_PIN);
}