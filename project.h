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

#define MOTOR_OFFSET 110 // jotta luukku olisi paremmin lokeron kohdalla
#define dispense_intervall 30 // pillereiden jako väli sekunteina

#define BUTTON 7 //kalibrointi, ohjelman aloitus
#define RESET_BUTTON 8 //resetointi, eepromin alustus
#define LED_PIN 20
#define LED_INTERVAL 200000 //ledin vilkkumisen väli

//epromin asetuksia:
#define I2C_PORT i2c0
#define I2C_SDA_PIN 16
#define I2C_SCL_PIN 17
#define EEPROM_ADDR 0x50

#define EEPROM_STATE_ADDR 0x0002

//lora

#define LORA_APPKEY "585b2d2ae83096eb49df206a0e47d774" //vaihda omaan appkey

#define UART_ID uart1
#define BAUD_RATE 9600
#define UART_TX_PIN 4
#define UART_RX_PIN 5


typedef  enum {
    BOOT =0x00,
    PILL = 0x01
}State;

//structi jossa dataa säilytetään, anturien ja moottorien tilatietoja
typedef  struct {
    bool calibrated;
    bool piezeo_hit;
    bool lora_connected;
    int step_counts; //askelmäärien keskiarvo
    int current_step; //0-7
    int pill_counter; //annettujen lääkkeiden määrä
    bool motor_running; //onko moottori pyörimäss, hyödynnetään jotta tiedetään pyörikö moottori virrankatkaisun aikana
    State state; //tilatieto
    uint64_t last_motor_time;
} program_data;




//funktiomäärittelyt:
void calib(program_data *motor);
void recalib(program_data *motor); //peruutetaan optosensorille asti ja sitten takas siihen luukulle missä oltiin

void run_motor(program_data *motor, int steps,bool direction);
bool run_motor_30(program_data *motor);
void sensorHit(uint gpio, uint32_t event_mask);

//eeprom:
void init_eeprom();
void write_status_to_eeprom(program_data state);
bool read_status_from_eeprom(program_data* state);

bool set_up_lora();
void sen_lora_msg(const char *msg);

#endif //PROJECT_H