#include <stdio.h>
#include "pico/stdlib.h"
#include "project.h"


// Puoliaskel-sekvenssi (8 vaihetta)
const uint8_t halfstep_sequence[8][4] = {
    {1, 0, 0, 0},
    {1, 1, 0, 0},
    {0, 1, 0, 0},
    {0, 1, 1, 0},
    {0, 0, 1, 0},
    {0, 0, 1, 1},
    {0, 0, 0, 1},
    {1, 0, 0, 1}
};

void step_once(int step_index) {
    gpio_put(IN1, halfstep_sequence[step_index][0]);
    gpio_put(IN2, halfstep_sequence[step_index][1]);
    gpio_put(IN3, halfstep_sequence[step_index][2]);
    gpio_put(IN4, halfstep_sequence[step_index][3]);
}

void run_motor(program_data motor,int steps) {
    if(motor.calibrated==false) {
        printf("No calibrated\n");
        return;
    }

    for(int i=0;i<=steps;i++){
        sleep_ms(MOTOR_SPEED_DELAY);

        if(motor.current_step==8){ //curent step pyörii väliä 0-7
            motor.current_step=0;
        }

        step_once(motor.current_step);
        motor.current_step++;
    }
}


void calib(program_data *motor) {

    int step_counter=0;
    int counter=0; //kierrosten määrä

    bool status= gpio_get(OPTO_PIN);
    bool new_status;

    int i=motor->current_step;
    while (1){

        if(i==8){ //i pyörii 0-7
            i=0;
        }


        step_once(i);
        sleep_ms(MOTOR_SPEED_DELAY);
        if(counter>0) { //ekalta kierrokselta ei lasketa
            step_counter++;
        }
        new_status = gpio_get(OPTO_PIN);

        if(status ==1  && new_status==0){  //tilanmuuts 1--->0 havaittu
            counter++;
        }
        status = new_status;  // Päivitetään status

        if(counter==4){
            break;
        }
        i++;
    }

    motor->current_step = i;
    motor->calibrated = true;
    motor->step_counts = step_counter/3; //lasketaan keskiarvo


    run_motor(*motor,110); //pyöritetään hiukan jotta luukku osuu paremmin kohdalle


}
