#include <stdio.h>
#include "pico/stdlib.h"
#include "project.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"


static  program_data data;



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
    gpio_init(PIEZO_SENSOR);
    gpio_pull_up(PIEZO_SENSOR);

    gpio_set_irq_enabled_with_callback(PIEZO_SENSOR, GPIO_IRQ_EDGE_FALL, true, &sensorHit); //keskeytys

    //eeprom yhteyden alustus, i2c:
    i2c_init(I2C_PORT, 100 * 1000); // 100 kHz
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);


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


void sensorHit(uint gpio, uint32_t event_mask) {
    data.piezeo_hit = true;
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

    data.calibrated = false;
    data.current_step = 0;
    data.step_counts = 0;
    data.piezeo_hit = false;
    data.pill_counter = 0;

    //tilan alustus
    data.state = BOOT;

    uint64_t last_toggle = time_us_64();  // nykyinen aika mikrosekunteina
    bool led_state = false;

    /*
     state 0/BOOT = käyttäjän odottaminen, kalibrointi
     state 1/PILL = dosetin toiminta
     */
    if (read_status_from_eeprom(&data)==false){
        //tilaa ei voitu lukea, asetetaan tilaksi BOOT
        data.state = BOOT;
        printf("EEPROM-luku epäonnistui");
    } else{
        //tulostetaan tila, debug:
        printf("state: %d\n", data.state);
        printf("kalibroitu: %d\n", data.calibrated);
        printf("askelmäärä: %d\n", data.step_counts);
        printf("pillerimäärä: %d\n", data.pill_counter);
    }



    while (1){
        switch (data.state) {
            case 0:
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
                    data.state = PILL; //vaihdetaan tila
                     write_status_to_eeprom(data); //tallennetaan tila eepromiin
                }
                break;

            case 1:
                //tähän pyöritys 30sec välein ja pilerin tiptumisen tunnistus
                if (run_motor_30(&data)) { // 30 sek välein pyörii
                    data.pill_counter++;
                    //tallennetaan tila eepromiin
                    write_status_to_eeprom(data);
                }
                if (data.piezeo_hit) {
                    //tähän mitä tapahtuu kun pilleri tunnistetaan
                    printf("hit!");
                    data.piezeo_hit = false;
                }

                if (data.piezeo_hit) {
                    //jos pilleriä ei tunnisteta, led vilkkuu 5x
                    // korjataan. blink led blokkaa nyt piezo sensorin jatkuvan seuraamisen
                    //blink_led(5, 500);
                    sleep_ms(5); // adjusting the piezo sensor checking speed.
                }
                break;
                }

        }
    }