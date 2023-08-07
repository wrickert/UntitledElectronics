#include <Arduino.h>
#include <RT_Button.h>
#include <RT_Disp.h>
#include <Encoder.h>


#define UPbtnPin 45
#define DNbtnPin 42
#define LTbtnPin 8
#define RTbtnPin 7
#define AbtnPin 14
#define BbtnPin 15
#define STbtnPin 17
#define SELbtnPin 18

#define BOOTbtnPin 0

#define ENupPin 16
#define ENdownPin 9
#define ENbtnPin 10

//A flag to set if we need to display the menu.
bool NeedMenu = false;
bool bootSpecial = false;


void RT_Button_init(){
    pinMode(UPbtnPin, INPUT_PULLUP);
    pinMode(DNbtnPin, INPUT_PULLUP);
    pinMode(LTbtnPin, INPUT_PULLUP);
    pinMode(RTbtnPin, INPUT_PULLUP);
    pinMode(AbtnPin, INPUT_PULLUP);
    pinMode(BbtnPin, INPUT_PULLUP);
    pinMode(STbtnPin, INPUT_PULLUP);
    pinMode(SELbtnPin, INPUT_PULLUP);

    pinMode(BOOTbtnPin, INPUT_PULLUP);

    pinMode(ENupPin, INPUT_PULLUP);
    pinMode(ENdownPin, INPUT_PULLUP);
    pinMode(ENbtnPin, INPUT_PULLUP);

Serial.println("Button Initalisation done.");

}

int getUp(){
    return digitalRead(UPbtnPin);
}

int getDown(){
    return digitalRead(DNbtnPin);
}

int getLeft(){
    return digitalRead(LTbtnPin);
}

int getRight(){
    return digitalRead(RTbtnPin);
}

int getA(){
    return digitalRead(AbtnPin);
}

int getB(){
    return digitalRead(BbtnPin);
}

int upRead(){
    return digitalRead(DNbtnPin);
}

Encoder knob(ENupPin,ENdownPin);

int encRead(){
    return knob.read();
}

int encReadErase(){
    return knob.readAndReset();
}

int encButtonRead(){
    return digitalRead(ENbtnPin);
}

int bootButtonRead(){
    return digitalRead(BOOTbtnPin);
}

bool getNeedMenu(){
    return NeedMenu;
}

void setNeedMenu(bool newMenu){
    NeedMenu = newMenu;
}

bool getBootSpecial(){
    return bootSpecial;
}

void setBootSpecial(bool newBoot){
    bootSpecial = newBoot;
}


//variables to keep track of the timing of recent interrupts
unsigned long button_time = 0;  
unsigned long last_button_time = 0; 
//int x = 0;

/*
void IRAM_ATTR iRT(){
    button_time = millis();
    if (button_time - last_button_time > 250){
        Serial.println("Found Button");
        //x = getX
        setX(getX()+10);
        last_button_time = button_time;
    }
}

void IRAM_ATTR iLT(){
    button_time = millis();
    if (button_time - last_button_time > 250){
        Serial.println("Found Button");
        setX(getX()-10);
        last_button_time = button_time;
    }
}

void IRAM_ATTR iUP(){
    button_time = millis();
    if (button_time - last_button_time > 250){
        Serial.println("Found up Button");
        setY(getY()-10);
        last_button_time = button_time;
    }
}

void IRAM_ATTR iDN(){
    button_time = millis();
    if (button_time - last_button_time > 250){
        Serial.println("Found down Button");
        setY(getY()+10);
        last_button_time = button_time;
    }
}
*/

/*
void IRAM_ATTR iA(){
    button_time = millis();
    if (button_time - last_button_time > 250){
        Serial.println("Found A Button");
        last_button_time = button_time;
    }
}

void IRAM_ATTR iB(){
    button_time = millis();
    if (button_time - last_button_time > 250){
        Serial.println("Found B Button");
        last_button_time = button_time;
    }
}
*/

void IRAM_ATTR iST(){
    button_time = millis();
    if (button_time - last_button_time > 250){
        Serial.println("Found Start Button");
        last_button_time = button_time;
    }
}

void IRAM_ATTR iSEL(){
    button_time = millis();
    if (button_time - last_button_time > 250){
        Serial.println("Found SEL Button");
        last_button_time = button_time;
    }
}

/*
void IRAM_ATTR iENup(){
    button_time = millis();
    if (button_time - last_button_time > 250){
        Serial.println("Found Encoder Up Move");
        //x = getX
        setX(getX()+10);
        last_button_time = button_time;
    }
}

void IRAM_ATTR iENdn(){
    button_time = millis();
    if (button_time - last_button_time > 250){
        Serial.println("Found Encoder Down Move");
        //x = getX
        setX(getX()+10);
        last_button_time = button_time;
    }
}
*/


void IRAM_ATTR iENbtn(){
    button_time = millis();
    if (button_time - last_button_time > 250){
        last_button_time = button_time;
        detachInterrupt(ENbtnPin);
        NeedMenu = true;
    }
}

void IRAM_ATTR iBoot(){
    button_time = millis();
    if (button_time - last_button_time > 250){
        last_button_time = button_time;
        detachInterrupt(BOOTbtnPin);
        bootSpecial = true;
    }
}

void enableBootInturrupt(){
    attachInterrupt(BOOTbtnPin, iBoot, FALLING);
}

void enableEncoderInturrupt(){
    attachInterrupt(ENbtnPin, iENbtn, FALLING);
}

void RT_Button_Interrupt_init(){
    /*        
    attachInterrupt(RTbtnPin, iRT, FALLING);
    attachInterrupt(LTbtnPin, iLT, FALLING);
    attachInterrupt(UPbtnPin, iUP, FALLING);
    attachInterrupt(DNbtnPin, iDN, FALLING);
    */
    /*
    attachInterrupt(AbtnPin, iA, FALLING);
    attachInterrupt(BbtnPin, iB, FALLING);
    attachInterrupt(STbtnPin, iST, FALLING);
    attachInterrupt(SELbtnPin, iSEL, FALLING);
    */

    attachInterrupt(BOOTbtnPin, iBoot, FALLING);

 //   attachInterrupt(ENupPin, iENup, CHANGE);
 //  attachInterrupt(ENdownPin, iENdn, CHANGE);
    attachInterrupt(ENbtnPin, iENbtn, FALLING);

    Serial.println("Buttion Inturrupts Attached.");
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