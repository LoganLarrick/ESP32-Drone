#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include <Adafruit_VL53L1X.h>
//setup dabble for bluetooth connectivity
#define CUSTOM_SETTINGS
#define INCLUDE_GAMEPAD_MODULE //just use the gamepad module
#include <DabbleESP32.h>

#define IRQ_PIN 25
#define XSHUT_PIN 17

//create sensor objects
Adafruit_VL53L1X VL53 = Adafruit_VL53L1X(XSHUT_PIN, IRQ_PIN);
Adafruit_BNO055 BNO = Adafruit_BNO055(55, 0x28);

//structure to store data
struct droneSensors{
  float roll;
  float pitch;
  float yaw;


  int altitude;

  float gyroX;
  float gyroY;
  float gyroZ;
};

struct desiredPositions{
  float desiredRoll;
  float desiredPitch;
  float desiredYawRate; //rate because we are just rotating and stuff

  float desiredAltitude;
};

struct droneCommands{
  int up_down;
  int cw_ccw;
  int forward_backward;
  int right_left;
  bool on_off;
};

//initialize a dronesensors object
droneSensors sensorData;
desiredPositions desiredData = {0, 0, 0, 1000};
droneCommands inputs = {0, 0, 0, 0, 0};

//create vars for motors
uint8_t frontRightMotor = 4;
uint8_t backRightMotor = 13;
uint8_t backLeftMotor = 14;
uint8_t frontLeftMotor = 16;
//create vars for LEDs
uint8_t greenLED = 21;
uint8_t redLED = 26;

bool isActivated = false;

#define maxTilt = 15; //make this like the max amount that one can tilt either directon
#define directionStep = 1.0; //each cycle of holding will change it by this amount
#define heightStep = 5.0; //each cycle of holding will change it by this amount
#define maxHeight = 3000; //conservative estimate to prevent going too high


void setup() {
  Serial.begin(115200);
  Wire.begin();
  Dabble.begin("ESP_32_Drone");

  Serial.println("Starting sensors...");

  //set pinmodes for outputs
  pinMode(frontRightMotor, OUTPUT);
  pinMode(backRightMotor, OUTPUT);
  pinMode(backLeftMotor, OUTPUT);
  pinMode(frontLeftMotor, OUTPUT);
  pinMode(greenLED, OUTPUT);
  pinMode(redLED, OUTPUT);

  //initialize the VL53L1X
  if (!VL53.begin(0x29, &Wire)){
    Serial.println("VL53L1X not detected!");
    Serial.println(VL53.vl_status);
    //quick flash red means VL53L1X calibrating
    digitalWrite(redLED, HIGH);
    delay(20);
    digitalWrite(redLED, LOW);
      
    while (true); //keep checking until sensor begins
  }
  Serial.println("VL53L1x detected!");
  Serial.print("VL53L1x ID: 0x");
  Serial.println(VL53.sensorID(), HEX);
  VL53.startRanging();
  Serial.println("VL53 ranging started!");

  //initialize the BNO055
  if (!BNO.begin()){
    Serial.println("No BNO055 detected");

    //quick flash green means BNO getting setup
    digitalWrite(greenLED, HIGH);
    delay(20);
    digitalWrite(greenLED, LOW);
    while (true);
  }
  Serial.println("BNO055 detected!");
  

  delay(1000);
    
  //green LED on, no red LED ==== setup complete
  digitalWrite(greenLED, HIGH);
  digitalWrite(redLED, LOW);


  
}

void readBNO(){
  //read the euler (fused) data from the bno
  sensors_event_t orientationData;
  BNO.getEvent(&orientationData, Adafruit_BNO055::VECTOR_EULER);
  //place data into the global struct for the data
  sensorData.yaw = orientationData.orientation.x;
  sensorData.roll = orientationData.orientation.y;
  sensorData.pitch = orientationData.orientation.z;
  //print out via serial monitor
  Serial.print("yaw: ");
  Serial.println(sensorData.yaw);
  Serial.print("roll: ");
  Serial.println(sensorData.roll);
  Serial.print("pitch: ");
  Serial.println(sensorData.pitch);

  //also get angular velocity data to know speed drone rotates
  sensors_event_t gyroData;
  BNO.getEvent(&gyroData, Adafruit_BNO055::VECTOR_GYROSCOPE+;
  //place data into the global struct for the data
  sensorData.gyroX = gyroData.gyro.x;
  sensorData.gyroY = gyroData.gyro.y;
  sensorData.gyroZ = gyroData.gyro.z;
  //print out via serial monitor
  Serial.print("gyroX: ");
  Serial.println(sensorData.gyroX);
  Serial.print("gyroY: ");
  Serial.println(sensorData.gyroY);
  Serial.print("gyroZ: ");
  Serial.println(sensorData.gyroZ);
}

void readVL53(){
  if (!VL53.dataReady()){
    return;
  }

  sensorData.altitude = VL53.distance();

  if (sensorData.altitude == -1){
    //its cooked browski
    Serial.print("Could not retrieve altitude: ");
    Serial.println(VL53.vl_status);
    VL53.clearInterrupt();
    return;
  }
  //print altitude
  Serial.print("altitude: ");
  Serial.print(sensorData.altitude);
  Serial.println(" mm");
  //clear for next reading
  VL53.clearInterrupt();
}

droneCommands readInputs(){
  //make instnace of structure, transfer into this the value of isActivated, which should be a global variable i assume)
  droneCommands inputs = {0, 0, 0, 0, isActivated};

  Dabble.processInput();
  Serial.print("Button Pressed: ");

  if (GamePad.isUpPressed()){
    //go up
    inputs.up_down = 1;
    Serial.print("Up");
  }

  if (GamePad.isDownPressed()){
    //go down
    inputs.up_down = -1;
    Serial.print("Down");
  }

  if (GamePad.isLeftPressed()){
    //rotate ccw
    inputs.cw_ccw = -1;
    Serial.print("Left");
  }

  if (GamePad.isRightPressed()){
    //rotate cw
    inputs.cw_ccw = 1;
    Serial.print("Right");
  }

  if (GamePad.isSquarePressed()){
    //move left
    inputs.right_left = -1;
    Serial.print("Square");
  }

  if (GamePad.isCirclePressed()){
    //move right
    inputs.right_left = 1;
    Serial.print("Circle");
  }

  if (GamePad.isCrossPressed()){
    //move back
    inputs.forward_backward = -1;
    Serial.print("Cross");
  }

  if (GamePad.isTrianglePressed()){
    //move forward
    inputs.forward_backward = 1;
    Serial.print("Triangle");
  }

  if (GamePad.isStartPressed()){
    //toggle on and off power
    isActivated = !isActivated;
    Serial.print("Start");
  }

  if (GamePad.isSelectPressed()){
    //nothing for now
    Serial.print("Select");
  }

  return inputs;
}


void loop() {
  // put your main code here, to run repeatedly:
  readBNO();
  readVL53();
  droneCommands userInputs = readInputs();

  delay(100);

}


/*if (GamePad.isUpPressed()){
    //Go Up
    desiredData.altitude += heightStep;

    if(desiredData.altitude >= maxHeight){
      //prevent from going too high
      desiredData.altitude = maxHeight;
    }
    
    Serial.print("Up");
  }

  if (GamePad.isDownPressed()){
    //go down
    desiredData.altitude -= heightStep;


    if(desiredData.altitude < 0){
      //no negative altitudes
      desiredData.altitude = 0;
    }
    Serial.print("Down");
  }

  if (GamePad.isLeftPressed()){
    //rotate ccw
    desiredData.desiredYawRate = -45.0;
    Serial.print("Left");
  }

  if (GamePad.isRightPressed()){
    //rotate cw
    desiredData.desiredYawRate = 45.0;
    Serial.print("Right");
  }

  if (GamePad.isSquarePressed()){
    //move left
    desiredData.roll -= directionStep;

    if(desiredData.roll >= maxTilt){
      //prevent from more than tilt degree tilt
      desiredData.roll = maxTilt;
    }
    Serial.print("Square");
  }

  if (GamePad.isCirclePressed()){
    //move right
    desiredData.roll += directionStep;

    if(desiredData.roll >= maxTilt){
      //prevent from more than tilt degree tilt
      desiredData.roll = maxTilt;
    }
    Serial.print("Circle");
  }

  if (GamePad.isCrossPressed()){
    //move back
    
    desiredData.pitch -= directionStep;

    if(desiredData.pitch >= maxTilt){
      //prevent from more than tilt degree tilt
      desiredData.pitch = maxTilt;
    }

    Serial.print("Cross");
  }

  if (GamePad.isTrianglePressed()){
    //move forward
    desiredData.pitch += directionStep;

    if(desiredData.pitch >= maxTilt){
      //prevent from more than tilt degree tilt
      desiredData.pitch = maxTilt;
    }
    Serial.print("Triangle");
  }

  if (GamePad.isStartPressed()){
    //toggle on and off power
    isActivated = !isActivated;
    Serial.print("Start");
  }

  if (GamePad.isSelectPressed()){
    //nothing for now
    Serial.print("Select");
  }

  delay (10);
}*/