//------stuck protection------------
float depth_var=0.5;
int stuck_time_min = 1;
float stuck_check_min_depth_meter=1.0;
long start_stuck_time_sec;
long current_stuck_time_sec;
bool am_i_stuck=false;
bool enough_data=false;
bool engine_on_when_stuck;
#define DEPTH_LENGTH 30
float last_depth_arr[DEPTH_LENGTH];

int last_depth_index=0;
float min_depth,max_depth;
void setStuckParams(float x,float dp,int t){
  depth_var=dp;
  stuck_time_min=t;
  stuck_check_min_depth_meter=x;
}
void stuckStartFresh(){
  am_i_stuck=false;
  enough_data=false;
  initArrays();
}
void initArrays(){
  for(int kks=0;kks<DEPTH_LENGTH;kks++){
    last_depth_arr[kks]=0.0;
  }
}
void release_from_stuck(){
  String str="release_from_stuck:: ";
  str.concat("  engine_on_when_stuck=");
  if(engine_on_when_stuck){
    str.concat("true");
  }else {
    str.concat("false");
  }
  saveLineToCsv(str);

  if(engine_on_when_stuck){
  Serial.println(" ");
  Serial.println("-------------------------------------release_from_stuck--------------------------");
  Serial.print("    stopping engine");
  Serial.println(" ");
  Serial.println("-------------------------------------release_from_stuck--------------------------");
    //trusther working
    ng_stop();
  }
  else {
    // start trusther
    ng_fullSpeedUP();
  Serial.println(" ");
  Serial.println("-------------------------------------release_from_stuck--------------------------");
  Serial.print("    full speed ahead");
  Serial.println(" ");
  Serial.println("-------------------------------------release_from_stuck--------------------------");
  }
}
bool check_if_stuck(){
  bool ret=false;
  last_depth_arr[last_depth_index]=currentDepth;
  last_depth_index++;
  if(last_depth_index>=DEPTH_LENGTH){
    enough_data=true;
    last_depth_index=0;
  }
//  last_depth_index = (++last_depth_index) % DEPTH_LENGTH;
  if(enough_data){
    min_depth=last_depth_arr[0];
    max_depth=last_depth_arr[0];
    for(int kk=1;kk<DEPTH_LENGTH;kk++){
      if(min_depth > last_depth_arr[kk] ){
        min_depth=last_depth_arr[kk];
      }
      if(max_depth < last_depth_arr[kk] ){
        max_depth=last_depth_arr[kk];
      }
    }
    Serial.println(" ");
    Serial.print("---check_if_stuck----");
    Serial.print("    min_depth=");
    Serial.print(min_depth,DEC);
    Serial.print("    max_depth=");
    Serial.print(max_depth,DEC);
    Serial.print("    currentDepth=");
    Serial.print(currentDepth,DEC);
    Serial.print("    stuck_check_min_depth_meter=");
    Serial.print(stuck_check_min_depth_meter,DEC);
    Serial.println(" ");
    if((currentDepth < minDepthForThrusterWorking)||(currentDepth < stuck_check_min_depth_meter)){
      am_i_stuck=false;
    }
    else {
      if(abs(min_depth-max_depth)<=depth_var){
        current_stuck_time_sec=floor(millis()/1000);
        
  String str="check_if_stuck:: current_stuck_time_sec=";
  str.concat(String(current_stuck_time_sec));
  str.concat("  start_stuck_time_sec=");
  str.concat(String(start_stuck_time_sec));
  str.concat("  stuck_time_min=");
  str.concat(String(stuck_time_min));
  str.concat("  min_depth=");
  str.concat(String(min_depth));
  str.concat("  max_depth=");
  str.concat(String(max_depth));
  str.concat("  depth_var=");
  str.concat(String(depth_var));
  str.concat("  am_i_stuck=");
  if(am_i_stuck){
    str.concat("true");
  }else {
    str.concat("false");
  }
  saveLineToCsv(str);
  
        if(am_i_stuck){
          Serial.println("---check_if_stuck-- i am stuck--");
          int stuck_sec=current_stuck_time_sec - start_stuck_time_sec ;
          int stuck_lim_sec = stuck_time_min*60;
          str="release check:: stuck_sec=";
          str.concat(String(stuck_sec));
          str.concat("  stuck_lim_sec=");
          str.concat(String(stuck_lim_sec));
          saveLineToCsv(str);
          if(stuck_sec >= stuck_lim_sec){
//          if(current_stuck_time_sec - start_stuck_time_sec >= stuck_time_min*60){
            release_from_stuck();
          }
        }else {
          Serial.println("---check_if_stuck-- counting stuck time--");
          am_i_stuck=true;
          ret=true;
          start_stuck_time_sec=current_stuck_time_sec;
          engine_on_when_stuck=(ng_curr_speed == esc_zero_microsec)?false:true;
        }
      } else {
        am_i_stuck=false;
      }
    }
  }
  return ret;
}
//----------------------------------
