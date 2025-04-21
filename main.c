#include <stdio.h>
#include "pico/stdlib.h"
#include "project.h"
#include "hardware/adc.h"

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

    //piezo sensor
    adc_init();
    adc_gpio_init(PIEZO_SENSOR);
    adc_select_input(1);
}
//käytetään jos piezo ei tunnistanut mitään
void blink_led(int times, int duration_ms) {
    for (int i = 0; i < times; i++) {
        gpio_put(LED_PIN, 1);
        sleep_ms(duration_ms);
        gpio_put(LED_PIN, 0);
        sleep_ms(duration_ms);
    }
}

bool sensorHit(program_data *data) {
    uint16_t value = adc_read();
    if (value > 150) {
        printf("Hit, adc value: %d\n", value); // voi poistaa
        data->piezo_hit = true;
        return true;
    }
    return false;
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

    int state =1;
    /*
     state 1 = käyttäjän odottaminen, kalibrointi
     state 2 = dosetin toiminta
     */

    while (1){
        switch (state) {
            case 1:
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
            calib(&data); //kalibroidaan
            if (data.calibrated==true) { //varmistetaan että calibrointi onnistui
                state =2;
            }
            break;
            case 2:
                //tähän pyöritys 30sec välein ja pilerin tippumisen tunnistus
                    if (sensorHit(&data)) {
                        printf("Pill detected\n"); // tähän mitä tapahtuu kun tunnistetaan pilleri
                        // break; //pysähdytään

                    }
            sleep_ms(1);
        }

        if (!data.piezo_hit) {
            blink_led(5, 400); //jos pilleriä ei tunnisteta, led vilkkuu
        }
    }

}