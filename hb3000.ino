#include "FastLED.h"


const int micPin = A0;
const int spkPin = A1;
const int ledPin = 4;
const int ledNum = 12;
const int cheatPin = 7;

CRGB leds[ledNum];

class Mike {
  private:
    int pin;
    int dc_bias;
    float mean_factor;
    float mean;
  public:
    Mike(int pin, float mean_factor);
    int raw_read() { return analogRead(this->pin); }
    int read_volume() { return abs(raw_read() - dc_bias); }
    float read_mean();
    void calibrate();
};

Mike::Mike(int pin, float mean_factor) {
  this->pin = pin;
  this->mean_factor = mean_factor;
  this->dc_bias = 0;
  this->mean = 0.0f;
}

void Mike::calibrate() {
  int tmp = 0;
  const int initial_reads = 100;
  for (int i=0; i<initial_reads; i++) {
    float progress = (float)i/initial_reads;
    int leds_on = ledNum*progress;
    for ( int j=0;j<ledNum; j++) {
      if ( j<=leds_on ) {
        leds[j] = CRGB::Yellow;
      } else {
        leds[j] = CRGB::Black;
      }
    }
    FastLED.show();
    tmp += this->raw_read();
  }
  this->dc_bias = tmp / initial_reads;
  Serial.print("Bias: ");
  Serial.println(this->dc_bias);
}

float Mike::read_mean() {
  float current = this->read_volume();
  this->mean = this->mean * this->mean_factor + current * (1.0f - this->mean_factor);
  return this->mean;
}

// --

Mike mike(A0, 0.99);

bool cheating = false;


void setup() {
  #if defined (__AVR_ATtiny85__)
    if (F_CPU == 16000000) clock_prescale_set(clock_div_1);
  #endif

  FastLED.addLeds<WS2812B, ledPin, GRB>(leds, ledNum);

  // strip.begin();

  Serial.begin(9600);

  pinMode(cheatPin, INPUT_PULLUP);
  mike.calibrate();
}

void read_controls() {
  cheating = digitalRead(cheatPin) == LOW;
}


void rainbow(int steps) {
  for ( int i=0; i<steps; i++ ) {
    uint8_t start_hue = i % 256;
    fill_rainbow(leds, ledNum, start_hue, 256/ledNum);
    FastLED.show();
  }
}

const float maxVol = 60.0;
const int timeUntilGood = 2000;
const float goodnessThreshold = 0.75 * maxVol;
int goodSteps = 0;

static void updled(float vol) {
  if ( cheating ) {
    // you bloody cheater!
    vol = maxVol*0.8;
  }
  
  // how many light?
  int activeLeds = floor(ledNum * (vol/maxVol));

  // how light colour??
  if ( vol >= goodnessThreshold ) {
    goodSteps += 1;
  } else {
    goodSteps = 0;
  }

  if ( goodSteps >= timeUntilGood ) {
    // wow, I guess there actually is a place called candy mountain
    rainbow(1500);
    // reset progress
    goodSteps = 0;
    // clear:
    fill_solid(leds, ledNum, CRGB::Black);
    FastLED.show();
    return;
  }
  
  float progress = min(1.0, (float) goodSteps / timeUntilGood);
  CRGB colour = CRGB(255*(1.-progress), 255*progress, 0);
  // uint32_t colour = strip.Color(255*(1.-progress), 255*progress, 0);
  
  for ( int i = 0; i < ledNum; i++ ) {
    if ( activeLeds > i ) {
      leds[i] = colour;
    } else {
      leds[i] = CRGB::Black;
    }
  }
  FastLED.show();
}

int i = 0;
const int volume_offset = 10;

void loop() {
  i++;
  read_controls();

  float mean = mike.read_mean();
  float vol = max(0, mean - volume_offset);
  updled(vol);
  if ( i%100 == 1 ) {
    Serial.println(vol);
  }
}
