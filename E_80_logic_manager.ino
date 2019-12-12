/*
 * logic should be adjusted to working with
 * state machine in the thruster control
 */

bool scaleSpeed(){
  bool ret=false;
  Serial.print("scaleSpeed::");
  Serial.print("   highest_val=");
  Serial.print(highest_val,DEC);
  Serial.print("    deepest_val=");
  Serial.print(deepest_val,DEC);
  Serial.print("    currentDepth=");
  Serial.print(currentDepth,DEC);
  Serial.println(" ");

    if(currentDepth < highest_val){ // too high
      finish_speed_cal();
      isCalibrating=false;
      //shut engine off
      ng_stop();
      ret=true;
    }
    else if(currentDepth < deepest_val){ 
      // we are in the range
      // continue with engine on
    }
    else {
        // in here, depth is more than lower limit
        if(!isCalibrating){
          if(ng_curr_speed == esc_max_microsec){ // wait 4 full speed
            isCalibrating=true;
            // save time
            start_calibration_section();
          }
        }
  //set thruster to start working
        ng_fullSpeedUP();
    }
  return ret;
}

void keepDepth(){
//  Serial.print("keepDepth::");
//  Serial.print("   dst_lowerLimit=");
//  Serial.print(dst_lowerLimit,DEC);
//  Serial.print("    dst_upperLimit=");
//  Serial.print(dst_upperLimit,DEC);
//  Serial.print("    currentDepth=");
//  Serial.print(currentDepth,DEC);
//  Serial.println(" ");

  if(shouldIpanic()){
      ng_emergancyMoveUP();
      Serial.println("panic");
  } else {
      if(currentDepth < dst_upperLimit){ // too high
        //shut engine off
        ng_stop();
      }
      else if(currentDepth < dst_lowerLimit){ 
        // we are in the range
        // continue with engine on
        // lower half of the range - full speed
        // upper half of range - half speed
          if(goingUp){
              if(currentDepth > midRange){ //slowing down towards the upper limit
//                ng_halfSpeedUP();
              }
          }
      }
      else {
          // in here, depth is more than lower limit
          ng_wantedSpeedUP();
      }
  }
}


void keepTemp(){
//  Serial.print("keepTemp::");
//  Serial.print("   dst_lowerLimit=");
//  Serial.print(dst_lowerLimit,DEC);
//  Serial.print("    dst_upperLimit=");
//  Serial.print(dst_upperLimit,DEC);
//  Serial.print("    currentTemp=");
//  Serial.print(currentTemp,DEC);
////  Serial.print("    ng_curr_speed=");
////  Serial.print(ng_curr_speed,DEC);
////  Serial.print("    ng_wanted_speed=");
////  Serial.print(ng_wanted_speed,DEC);
//  Serial.println(" ");

  if(shouldIpanic()){
      ng_emergancyMoveUP();
      Serial.println("panic");
  } else {
      if(currentTemp > dst_upperLimit){ // too high
        //shut engine off
        ng_stop();
        //debugln("stoping");
      }
      else if(currentTemp < dst_lowerLimit){ // too low
          ng_wantedSpeedUP();
      }
      else {
          if(goingUp){
//              if(currentTemp > midRange){ //slowing down towards the upper limit
//                ng_halfSpeedUP();
//              }
//              else {
//                ng_fullSpeedUP();
//              }
          }
      }
  }
}

bool isBoosting=false;
void boostFullOnDepth(){
  //check for working time, and stop if needed
  if((isBoosting)&&(millis()-boostStartTime > courseParams[lineIndex].pValL)){
    isBoosting=false;
    ng_stop();
  }
  if(currentDepth > dst_lowerLimit){
    isBoosting=true;
    ng_fullPower();
  }
}
void boostHalfOnDepth(){
  //check for working time, and stop if needed
  if((isBoosting)&&(millis()-boostStartTime > courseParams[lineIndex].pValL)){
    isBoosting=false;
    ng_stop();
  }
  if(currentDepth > dst_lowerLimit){
    isBoosting=true;
    ng_HalfPower();
  }
}
bool keepCurrentDepth(){
  bool ret=true;
  ret=checkForNextHop();
  if(ret){
    switch(courseParams[lineIndex].pType){
      case 'c':
      case 'C':scaleSpeed();break;
      case 'd':
      case 'D':keepDepth();break;
      case 't':
      case 'T':keepTemp();break;
      case 'f':
      case 'F':boostFullOnDepth();break;
      case 'h':
      case 'H':boostHalfOnDepth();break;
    }
  }
  return ret;
}

bool shouldIpanic(){
  bool ret=false;
  if(currentDepth > maximum_allowed_depth){
    ret = true;
  }
  return ret;
}

long lastHopStartTime;
long currentHopDuration;
void startHop(long dur){
  lastHopStartTime=floor(millis()/60000);
  currentHopDuration=dur;
  Serial.print("startHop::");
  Serial.print("   lastHopStartTime=");
  Serial.print(lastHopStartTime,DEC);
  Serial.print("    currentHopDuration=");
  Serial.print(currentHopDuration,DEC);
  Serial.println(" ");
}
void startFirstHop(){
  lineIndex=0;
  getNextHop();
}
void startFirstHopAfterCalibration(){
  lineIndex=1;
  getNextHop();
}
bool getNextHop(){
  bool ret=true;
  long int dur;
  float m2sec;
  if(courseParams[lineIndex].pType == '\0'){
    ret=false;
  }
  else {
    Serial.print("    courseParams[lineIndex].pDurationMin=");
    Serial.print(courseParams[lineIndex].pDurationMin,DEC);
    Serial.println(" ");
    switch(courseParams[lineIndex].pType){
      case 'd':
      case 'D': setDepthRange(courseParams[lineIndex].pValH,courseParams[lineIndex].pValL);
                Serial.println(" ******************************************************************");
                Serial.print("    courseParams[lineIndex].pVelocity=");
                Serial.print(courseParams[lineIndex].pVelocity,DEC);
                if(doneCalibration){
                  if(courseParams[lineIndex].pVelocity > 0){
                    m2sec=abs(courseParams[lineIndex].pValH-courseParams[lineIndex].pValL)/courseParams[lineIndex].pVelocity;
                    Serial.print("    m2sec=");
                    Serial.print(m2sec,DEC);
                    speed_set(m2sec);
                  }
                }
                Serial.println(" ");
                Serial.println(" ******************************************************************");
                break;
      case 't':
      case 'T': setTempRange(courseParams[lineIndex].pValH,courseParams[lineIndex].pValL);
                if(doneCalibration){
                  if(courseParams[lineIndex].pVelocity > 0){
                    m2sec=abs(courseParams[lineIndex].pValH-courseParams[lineIndex].pValL)/courseParams[lineIndex].pVelocity;
                    speed_set(m2sec);
                  }
                }
                break;
      case 'f':
      case 'F':
      case 'h':
      case 'H':
              dst_lowerLimit=courseParams[lineIndex].pValH;
              break;
      case 's':
      case 'S':
                setStuckParams(courseParams[lineIndex].pValH,courseParams[lineIndex].pValL,courseParams[lineIndex].pDurationMin);
                courseParams[lineIndex].pDurationMin=0;
                break;
      case 'e':
      case 'E':
              ret=false; // end of scenario, back to END_OF_JOB
              break;
    }
    dur=courseParams[lineIndex].pDurationMin;
    Serial.print("    courseParams[lineIndex].pDurationMin=");
    Serial.print(courseParams[lineIndex].pDurationMin,DEC);
    Serial.print("    dur=");
    Serial.print(dur,DEC);
    Serial.println(" ");
    if(courseParams[lineIndex].pDurationMin>0){
      startHop(courseParams[lineIndex].pDurationMin);
    }
    else{
      switch(courseParams[lineIndex].pType){
        case 'd':
        case 'D':workingMode=STEADY_DEPTH;break;
        case 't':
        case 'T':workingMode=STEADY_TEMP;break;
        case 's':
        case 'S':
              lineIndex++;
              ret=getNextHop();//end of scenario return false
      }
    }
  }
  return ret;
}
bool checkForNextHop(){
  long dd;
  bool ret=true;
  light_Off();
  Serial.print("checkForNextHop currentHopDuration=");
  Serial.print(currentHopDuration);
  Serial.print("   lastHopStartTime=");
  Serial.print(lastHopStartTime);
  Serial.print("   lineIndex=");
  Serial.println(lineIndex);
  if(currentHopDuration > 0){
    long nowmili=floor(millis()/60000);
  Serial.print("   nowmili=");
  Serial.println(nowmili);
    if( nowmili > lastHopStartTime+currentHopDuration){
      lineIndex++;
      Serial.println("--------------------checkForNextHop in the inner if");
      ret=getNextHop();//end of scenario return false
      if(ret){
        lightTheLed(CLR_ORANGE);
      }
    }
  }
  return ret;
}
