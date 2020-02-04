/*
 * Send a stopped signal (1500 microseconds) 
 * for a few seconds 
 * to initialize the ESC. 
 * 
 * You will hear two tones indicating initialization, 
 * and then you can send a signal from 1100-1900 µs to operate the thruster.
 * 1500 = no spin
 * less than 1500 = one direction
 *  > 1500 = other direction
 * 
 * 
 * using library:
 * https://www.robotshop.com/community/blog/show/rc-speed-controller-esc-arduino-library
 */


#define ngPin 5  //should be a PWM  pin

#define esc_arm_microsec    1500
#define esc_zero_microsec   1500
#define esc_min_microsec    1900
#define esc_half_microsec   1200
#define esc_max_microsec    1100
//#define esc_half_microsec   1540
//#define esc_max_microsec    1570
#define ESC_HALF_POWER      1300
#define ESC_FULL_POWER      1100
#define esc_start_speed     1490

#include "ESC.h"

ESC myESC (ngPin, 1100, 1900, esc_arm_microsec);         // ESC_Name (PIN, Minimum Value, Maximum Value, Arm Value)
//ESC myESC (ngPin, esc_min_microsec, ESC_FULL_POWER, esc_arm_microsec);         // ESC_Name (PIN, Minimum Value, Maximum Value, Arm Value)

int thruster_calibrated_speed=ESC_FULL_POWER;
/*
 * need to change the working mode into state machine 
 * in order to implement soft start and soft stop
 * 
 * main loop has 100 ms delay. it should be enough for the counter of the soft transitions
 */
void ng_loop_sm(){
  short next_state=ng_curr_state;
//  Serial.print("ng_curr_speed=");
//  Serial.print(ng_curr_speed,DEC);
//  Serial.print("  ng_curr_state=");
//  Serial.println(ng_curr_state,DEC);
  switch(ng_curr_state){
    case NG_IDLE:
                  next_state = NG_IDLE;
                  goingUp=false;
                  ng_stop();
                  break;
    case NG_GET_STARTED:
                  next_state = NG_STARTING;
                  if(ng_curr_speed > ng_wanted_speed){
                    ng_step=1;
                    ng_direction=1;
                    if(ng_curr_speed > esc_start_speed){
                      ng_curr_speed=esc_start_speed;
                    }
                  }
                  else if(ng_curr_speed < ng_wanted_speed){
                    ng_step=1;
                    ng_direction=-1;
                  }
                  else {
                    ng_step=0;
                  }
                  ng_curr_speed-=ng_direction*ng_step;
                  ng_on(ng_curr_speed);
                  goingUp=true;
                  boostStartTime=millis();
                  Serial.print("NG_GET_STARTED ng_step=");
                  Serial.print(ng_step);
                  Serial.print("  ng_curr_speed=");
                  Serial.println(ng_curr_speed);
                  break;
    case NG_STARTING:
                  ng_step=ceil(ng_step*step_base);
                  ng_curr_speed-=ng_direction*ng_step;
                  if(ng_curr_speed < ESC_FULL_POWER){
                    ng_curr_speed = ESC_FULL_POWER;
                  }
                  if(ng_curr_speed < ng_wanted_speed){
                    ng_curr_speed = ng_wanted_speed;
                  }
                  if(ng_curr_speed > esc_min_microsec){
                    ng_curr_speed = esc_min_microsec;
                  }
                  ng_on(ng_curr_speed);
                  goingUp=true;
                  if(ng_step > 0){
                    next_state = (ng_curr_speed > ng_wanted_speed) ? NG_STARTING : NG_WORKING;
                  } else {
                    next_state = (ng_curr_speed < ng_wanted_speed) ? NG_STARTING : NG_WORKING;
                  }
                  Serial.print("NG_STARTING  ng_step=");
                  Serial.print(ng_step);
                  Serial.print("  ng_curr_speed=");
                  Serial.println(ng_curr_speed);
                  break;
    case NG_WORKING:
                  Serial.print("NG_WORKING  ng_curr_speed=");
                  Serial.println(ng_curr_speed);
                  ng_on(ng_curr_speed);
                  goingUp=true;
                  next_state = NG_WORKING;
                  break;
    case NG_READY_TO_STOP:
                  next_state = NG_STOPPING;
                  goingUp=false;
                  if(ng_curr_speed < ng_wanted_speed){
                    ng_step=floor((ng_wanted_speed - ng_curr_speed)/2);
                  }
                  else if(ng_curr_speed > ng_wanted_speed){
                    next_state = NG_GET_STARTED;
                  }
                  else {
                    ng_step=0;
                  }
                  ng_curr_speed =ng_curr_speed+ng_step;
                  ng_on(ng_curr_speed);
                  break;
    case NG_STOPPING:
                  goingUp=false;
                  ng_step=floor(ng_step/step_base);
                  if(ng_step < 1){
                    ng_step=1;
                  }
                  ng_curr_speed =ng_curr_speed+ng_step;
                  if(ng_curr_speed > ng_wanted_speed){
                    ng_curr_speed = ng_wanted_speed;
                  }
                  ng_on(ng_curr_speed);
                  next_state = (ng_curr_speed < ng_wanted_speed) ? NG_STOPPING : NG_IDLE;
                  break;
  }
  ng_curr_state=next_state;
  
}

void ng_full_stop(){
    ng_wanted_speed=esc_zero_microsec;
    ng_curr_speed=ng_wanted_speed;
    ng_on(ng_curr_speed);
    ng_curr_state=NG_IDLE;
}
void ng_setup(){
  myESC.arm();      // Send the Arm value
  ng_curr_speed=esc_arm_microsec;
}
void ng_stop(){
  if((ng_curr_state == NG_READY_TO_STOP)||(ng_curr_state == NG_STOPPING)||(ng_curr_state == NG_IDLE)){
    //no need to do something at these states
  } else {
    ng_wanted_speed=esc_zero_microsec;
    ng_curr_state=NG_READY_TO_STOP;
  }
}
void ng_emergancyMoveUP(){
    ng_wanted_speed=ESC_FULL_POWER;
    ng_curr_state=NG_GET_STARTED;
}
void ng_wantedSpeedUP(){
  setNewSpeed(thruster_calibrated_speed);
}
void ng_fullSpeedUP(){
  setNewSpeed(esc_max_microsec);
}
void ng_halfSpeedUP(){
  setNewSpeed(esc_half_microsec);
}
void ng_fullPower(){
  setNewSpeed(ESC_FULL_POWER);
}
void ng_HalfPower(){
  setNewSpeed(ESC_HALF_POWER);
}
void setNewSpeed(int spd){
        Serial.print("setNewSpeed spd= ");
        Serial.print(spd,DEC);
        Serial.print("   ng_wanted_speed= ");
        Serial.print(ng_wanted_speed,DEC);
        Serial.println("");
  if((ng_curr_state == NG_GET_STARTED)||(ng_curr_state == NG_STARTING)||(ng_curr_state == NG_WORKING)){
    if(ng_wanted_speed != spd){
      ng_wanted_speed=spd;
      ng_curr_state=NG_GET_STARTED;
    }
  } else {
    ng_wanted_speed=spd;
    ng_curr_state=NG_GET_STARTED;
  }
}
void ng_on(int spd){
  if(currentDepth < minDepthForThrusterWorking){
      light_Red();
      Serial.println("ng_on: ABOVE WATER -- stopping");
      saveLineToCsv("ng_on: ABOVE WATER -- stopping");
      myESC.speed(esc_zero_microsec); 
      delay(15);    
      light_Off();
  } else {
      Serial.print("ng_on: spd=");
      Serial.print(spd,DEC);
      myESC.speed(spd);     // sets the ESC speed according to the scaled value
//      Serial.print("......... activated");
      Serial.println(" ");
  }
}
void stopNow(){
  myESC.speed(esc_zero_microsec);
  saveLineToCsv("stopNow - stopping the thruster");

}

 
