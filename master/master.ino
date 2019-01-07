#include <FastLED.h>                                       // FastLED library.

// Fixed definitions cannot change on the fly.
#define LED_DT  3                                       // Data pin to connect to the strip.
#define COLOR_ORDER RGB                                      // It's GRB for WS2812 and BGR for APA102.
#define LED_TYPE WS2812B                                      // Using APA102, WS2812, WS2801. Don't forget to modify LEDS.addLeds to suit.
#define NUM_LEDS 60                                         // Number of LED's.
#define SEN 0

int oldwaarde = 0;
int slope;
int oldPeak;
int amplitude;
int newWaarde;
int firstPeak;

// Global variables can be changed on the fly.
uint8_t max_bright = 255;                                      // Overall brightness definition. It can be changed on the fly.

CRGBPalette16 currentPalette;
TBlendType    currentBlending;

struct CRGB leds[NUM_LEDS];                                   // Initialize our LED array.
CRGB rgbval(50, 0, 500);
void setup() {
  Wire.begin();
  currentPalette = RainbowColors_p;
  currentBlending = LINEARBLEND;
  Serial.begin(9600);                                        // Initialize serial port for debugging.
  delay(1000);                                                // Soft startup to ease the flow of electrons.

  //  LEDS.addLeds<LED_TYPE, LED_DT, LED_CK, COLOR_ORDER>(leds, NUM_LEDS);  // Use this for WS2801 or APA102
  LEDS.addLeds<LED_TYPE, LED_DT, COLOR_ORDER>(leds, NUM_LEDS);  // Use this for WS2812

  FastLED.setBrightness(max_bright);
  set_max_power_in_volts_and_milliamps(5, 500);               // FastLED Power management set at 5V, 500mA.  
} // setup()

/* --- HERE THE REST */
void ledBar(int bar){
      fill_solid(leds, bar, rgbval );
      int colorIndex = 0;
      for ( int i = 0; i < bar; i++) {
        leds[i] = ColorFromPalette( currentPalette, colorIndex, 60, currentBlending);
        colorIndex += 3;
      }
      FastLED.show();
}

int hz(){
  Wire.requestFrom(8, 6);    // request 6 bytes from slave device #8
  while (Wire.available()) { // slave may send less than requested
    char c = Wire.read(); // receive a byte as character
    return c;
  }
}

void ledBar(int bar){
      fill_solid(leds, bar, rgbval );
      int colorIndex = 0;
      for ( int i = 0; i < bar; i++) {
        leds[i] = ColorFromPalette( currentPalette, colorIndex, 60, currentBlending);
        colorIndex += 3;
      }
      FastLED.show();
}

void loop() {
  // put your main code here, to run repeatedly:
  oldwaarde = newWaarde;
  newWaarde = analogRead(A0);
  if ((slope > 0 && (newWaarde - oldwaarde < 0)) || (slope < 0 && (newWaarde - oldwaarde > 0))) {
    firstPeak = oldwaarde;
    amplitude = firstPeak - oldPeak;
    oldPeak = firstPeak;
    if (amplitude < 0) {
      amplitude *= -1;
    }
    if (amplitude > 2) {
      if (amplitude > 50) {
        amplitude = 50;
      }
      Serial.print(amplitude);
      Serial.print("\t");
      int bar = map(amplitude, 0, 50, 0, NUM_LEDS);
      ledBar(bar);
      fill_solid(leds, bar, CRGB::Black);

      Serial.println(bar);
      //delay(10);
    }
  }
  slope = newWaarde - oldwaarde;
  delay(map(analogRead(A1), 0, 255, 0, 100));
}
