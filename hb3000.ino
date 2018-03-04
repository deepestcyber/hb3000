#include <Adafruit_NeoPixel.h>


const int micPin = A0;
const int spkPin = A1;
const int ledPin = 4;
const int ledNum = 12;

#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))

Adafruit_NeoPixel strip = Adafruit_NeoPixel(ledNum, ledPin, NEO_GRB + NEO_KHZ800);

void setup()
{
//  sbi(ADCSRA, ADPS2);
//  cbi(ADCSRA, ADPS1);
//  cbi(ADCSRA, ADPS0);
  #if defined (__AVR_ATtiny85__)
    if (F_CPU == 16000000) clock_prescale_set(clock_div_1);
  #endif

  strip.begin();

  Serial.begin(9600);
}

uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

static void rainbowSteps(uint16_t steps, uint8_t wait) {
  uint16_t i;
  for(uint16_t j=0; j < steps; j++) {
    for(i=0; i< strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

// Slightly different, this makes the rainbow equally distributed throughout
static void rainbowCycle(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256*5; j++) { // 5 cycles of all colors on wheel
    for(i=0; i< strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
    }
    strip.show();
    delay(wait);
  }
}


const float maxVol = 60.0;
const int timeUntilGood = 2000;
const float goodnessThreshold = 0.75 * maxVol;
int goodSteps = 0;

static void updled(float vol) {
  // how many light?
  int activeLeds = floor(ledNum * (vol/maxVol));

  // how light colour??
  if ( vol >= goodnessThreshold ) {
    goodSteps += 1;
  } else {
    goodSteps = 0;
  }

  if ( goodSteps >= timeUntilGood ) {
    // do rainbow, candy mountain!
    rainbowSteps(2000, 1);
    goodSteps = 0;
    return;
  }
  
  float progress = min(1.0, (float) goodSteps / timeUntilGood);
  uint32_t colour = strip.Color(255*(1.-progress), 255*progress, 0);
  
  for ( int i = 0; i < ledNum; i++ ) {
    if ( activeLeds > i ) {
      strip.setPixelColor(i, colour);
    } else {
      strip.setPixelColor(i, strip.Color(0, 0, 0));
    }
  }
  strip.show();
}

int aVal = 0;
float al_background = 0.99;
float al_current = 0.99;
int i = 0;
float vol_background = 0.;
float vol_current = 0;
float vol_average = 0;
int calibrated = 0;

const int calibration_time = 2000;
const int volume_offset = 10;

void loop() {
  aVal = analogRead(micPin);
  i++;
  
  if (!calibrated && i <= calibration_time) {
    vol_background = (1 - al_background) * aVal + \
                      al_background * vol_background;

    for(int k=0; k < strip.numPixels(); k++) {
      strip.setPixelColor(k, Wheel(((k * 256 / strip.numPixels()) + i) & 255));
    }
    strip.show();
    calibrated = (i == calibration_time);
  }

  if (calibrated) {
    vol_current = abs(aVal - vol_background);
    vol_average = (1 - al_current) * vol_current + al_current * vol_average; 
  }

  float vol = max(0, vol_average - volume_offset);
  if (calibrated) {
    updled(vol);
  }
  
  if (i % 100 == 0) {
    if (!calibrated) {
      Serial.print("IN CALIBRATION -- ");
    }
    Serial.print("sensor slow = ");
    Serial.print(vol_background);
    Serial.print(", sensor fast = ");
    Serial.print(vol_current);
    Serial.print(", volume avg = ");
    Serial.print(vol_average);
    Serial.print(", volume = ");
    Serial.print(vol);
    Serial.println();
  }
}
