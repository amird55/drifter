// speed calibration
//-------------------
long speed_cal_start_time;
long speed_cal_finish_time;
float max_speed_m2sec;
float highest_val=0.5;
float deepest_val=0;
bool isCalibrating=false;

#define CAL_STUCK_LEN 15 // 5sec * 3 samples at sec
float calibration_depth_history[CAL_STUCK_LEN];
int cal_hist_index=0;
bool cal_hist_enough_data=false;
long cal_start_stuck_time_sec=0;
long cal_stuck_time_diff;

bool calibrationNeeded(){
  bool ret=false;
  int cal_line_num=-1;
  for (int k=0;k<MAX_SECTIONS;k++){
    Serial.print("calibrationNeeded:courseParams[");
    Serial.print(k,DEC);
    Serial.print("].pType=");
    Serial.println(courseParams[k].pType);
    if((courseParams[k].pType=='c')||(courseParams[k].pType=='C')){
      cal_line_num=k;
      if(courseParams[k].pValH > courseParams[k].pValL){
        deepest_val=courseParams[k].pValH;
        highest_val=courseParams[k].pValL;
      } else {
        deepest_val=courseParams[k].pValL;
        highest_val=courseParams[k].pValH;
      }
      ret=true;
      break;
    }
  }
  if(cal_line_num >=0){ // remove line and shrink
    for(int k=cal_line_num;k<MAX_SECTIONS-1;k++){
      courseParams[k]=courseParams[k+1];
    }
    courseParams[MAX_SECTIONS-1].pType='\0';
  }
  return  ret;
}

// find deepest point at file
void find_deepest_point(){
  for (int k=0;k<MAX_SECTIONS;k++){
    if((courseParams[k].pType=='E')||(courseParams[k].pType=='e')){
      break;
    }
    if((courseParams[k].pType=='D')||(courseParams[k].pType=='d')){
  Serial.print("find_deepest_point::");
  Serial.print("   courseParams[k].pType=");
  Serial.print(courseParams[k].pType);
  Serial.print("    courseParams[k].pValH=");
  Serial.print(courseParams[k].pValH,DEC);
  Serial.print("    courseParams[k].pValL=");
  Serial.print(courseParams[k].pValL,DEC);
  Serial.print("    deepest_val=");
  Serial.print(deepest_val,DEC);
  Serial.println(" ");
      if (courseParams[k].pValH > deepest_val){
        deepest_val=courseParams[k].pValH;
      }
      if (courseParams[k].pValL > deepest_val){
        deepest_val=courseParams[k].pValL;
      }
    }
  }
}

bool cal_wait_4_full_speed=false;
void cal_accellaration(){
    if(!isCalibrating){
      if(ng_curr_speed == esc_max_microsec){ // wait 4 full speed
        isCalibrating=true;
        cal_wait_4_full_speed=false;
        // save time
        start_calibration_section();
      }
    }
}
void cal_start_engine(){
  if(!isCalibrating){
    ng_fullSpeedUP();
    cal_wait_4_full_speed=true;
  }
}
void cal_just_finished(){
      //set calibrating data
      finish_speed_cal();
      //-------
      isCalibrating=false;
      cal_wait_4_full_speed=false;
      //shut engine off
      ng_stop();
}
void start_calibration_section(){
  // save time
  speed_cal_start_time=floor(millis()/1000);
  deepest_val=currentDepth; //setting the depth calibration started
  Serial.print("-+-+-+-+-+-+-+-+----- start_calibration_section    deepest_val=");
  Serial.print(deepest_val,DEC);
}
void set_finish_cal_time(char timeType){
  // save time of finish
  switch(timeType){
    case 'c': // for current
            speed_cal_finish_time=floor(millis()/1000);
            break;
    case 's': // by stuck
            speed_cal_finish_time=floor(millis()/1000) - cal_stuck_time_diff;
            break;
  }
  highest_val=currentDepth; //setting the depth calibration ended
  Serial.print("-+-+-+-+-+-+-+-+----- set_finish_cal_time    highest_val=");
  Serial.println(highest_val,DEC);
}
void finish_speed_cal(){
  // calculate co-eficient for speed
  int cal_time=speed_cal_finish_time-speed_cal_start_time;
  max_speed_m2sec=(deepest_val-highest_val)/cal_time;
  
  String str="finish_speed_cal:: deepest_val=";
  str.concat(String(deepest_val));
  str.concat("  highest_val=");
  str.concat(String(highest_val));
  str.concat("  cal_time=");
  str.concat(String(cal_time));
  str.concat("  max_speed_m2sec=");
  str.concat(String(max_speed_m2sec));
  saveLineToCsv(str);
  Serial.println(str);
  
}
//---------------------

// speed set
void speed_set(float m2sec){
  float ratio = (m2sec < max_speed_m2sec) ? m2sec / max_speed_m2sec : 1;
  thruster_calibrated_speed=esc_zero_microsec+ratio*(esc_max_microsec-esc_zero_microsec);
  Serial.println(" ");
  Serial.println("-------------------------------------speed_set--------------------------");
  Serial.print("    m2sec=");
  Serial.print(m2sec,DEC);
  Serial.print("    max_speed_m2sec=");
  Serial.print(max_speed_m2sec,DEC);
  Serial.print("    ratio=");
  Serial.print(ratio,DEC);
  Serial.print("    thruster_calibrated_speed=");
  Serial.print(thruster_calibrated_speed,DEC);
  Serial.println(" ");
  Serial.println("-------------------------------------speed_set--------------------------");
  String str="speed_set:: m2sec=";
  str.concat(String(m2sec));
  str.concat("  max_speed_m2sec=");
  str.concat(String(max_speed_m2sec));
  str.concat("  thruster_calibrated_speed=");
  str.concat(String(thruster_calibrated_speed));
  saveLineToCsv(str);
}


void calibration_collect_hist(){
  if((!cal_hist_enough_data) && (cal_hist_index==0)){
    cal_start_stuck_time_sec=floor(millis()/1000);
  }
  calibration_depth_history[cal_hist_index]=currentDepth;
  cal_hist_index++;
  if(cal_hist_index>=CAL_STUCK_LEN){
    if(!cal_hist_enough_data){
      cal_stuck_time_diff=floor(millis()/1000) - cal_start_stuck_time_sec;
    }
    cal_hist_enough_data=true;
    cal_hist_index=0;
  }
}
bool calibration_stuck_check(){
  bool ret=false;
  float min_depth;
  float max_depth;
  float cal_depth_var=0.3;
  if(cal_hist_enough_data){
    min_depth=calibration_depth_history[0];
    max_depth=calibration_depth_history[0];
    for(int kk=1;kk<CAL_STUCK_LEN;kk++){
      if(min_depth > calibration_depth_history[kk] ){
        min_depth=calibration_depth_history[kk];
      }
      if(max_depth < calibration_depth_history[kk] ){
        max_depth=calibration_depth_history[kk];
      }
    }

    if(abs(min_depth-max_depth)<=cal_depth_var){
      ret=true;
    }
    String str="calibration_stuck_check:: ";
    str.concat("  min_depth=");
    str.concat(String(min_depth));
    str.concat("  max_depth=");
    str.concat(String(max_depth));
    str.concat("  cal_depth_var=");
    str.concat(String(cal_depth_var));
    str.concat("  result=");
    if(ret){
      str.concat("true");
    }else {
      str.concat("false");
    }
    saveLineToCsv(str);
  
  
  }
  return ret;
}
