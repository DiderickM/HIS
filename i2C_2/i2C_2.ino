#include <Wire.h>
#define ADXL345_POWER_CTL 45

#define interuptPin 2

#define ledPin 4
const int MPU=0x68; 
int16_t AcX,AcY,AcZ,Tmp,GyX,GyY,GyZ;
double dAcX, dAcY, dAcZ;

byte values[6];
char output[512];
void setup() {
  Wire.begin();
  Wire.beginTransmission(MPU);
  Wire.write(0x6B); 
  Wire.write(0);    
  Wire.endTransmission(true);
  Serial.begin(9600);

  pinMode(ledPin, OUTPUT); 
  
  attachInterrupt(digitalPinToInterrupt(interuptPin), ISR_fuction, FALLING);
}

bool buttonPressed = false;

void ISR_fuction() {
  if(buttonPressed) {
    buttonPressed = false;
  } else {
    buttonPressed = true;
  }
}



void loop() {
  //Serial.print("OK");
  digitalWrite(ledPin, buttonPressed);
  if(!buttonPressed) {
      Wire.beginTransmission(MPU);
  Wire.write(0x3B);  
  Wire.endTransmission(false);
  Wire.requestFrom(MPU,14,true);  
  AcX=Wire.read()<<8|Wire.read();    
  AcY=Wire.read()<<8|Wire.read();  
  AcZ=Wire.read()<<8|Wire.read();
  int temp =Wire.read()<<8|Wire.read();;
  GyX=Wire.read()<<8|Wire.read();  
  GyY=Wire.read()<<8|Wire.read();  
  GyZ=Wire.read()<<8|Wire.read();

  temp = (temp)/340 + 36.53;
  dAcX = AcX / 32768.0 * (2 *9.81);
  dAcY = AcY / 32768.0* (2 *9.81);
  dAcZ = AcZ / 32768.0 * (2 *9.81);
  
  Serial.print("Accelerometer: ");
  Serial.print("X = "); Serial.print(dAcX);
  Serial.print(" | Y = "); Serial.print(dAcY);
  Serial.print(" | Z = "); Serial.println(dAcZ); 

  Serial.println("temp");
  Serial.print(temp);
  Serial.println("");
  
  Serial.print("Gyroscope: ");
  Serial.print("X = "); Serial.print(GyX);
  Serial.print(" | Y = "); Serial.print(GyY);
  Serial.print(" | Z = "); Serial.println(GyZ);
  Serial.println(" ");
  delay(10);
  } else {
    Wire.requestFrom(0x4A, 1);
    int temp = 0;
    while(Wire.available()) {
      temp = Wire.read();
    }
    Serial.print("teprature is: ");
    Serial.println(temp);
  }  
  delay(500);
}
