#include <Wire.h>
#include "MS5837.h"

MS5837 sensor;

void init_bar(){
  Wire.begin();

  // Initialize pressure sensor
  // Returns true if initialization was successful
  // We can't continue with the rest of the program unless we can initialize the sensor
  while (!sensor.init()) {
    Serial.println("Init failed!");
    Serial.println("Are SDA/SCL connected correctly?");
    Serial.println("Blue Robotics Bar30: White=SDA, Green=SCL");
    Serial.println("\n\n\n");

    for(short k=0;k<10;k++){
      light_Red();
      delay(250);
      light_Green();
      delay(250);
      light_Off();
    }    
  }
  sensor.setModel(MS5837::MS5837_30BA);
  switch(waterType){
    case FRESH_WATER:
                    sensor.setFluidDensity(997); // kg/m^3 
                    break;
    case SALT_WATER:
                    sensor.setFluidDensity(1029); // kg/m^3
                    break;
  }

}

/*----------------------------------------------------------
 * next lines uses read which takes 40ms
 * it assumes we check ONLY 1 parameter
 * (depth OR temprature) so only one function is needed
 * 
 * if need both values, one read is enough (but should be written 
 * in new function
 */
unsigned long currentSeconds;
void getSensorValues(){
  unsigned long tmp=floor(millis()/1000);
  
  if(deskDebug){
    float dpdiv=0.2;
    if(ng_curr_speed < 1500){
      dpdiv=-0.05;//0.7*(ng_curr_speed-1500)/1500;
    }
//    if((currentSeconds>30)&&(currentSeconds<90)){
//      dpdiv=dpdiv/10;
//    }
    currentDepth+=dpdiv;
    if((currentSeconds>10)&&(currentDepth < 0.4)){
      currentDepth=0.4;
    }
//    else if(currentDepth >1.6){ // for checking stuck before calibration
//      currentDepth=1.6;
//    }
    currentTemp=currentDepth;
  }
  else {
    //if(tmp > currentSeconds){ //getting samples only once a second
      sensor.read();
      delay(3);
      float d=sensor.depth();
      if(d < 100){
        currentDepth=d;
        currentTemp=sensor.temperature();
      }
    //}
  }
  currentSeconds=tmp;
  
  if(currentDepth < minDepthForThrusterWorking){
      light_Red();
      Serial.println("getSensorValues: ABOVE WATER -- stopping");
      saveLineToCsv("getSensorValues: ABOVE WATER -- stopping");
      stopNow();
      delay(15);    
      light_Off();
  }
  
//    Serial.print("-------------getSensorValues:   currentSeconds=");
//    Serial.print(currentSeconds);
//    Serial.print(", depth=");
//    Serial.print(currentDepth);
//    Serial.print(", temprature=");
//    Serial.println(currentTemp);
}
/** Depth returned in meters (valid for operation in incompressible
 *  liquids only. Uses density that is set for fresh or seawater.
 */
// float getWaterDepth(){
//  float ret=0;
//  sensor.read();
//  ret=sensor.depth();
//  currentDepth=sensor.depth();
//  return ret;
//}
//// Temperature returned in deg C.
// float getWaterTemperature(){
//  float ret=0;
//  sensor.read();
//  ret=sensor.temperature();
//  currentDepth=sensor.depth();
//  return ret;
//}
/*----------------------------------------------------------------------*/
