
#ifndef PROJECT_H
#define PROJECT_H

#define OPTO_PIN 28 //opto pinni
#define PIEZO_SENSOR 27 //piezo sensori

#define MOTOR_SPEED_DELAY 1  //moottorin nopeus
#define FALL_TIME 85 // voidaan käyttää jos tarvitsee. luukulta sensorille kuluva aika


//askelmoottori pinnit:
#define IN1 2
#define IN2 3
#define IN3 6
#define IN4 13

#define BUTTON 7
#define LED_PIN 20
#define LED_INTERVAL 200000 //ledin vilkkumisen väli

//epromin asetuksia:
#define I2C_PORT i2c0
#define I2C_SDA_PIN 16
#define I2C_SCL_PIN 17
#define EEPROM_ADDR 0x50

//tilan osoite, seuraava muistipaikka käytetään käänteiselle tilalle, joten sen oltava saatavilla
#define EEPROM_STATE_ADDR 0x0002



/*
//tallennetaan myös moottorin kalibrointi ym tiedot eepromiin
 EEPROM DATA STRUCT:
 tavut 0, dosetin tila, (uint 8)
 tavut 1, calibroitu, boolean, 0x00, tai 0x01
 tavut 2-3, 2 tavua step_counts (uint_16)
 tavu 4, pill_counter (uint 8)
 tavu 5, tarkistussumma uint_8 //tietojen oikeellisuuden tarkistamiseen

 */

typedef  enum {
    BOOT =0x00,
    PILL = 0x01,
    DISPENSED = 0x02,
    NOT_DISPENSED = 0x03,
    EMPTY = 0x04
}State;

//structi jossa dataa säilytetään, anturien ja moottorien tilatietoja
typedef  struct {
    bool calibrated;
    bool piezeo_hit;
    int step_counts; //askelmäärien keskiarvo
    int current_step; //0-7
    int pill_counter; //annettujen lääkkeiden määrä
    State state; //tilatieto

} program_data;




//funktiomäärittelyt:
void calib(program_data *motor);
void run_motor(program_data *motor,int steps);
bool run_motor_30(program_data *motor);
void sensorHit(uint gpio, uint32_t event_mask);

//eeprom:
void write_status_to_eeprom(program_data state);
bool read_status_from_eeprom(program_data* state);


#endif //PROJECT_H
