#include <Arduino.h>

#define UPbtnPin 45
#define DNbtnPin 42
#define LTbtnPin 8
#define RTbtnPin 7
#define AbtnPin 14
#define BbtnPin 15
#define STbtnPin 17
#define SELbtnPin 18

void RT_Button_init(){
    pinMode(UPbtnPin, INPUT_PULLUP);
    pinMode(DNbtnPin, INPUT_PULLUP);
    pinMode(LTbtnPin, INPUT_PULLUP);
    pinMode(RTbtnPin, INPUT_PULLUP);
    pinMode(AbtnPin, INPUT_PULLUP);
    pinMode(BbtnPin, INPUT_PULLUP);
    pinMode(STbtnPin, INPUT_PULLUP);
    pinMode(SELbtnPin, INPUT_PULLUP);

}

//TODO turn this to a binary so I can catch multiple presses
int RT_Button_Scan(){
    if(digitalRead(UPbtnPin) == 0){
        //Serial.println("Up");
        return 1;}
    if(digitalRead(DNbtnPin) == 0){return 2;}
    if(digitalRead(LTbtnPin) == 0){return 3;}
    if(digitalRead(RTbtnPin) == 0){return 4;}

    if(digitalRead(AbtnPin) == 0){return 5;}

    else return 0;
}