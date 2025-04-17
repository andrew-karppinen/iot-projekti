#include <stdio.h>
#include "pico/stdlib.h"

int main() {

    const uint led_pin_1 = 20;
    const uint led_pin_2 = 21;
    const uint led_pin_3 = 22;


    uint count = 0;

    // Initialize LED pin
    gpio_init(led_pin_1);
    gpio_set_dir(led_pin_1, GPIO_OUT);

    // Initialize chosen serial port
    stdio_init_all();

    gpio_pull_up(led_pin_1);

    return 0;

}