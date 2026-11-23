#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include <Adafruit_VL53L1X.h>

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
};

//initialize a dronesensors object
droneSensors sensorData;

//create vars for motors
uint8_t frontRightMotor = 4;
uint8_t backRightMotor = 13;
uint8_t backLeftMotor = 14;
uint8_t frontLeftMotor = 16;
//create vars for LEDs
uint8_t greenLED = 21;
uint8_t redLED = 26;

void setup() {
  Serial.begin(115200);
  Wire.begin();

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
    delay(20)
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
    delay(20)
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
}

void readVL53(){
  if (!VL53.dataReady()){
    return;
  }

  sensorData.altitude = VL53.distance();

  if (sensorData.altitude == -1){
    //its cooked browski
    Serial.print("Could not retried altitude: ");
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




void loop() {
  // put your main code here, to run repeatedly:
  readBNO();
  readVL53();

  delay(100);

}
