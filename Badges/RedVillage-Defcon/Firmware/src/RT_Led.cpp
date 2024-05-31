#include "RT_Led.h"
#include <Arduino.h>

#define PRpin 34
#define PBpin 33
#define PGpin 35
#define WRpin 38
#define WBpin 37
#define WGpin 36

#define BLPin 1

unsigned long startTime = millis();
bool LightsOut = false;

void RT_Led_init(){
    pinMode(PRpin, OUTPUT);
    pinMode(PBpin, OUTPUT);
    pinMode(PGpin, OUTPUT);
    pinMode(WRpin, OUTPUT);
    pinMode(WBpin, OUTPUT);
    pinMode(WGpin, OUTPUT);

    pinMode(BLPin, OUTPUT);
    
    startTime = millis();

    Serial.println("LED Initialisation done.");
}

void setLightsOut(bool value){
    LightsOut = value;
}

bool getLightsOut(){
    return LightsOut;
}

void Backlight_on(){
    digitalWrite(BLPin, HIGH);
}

void Backlight_off(){
    digitalWrite(BLPin, LOW);
}

void PowerLed_on(){
    if(!LightsOut){
        digitalWrite(PRpin, HIGH);
        digitalWrite(PBpin, HIGH);
        digitalWrite(PGpin, LOW);
    }
    else{
        digitalWrite(PRpin, HIGH);
        digitalWrite(PBpin, HIGH);
        digitalWrite(PGpin, HIGH);

    }
}

unsigned long lastChange;
int currentColor = 0;

int breatheStep = 20;
int brightness = 1;


void powerBreathe(){

        unsigned long currentTime = millis();
        if(currentTime - startTime >= 30) {
            analogWrite(PGpin, brightness);
              // change the brightness for next time through the loop:
  brightness = brightness + breatheStep;

  // reverse the direction of the fading at the ends of the fade:
  if (brightness <= 0 || brightness >= 255) {
    breatheStep = -breatheStep;
  }
        }
}

void WifiCycle(){
    if(!LightsOut){
        unsigned long currentTime = millis();
        if(currentTime - startTime >= 1000) {
            currentColor++;
            startTime = currentTime;
            if(currentColor > 3)
                currentColor = 1;
            if(currentColor == 1){
                digitalWrite(WRpin, LOW);
                digitalWrite(WBpin, HIGH);
                digitalWrite(WGpin, HIGH);
            }
            if(currentColor == 2){
                digitalWrite(WRpin, HIGH);
                digitalWrite(WBpin, LOW);
                digitalWrite(WGpin, HIGH);
            }
            if(currentColor == 3){
                digitalWrite(WRpin, HIGH);
                digitalWrite(WBpin, HIGH);
                digitalWrite(WGpin, LOW);
            }
        }
    }
    else{
        digitalWrite(WRpin, HIGH);
        digitalWrite(WBpin, HIGH);
        digitalWrite(WGpin, HIGH);

    }

}