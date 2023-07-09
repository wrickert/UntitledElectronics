#include "RT_Led.h"
#include <Arduino.h>

#define PRpin 34
#define PBpin 33
#define PGpin 35
#define WRpin 38
#define WBpin 37
#define WGpin 36

unsigned long startTime = millis();

void RT_Led_init(){
    pinMode(PRpin, OUTPUT);
    pinMode(PBpin, OUTPUT);
    pinMode(PGpin, OUTPUT);
    pinMode(WRpin, OUTPUT);
    pinMode(WBpin, OUTPUT);
    pinMode(WGpin, OUTPUT);
    startTime = millis();
}

void PowerLed_on(){
    digitalWrite(PRpin, LOW);

}

unsigned long lastChange;
int currentColor = 0;

void WifiCycle(){
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