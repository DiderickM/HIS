
#define FASTLED_INTERRUPT_RETRY_COUNT 0 
#define FASTLED_ALLOW_INTERRUPTS 0
#include <FastLED.h>
#include <ESP8266WiFi.h>
#include <WiFiUDP.h>
#include "reactive_common.h"

#define LED_PIN 2
#define NUM_LEDS 60

//predifine the sensor values
#define MIC_LOW 530
#define MIC_HIGH 600

#define SAMPLE_SIZE 5
#define LONG_TERM_SAMPLES 150
#define BUFFER_DEVIATION 400
#define BUFFER_SIZE 3

#define LAMP_ID 3
WiFiUDP UDP;

const char *ssid = "sound_reactive"; // The SSID (name) of the Wi-Fi network you want to connect to
const char *password = "123456789";  // The password of the Wi-Fi network

CRGB leds[NUM_LEDS];

struct averageCounter *samples;
struct averageCounter *longTermSamples;
struct averageCounter* sanityBuffer;

float globalHue;
float globalBrightness = 255;
int hueOffset = 120;
float fadeScale = 1.3;
float hueIncrement = 0.7;

struct led_command {
  uint8_t opmode;
  uint32_t data;
};

unsigned long lastReceived = 0;
unsigned long lastHeartBeatSent;
const int heartBeatInterval = 100;
bool fade = false;

struct led_command cmd;
void connectToWifi();

void setup()
{
  globalHue = 0;
  samples = new averageCounter(SAMPLE_SIZE);
  longTermSamples = new averageCounter(LONG_TERM_SAMPLES);
  sanityBuffer    = new averageCounter(BUFFER_SIZE);
  
  while(sanityBuffer->setSample(250) == true) {} //set sanityBuffer
  while (longTermSamples->setSample(200) == true) {} //set logTerSamples

  FastLED.addLeds<WS2812B, LED_PIN, RGB>(leds, NUM_LEDS);

  Serial.begin(115200); // Start the Serial communication to send messages to the computer
  delay(10);
  Serial.println('\n');

  WiFi.begin(ssid, password); // Connect to the network
  Serial.print("Connecting to ");
  Serial.print(ssid);
  Serial.println(" ...");

  connectToWifi();
  sendHeartBeat();
  UDP.begin(7001); //Initializes the ethernet UDP library and network settings.
}

void sendHeartBeat() { //send info to master that it has been connected
    struct heartbeat_message hbm;
    hbm.client_id = LAMP_ID;
    hbm.chk = 77777; //make shure to send some info
    Serial.println("Sending heartbeat");
    IPAddress ip(192,168,4,1);
    UDP.beginPacket(ip, 7171); 
    int ret = UDP.write((char*)&hbm,sizeof(hbm));
    printf("Returned: %d, also sizeof hbm: %d \n", ret, sizeof(hbm));
    UDP.endPacket();
    lastHeartBeatSent = millis(); //reset timer
}

void loop()
{
  if (millis() - lastHeartBeatSent > heartBeatInterval) { //make shure the master knows the ESP is connected
    sendHeartBeat();
  }


  
  int packetSize = UDP.parsePacket();
  if (packetSize)
  {
    UDP.read((char *)&cmd, sizeof(struct led_command));
    lastReceived = millis(); //reset timer, Lastrecived keeps track on the time it takes to get a value
  }

  if(millis() - lastReceived >= 5000) //if it takes longer than 5000 ms to get data, it means it is probably not connected. So go try reconnect
  {
    connectToWifi();
  }
    
  int opMode = cmd.opmode; //this os for the mode the led pillar should display.
  int analogRaw = cmd.data;

  switch (opMode) {
    case 1:
      fade = true;
      soundReactive(analogRaw);
      break;

    case 2:
      fade = false;
      allWhite(); //make all led white
      break;

    case 3:
      chillFade(); //make the leds do a chill fade effect
      break;
  }
  
}

void allWhite() {
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB(255, 255, 235);
  }
  delay(5); //Fast led sometimes needs some time to load
  FastLED.show();
}

void chillFade() {
  static int fadeVal = 0;
  static int counter = 0;
  static int from[3] = {0, 234, 255};
  static int to[3]   = {255, 0, 214};
  static int i, j;
  static double dsteps = 500.0;
  static double s1, s2, s3, tmp1, tmp2, tmp3;
  static bool reverse = false;
  if (fade == false) {
    for (int i = 0; i < NUM_LEDS; i++) {
      leds[i] = CRGB(from[0], from[1], from[2]);
    }
    s1 = double((to[0] - from[0])) / dsteps; 
    s2 = double((to[1] - from[1])) / dsteps; 
    s3 = double((to[2] - from[2])) / dsteps; 
    tmp1 = from[0], tmp2 = from[1], tmp3 = from[2];
    fade = true;
  }

  if (!reverse) 
  {
    tmp1 += s1;
    tmp2 += s2; 
    tmp3 += s3; 
  }
  else 
  {
    tmp1 -= s1;
    tmp2 -= s2; 
    tmp3 -= s3; 
  }

  for (j = 0; j < NUM_LEDS; j++)
    leds[j] = CRGB((int)round(tmp1), (int)round(tmp2), (int)round(tmp3)); 
  FastLED.show(); 
  delay(5);

  counter++;
  if (counter == (int)dsteps) {
    reverse = !reverse;
    tmp1 = to[0], tmp2 = to[1], tmp3 = to[2];
    counter = 0;
  }
}

void soundReactive(int analogRaw) {

 int sanityValue = sanityBuffer->computeAverage(); //calculate the sanityValue it is the average of how far the value array is filled
 if (!(abs(analogRaw - sanityValue) > BUFFER_DEVIATION)) { //(abs == Calculates the absolute value of a number.) The analogRaw - sanityValue should be lower than de BUFFER_DEVIATION to make shure that the sanity buffer is right.
    sanityBuffer->setSample(analogRaw);
 }
  analogRaw = fscale(MIC_LOW, MIC_HIGH, MIC_LOW, MIC_HIGH, analogRaw, 0.6); //make shure the analogRaw value is between the defined sensor values.

  if(samples->setSample(analogRaw))
    return;
    
  uint16_t longTermAverage = longTermSamples->computeAverage(); //compute the average of the long term saples, this changes the 'hue' of the rainbow
  uint16_t useVal = samples->computeAverage(); //get the average of the sample values. This also changes the 'hue', also this value is loaded in the longTermSamples
  longTermSamples->setSample(useVal); 

  int diff = (useVal - longTermAverage);
  if (diff > 5) //if the differance is bigger than 5 change the 'hue'
  {
    if (globalHue < 235)
    {
      globalHue += hueIncrement;
    }
  }
  else if (diff < -5)
  {
    if (globalHue > 2)
    {
      globalHue -= hueIncrement;
    }
  }


  int curshow = fscale(MIC_LOW, MIC_HIGH, 0.0, (float)NUM_LEDS, (float)useVal, 1.2); //this is how many leds are actually lit. Scale the sensor value to the amout of led that need to be lit. The 1.2 is for the curve
  //int curshow = map(useVal, MIC_LOW, MIC_HIGH, 0, NUM_LEDS) //same as above but this is linear

  for (int i = 0; i < NUM_LEDS; i++)
  {
    if (i < curshow)
    {
      leds[i] = CHSV(globalHue + hueOffset + (i * 2), 255, 255); //calculate the color
    }
    else
    {
      leds[i] = CRGB(leds[i].r / fadeScale, leds[i].g / fadeScale, leds[i].b / fadeScale); //make the leds fade to off
    }
    
  }
  delay(5);
  FastLED.show(); 
}

void connectToWifi() {
  WiFi.mode(WIFI_STA);
  for (int i = 0; i < NUM_LEDS; i++) //clear led
  {
    leds[i] = CHSV(0, 0, 0);
  }
  leds[0] = CRGB(0, 255, 0); //make first led red
  FastLED.show();
  
  int i = 0;
  while (WiFi.status() != WL_CONNECTED)
  { // Wait for the Wi-Fi to connect
    delay(1000);
    Serial.print(++i);
    Serial.print(' ');
  }
  Serial.println('\n');
  Serial.println("Connection established!");
  Serial.print("IP address:\t");
  Serial.println(WiFi.localIP()); // Send the IP address of the ESP8266 to the computer
  leds[0] = CRGB(0, 0, 255); //make first led blue
  FastLED.show();
  lastReceived = millis(); //reset lastrecieved, so make shure it stays connected
}

float fscale(float originalMin, float originalMax, float newBegin, float newEnd, float inputValue, float curve) //the standard fscale function stolen from internet. 
//documentation: http://playground.arduino.cc/main/fscale
{

  float OriginalRange = 0;
  float NewRange = 0;
  float zeroRefCurVal = 0;
  float normalizedCurVal = 0;
  float rangedValue = 0;
  boolean invFlag = 0;

  // condition curve parameter
  // limit range

  if (curve > 10)
    curve = 10;
  if (curve < -10)
    curve = -10;

  curve = (curve * -.1);  // - invert and scale - this seems more intuitive - postive numbers give more weight to high end on output
  curve = pow(10, curve); // convert linear scale into lograthimic exponent for other pow function

  // Check for out of range inputValues
  if (inputValue < originalMin)
  {
    inputValue = originalMin;
  }
  if (inputValue > originalMax)
  {
    inputValue = originalMax;
  }

  // Zero Refference the values
  OriginalRange = originalMax - originalMin;

  if (newEnd > newBegin)
  {
    NewRange = newEnd - newBegin;
  }
  else
  {
    NewRange = newBegin - newEnd;
    invFlag = 1;
  }

  zeroRefCurVal = inputValue - originalMin;
  normalizedCurVal = zeroRefCurVal / OriginalRange; // normalize to 0 - 1 float

  // Check for originalMin > originalMax  - the math for all other cases i.e. negative numbers seems to work out fine
  if (originalMin > originalMax)
  {
    return 0;
  }

  if (invFlag == 0)
  {
    rangedValue = (pow(normalizedCurVal, curve) * NewRange) + newBegin;
  }
  else // invert the ranges
  {
    rangedValue = newBegin - (pow(normalizedCurVal, curve) * NewRange);
  }

  return rangedValue;
}
