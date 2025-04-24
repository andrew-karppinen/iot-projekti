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
    i2c_init(I2C_PORT, 100 * 1000); // 100 kHz
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);
}

void sensorHit(uint gpio, uint32_t event_mask) {
    data.piezeo_hit = true;
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
}

int main() {
    init_pins();

    init_data(&data);

    uint64_t last_toggle = time_us_64();
    bool led_state = false;
    while (1) {
        if (!read_button(RESET_BUTTON)) {
            printf("resetointi\n");
            init_data(&data);
            write_status_to_eeprom(data);
        }

        // Tällä hetkellä vaatii lora yhteyden kalibroinnin aloittamiseen
        switch (data.state) {
            case BOOT:
                printf("Connecting to Lora...\n");
                    init_lora();
            // Ping
            if (!ping_lora()) {
                printf("LoRa ping failed\n");
                break;
            }
            //Configure
            if (!configure_lora()) {
                printf("LoRa configure failed\n");
                break;
            }
            // Join
            if (!join_lora()) {
                printf("LoRa join failed\n");
                break;
            }
            printf("LoRa ready\n");

            // odotetaan käyttäjää
            while (read_button(BUTTON)) {
                uint64_t now = time_us_64();
                if (now - last_toggle >= LED_INTERVAL) {
                    led_state = !led_state;
                    gpio_put(LED_PIN, led_state);
                    last_toggle = now;
                }
            }
            calib(&data);
            if (data.calibrated) {
                data.state = PILL;
                write_status_to_eeprom(data);
                sen_lora_msg("Calibrated");
            }
            break;

            case PILL: {
                static uint64_t last_motor_time = 0;
                static bool no_pill_sent = true;

                uint64_t now = time_us_64();

                // moottori
                if (run_motor_30(&data)) {
                    data.pill_counter++;
                    write_status_to_eeprom(data);

                    char buf[32];
                    snprintf(buf, sizeof(buf), "Current lokero:%d", data.pill_counter);
                    sen_lora_msg(buf);

                    last_motor_time = now;
                    no_pill_sent = false;
                }

                // Ei tällä hetkellä jostain syystä lähetä pilleri tunnistetty viestiä vaikka piezo sen tunnistaa
                // Pill detected
                if (data.piezeo_hit) {
                    char buffer[32];
                    snprintf(buffer, sizeof(buffer), "Pill detected from lokero: %d", data.pill_counter);
                    sen_lora_msg(buffer);
                    printf("hit!\n");
                    data.piezeo_hit = false;
                    no_pill_sent = true;
                }

                // jos pilleriä ei tunnisteta 10 sekunnin sisällä lähetetään viesti
                if (!data.piezeo_hit && !no_pill_sent && (now - last_motor_time > 10 * 1000 * 1000)) {
                    char buffer[32];
                    snprintf(buffer, sizeof(buffer), "No pill detected from lokero: %d", data.pill_counter);
                    sen_lora_msg(buffer);
                    no_pill_sent = true;
                }

                break;
            }
        }
    }
}