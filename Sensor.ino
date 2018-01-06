/*
  Sensor.ino - Sensor Library for the Infinitag System.
  Created by Jani Taxidis & Tobias Stewen & Florian Kleene.
  Info: www.infinitag.io

  All files are published under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International (CC BY-NC-SA 4.0)
  License: https://creativecommons.org/licenses/by-nc-sa/4.0/
*/
// Infinitag Libs
#include <Infinitag_Core.h>

// Vendor Libs
#include <Adafruit_NeoPixel.h>
#include <Wire.h>
#include <EEPROM.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif
#include <IRremote.h>
//#include <sensor_dhcp.h>

// Settings
#include "Settings.h"

// Infinitag Inits
Infinitag_Core infinitagCore;

// Vendor Inits
Adafruit_NeoPixel strip = Adafruit_NeoPixel(ledAmount, ledPin, NEO_GRBW + NEO_KHZ800);
decode_results irResults;
IRrecv irRecv(irReceivePin);

// Animations
byte animatePattern[7][20][4] = {
  {
    {0, 0, 0, 0},
  },
  {
    {100, 100, 100, 100},
  },
  {
    {5, 5, 5, 5},
  },
  {
    {100, 0, 1, 15},
    {15, 100, 0, 1},
    {1, 15, 100, 0},
    {0, 1, 15, 100}
  },
  {
    {100, 0, 0, 0},
    {0, 100, 0, 0},
    {0, 0, 100, 0},
    {0, 0, 0, 100},
    {0, 0, 100, 0},
    {0, 100, 0, 0}
  },
  {
    {100, 100, 0, 0},
    {0, 0, 0, 0},
    {100, 100, 0, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0},
    {0, 0, 100, 100},
    {0, 0, 0, 0},
    {0, 0, 100, 100},
    {0, 0, 0, 0},
    {0, 0, 0, 0}
  },
  {
    {100, 100, 100, 100},
    {90, 90, 90, 90},
    {80, 80, 80, 80},
    {70, 70, 70, 70},
    {60, 60, 60, 60},
    {50, 50, 50, 50},
    {40, 40, 40, 40},
    {30, 30, 30, 30},
    {20, 20, 20, 20},
    {10, 10, 10, 10},
    {1, 1, 1, 1},
    {10, 10, 10, 10},
    {20, 20, 20, 20},
    {30, 30, 30, 30},
    {40, 40, 40, 40},
    {50, 50, 50, 50},
    {60, 60, 60, 60},
    {70, 70, 70, 70},
    {80, 80, 80, 80},
    {90, 90, 90, 90}
  }
};
byte animateSteps[7] = {1, 1, 1, 4, 6, 10, 20};
byte animateCurrentStep = 0;
unsigned long animateNextStep = 0;
byte animateCurrentAnimation = 0;
unsigned int animateTime = 100;
uint8_t animateColor[4] = {0, 0 , 0, 0};


void setup() {  
  Serial.begin(57600);
  
  // LEDs
  strip.begin();
  waitLed(2);

  //Wait For Master to Boot 
  gI2cAddress = 0x22;
  //gI2cAddress = queryI2CAddressFromMaster();
  Wire.begin(gI2cAddress);
  Wire.onReceive(receiveEvent);
  Wire.onRequest(requestEvent);
  
  // IR
  irRecv.enableIRIn();

  // Animate
  animateNextStep = millis();
}

void loop() {
  if (irRecv.decode(&irResults)) {
    infinitagCore.irDecode(irResults.value);
    
    if(infinitagCore.irRecvTeamId != playerTeamId)
    {
      setValidIrShot(irResults.value);
    }
    irRecv.enableIRIn();
  }

  animation();
  
  delay(10);
}

void waitLed(int loops){
  for (int i = 0; i < (loops * 2); i++) {
    strip.setPixelColor(0, strip.Color(0,0,0,255));
    strip.setPixelColor(1, strip.Color(0,0,0,0));
    strip.setPixelColor(2, strip.Color(0,0,0,0));
    strip.setPixelColor(3, strip.Color(0,0,0,0));
    strip.show();
    delay(166);
  
    strip.setPixelColor(0, strip.Color(0,0,0,0));
    strip.setPixelColor(1, strip.Color(0,0,0,255));
    strip.setPixelColor(2, strip.Color(0,0,0,0));
    strip.setPixelColor(3, strip.Color(0,0,0,0));
    strip.show();
    delay(167);
  
    strip.setPixelColor(0, strip.Color(0,0,0,0));
    strip.setPixelColor(1, strip.Color(0,0,0,0));
    strip.setPixelColor(2, strip.Color(0,0,0,255));
    strip.setPixelColor(3, strip.Color(0,0,0,0));
    strip.show();
    delay(167);
  
    strip.setPixelColor(0, strip.Color(0,0,0,0));
    strip.setPixelColor(1, strip.Color(0,0,0,0));
    strip.setPixelColor(2, strip.Color(0,0,0,0));
    strip.setPixelColor(3, strip.Color(0,0,0,255));
    strip.show();
    delay(167);
  }
}

void receiveEvent(int howMany) {
  int byteCounter = 0;
  byte data[9] = {
    B0,
    B0,
    B0,
    B0,
    B0,
    B0,
    B0,
    B0,
    B0,
  };
  
  while (Wire.available()) {
    data[byteCounter] = Wire.read();
    byteCounter++;
  }

  switch (data[0]) {
    // Set TeamId
    case 0x02:
      if (byteCounter == 2) {
        playerTeamId = data[1];
      }
      break;
      
    // Set PlayerId
    case 0x03:
      if (byteCounter == 2) {
        playerId = data[1];
      }
      break;
      
    // Set Animation
    case 0x04:
      if (byteCounter == 9) {
        animateCurrentStep = 0;
        animateNextStep = 0;
        animateCurrentAnimation = data[1];
        animateTime = data[3] << 8 | data[2];
        animateColor[0] = data[4];
        animateColor[1] = data[5];
        animateColor[2] = data[6];
        animateColor[3] = data[7];
      }
      break;
      
    // Set Alive
    case 0x07:
      if (byteCounter == 2) {
        setAlive((data[1] == 0x01));
      }
      break;
  }
}

void requestEvent() {
  Wire.write(lastShot, 4);
  lastShot[0] = 0;
  lastShot[1] = 0;
  lastShot[2] = 0;
  lastShot[3] = 0;
}

void setValidIrShot(unsigned long code) {
  byte result[3];
  
  infinitagCore.irToBytes(code, &result[0]);

  lastShot[0] = 0x06;
  lastShot[1] = result[0];
  lastShot[2] = result[1];
  lastShot[3] = result[2];
}

void setAlive(bool alive) {
  playerAlive = alive;
}

void animation() {
  if (animateCurrentAnimation > 0) {
    if (animateNextStep <= millis()) {
      for (int i = 0; i < ledAmount; i++) {
        byte ledBrightness = animatePattern[animateCurrentAnimation][animateCurrentStep][i];
        strip.setPixelColor(i, strip.Color(
            ledBrightness * animateColor[0] / 100,
            ledBrightness * animateColor[1] / 100,
            ledBrightness * animateColor[2] / 100,
            ledBrightness * animateColor[3] / 100
        ));
      }
      strip.show();
  
      animateNextStep = millis() + animateTime;
      animateCurrentStep++;
      if (animateCurrentStep > animateSteps[animateCurrentAnimation] - 1) {
        animateCurrentStep = 0;
      }
    }
  } else {
    for (int i = 0; i < ledAmount; i++) {
      strip.setPixelColor(i, strip.Color(0,0,0,0));
    }
    strip.show();
  }
}

