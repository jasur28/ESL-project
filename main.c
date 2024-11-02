#include <stdbool.h>
#include <stdint.h>
#include "nrf_delay.h"
#include "nrf_gpio.h"

// Convert port and pin into pin number
#define YELLOW_LED_PIN  NRF_GPIO_PIN_MAP(0,6)
#define RED_LED_PIN  NRF_GPIO_PIN_MAP(0,8)
#define GREEN_LED_PIN  NRF_GPIO_PIN_MAP(1,9)
#define BLUE_LED_PIN  NRF_GPIO_PIN_MAP(0,12)
#define SW_BUTTON_PIN  NRF_GPIO_PIN_MAP(1,6)

#define LEDS_NUMBER 4

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
    // Return true if button pressed, as i used internal pull_up resistor i compare with 0
    return !nrf_gpio_pin_read(SW_BUTTON_PIN);
}

int main(void)
{
    gpio_init();

    // Real device id is 7200, but for workshops used 7214
    const int device_id[LEDS_NUMBER]={7,2,1,4};
    const int led_pins[LEDS_NUMBER]={YELLOW_LED_PIN,RED_LED_PIN,GREEN_LED_PIN,BLUE_LED_PIN};
    
    int current_led=0;
    int next_blink=0;
    
    led_off();
    while (true)
    {
        if (button_pressed()) 
        {
            for (int i = current_led; i < LEDS_NUMBER; i++) 
            {
                for (int j = next_blink; j < device_id[i]; j++) 
                {
                    led_on(led_pins[i] );
                    delay_700_ms;
                    led_off();
                    delay_700_ms;

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

