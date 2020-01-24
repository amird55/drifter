#define MAX_SECTIONS  20
#define RANGE_PRCNT   5

typedef struct courseItemsParams{
  char pType='\0';
  float pValH;
  float pValL;
  short pDurationMin;
  short pVelocity;
}courseItemsParams;

courseItemsParams courseParams[MAX_SECTIONS];

bool verifySetupFile(){
  bool ret=true;
  int goodLine=0;
  bool fileIsFinished=false;
  String str="verifySetupFile:: started";
  saveLineToCsv(str);
  for (int k=0;k<MAX_SECTIONS;k++){
    Serial.print("verifySetupFile:courseParams[");
    Serial.print(k,DEC);
    Serial.print("].pType=");
    Serial.print(courseParams[k].pType);
    Serial.print("  goodLine=");
    Serial.print(goodLine,DEC);
    Serial.print("  fileIsFinished=");
    Serial.println(fileIsFinished,DEC);
    if((courseParams[k].pType=='\0')||(courseParams[k].pType==' ')){
      //empty line
      if(!fileIsFinished){
        fileIsFinished=true;
        if(goodLine == 0){ //no data given yet
          ret=false;
          str="verifySetupFile failed on line ";
          str.concat(k);
          str.concat("- empty line at start");
          saveLineToCsv(str);
          Serial.println(str);
        }
      }
    }
    else {
      
      if(fileIsFinished){
        //good line after an empty one
        ret=false;
        str="verifySetupFile failed on line ";
        str.concat(k);
        str.concat("- good line after an empty one");
        saveLineToCsv(str);
        Serial.println(str);
      }
      else {
        switch(courseParams[k].pType){
          case 'd':
          case 'D':
          case 't':
          case 'T': 
                    //nothing to check her, every parameter is ok
                    goodLine++;
//                    if((courseParams[k].pVelocity   courseParams[k].pValH-courseParams[k].pValL
                    break;
          case 'f':
          case 'F':
          case 'h':
          case 'H':
                    //check for not too many params 
                    if(courseParams[k].pValL < 500){
                      //too short thruster time
                      ret=false;
                      str="verifySetupFile failed on line ";
                      str.concat(k);
                      str.concat("- too short thruster time");
                      saveLineToCsv(str);
                      Serial.println(str);
                    }
                    else {
                      goodLine++;
                    }
                    break;
          case 's':
          case 'S':
          case 'e':
          case 'E':  
                    //nothing to check here
                    break;
        }
      }
    }
  }
  return  ret;
}
bool isPrd2minCanceled(){
  bool ret=false;
  int N_line_num=-1;
  String str="isPrd2minCanceled:: started";
  saveLineToCsv(str);
  for (int k=0;k<MAX_SECTIONS;k++){
    Serial.print("isPrd2minCanceled:courseParams[");
    Serial.print(k,DEC);
    Serial.print("].pType=");
    Serial.println(courseParams[k].pType);
    if((courseParams[k].pType=='n')||(courseParams[k].pType=='N')){
      N_line_num=k;
      str="isPrd2minCanceled:: found ";
      str.concat(String(courseParams[k].pType));
      str.concat("  on line ");
      str.concat(String(k));
      saveLineToCsv(str);
      ret=true;
      break;
    }
  }
  if(N_line_num >=0){ // remove line and shrink
    for(int k=N_line_num;k<MAX_SECTIONS-1;k++){
      courseParams[k]=courseParams[k+1];
    }
    courseParams[MAX_SECTIONS-1].pType='\0';
  }
  return  ret;
}

int getMinDepthForThruster(){
  int ret=0;
  int T_line_num=-1;
  String str="getMinDepthForThruster:: started";
  saveLineToCsv(str);
  for (int k=0;k<MAX_SECTIONS;k++){
    Serial.print("getMinDepthForThruster:courseParams[");
    Serial.print(k,DEC);
    Serial.print("].pType=");
    Serial.println(courseParams[k].pType);
    if((courseParams[k].pType=='p')||(courseParams[k].pType=='P')){
      T_line_num=k;
      str="getMinDepthForThruster:: found ";
      str.concat(String(courseParams[k].pType));
      str.concat("  on line ");
      str.concat(String(k));
      saveLineToCsv(str);
      ret=courseParams[k].pValH;
      break;
    }
  }
  if(T_line_num >=0){ // remove line and shrink
    for(int k=T_line_num;k<MAX_SECTIONS-1;k++){
      courseParams[k]=courseParams[k+1];
    }
    courseParams[MAX_SECTIONS-1].pType='\0';
  }
  return  ret;
}



float dst_upperLimit;
float dst_lowerLimit;
float midRange;

float maximum_allowed_depth = 80; // after that device will be broken
 
void setDepthRange(float wantedDepth_H,float wantedDepth_L){
  //lower limit is bigger value
  if(wantedDepth_H > wantedDepth_L){
    dst_upperLimit=wantedDepth_L;
    dst_lowerLimit=wantedDepth_H;
  } else {
    dst_upperLimit=wantedDepth_H;
    dst_lowerLimit=wantedDepth_L;
  }
  midRange = (dst_lowerLimit+dst_upperLimit)/2;
}
void setTempRange(float wantedDepth_H,float wantedDepth_L){
  //lower limit is smaller value
  if(wantedDepth_H < wantedDepth_L){
    dst_upperLimit=wantedDepth_L;
    dst_lowerLimit=wantedDepth_H;
  } else {
    dst_upperLimit=wantedDepth_H;
    dst_lowerLimit=wantedDepth_L;
  }
  midRange = (dst_lowerLimit+dst_upperLimit)/2;
}
void setParams(){
  workingMode=CHANGING_DEPTH;   //   STEADY_DEPTH  /  STEADY_TEMP  /  CHANGING_DEPTH
  waterType=SALT_WATER;   //  FRESH_WATER   /  SALT_WATER

//  setRange(0.5,1.5);
}
