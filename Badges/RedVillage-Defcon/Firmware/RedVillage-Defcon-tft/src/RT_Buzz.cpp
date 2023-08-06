#include <Arduino.h>

const int BZpin = 26;

void RT_Buzzer_init(){
    pinMode(BZpin, OUTPUT);
    Serial.println("Buzzer Initalisation done.");
}

void Welcome_Buzz(){
    tone(BZpin, 400, 300);
    tone(BZpin, 500, 300);
    tone(BZpin, 600, 300);
}

void Buzz_Spin(){
    for(int i =400;i<900;i+=50){
        Serial.println(i);
        tone(BZpin,i,50);
    }
}

void Step_Chirp(){
    tone(BZpin, 200,20);
}

void Menu_Beep(){
    tone(BZpin, 700, 200);
    tone(BZpin, 900, 200);
}