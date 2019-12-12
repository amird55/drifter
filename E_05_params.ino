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

bool calibrationNeeded(){
  bool ret=false;
  for (int k=0;k<MAX_SECTIONS;k++){
      Serial.print("calibrationNeeded:courseParams[");
      Serial.print(k,DEC);
      Serial.print("].pType=");
      Serial.println(courseParams[k].pType);
    if((courseParams[k].pType=='c')||(courseParams[k].pType=='C')){
      ret=true;
      break;
    }
  }
  return  ret;
}
bool isPrd2minCanceled(){
  bool ret=false;
  String str="isPrd2minCanceled:: started";
  saveLineToCsv(str);
  for (int k=0;k<MAX_SECTIONS;k++){
      Serial.print("calibrationNeeded:courseParams[");
      Serial.print(k,DEC);
      Serial.print("].pType=");
      Serial.println(courseParams[k].pType);
    if((courseParams[k].pType=='n')||(courseParams[k].pType=='N')){
      str="isPrd2minCanceled:: found ";
      str.concat(String(courseParams[k].pType));
      str.concat("  on line ");
      str.concat(String(k));
      saveLineToCsv(str);
      ret=true;
      break;
    }
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
