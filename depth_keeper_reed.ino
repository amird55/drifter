/*
 * SD card manager - to get parameter to follow on:
 *          1. temprature
 *          2. depth (1 value for all the experiment)
 *          3. depths per time - a depth for a period of time and then other depth
 *          
 *  RTC - needed for calculating time periods
 *        can be considered to calculate periods with time() and not RTC
 *        
 *  BAR sensor - https://www.bluerobotics.com/store/sensors-sonars-cameras/sensors/bar30-sensor-r1/
 *          getting the depth
 *          using library: https://github.com/bluerobotics/BlueRobotics_MS5837_Library
 *          example of use: https://github.com/bluerobotics/BlueRobotics_MS5837_Library/blob/master/examples/MS5837_Example/MS5837_Example.ino
 *          
 *          
 *  thruster control
 *        thruster - https://www.bluerobotics.com/store/thrusters/t100-t200-thrusters/t100-thruster/
 *        speed controller - https://www.bluerobotics.com/store/thrusters/speed-controllers/besc30-r3/
 *        I2C converter - https://www.bluerobotics.com/store/sensors-sonars-cameras/sensors/level-converter-r1/
 *        
 *  main manager - handling all components above for controlling the drifter
 *        
 */


/*
 * 5 , 5 , 10,10,20,20,40,40
 * max power all the time
 * 1 min break
 * 20 min wait at start
 */
#include <stdlib.h>

bool deskDebug=true;
bool initWait=false;
//bool deskDebug=false;
//bool initWait=true;

//this parmeter can be configured from SD
//bool prd2Min_On=true; // wait  2 minutes after being under water
bool prd2Min_On=false; // avoid waiting 2 minutes after being under water


#define pinRedLed 6
#define pinGreenLed 3

#define pinAnalogVin A11
int vinVal;


 #define NG_IDLE          1
 #define NG_GET_STARTED   2
 #define NG_STARTING      3
 #define NG_READY_TO_STOP 4
 #define NG_STOPPING      5
 #define NG_WORKING       6
 
 short ng_curr_state=NG_IDLE;
 int ng_curr_speed;
 int ng_wanted_speed;
 int ng_step;
 int ng_direction=1;
float step_base=1.9;

bool goingUp=false;
bool doneCalibration=false;

long boostStartTime; 

const int short_cycle_ms=300;
const int log_cycle_sec=1;
int log_cntr;

float currentDepth; // used for panic floating
float currentTemp;

float minDepthForThrusterWorking=0;

short workingMode;   
short waterType; 
#define FRESH_WATER 1
#define SALT_WATER  2

#define STEADY_DEPTH      11
#define STEADY_TEMP       12
#define CHANGING_DEPTH    13
#define CALIBRATE_SPEED   14

#define CLR_RED     1
#define CLR_GREEN   2
#define CLR_ORANGE  3
#define CLR_YELLOW  4

void lightTheLed(int clr){
  switch(clr){
    case CLR_RED:     light_Red();    break;
    case CLR_GREEN:   light_Green();  break;
    case CLR_ORANGE:  light_Orange(); break;
    case CLR_YELLOW:  light_Yellow(); break;
  }
}
void light_Orange(){
  analogWrite(pinRedLed, 190);
  analogWrite(pinGreenLed, 200);
}
void light_Yellow(){
  analogWrite(pinRedLed, 150);
  analogWrite(pinGreenLed, 200);
}
void light_Red(){
  digitalWrite(pinRedLed, HIGH);
  digitalWrite(pinGreenLed, LOW);
}
void light_Green(){
  digitalWrite(pinRedLed, LOW);
  digitalWrite(pinGreenLed, HIGH);
}
void light_Off(){
  digitalWrite(pinRedLed, LOW);
  digitalWrite(pinGreenLed, LOW);
}

int pcnt=0;
short pidx=0;

#define WAIT_TO_WATER     41
#define WAIT_1_MIN        42
#define WAIT_2_MIN        43
#define START_TO_WORK     44
#define WORK              45
#define END_OF_JOB        46
#define CALIBRATE         47

#define MAGNET_PIN  2
int currentPhase;
void setup() {
  
  Serial.begin(9600);
  Serial.println("Starting");
  
  pinMode(MAGNET_PIN, INPUT_PULLUP);
  
  pinMode(pinRedLed, OUTPUT);
  pinMode(pinGreenLed, OUTPUT);
  light_Off();

  setParams();
  stuckStartFresh();
  sd_setup();
  readFromLogFile();
  delay(1000);
  if(!deskDebug){
    init_bar();
  }
  
  delay(1000);
  ng_setup();
  delay(1000);
//  workingMode=1;//for debug


  if(!deskDebug){
    currentPhase=WAIT_TO_WATER;
  } else {
    currentPhase=START_TO_WORK;
  }

  
  log_cntr=floor(log_cycle_sec*1000 / short_cycle_ms);

  if(calibrationNeeded()){
    workingMode=CALIBRATE_SPEED;
    //find_deepest_point();
  }
  if(isPrd2minCanceled()){
//    prd2Min_On=false;
//    saveLineToCsv("2 min cancelled");
    prd2Min_On=true;
    saveLineToCsv("2 min wait is on");
  }
  
  minDepthForThrusterWorking=getMinDepthForThruster();
}


int working_cnt = 17; // every 3 is a second
int rest_cnt = 90; // every 3 is a second

#define PERIOD_NUM 8
int periods[PERIOD_NUM]={15,15,30,30,60,60,120,120};
//int periods[PERIOD_NUM]={5,5,10,10,20,20,40,40}; //seconds
bool inRest=false;
bool hasRest=true;

int workSpeed=1100;
int thruster_speed=workSpeed; //start with power

  int depthToStart=2;
  int safeguard=40; // how many good reading before start
  int safeguard_cnt=0;
bool wait_for_water(){
  bool ret=true;

  getSensorValues();
  saveToLogFile();
  if(currentDepth < depthToStart){
    lightTheLed(CLR_YELLOW);
    delay(1000);
    light_Off();
  }
  else {
    lightTheLed(CLR_RED);
    delay(100);
    safeguard_cnt++;
    Serial.print("******************* safeguard_cnt=");
    Serial.println(safeguard_cnt);
    if(safeguard_cnt > safeguard){
      ret=false;
    }
  }
  return ret;
}
int min_cnt;
int wait_clr;
bool wait_1_min(){
  bool ret=true;
  lightTheLed(wait_clr);
  getSensorValues();
  saveToLogFile();
  delay(500);
  light_Off();
  delay(500);
  min_cnt++;
  if(min_cnt>60){
    ret=false;
  }
  return ret;
}

bool magnet_close(){
  bool ret=false;
  if(digitalRead(MAGNET_PIN)==LOW){
    ret=true;
    Serial.println("............found a magnet .....................");
  }
  return ret;
}
void startFresh(){
  Serial.println("............started FRESH");
  safeguard_cnt=0;
  currentPhase=WAIT_TO_WATER;
  working_cnt = 17;
  rest_cnt = 90; 
  pcnt=0;
  pidx=0;
  inRest=false;

  stuckStartFresh();
}
void loop() {
  calibration_collect_hist();
  int nextPhase=currentPhase;
  switch(currentPhase){
    case WAIT_TO_WATER:
                      if(wait_for_water()){
                        nextPhase=WAIT_TO_WATER;
                      }
                      else {
                        if(prd2Min_On){
                          saveLineToCsv("wait_for_water() finished. prd2Min_On=true. move to WAIT_1_MIN" );
                          nextPhase=WAIT_1_MIN;
                          min_cnt=0;
                          wait_clr=CLR_GREEN;
                        } else {
                          saveLineToCsv("wait_for_water() finished. prd2Min_On=false. move to START_TO_WORK" );
                          nextPhase=START_TO_WORK;
                        }
                      }
                      break;
    case WAIT_1_MIN:
                      if(wait_1_min()){
                        nextPhase=WAIT_1_MIN;
                      }
                      else {
                        nextPhase=WAIT_2_MIN;
                        min_cnt=0;
                        wait_clr=CLR_RED;
                      }
                      break;
    case WAIT_2_MIN:
                      nextPhase=(wait_1_min())?WAIT_2_MIN:START_TO_WORK;
                      break;
    case START_TO_WORK:
      Serial.println("START_TO_WORK");
                      nextPhase=WORK;
                      if(workingMode==CHANGING_DEPTH){
                        startFirstHop();
                      }
                      if(workingMode==CALIBRATE_SPEED){
                        nextPhase=CALIBRATE;
                      }
                      break;
    case CALIBRATE:
      Serial.println("CALIBRATE");
                      nextPhase=CALIBRATE;
                      if(calibratePhase()){
                        doneCalibration=true;
                        workingMode=CHANGING_DEPTH;
                        nextPhase=WORK;
                        startFirstHop();
//                        startFirstHopAfterCalibration();
                      }
                      break;
    case WORK:
      Serial.println("WORK");
                      nextPhase=(workPhase())?WORK:END_OF_JOB;
                      break;
    case END_OF_JOB:
                      ng_stop();
                      ng_loop_sm();    
                      delay(short_cycle_ms);
                      nextPhase=END_OF_JOB;
                      break;
  }
  currentPhase=nextPhase;
  if(magnet_close()){
    startFresh();
  }
}
bool calibratePhase(){
  bool ret=false;
  int th=floor(log_cycle_sec*500 / short_cycle_ms);
  if(log_cntr > th){
    light_Orange();
  }else {
    light_Green();
  }
  getSensorValues();
  ret=scaleSpeed();
  ng_loop_sm();    
  delay(short_cycle_ms);

  log_cntr--;
  if(log_cntr == 0){
    saveToLogFile();
    log_cntr=floor(log_cycle_sec*1000 / short_cycle_ms);
  }
  if(ret){
    light_Off();
  }
  return ret;
}
bool workPhase(){
  bool ret=true;
  getSensorValues();
  if(check_if_stuck()){
    release_from_stuck();
  } else {
    switch(workingMode){
      case STEADY_DEPTH:
                        keepDepth();
                        break;
      case STEADY_TEMP:
                        keepTemp();
                        break;
      case CHANGING_DEPTH:
                        ret=keepCurrentDepth();
                        break;
    }
  }
  ng_loop_sm();    
  delay(short_cycle_ms);

  log_cntr--;
  if(log_cntr < 3){
    light_Green();
  } else {
    light_Off();
  }
  if(log_cntr == 0){
    saveToLogFile();
    log_cntr=floor(log_cycle_sec*1000 / short_cycle_ms);
  }
  
  return ret;
}
