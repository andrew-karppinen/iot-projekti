#include <stdio.h>
#include "pico/stdlib.h"
#include "project.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"

/*
// EEPROM DATA STRUCT:
 tavut 0: dosetin tila, (uint8_t)
 tavu 1: calibroitu, boolean, 0x00 tai 0x01
 tavut 2-3: step_counts (uint16_t)
 tavu 4: motor_running (uint8_t)
 tavu 5: pill_counter (uint8_t)
 tavu 6: CRC8-tarkistussumma (uint8_t)
*/

// CRC8 laskenta
uint8_t calculate_crc8(const uint8_t *data, size_t len) {
    uint8_t crc = 0x00;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x80)
                crc = (crc << 1) ^ 0x07;
            else
                crc <<= 1;
        }
    }
    return crc;
}

void init_eeprom() {
    // EEPROM-yhteyden alustus I2C:llä
    i2c_init(I2C_PORT, 100 * 1000); // 100 kHz
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);
}

void write_status_to_eeprom(program_data state) {
    uint8_t buffer[9];

    buffer[0] = (EEPROM_STATE_ADDR >> 8) & 0xFF; // MSB
    buffer[1] = EEPROM_STATE_ADDR & 0xFF;        // LSB

    // EEPROM DATA STRUCT alkaa alkiosta 2
    buffer[2] = (uint8_t)state.state;                    // tavu 0
    buffer[3] = state.calibrated ? 0x01 : 0x00;          // tavu 1
    buffer[4] = (state.step_counts >> 8) & 0xFF;         // tavu 2
    buffer[5] = state.step_counts & 0xFF;                // tavu 3
    buffer[6] = state.motor_running ? 0x01 : 0x00;       // tavu 4
    buffer[7] = (uint8_t)state.pill_counter;             // tavu 5

    // CRC8-tarkistussumma tavuista 2–7
    buffer[8] = calculate_crc8(&buffer[2], 6);

    int ret = i2c_write_blocking(I2C_PORT, EEPROM_ADDR, buffer, 9, false);
    if (ret < 0) {
        printf("EEPROM-kirjoitus epäonnistui\n");
    }

    sleep_ms(20);
}

bool read_status_from_eeprom(program_data* state) {
    /*
    palauttaa false jos luku epäonnistui,
    vika eeprom-yhteydessä tai data korruptoitunut
    */

    uint8_t data[8] = {0};  // sisältää 6 kenttää + 1 tarkistussumma

    uint8_t addr[2] = {(EEPROM_STATE_ADDR >> 8) & 0xFF, EEPROM_STATE_ADDR & 0xFF};    // EEPROM-osoite
    int ret = i2c_write_blocking(I2C_PORT, EEPROM_ADDR, addr, 2, true);
    if (ret < 0) {
        return false;
    }

    ret = i2c_read_blocking(I2C_PORT, EEPROM_ADDR, data, sizeof(data), false);
    if (ret < 0) {
        return false;
    }

    // CRC8 on viimeisessä tavussa
    uint8_t expected_crc = data[6];
    if (calculate_crc8(data, 7) != 0) { //tarkistussumma mukaanluettuna tuloksen pitäisi olla 0
        return false;
    }

    // Aseta luettu tila
    state->state = (State)data[0];
    state->calibrated = (data[1] == 0x01);
    state->step_counts = ((uint16_t)data[2] << 8) | data[3];
    state->motor_running = (data[4] == 0x01);
    state->pill_counter = data[5];

    sleep_ms(20);
    return true;
}
