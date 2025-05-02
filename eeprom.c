#include <stdio.h>
#include "pico/stdlib.h"
#include "project.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"


/*
//tallennetaan myös moottorin kalibrointi ym tiedot eepromiin
 EEPROM DATA STRUCT:
 tavut 0, dosetin tila, (uint 8)
 tavut 1, calibroitu, boolean, 0x00, tai 0x01
 tavut 2-3, 2 tavua step_counts (uint_16)
 tavu 4, pill_counter (uint 8)
 tavu 5, moottorin pyöritys käynnissä, boolean, 0x00, tai 0x01
 tavu 6, tarkistussumma uint_8 //tietojen oikeellisuuden tarkistamiseen
 */


void init_eeprom() {
    //eeprom yhteyden alustus, i2c:
    i2c_init(I2C_PORT, 100 * 1000); // 100 kHz
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);
}

void write_status_to_eeprom(program_data state)
{

    uint8_t buffer[9];

    buffer[0] = (EEPROM_STATE_ADDR >> 8) & 0xFF; // MSB
    buffer[1] = EEPROM_STATE_ADDR & 0xFF;        // LSB

    // EEPROM DATA STRUCT alkaa alkiosta 2
    buffer[2] = (uint8_t)state.state;                         // tavu 0: dosetin tila
    buffer[3] = state.calibrated ? 0x01 : 0x00;               // tavu 1: calibroitu
    buffer[4] = (state.step_counts >> 8) & 0xFF;              // tavu 2: step_counts MSB
    buffer[5] = state.step_counts & 0xFF;                     // tavu 3: step_counts LSB
    buffer[6] = state.motor_running ? 0x01 : 0x00;            // tavu 4, motor running
    buffer[7] = (uint8_t)state.pill_counter;                  // tavu 5: pill_counter

    // Tarkistussumma yksinkertaisena XOR:na kaikista edellisistä kentistä (tavuista 2–7)
    buffer[8] = buffer[2] ^ buffer[3] ^ buffer[4] ^ buffer[5] ^ buffer[6] ^ buffer[7]; // tavu 5: tarkistussumma


    int ret = i2c_write_blocking(I2C_PORT, EEPROM_ADDR, buffer, 9, false);
    if (ret < 0) {
        printf("EEPROM-kirjoitus epäonnistui\n");
    }

    sleep_ms(20);
}


bool read_status_from_eeprom(program_data* state)
{
    /*
    palauttaa false jos luku epäonnistui,
    vika eeprom yhteydessä tai data korruptoitunut
    */

    uint8_t data[8] = {0};  // puskuri, johon luetaan

    uint8_t addr[2] = {(EEPROM_STATE_ADDR >> 8) & 0xFF, EEPROM_STATE_ADDR & 0xFF};    //EEPROM-osoite
    int ret = i2c_write_blocking(I2C_PORT, EEPROM_ADDR, addr, 2, true);
    if (ret < 0) {
        return false;  // Luku epäonnistui
    }

    ret = i2c_read_blocking(I2C_PORT, EEPROM_ADDR, data, sizeof(data), false);
    if (ret < 0) {
        return false;  // Luku epäonnistui
    }

    // Tarkista, että data ei ole korruptoitunut (tarkistussumma)
    if (data[6] != (data[0] ^ data[1] ^ data[2] ^ data[3] ^ data[4] ^data[5])) {
        return false;  // Tarkistussumma ei täsmää, data on korruptoitunut
    }



    // Aseta luettu tila
    state->state = (State)data[0];  // Dosetin tila
    state->calibrated = (data[1] == 0x01);  // Kalibroitu (0x01 = true, muuten false)
    state->step_counts = ((uint16_t)data[2] << 8) | data[3];  // step_counts, yhdistetään kaksi tavua
    state->motor_running = (data[4] == 0x01);
    state->pill_counter = data[5];  // pill_counter

    sleep_ms(20);
    return true;  // Luku onnistui
}






