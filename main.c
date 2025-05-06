#include <stdio.h>
#include "pico/stdlib.h"
#include "project.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"

static program_data data;

void init_pins() {
    // kaikki pinnien alustukseen liittyvä tähän
    stdio_init_all();
    gpio_init(OPTO_PIN);
    gpio_set_dir(OPTO_PIN, GPIO_IN);
    gpio_pull_up(OPTO_PIN);  // opto fork pull-up

    gpio_pull_up(BUTTON);
    gpio_pull_up(RESET_BUTTON);

    gpio_init(IN1); gpio_set_dir(IN1, GPIO_OUT);
    gpio_init(IN2); gpio_set_dir(IN2, GPIO_OUT);
    gpio_init(IN3); gpio_set_dir(IN3, GPIO_OUT);
    gpio_init(IN4); gpio_set_dir(IN4, GPIO_OUT);
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    // piezo sensor
    gpio_init(PIEZO_SENSOR);
    gpio_pull_up(PIEZO_SENSOR);
    gpio_set_irq_enabled_with_callback(PIEZO_SENSOR, GPIO_IRQ_EDGE_FALL, true, &sensorHit);

    // eeprom yhteyden alustus, i2c:
    init_eeprom();
}

void sensorHit(uint gpio, uint32_t event_mask) {
    data.piezeo_hit = true;
}

void blink_led() {
    for (int j = 0; j<5; j++) {
        gpio_put(LED_PIN, 1);
        sleep_ms(500);
        gpio_put(LED_PIN, 0);
        sleep_ms(500);
    }
}

bool read_button(int button) {
    // false jos nappi pohjassa. eli samoin kuin gpio_get
    bool button_status = gpio_get(button);
    sleep_ms(10);
    if (!button_status && !gpio_get(button)) {
        return false;
    }
    return true;
}

void init_data(program_data *data) {
    data->calibrated = false;
    data->current_step = 0;
    data->step_counts = 0;
    data->piezeo_hit = false;
    data->pill_counter = 0;
    data->state = BOOT;
    data->motor_running = false;
}

int main() {
    init_pins();
    //alustetaan tietorakenne
    init_data(&data);

    uint64_t last_toggle = time_us_64();
    bool led_state = false;

    //lora:
    printf("Yritetaan lora yhteytta...\n");
    if (set_up_lora()) {
        data.lora_connected = true;
        sen_lora_msg("Connected to Lorawan");
        printf("Lorawan yhteys luotu\n");
    } else {
        data.lora_connected = false;
        printf("Error lorawan yhteyden luonnissa, yritettiin 2 kertaa\n");
    }

    /*
      state 0/BOOT = käyttäjän odottaminen, kalibrointi
      state 1/PILL = dosetin toiminta
      */
    if (read_status_from_eeprom(&data)==false){
        //tilaa ei voitu lukea, asetetaan tilaksi BOOT
        data.state = BOOT;
        printf("EEPROM-luku epäonnistui\n");
    } else{
        //tulostetaan tila, debug:
        printf("state: %d\n", data.state);
        printf("kalibroitu: %d\n", data.calibrated);
        printf("askelmäärä: %d\n", data.step_counts);
        printf("pillerimäärä: %d\n", data.pill_counter);
        printf("moottori pyöri kun sammutettiin: %d\n",data.motor_running);
        if(data.state != BOOT) {
            if (data.motor_running==true) {
                recalib(&data);
                if(data.lora_connected) {
                    sen_lora_msg("Power off during turning, recalibrated");
                }
            }
        }
    }


    while (1) {
        if (!read_button(RESET_BUTTON)) {
            printf("resetointi\n");
            //resetointi, palautetaan tilatiedot oletusarvoihin
            init_data(&data);
            write_status_to_eeprom(data);
            if (data.lora_connected) {
                sen_lora_msg("Reset");
            }
        }

        switch (data.state) {
            case BOOT:{

                //odotetaan käyttäjää, kalibrointi alkaa
                printf("Paina nappia\n");
                while (read_button(BUTTON)) {
                    uint64_t now = time_us_64();
                    if (now - last_toggle >= LED_INTERVAL) {
                        led_state = !led_state;
                        gpio_put(LED_PIN, led_state);
                        last_toggle = now;
                    }
                }

                init_data(&data);
                write_status_to_eeprom(data); //päivitetään status eepromiin
                calib(&data);
                printf("Paina nappia\n");


                //odotetaan käyttäjää, pillereiden jako alkaa
                while (read_button(BUTTON)) {
                    uint64_t now = time_us_64();
                    if (now - last_toggle >= LED_INTERVAL) {
                        led_state = !led_state;
                        gpio_put(LED_PIN, led_state);
                        last_toggle = now;
                    }
                }
                if (data.calibrated) {
                    data.state = PILL;
                    write_status_to_eeprom(data);
                    if (data.lora_connected) {
                        sen_lora_msg("Calibrated, aloitetaan pillereiden jako");
                    }
                    break;
                }
            }

            case PILL: {
                static uint64_t last_motor_time = 0;
                static bool no_pill_sent = true;

                uint64_t now = time_us_64();

                // moottori
                if (run_motor_30(&data)) {

                    write_status_to_eeprom(data);

                    char buf[32];
                    snprintf(buf, sizeof(buf), "Current lokero:%d", data.pill_counter);
                    if (data.lora_connected) {
                        sen_lora_msg(buf);
                    }

                    last_motor_time = now;
                    no_pill_sent = false;
                }

                // Pilleri tunnistetaan
                if (data.piezeo_hit) {
                    if (data.lora_connected) {
                        char buffer[64];
                        snprintf(buffer, sizeof(buffer), "Pill detected from lokero: %d", data.pill_counter);
                        sen_lora_msg(buffer);
                    }
                    printf("hit!\n");
                    data.piezeo_hit = false;
                    no_pill_sent = true;
                }

                // jos pilleriä ei tunnisteta 4 sekunnin sisällä lähetetään viesti
                if (!data.piezeo_hit && !no_pill_sent && (now - last_motor_time > 4 * 1000 * 1000)) {
                    gpio_put(LED_PIN,0);
                    blink_led(); // led vilkkuu 5 kertaa
                    char buffer[64];
                    snprintf(buffer, sizeof(buffer), "No pill detected from lokero: %d", data.pill_counter);
                    if (data.lora_connected) {
                        sen_lora_msg(buffer);
                    }
                    no_pill_sent = true;
                }
                //siirretty tänne jotta lähettää dosetti tyhjä viimeisenä viestinä. "current lokero x" jälkeen
                if(data.pill_counter >=7) { //dosetti pyörähtänyt ympäri
                    if (data.lora_connected) {
                        sen_lora_msg("Dosetti tyhja!");
                    }
                    blink_led();
                    data.state = BOOT;
                }
                break;
            }
        }
    }
}