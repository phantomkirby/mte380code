/////////////////////////////////////////////////////////////////////////////////////////////////
//Libraries
/////////////////////////////////////////////////////////////////////////////////////////////////
//IMU
#include <MPU9250.h>
#include <quaternionFilters.h>
//Motors
#include <Servo.h>
//Remote Control
/*#include "IRremote.h"*/
//Pressure Sensor
//#include <SparkFun_MS5803_I2C.h>
#include <Wire.h>
//XBOX USB
#include <XBOXUSB.h>
#ifdef dobogusinclude
#include <spi4teensy3.h>
#endif
//SD Card
#include <SPI.h>
#include <SD.h>

/////////////////////////////////////////////////////////////////////////////////////////////////
//Initialization of variables
/////////////////////////////////////////////////////////////////////////////////////////////////
void updateIMUSensor(void);

/*boolean to check if the ROV is going forward so that 
we can call 'fix any lateral drift' AKA pointNorth*/
bool goingForward = false;

//USB
USB Usb;
XBOXUSB Xbox(&Usb);

//IMU Sensor
MPU9250 imuSensor;
float mDirection;
float mX, mY, mZ;
float north, forwardYaw;

//Pressure sensor
/*MS5803 pressureSensor(0x76);
//Create variables to store results
double pressure_abs, pressure_relative, altitude_delta, pressure_baseline;
double base_altitude = 329.0; // Altitude of Waterloo Ontario (m)*/

//SD Card
const int sdChipSelect = 49;
File recordingFile;
File testFile;
bool isRecording = false;

//Test constants
bool testingMode = false;
int testDuration = 1;

//Filter Library
#define SMA_LENGTH 10
float yawfilter[SMA_LENGTH]; //magnometer X
float pitchfilter[SMA_LENGTH]; //magnometer Y
float rollfilter[SMA_LENGTH]; //magnometer Z
float latestYaw = 0;

float aXfilter[SMA_LENGTH]; //A X
float aYfilter[SMA_LENGTH]; //A Y
float aZfilter[SMA_LENGTH]; //A Z

//float pFilter[SMA_LENGTH]; //pressure

/*//IR Remote-Controller
int receiver = 8; //pin number for IR receiver
IRrecv irrecv(receiver);     // create instance of 'irrecv'
decode_results results;      // create instance of 'decode_results'
void translateIR() // takes action based on IR code received
// describing Remote IR codes 
{
  String buttonPressed = "";
  
  switch(results.value)
  {
  case 0xFF6897: //0
    StopZAxis();   
    buttonPressed = "StopZAxis";
    break;
  case 0xFF30CF: //1
    if (testingMode == true) { 
      testForXDuration("Up"); 
    } else {
      Up();
    }
    buttonPressed = "Up";    
    break;
  case 0xFF18E7: //2
    if (testingMode == true) { 
      testForXDuration("Forward");
    } else {
      Forward();
    }
    buttonPressed = "Forward";    
    break;
  case 0xFF7A85: //3
    if (testingMode == true) { 
      testForXDuration("Down"); 
    } else {
      Down();
    }
    buttonPressed = "Down";
    break;
  case 0xFF10EF: //4
    Left();
    if (testingMode == true) { 
      testForXDuration("Left"); 
    } else {
      Left();
    }
    buttonPressed = "Left";   
    break;
  case 0xFF38C7: //5
    StopLat();
    buttonPressed = "StopLat";
    break;
  case 0xFF5AA5: //6
    if (testingMode == true) { 
      testForXDuration("Right"); 
    } else {
      Right();
    }
    buttonPressed = "Right";
    break;
  case 0xFF42BD: //7
    if (testingMode == true) { 
      testForXDuration("TiltLeft"); 
    } else {
      TiltLeft(); 
    }
    buttonPressed = "TiltLeft";
    break;
  case 0xFF4AB5: //8
    if (testingMode == true) { 
      testForXDuration("Backward"); 
    } else {
      Backward();
    }
    buttonPressed = "Backward";
    break;
  case 0xFF52AD: //9 
    if (testingMode == true) { 
      testForXDuration("TiltRight"); 
    } else {
      TiltRight();
    }
    buttonPressed = "TiltRight";
    break;
  ///////////////////////////
  case 0xFF9867: //EQ
    updateIMUSensor();
    north = yawfilter[SMA_LENGTH-1];
    Serial.println("New North: " + String(north));
    break;
  case 0xFFB04F: //ST/REPT
    pointNorth("north"); 
    buttonPressed = "pointNorth";
    break; 
  case 0xFF02FD: //play/pause
    if (isRecording == true) {
      buttonPressed = "STOP RECORDING";
      isRecording = false; 
      stopRecording();
    }
    else {
      buttonPressed = "START RECORDING";
      isRecording = true;
      startRecording();
    }
    break;
  case 0xFFE21D: //FUNC/STOP
    if (testingMode) {
      buttonPressed = "STOP TESTING MODE";
      testingMode = false;
      Serial.println("STOP TESTING MODE");
      testFile.close();
      buzzXTimes(3);
      
    } else {
      buttonPressed = "START TESTING MODE";
      testingMode = true;
      Serial.println("START TESTING MODE");
      testFile = SD.open("t" + String(millis()/1000) + ".txt", FILE_WRITE);
      if (testFile) {
        buzzXTimes(3);
        Serial.println("TEST FILE OPENED SUCCESSFULLY");
      }
      else {
        Serial.println("TEST FILE DID NOT OPEN SUCCESSFULLY");
        buzzXTimes(1);
      }
    }
    break;
  case 0xFF22DD: //FAST BACK
    testDuration = testDuration - 1;
    if (testDuration < 1) {
      testDuration = 1;
    }
    buttonPressed = "CHANGED TEST DURATION: " + String(testDuration);
    Serial.println("New Test Duration: " + String(testDuration));
    buzzXTimes(testDuration);
    break;
  case 0xFFC23D: //FAST FORWARD
    testDuration = testDuration + 1;
    if (testDuration > 5) {
      testDuration = 5;
    }
    buttonPressed = "CHANGED TEST DURATION: " + String(testDuration);
    Serial.println("New Test Duration: " + String(testDuration));
    buzzXTimes(testDuration);
    break;
  }// End Case

  if (isRecording == true) {
    recordingFile.println(buttonPressed);
    recordingFile.println(String(millis()));
  }
}*/

//Defining motors
#define NUMMOTORS 4
typedef struct MotorDef
{
    Servo   Motor; 
    int     Pin;   // Indicates the Pin this motor is connected to
};
MotorDef Motors[NUMMOTORS];

//ESC Settings
//Stores the settings for all ESC. Medium means "no movement". High/Low are max speed in cw/ccw dir.
typedef struct ESCSettingsDef
{
  int LowSpeed;
  int HighSpeed;
  int MediumSpeed;
};
ESCSettingsDef ESCSettings; 
//Specified baud rates for Afro ESC configuration
#define ESC_HIGH_DEFAULT 1860
#define ESC_LOW_DEFAULT 1060
#define ESC_MED_DEFAULT 1460
/////////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////////////
//Setup
/////////////////////////////////////////////////////////////////////////////////////////////////
void setup() {
  //Serial Monitor @ 115200 bps
  Serial.begin(115200);

  if (Usb.Init() == -1) {
    Serial.println("OSC did not start");
    while (1); //halt
  }
  Serial.println("XBOX USB Library Started");

  pinMode(sdChipSelect, OUTPUT); //start the SD card
  if (!SD.begin(sdChipSelect)) {
    Serial.println("SD Card initialization failed!");
  } else {
     Serial.println("SD Card initialization done.");
  }
  
  //irrecv.enableIRIn(); // Start the receiver
  //Serial.println("IR receiver enabled");

  //Set up the motor pins
  Motors[0].Pin = 2; //z-axis left
  Motors[1].Pin = 3; //z-axis right
  Motors[2].Pin = 4; //left
  Motors[3].Pin = 5; //right
  for(int i = 0; i < NUMMOTORS; i++)
  {
    int pin = Motors[i].Pin;
    Motors[i].Motor.attach(pin);
  }

  //Set ESC settings
  ESCSettings.LowSpeed   = ESC_LOW_DEFAULT;
  ESCSettings.HighSpeed  = ESC_HIGH_DEFAULT;
  ESCSettings.MediumSpeed = ESC_MED_DEFAULT;

  //Set motor speeds to 0
  for (int i = 0; i < NUMMOTORS; i++)
  {
    Motors[i].Motor.writeMicroseconds(ESCSettings.MediumSpeed);
  }
  Serial.println("Motors initialized");

  /*//Initialize the pressure sensor
  pressureSensor.reset();
  pressureSensor.begin();
  pressure_baseline = pressureSensor.getPressure(ADC_4096);
  Serial.println("Pressure sensor initialized");*/

  //Initialize IMU sensor
  Wire.begin(); //initialized the arduino board itself as a master
  Wire.begin(0x68); //imu
  Wire.begin(0x76); //pressure
  Serial.println("I2C initialized");

  int intPin = 13;
  pinMode(intPin, INPUT);
  digitalWrite(intPin, LOW);
  Serial.println("I2C initialized");
  
  //imuSensor.MPU9250SelfTest(imuSensor.selfTest);
  imuSensor.calibrateMPU9250(imuSensor.gyroBias, imuSensor.accelBias);
  imuSensor.initMPU9250();
  imuSensor.initAK8963(imuSensor.factoryMagCalibration);
  imuSensor.getAres();
  imuSensor.getGres();
  imuSensor.getMres();
  imuSensor.magBias[0] = 53.06;
  imuSensor.magBias[1] = 28.30;
  imuSensor.magBias[2] = -157.34;
  imuSensor.magScale[0] = 0.88;
  imuSensor.magScale[1] = 1.07;
  imuSensor.magScale[2] = 1.08;
  imuSensor.factoryMagCalibration[0] = 1.18;
  imuSensor.factoryMagCalibration[1] = 1.18;
  imuSensor.factoryMagCalibration[2] = 1.14;
  Serial.println("Calibrating north...");
  updateIMUSensor();
  north = imuSensor.yaw;
  Serial.println("North Calibrated To Be: " + String(north));
}
/////////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////////////
//FILTER FUNCTION
/////////////////////////////////////////////////////////////////////////////////////////////////
float sma_filter(float current_value, float* history_SMA)
{ 
  float sum=0;
  float average=0;
  int i;

  for(i=1;i<SMA_LENGTH;i++)
  {
    history_SMA[i-1]=history_SMA[i];
  }
  history_SMA[SMA_LENGTH-1]=current_value;
  
  for(i=0;i<SMA_LENGTH;i++)
  {
    sum+=history_SMA[i];
  }
  average=sum/SMA_LENGTH;

  return average;
}
/////////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////////////
//IMU
/////////////////////////////////////////////////////////////////////////////////////////////////
float tol = 9;
void pointNorth(String orientation) {
  bool atNorth = false;
  float val = 0;
  float delta = 0;
  
  while (atNorth == false) {
    updateIMUSensor();
    val = latestYaw; //get latest filtered yaw
    Serial.println("updatedYaw: " + String(val));

    if (orientation == "forward") { //fixes any lateral tilt while in 'forward' mode
      delta = val - forwardYaw;
    } else { //reorient to 'north' of the pool
      delta = val - north; //north is a constant set-up at initial
    }
    
    if (delta < -180) {
      delta = delta + 360;
    } 
    else if (delta > 180) {
      delta = delta - 360;
    }
  
    if (-180 <= delta && delta <= -tol) {
      Left();
      if (orientation == "forward") {
        goingForward = true;
      }
    } 
    else if (-tol <= delta && delta <= 0) {
      if (orientation != "forward") { 
        StopLat();
      }
      atNorth = true;
    } 
    else if (0 <= delta && delta <= tol) {
      if (orientation != "forward") { 
        StopLat();
      }
      atNorth = true;
    }
    else if (tol <= delta && delta <= 180) {
      Right();
      if (orientation == "forward") {
        goingForward = true;
      }
    }
    delay(25);
  }
}

void updateIMUSensor() {
  // If intPin goes high, all data registers have new data
  // On interrupt, check if data ready interrupt
  if (imuSensor.readByte(MPU9250_ADDRESS, INT_STATUS) & 0x01) 
  {
    imuSensor.readAccelData(imuSensor.accelCount);
    imuSensor.ax = (float)imuSensor.accelCount[0] * imuSensor.aRes; // - imuSensor.accelBias[0];
    sma_filter(imuSensor.ax, aXfilter);
    imuSensor.ay = (float)imuSensor.accelCount[1] * imuSensor.aRes; // - imuSensor.accelBias[1];
    sma_filter(imuSensor.ay, aYfilter);
    imuSensor.az = (float)imuSensor.accelCount[2] * imuSensor.aRes; // - imuSensor.accelBias[2];
    sma_filter(imuSensor.az, aZfilter);
    imuSensor.readGyroData(imuSensor.gyroCount);
    imuSensor.gx = (float)imuSensor.gyroCount[0] * imuSensor.gRes;
    imuSensor.gy = (float)imuSensor.gyroCount[1] * imuSensor.gRes;
    imuSensor.gz = (float)imuSensor.gyroCount[2] * imuSensor.gRes;
    imuSensor.readMagData(imuSensor.magCount);
    imuSensor.mx = (float)imuSensor.magCount[0] * imuSensor.mRes
               * imuSensor.factoryMagCalibration[0] - imuSensor.magBias[0];
    imuSensor.my = (float)imuSensor.magCount[1] * imuSensor.mRes
               * imuSensor.factoryMagCalibration[1] - imuSensor.magBias[1];
    imuSensor.mz = (float)imuSensor.magCount[2] * imuSensor.mRes
               * imuSensor.factoryMagCalibration[2] - imuSensor.magBias[2];
  } else {
    Serial.println("did not update");
  }
  imuSensor.updateTime();
  MahonyQuaternionUpdate(imuSensor.ax, imuSensor.ay, imuSensor.az, imuSensor.gx * DEG_TO_RAD,
                         imuSensor.gy * DEG_TO_RAD, imuSensor.gz * DEG_TO_RAD, imuSensor.my,
                         imuSensor.mx, imuSensor.mz, imuSensor.deltat);
  imuSensor.delt_t = millis() - imuSensor.count;
  if (imuSensor.delt_t > 25)
  {
    imuSensor.yaw   = atan2(2.0f * (*(getQ()+1) * *(getQ()+2) + *getQ()
                    * *(getQ()+3)), *getQ() * *getQ() + *(getQ()+1)
                    * *(getQ()+1) - *(getQ()+2) * *(getQ()+2) - *(getQ()+3)
                    * *(getQ()+3));
    imuSensor.pitch = -asin(2.0f * (*(getQ()+1) * *(getQ()+3) - *getQ()
                  * *(getQ()+2)));
    imuSensor.roll  = atan2(2.0f * (*getQ() * *(getQ()+1) + *(getQ()+2)
                  * *(getQ()+3)), *getQ() * *getQ() - *(getQ()+1)
                  * *(getQ()+1) - *(getQ()+2) * *(getQ()+2) + *(getQ()+3)
                  * *(getQ()+3));
    imuSensor.pitch *= RAD_TO_DEG;
    imuSensor.yaw   *= RAD_TO_DEG;
    imuSensor.yaw  -= 9.62; //for University of Waterloo
    imuSensor.roll *= RAD_TO_DEG;

    //Serial.print("Yaw, Pitch, Roll: ");
    //Serial.print(imuSensor.yaw, 2);
    //Serial.print(", ");
    //Serial.print(imuSensor.pitch, 2);
    //Serial.print(", ");
    //Serial.println(imuSensor.roll, 2);

    latestYaw = sma_filter(imuSensor.yaw, yawfilter);
    float SMApitch = sma_filter(imuSensor.pitch, pitchfilter);
    float SMAroll = sma_filter(imuSensor.roll, rollfilter);

    //Serial.print("SMAYaw, SMAPitch, SMARoll: ");
    //Serial.print(latestYaw);
    //Serial.print(", ");
    //Serial.print(SMApitch);
    //Serial.print(", ");
    //Serial.println(SMAroll);

    /*if (isRecording == true) {
      recordingFile.println("SMAYaw, SMAPitch, SMARoll: " + String(latestYaw) + " " + String(SMApitch) + " " + String(SMAroll));
      recordingFile.println("RawYaw, RawPitch, RawRoll: " + String(imuSensor.yaw) + " " + String(imuSensor.pitch) + " " + String(imuSensor.roll));
      recordingFile.println(String(millis()));
    }*/

    imuSensor.count = millis();
    imuSensor.sumCount = 0;
    imuSensor.sum = 0;
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////////////////////////
//Code for 4 Motor Movements
/////////////////////////////////////////////////////////////////////////////////////////////////
int change = 85;
int handicap = 5;

void Forward() {
  Serial.println("Forward");
  Motors[2].Motor.writeMicroseconds(ESCSettings.MediumSpeed - change); //left
  Motors[3].Motor.writeMicroseconds(ESCSettings.MediumSpeed + change); //right
  Motors[0].Motor.writeMicroseconds(ESCSettings.MediumSpeed); //z-axis left
  Motors[1].Motor.writeMicroseconds(ESCSettings.MediumSpeed); //z-axis right
  
  if (testingMode == true) {
     delay(testDuration);
     StopLat();
     StopZAxis();
  }
}

void Backward() {
  Serial.println("Backward");
  Motors[2].Motor.writeMicroseconds(ESCSettings.MediumSpeed + change); //left
  Motors[3].Motor.writeMicroseconds(ESCSettings.MediumSpeed - change); //right
  if (testingMode == true) {
     delay(testDuration);
     StopLat();
     StopZAxis();
  }
}

void Left() {
  Serial.println("Left");
  Motors[2].Motor.writeMicroseconds(ESCSettings.MediumSpeed - change - 40); //left
  Motors[3].Motor.writeMicroseconds(ESCSettings.MediumSpeed + change - 10); //right
  if (testingMode == true) {
     delay(testDuration);
     StopLat();
     StopZAxis();
  }
}

void Right() {
  Serial.println("Right");
  Motors[2].Motor.writeMicroseconds(ESCSettings.MediumSpeed - change + 10); //left
  Motors[3].Motor.writeMicroseconds(ESCSettings.MediumSpeed + change + 40); //right
  if (testingMode == true) {
     delay(testDuration);
     StopLat();
     StopZAxis();
  }
}

void Up() {
  Serial.println("Up");
  Motors[0].Motor.writeMicroseconds(ESCSettings.MediumSpeed + change); //z-axis left
  Motors[1].Motor.writeMicroseconds(ESCSettings.MediumSpeed - change); //z-axis right
  if (testingMode == true) {
     delay(testDuration);
     StopLat();
     StopZAxis();
  }
}

void Down() {
  Serial.println("Down");
  Motors[0].Motor.writeMicroseconds(ESCSettings.MediumSpeed - change); //z-axis left
  Motors[1].Motor.writeMicroseconds(ESCSettings.MediumSpeed + change); //z-axis right
  if (testingMode == true) {
     delay(testDuration);
     StopLat();
     StopZAxis();
  }
}

void TiltRight() {
  Serial.println("Tilt Right");
  Motors[0].Motor.writeMicroseconds(ESCSettings.MediumSpeed - change); //z-axis left
  Motors[1].Motor.writeMicroseconds(ESCSettings.MediumSpeed - change); //z-axis right
  if (testingMode == true) {
     delay(testDuration);
     StopLat();
     StopZAxis();
  }
}

void TiltLeft() {
  Serial.println("Tilt Left");
  Motors[0].Motor.writeMicroseconds(ESCSettings.MediumSpeed + change); //z-axis left
  Motors[1].Motor.writeMicroseconds(ESCSettings.MediumSpeed + change); //z-axis right
  if (testingMode == true) {
     delay(testDuration);
     StopLat();
     StopZAxis();
  }
}

void StopLat() {
  Serial.println("Stop Lateral");
  goingForward = false;
  Motors[2].Motor.writeMicroseconds(ESCSettings.MediumSpeed);
  Motors[3].Motor.writeMicroseconds(ESCSettings.MediumSpeed);
}

void StopZAxis() {
  Serial.println("Stop Z-Axis");
  goingForward = false;
  Motors[0].Motor.writeMicroseconds(ESCSettings.MediumSpeed);
  Motors[1].Motor.writeMicroseconds(ESCSettings.MediumSpeed);
}
/////////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////////////
//Pressure Sensor
/////////////////////////////////////////////////////////////////////////////////////////////////
/*void updatePressureSensor() {
  pressure_abs = pressureSensor.getPressure(ADC_4096);
  //pressure_relative = sealevel(pressure_abs, base_altitude);
  float wDepth = sma_filter(waterDepth(pressure_abs - pressure_baseline), pFilter);

  //Serial.print("Pressure abs (mbar)= ");
  //Serial.println(pressure_abs);

  //Serial.print("SMAPressure abs: ");
  //Serial.println(sma_filter(pressure_abs, pFilter));

  //Serial.print("Pressure relative (mbar)= ");
  //Serial.println(pressure_relative);

  //Serial.print("Water Depth (m) = ");
  //Serial.println(wDepth);

  if (isRecording == true) {
    recordingFile.println("Water Depth (m) = " + String(wDepth));
    recordingFile.println(String(millis()));
  }
}

double waterDepth(double pRelative) {
  return(pRelative*100/(998*9.81));
}*/

/////////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////////////
//Xbox Controller
/////////////////////////////////////////////////////////////////////////////////////////////////
void translateXbox() {
  String buttonPressed = "";

  if (Xbox.getButtonClick(B)) { //emergency stop
    StopLat();
    StopZAxis();
    delay(500);
  } else {
    if (Xbox.getAnalogHat(LeftHatX) > 7500) { //RIGHT
      Right();
    }
    else if (Xbox.getAnalogHat(LeftHatX) < -7500) { //LEFT
      Left();
    }
    else if (Xbox.getAnalogHat(LeftHatY) > 7500) { //FORWARD
      Forward();
    }
    else if (Xbox.getAnalogHat(LeftHatY) < -7500) { //BACKWARD
      Backward();
    }
    else {
      StopLat();
    }
  
    if (Xbox.getAnalogHat(RightHatX) > 7500) { //TILT RIGHT
      TiltRight();
    }
    else if (Xbox.getAnalogHat(RightHatX) < -7500) { //TILT LEFT
      TiltLeft();
    }
    else if (Xbox.getAnalogHat(RightHatY) > 7500) { //UP
      Up();
    }
    else if (Xbox.getAnalogHat(RightHatY) < -7500) { //DOWN
      Down();
    }
    else {
      StopZAxis();
    }
  }

  if (Xbox.getButtonClick(A)) { //testing mode
    if (testingMode) {
      buttonPressed = "STOP TESTING MODE";
      testingMode = false;
      Serial.println("STOP TESTING MODE");
      testFile.close();
    } else {
      buttonPressed = "START TESTING MODE";
      testingMode = true;
      Serial.println("START TESTING MODE");
      testFile = SD.open("t" + String(millis()/1000) + ".txt", FILE_WRITE);
      if (testFile) {
        Serial.println("TEST FILE OPENED SUCCESSFULLY");
      }
      else {
        Serial.println("TEST FILE DID NOT OPEN SUCCESSFULLY");
      }
    }
  }
  if (Xbox.getButtonClick(L1)) {
    testDuration = testDuration - 1;
    if (testDuration < 1) {
      testDuration = 1;
    }
    buttonPressed = "CHANGED TEST DURATION: " + String(testDuration);
    Serial.println("New Test Duration: " + String(testDuration));
  }
  if (Xbox.getButtonClick(R1)) {
    testDuration = testDuration + 1;
    if (testDuration > 5) {
      testDuration = 5;
    }
    buttonPressed = "CHANGED TEST DURATION: " + String(testDuration);
    Serial.println("New Test Duration: " + String(testDuration));
  }

  if (Xbox.getButtonClick(XBOX)) {
    if (isRecording == true) {
      buttonPressed = "STOP RECORDING";
      isRecording = false; 
      stopRecording();
    }
    else {
      buttonPressed = "START RECORDING";
      isRecording = true;
      startRecording();
    }
  }

  if (Xbox.getButtonClick(X)) { //new north
    updateIMUSensor();
    north = yawfilter[SMA_LENGTH-1];
    Serial.println("New North: " + String(north));
  }
  if (Xbox.getButtonClick(Y)) { //point north
    pointNorth("north"); 
    buttonPressed = "pointNorth";
  }

  if (Xbox.getButtonClick(START)) {
    startAutonomous();
  }

  if (isRecording == true) {
    recordingFile.println(buttonPressed);
    recordingFile.println(String(millis()));
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//Start Autonomous
/////////////////////////////////////////////////////////////////////////////////////////////////
//LATERAL SPEED: 60 inches/3.65s = 16.4383561644 inches/sec
//UP SPEED: 
float lateralSpeed = 16.4383561644;
float upSpeed = 14.5498;
void startAutonomous() {
  //go under hanging obstacle for 72+18 inches.
  Forward();
  delay((72+18)/lateralSpeed*1000);
  
  //go over table and through third obstacle
  Up();
  delay((22.5/upSpeed*1000));
  StopZAxis();
  
  
  //go to landing pad
  
}
/////////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////////////
//SD Card
/////////////////////////////////////////////////////////////////////////////////////////////////
void startRecording() {
  recordingFile = SD.open("r" + String(millis()/1000) + ".txt", FILE_WRITE);
  delay(100);
  if (recordingFile) {
    Serial.println("START RECORDING SUCCESS");
  }
  else {
    Serial.println("START RECORDING FAILURE");
  }
}

void stopRecording() {
  recordingFile.close();
  Serial.println("STOP RECORDING");
}
/////////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////////////
//Testing Displacement
/////////////////////////////////////////////////////////////////////////////////////////////////
float ax, ay, az, sx, sy, sz, x, y, z, old_ax, old_ay, old_az, old_sx, old_sy, old_sz;

void testForXDuration(String direction) {
  testFile.println("TESTING DISPLACEMENT:" + direction + " AT TIME = " + String(millis()) + " msec FOR DURATION " + String(testDuration) + " sec");
  
  sx = 0;
  sy = 0;
  sz = 0;
  ax = aXfilter[SMA_LENGTH-1];
  ay = aYfilter[SMA_LENGTH-1];
  az = aZfilter[SMA_LENGTH-1];

  double startTime = millis();
  double lastTime = 0;

  if (direction == "Up") {
    Up();
  } else if (direction == "Forward") {
    Forward();
  } else if (direction == "Down") {
    Down();
  } else if (direction == "Left") {
    Left();
  } else if (direction == "Right") {
    Right();
  } else if (direction == "TiltLeft") {
    TiltLeft();
  } else if (direction == "TiltRight") {
    TiltRight();
  } else if (direction == "Backward") {
    Backward();
  }
  
  while(millis() - startTime < testDuration*1000) {
    old_ax = ax;
    old_ay = ay;
    old_az = az;
    old_sx = sx;
    old_sy = sy;
    old_sz = sz;

    updateIMUSensor();
    ax = aXfilter[SMA_LENGTH-1];
    ay = aYfilter[SMA_LENGTH-1];
    az = aZfilter[SMA_LENGTH-1];

    double delta_t = (millis() - lastTime)/1000.0;
    lastTime = millis();
    
    sx = (old_ax+ax) * delta_t / 2.0;
    sy = (old_ay+ay) * delta_t / 2.0;
    sz = (old_az+az) * delta_t / 2.0;
    
    x = (old_sx+sx) * delta_t / 2.0;
    y = (old_sy+sy) * delta_t / 2.0;
    z = (old_sz+sz) * delta_t / 2.0;

    testFile.println("Time: " + String(lastTime) + " (x,y,z): (" + String(x) + ", " + String(y) + ", " + String(z) + ")");
  }
  testFile.println("FINISHED TESTING DISPLACEMENT");
  StopZAxis();
  StopLat();
}

/////////////////////////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////////////////////////
//Loop
/////////////////////////////////////////////////////////////////////////////////////////////////
void loop() {
  //XBOX
  Usb.Task();

  //Update Sensor Readings
  updateIMUSensor();
  //updatePressureSensor();
  
  /*if (irrecv.decode(&results)) // have we received an IR signal?
  {
    translateIR();
    irrecv.resume(); // receive the next value
  }*/
  if (Xbox.Xbox360Connected) { //Have we received an Xbox signal?
    translateXbox();
    delay(100);
  } else {
    StopLat();
    StopZAxis();
  }
  delay(1);
}
/////////////////////////////////////////////////////////////////////////////////////////////////
