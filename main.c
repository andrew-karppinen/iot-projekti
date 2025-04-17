#include <stdio.h>
#include "pico/stdlib.h"
#include "project.h"


void init_pins() {
    //kaikki pinnien alustukseen liittyvä tähän
    stdio_init_all();
    gpio_init(OPTO_PIN);
    gpio_set_dir(OPTO_PIN, GPIO_IN);
    gpio_pull_up(OPTO_PIN);  // opto fork pull-up

    gpio_pull_up(BUTTON);
    gpio_init(IN1); gpio_set_dir(IN1, GPIO_OUT);
    gpio_init(IN2); gpio_set_dir(IN2, GPIO_OUT);
    gpio_init(IN3); gpio_set_dir(IN3, GPIO_OUT);
    gpio_init(IN4); gpio_set_dir(IN4, GPIO_OUT);
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
}


bool read_button(int button)
{
    //false jos nappi pohjassa. eli samoiin kuin gpio_get
    bool button_status = false;

    button_status = gpio_get(button);
    sleep_ms(10);
    if(button_status==0 && gpio_get(button)==0){
        return false;
    }
    return true;
}



int main() {
    init_pins();
    //alustetaan tietorakenne:
    program_data data;
    data.calibrated = false;
    data.current_step = 0;
    data.step_counts = 0;

    uint64_t last_toggle = time_us_64();  // nykyinen aika mikrosekunteina
    bool led_state = false;

    while (1) { //odotetaan napin painallusta
        if (read_button(BUTTON)==0) {
            break;
        }

        uint64_t now = time_us_64(); //ledin vilkutus
        if (now - last_toggle >= LED_INTERVAL) {
            led_state = !led_state;  // vaihdetaan tila
            gpio_put(LED_PIN, led_state);
            last_toggle = now;
        }

    }

    calib(&data);
    printf("Kalibroitu: %d",data.calibrated);

}