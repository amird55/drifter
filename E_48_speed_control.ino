// speed calibration
//-------------------
long speed_cal_start_time;
long speed_cal_finish_time;
float max_speed_m2sec;
float highest_val=0.5;
float deepest_val=0;
bool isCalibrating=false;
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

void start_calibration_section(){
  // save time
  speed_cal_start_time=floor(millis()/1000);
}
void finish_speed_cal(){
  // save time of finish
  speed_cal_finish_time=floor(millis()/1000);

  // calculate co-eficient for speed
  max_speed_m2sec=(deepest_val-highest_val)/(speed_cal_finish_time-speed_cal_start_time);
  
  int cal_time=speed_cal_finish_time-speed_cal_start_time;
  String str="finish_speed_cal:: deepest_val=";
  str.concat(String(deepest_val));
  str.concat("  highest_val=");
  str.concat(String(highest_val));
  str.concat("  cal_time=");
  str.concat(String(cal_time));
  str.concat("  max_speed_m2sec=");
  str.concat(String(max_speed_m2sec));
  saveLineToCsv(str);
  
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
