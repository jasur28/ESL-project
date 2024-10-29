#include <stdbool.h>
#include <stdint.h>
#include "nrf_delay.h"
#include "boards.h"
#include "nrf_gpio.h"

//convert port and pin into pin number
#define red_led_pin  NRF_GPIO_PIN_MAP(0,8)
#define green_led_pin  NRF_GPIO_PIN_MAP(1,9)
#define blue_led_pin  NRF_GPIO_PIN_MAP(0,12)
#define sw_button_pin  NRF_GPIO_PIN_MAP(1,6)

//configure GPIO pin as input and output
void gpio_init()
{
    nrf_gpio_cfg_output(red_led_pin);
    nrf_gpio_cfg_output(green_led_pin);
    nrf_gpio_cfg_output(blue_led_pin);
    nrf_gpio_cfg_input(sw_button_pin,NRF_GPIO_PIN_PULLUP);
}

void led_off()
{
    nrf_gpio_pin_write(green_led_pin,1);
    nrf_gpio_pin_write(red_led_pin,1);
    nrf_gpio_pin_write(blue_led_pin,1);
}

void led_on(int led_pin)
{
    nrf_gpio_pin_write(led_pin,0);
}

int main(void)
{
    gpio_init();
    int led_sequence[]={red_led_pin,red_led_pin,green_led_pin,green_led_pin,green_led_pin,blue_led_pin};
    int current_led=0;

    while (true)
    {
        if(nrf_gpio_pin_read(sw_button_pin)==0)
        {
            led_off();

            led_on(led_sequence[current_led]);
            current_led=(current_led + 1) % (sizeof(led_sequence) / sizeof(led_sequence[0]));
            nrf_delay_ms(500);
        }
        else
        {
            led_off();
        }
    }
}

