
#ifndef PROJECT_H
#define PROJECT_H

#define OPTO_PIN 28 //opto pinni

#define MOTOR_SPEED_DELAY 1  //moottorin nopeus

//askelmoottori pinnit:
#define IN1 2
#define IN2 3
#define IN3 6
#define IN4 13

#define BUTTON 7
#define LED_PIN 20
#define LED_INTERVAL 200000 // ledin vilkutuksen ajastus, 100 ms = 100000 mikrosekuntia


//structi jossa dataa ja ohjelman tilaa säilytetään
typedef  struct {
    bool calibrated;
    int step_counts; //askelmäärien keskiarvo
    int current_step; //0-7
} program_data;


//funktiomäärittelyt:

void calib(program_data *motor);
void run_motor(program_data motor,int steps);

#endif //PROJECT_H
