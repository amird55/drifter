/*
  SD card read/write

 This example shows how to read and write data to and from an SD card file
 The circuit:
 * SD card attached to SPI bus as follows:
 ** MOSI - pin 51 (11 for uno)
 ** MISO - pin 50 (12 for uno)
 ** CLK - pin 52 (13 for uno)
 ** CS - pin 53 (4 (for MKRZero SD: SDCARD_SS_PIN) for uno)

 created   Nov 2010
 by David A. Mellis
 modified 9 Apr 2012
 by Tom Igoe

 This example code is in the public domain.

 */

#include <SPI.h>
#include <SD.h>

const int chipSelect = 53;

short lineIndex=0;
short paramIndex=0;
short letterIndex=0;
char param[10];

File logFile;
File csvFile;
#define  csvFileName  "MEASURE.csv"
#define  logFileName  "MEASURE.LOG"
#define setupFileName "setup.txt"


void sd_setup(){

    Serial.println("Initializing SD card...");

  // see if the card is present and can be initialized:
  while (!SD.begin(chipSelect)) {
      Serial.println("Card failed, or not present");
      light_Red();
      delay(1000);
      light_Off();
  }

  csvFile = SD.open(csvFileName, FILE_WRITE);     
  if (csvFile) {
    csvFile.print("seconds");
    csvFile.print(",");
    csvFile.print("depth");
    csvFile.print(",");
    csvFile.print("temprature");
    csvFile.print(",");
    csvFile.print("thruster speed");
    csvFile.print(",");
    csvFile.println("thruster_calibrated_speed");
    csvFile.close();
  }
  Serial.println("card initialized.");

}
void trunkLogFiles(){
  light_Yellow();
  logFile = SD.open(logFileName, FILE_WRITE|O_TRUNC);     
    logFile.close();

  csvFile = SD.open(csvFileName, FILE_WRITE|O_TRUNC);     
  if (csvFile) {
    csvFile.print("seconds");
    csvFile.print(",");
    csvFile.print("depth");
    csvFile.print(",");
    csvFile.print("temprature");
    csvFile.print(",");
    csvFile.print("thruster speed");
    csvFile.print(",");
    csvFile.print("thruster_calibrated_speed");
  }
  csvFile.close();
  light_Off();
}
void saveLineToCsv(String str){
  csvFile = SD.open(csvFileName, FILE_WRITE);     

  // if the file is available, write to it:
  if (csvFile) {

    csvFile.print(str);
    csvFile.println(" ");
    csvFile.close();
  }
}
void saveToLogFile(){
  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  logFile = SD.open(logFileName, FILE_WRITE);     

  vinVal=analogRead(pinAnalogVin);
  
  // if the file is available, write to it:
  if (logFile) {
//    light_Green();

    logFile.print("seconds from start: ");
    logFile.print(currentSeconds);
    logFile.print(", depth=");
    logFile.print(currentDepth);
    logFile.print(", temprature=");
    logFile.println(currentTemp);
    logFile.close();
//    Serial.println("log saved");
//    Serial.print("seconds from start: ");
//    Serial.print(currentSeconds);
//    Serial.print(", depth=");
//    Serial.print(currentDepth);
//    Serial.print(", temprature=");
//    Serial.print(currentTemp);
//    Serial.print(", vinVal=");
//    Serial.println(vinVal);
  }
  // if the file isn't open, pop up an error:
  else {
//    light_Red();
    Serial.print("error opening ");
    Serial.println(logFileName);
    if (SD.exists(logFileName)) {
      Serial.println("  file exists.");
    } else {
      Serial.println("  file doesn't exist.");
    }
  }
  csvFile = SD.open(csvFileName, FILE_WRITE);     

  // if the file is available, write to it:
  if (csvFile) {

    csvFile.print(currentSeconds);
    csvFile.print(",");
    csvFile.print(currentDepth);
    csvFile.print(",");
    csvFile.print(currentTemp);
    csvFile.print(",");
    csvFile.print(ng_curr_speed);
    csvFile.print(",");
    if(isCalibrating){
      csvFile.print("CALIBRATING");
    } else {
      csvFile.print(thruster_calibrated_speed);
    }
    if(am_i_stuck){
      csvFile.print(",");
      csvFile.print("STUCK");
    }
    csvFile.println(" ");
    csvFile.close();
  }
//  light_Off();
}

void enterNewLineData(){
  if(paramIndex < 5){
    courseParams[lineIndex].pVelocity=-1;
  }
  if(paramIndex < 4){
    courseParams[lineIndex].pDurationMin=-1;
  }
  paramIndex=0;
  lineIndex++;
}
void setParam(){
  param[letterIndex]='\0';
  switch(paramIndex){
    case 0: courseParams[lineIndex].pType=param[0];           break;
    case 1: courseParams[lineIndex].pValH=atof(param);        break;
    case 2: courseParams[lineIndex].pValL=atof(param);        break;
    case 3: courseParams[lineIndex].pDurationMin=atoi(param); break;
    case 4: courseParams[lineIndex].pVelocity=atoi(param); break;
  }
  letterIndex=0;
  paramIndex++;
}
/*
 * configurations options in log file
 * type , low val , high val , duration, seconds to climb
 * -------------------------------------------------------
 * type = 
 * N - add 2 minutes wait after in water
 * C,startDepth,endDepth - need to calibrate speed
 * P,depth - setting minimal depth for thruster to work
 * S - stuck params (
 *                    minimal depth [m] to start checking for stuck (default=1.0) ,
 *                    depth variation during stuck [m]  (default=0.5) , 
 *                    minutes to be considered as stuck  (default=1) 
 * T - by temp
 * D - by depth
 * F - full speed by time period
 * H - half speed for time period
 * 
 * for F/H:
 *  first val is depth to start engine
 *  second val - time period (in millisec) of working thruster
 *  
 *  duration = minutes of this phase
 *  seconds to climb = how many seconds to climb to upper limit (effects thruster speed)
 */
void readFromLogFile(){
  
  logFile = SD.open(setupFileName, FILE_READ);     

//  String dataString;
  char readLetter;
  if (logFile) {
    bool notRemark=true;
    while (logFile.available()) {
      readLetter=logFile.read();
      switch(readLetter){
        case '#' :
                  Serial.println("REMARK");
                  notRemark=false;
                  break;
        case '\r' :
        case ' ' :
                  break;
        case '\n' :
                  if(notRemark){
                    if((letterIndex==0)&&(paramIndex==0)){
                      //no data in this line
                    } else {
                      setParam();
                      enterNewLineData();
                    }
                  }
                  notRemark=true;
                  break;
        case ',' :
                  if(notRemark){
                    setParam();
                  }
                  break;
        default:
                  if(notRemark){
                    param[letterIndex]=readLetter;
                    letterIndex++;
                  }
      }
    }
  }
  // if the file isn't open, pop up an error:
  else {
    Serial.print("error opening ");
    Serial.println(setupFileName);
    Serial.print(setupFileName);
    if (SD.exists(setupFileName)) {
      Serial.println("  exists.");
    } else {
      Serial.println("  doesn't exist.");
    }
  }

String str;
  for (int k=0;k<MAX_SECTIONS;k++){
    Serial.print(courseParams[k].pType);
    Serial.print("   ");
    Serial.print(courseParams[k].pValH);
    Serial.print("   ");
    Serial.print(courseParams[k].pValL);
    Serial.print("   ");
    Serial.print(courseParams[k].pDurationMin);
    Serial.print("   ");
    Serial.print(courseParams[k].pVelocity);
    Serial.println("   ");
    
    str="read setup file:: k=";
    str.concat(String(k));
    str.concat("  - ");
    str.concat(String(courseParams[k].pType));
    str.concat("   ");
    str.concat(String(courseParams[k].pValH));
    str.concat("   ");
    str.concat(String(courseParams[k].pValL));
    str.concat("   ");
    str.concat(String(courseParams[k].pDurationMin));
    str.concat("   ");
    str.concat(String(courseParams[k].pVelocity));
    saveLineToCsv(str);
  }
}
