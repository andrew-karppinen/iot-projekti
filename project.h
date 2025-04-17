
#ifndef PROJECT_H
#define PROJECT_H
#endif //PROJECT_H

#define OPTO_PIN 28 //opto pinni

#define MOTOR_SPEED_DELAY 1  //moottorin nopeus

//askelmoottori pinnit:
#define IN1 2
#define IN2 3
#define IN3 6
#define IN4 13

#define BUTTON 9

typedef  struct {
    bool calibrated;
    int step_counts; //askelmäärien keskiarvo
} state;


//funktiomäärittelyt:
void calib(state *motor);
void run_motor(state motor,int steps);

