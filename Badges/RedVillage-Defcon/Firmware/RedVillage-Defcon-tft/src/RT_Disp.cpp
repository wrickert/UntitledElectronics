#include <Arduino.h>
#include <PNGdec.h>
#include <TFT_eSPI.h>
#include "Assets/RedTeamVillage.h"
#include "Assets/CyberFloor.h"
#include "RT_Disp.h"
#include "RT_Button.h"
#include "RT_wifi.h"
#include "RT_Game.h"
#include "RT_EEPROM.h"
#include "RT_Buzz.h"
#include "RT_Led.h"
#include "Assets/Guy_30x38.h"
#include "Assets/TalkBackground.h"
#include "Assets/Open.h"
#include "Assets/Closed.h"
#include "Assets/ConvoStrings.h"
#include "Assets/BrickBottom.h"
#include "Assets/BrikTop.h"
#include "Assets/BrickSide.h"
#include "Assets/BrickBottomLeft.h"
#include "Assets/BrickBottomRight.h"
#include "Assets/BrickSideTop.h"
#include "Assets/BrickSideBottom.h"
#include "Assets/BrickTopLeft.h"
#include "Assets/BrickTopRight.h"
#include "Assets/information.h"
#include "Assets/OpenBoss.h"
#include "Assets/MissingNo.h"
#include "Assets/Nah.h"
#include "Assets/NahDown.h"
#include "Assets/WEPBoss.h"
#include "Assets/WPABoss.h"
#include "Assets/UELogoSmall.h"
//#include "Assets/Guy_smile_50x63.h"
//#include "Assets/Guy_open_50x63.h"
#include "Assets/NotoSansBold15.h"

#define ENbtnPin 10

PNG png; // PNG decoder inatance

#define MAX_IMAGE_WIDTH 320 // Adjust for your images
#define AA_FONT_SMALL NotoSansBold15

int16_t xpos = 0;
int16_t ypos = 0;
int16_t Gxpos = 0;
int16_t Gypos = 0;
int x=100;
int y=101;
bool learned = false;
unsigned long startFaceTime = millis();


TFT_eSPI tft = TFT_eSPI();         // Invoke custom library.
TFT_eSprite background = TFT_eSprite(&tft);
TFT_eSprite guy = TFT_eSprite(&tft);  // Create Sprite object "img" with pointer to "tft" object
                                      // the pointer is used by pushSprite() to push it onto the TFT
TFT_eSprite talkingGuy = TFT_eSprite(&tft);
TFT_eSprite brickTop =  TFT_eSprite(&tft);
TFT_eSprite brickBottom =  TFT_eSprite(&tft);
TFT_eSprite brickLeft =  TFT_eSprite(&tft);
TFT_eSprite brickRight =  TFT_eSprite(&tft);
//TFT_eSprite talkingGuyO = TFT_eSprite(&tft);
//TFT_eSprite text = TFT_eSprite(&tft);

void RT_Disp_init(){
  // Initialise the TFT
  tft.begin();
  tft.setRotation(3);
  tft.fillScreen(TFT_BLACK);

    startFaceTime = millis();

  Serial.println("\r\nTFT Initialisation done."); 
}

TFT_eSPI RT_GetTFT(){
    return tft;
}

void RT_Disp_Splash(){
  xpos = 57;
  int16_t rc = png.openFLASH((uint8_t *)RedTeamVillage, sizeof(RedTeamVillage), pngDraw);
  if (rc == PNG_SUCCESS) {
    Serial.println("Successfully opened png file");
    Serial.printf("image specs: (%d x %d), %d bpp, pixel type: %d\n", png.getWidth(), png.getHeight(), png.getBpp(), png.getPixelType());
    tft.startWrite();
    uint32_t dt = millis();
    rc = png.decode(NULL, 0);
    Serial.print(millis() - dt); Serial.println("ms");
    tft.endWrite();
    // png.close(); // not needed for memory->memory decode
  }
  delay(3000);

    tft.fillScreen(TFT_BLACK);
}

void RT_Playground_setup(){
    xpos = 0;
    ypos = 0;

/*
  //  tft.fillScreen(TFT_GOLD);
  int16_t rc = png.openFLASH((uint8_t *)CyberFloor, sizeof(CyberFloor), pngDraw);
  if (rc == PNG_SUCCESS) {
    Serial.println("Successfully opened png file");
    Serial.printf("image specs: (%d x %d), %d bpp, pixel type: %d\n", png.getWidth(), png.getHeight(), png.getBpp(), png.getPixelType());
    tft.startWrite();
    uint32_t dt = millis();
    rc = png.decode(NULL, 0);
    Serial.print(millis() - dt); Serial.println("ms");
    tft.endWrite();
    // png.close(); // not needed for memory->memory decode
  }
  */
}

void RT_Sprite_init(){
    tft.init();
    background.createSprite(126, 240);
    background.setSwapBytes(true);
    guy.createSprite(30,38);
    guy.setSwapBytes(true);
    talkingGuy.createSprite(14,9);
    talkingGuy.setSwapBytes(true);
    tft.setSwapBytes(true);

    brickBottom.createSprite(320,36);
    brickTop.createSprite(320,45);
//    talkingGuyO.createSprite(50,63);
//    talkingGuyO.setSwapBytes(true);
 //   text.createSprite(250,240);
  //  text.setSwapBytes(true);
}


void setX(int newX){
    x = newX;
}

void setY(int newY){
    y = newY;
}

int getX(){
    return x;
}

int getY(){
    return y;
}

void RT_background_refresh(int button){
//    background.pushImage(0,0,320,240,CyberFloor);

    guy.pushImage(0, 0, 30, 38, Guy_30x38);
    guy.pushToSprite(&background, x, y);    
 
    //Serial.print("Button is: ");
    //Serial.println(button);

    background.pushSprite(0,0);

    if(button == 4 && x < 300)
        x+=5;
    if(button == 3 && x > 0)
        x-=5;
    if(button == 2 && y < 230)
        y+=5;
    if(button == 1 && y > 0)
        y-=5;
}

void text_Box_in(int x, int y, int w, int l, u_int32_t color1, u_int32_t color2){
    Serial.println("in Text box");
    int xstep = (w - x)/10;
    int ystep= (l - y )/10;
    int xmiddle = (w - x)/2;
    int ymiddle = (l - y)/2;

    TFT_eSPI tft = RT_GetTFT();

    for(int i = 1; i<=5; i++){
        //Serial.printf("Doing box x: %d, y: %d, %n",(xmiddle-i*xstep),(ymiddle-i*ystep));
        tft.fillRectHGradient((xmiddle-i*xstep),(ymiddle-i*ystep),((i*2)*xstep),((i*2)*ystep),color1,color2);
        delay(100);
    }
}

// TODO: change this to use rectangles of black instead of filling the screen to only refill
void text_Box_out(int x, int y, int w, int l, u_int32_t color1, u_int32_t color2){
    Serial.println("in Text box out");

    int xstep = (w - x)/10;
    int ystep= (l - y )/10;
    int xmiddle = (w - x)/2;
    int ymiddle = (l - y)/2;

    TFT_eSPI tft = RT_GetTFT();

    for(int i = 4; i>=1; i--){
        //Serial.printf("Doing box x: %d, y: %d, %n",(xmiddle-i*xstep),(ymiddle-i*ystep));
        //tft.fillScreen(TFT_BLACK);
        tft.fillRect((xmiddle-(i+1)*xstep),(ymiddle-(i+1)*ystep),(((i+1)*2)*xstep),(((i+1)*2)*ystep),TFT_BLACK);
        tft.fillRectHGradient((xmiddle-i*xstep),(ymiddle-i*ystep),((i*2)*xstep),((i*2)*ystep),color1,color2);
        delay(100);
    }
    tft.fillScreen(TFT_BLACK);
}

void blackScreen(){
    tft.fillScreen(TFT_BLACK);
}

int getColorSchemeOne(){
    return TFT_RED;
}

int getColorSchemeTwo(){
    return TFT_PINK;
}

int aX = 120;
int aY = 28;
//M is the last menu position
int M = 0;

void RT_Menu(){

    tft.fillRectHGradient(20, 10, 90, 150, getColorSchemeOne(), getColorSchemeTwo());

    tft.loadFont(AA_FONT_SMALL);
    tft.setCursor(30,20);
    tft.println("Game");
    tft.setCursor(30,50);
    tft.println("WiFi");
    tft.setCursor(30,80);
    tft.println("Settings");
    tft.setCursor(30,110);
    tft.println("About");

        int er = 0;
        int oldEr = 0;

    Menu_Beep();

    while(encButtonRead() == 0){}

    //Okay, lets add a cursor
    //Arrow X and Y are the location of the left tip
    while(encButtonRead() == 1){
        tft.fillTriangle(aX-2,aY,aX+52,aY-17,aX+52,aY+17,TFT_BLACK);

        aY = 27 + 30 * M;
        tft.drawTriangle(aX,aY,aX+50,aY-15,aX+50,aY+15,TFT_WHITE);
        tft.fillTriangle(aX+2,aY,aX+49,aY-13,aX+49,aY+13,TFT_RED);

        er = encRead();

        if(er != oldEr){
            Serial.print("Er is: ");
            Serial.println(er);
            if(er > oldEr){
                Serial.println("Moving down");
                M--;
                if(M<0 && touchRead(4) < 100000)
                    M = 3;
                if(M<0 && touchRead(4) > 100000)
                    M = 4;
                oldEr = er;
            }
                oldEr = er;
            /*
            else if(er < oldEr){
                M++;
                Serial.println("Moving up");
                Serial.println(touchRead(4));
                if(M>3 && touchRead(4) < 100000)
                    M = 0;
                if(M>3 && touchRead(4) > 100000)
                    M = 4;
                oldEr = er;
            }
            */
        }

    }

    while(encButtonRead() == 0){}
    setNeedMenu(false);

    tft.unloadFont(); // Remove the font to recover memory used
    enableEncoderInturrupt();

    switch(M){
        //Game
        case 0:
            //RT_background_refresh(1);
            Pedagogy();
            break;
        //WiFi
        case 1:
            //RT_Wifi_Scan();
            InitalizeWifi();
            break;
        //Settings
        case 2:
            RT_Settings();
            break;
        //About
        case 3:
            RT_About();
            break;
        //Secret
        case 4:
            RT_Secret(1);
            break;

    }
}

void RT_Settings(){
    tft.loadFont(AA_FONT_SMALL);
    tft.fillScreen(TFT_BLACK);
    tft.fillRectHGradient(20, 20, 270, 190, getColorSchemeOne(), getColorSchemeTwo());

    tft.setCursor(30,60);
    tft.println("PRESS AND HOLD A FOR 5 SECONDS TO CLEAR GAME DATA");

    unsigned long resetGameTime = millis();

    for(int i = 0; i<60;i++){
        resetGameTime = millis();
        while(getA() == 0){
            if(millis() - resetGameTime == 5000){
                Sad();
                resetAllGameData();
                ESP.restart();
            }
            if(millis() - resetGameTime == 1000)
                Step_Chirp();
            delay(20);
        }
            delay(100);
            if(getNeedMenu()){
                setNeedMenu(false);
                RT_Menu();
            }
    }

}

void RT_About(){
    tft.loadFont(AA_FONT_SMALL);
    tft.fillScreen(TFT_BLACK);
    tft.fillRectHGradient(10, 10, 310, 220, getColorSchemeOne(), getColorSchemeTwo());

    tft.setCursor(20,20);
    tft.println("Made with love and significant snark ");
    tft.setCursor(40,40);
    tft.println("by:");
    tft.pushImage(30, 60, 250, 90, UELogoSmall);
    tft.setCursor(20,160);
    tft.println("RocketGeek, Kevin, and Ashton");
    tft.setCursor(20,180);
    tft.println("THANK YOU RED TEAM VILLAGE FOR ");
    tft.setCursor(20,200);
    tft.println("LETTING US HAVE SO MUCH FUN!");
    delay(6000);
    tft.unloadFont();
    tft.fillScreen(TFT_BLACK);
    RT_Menu();
}

void RT_Secret(int number){
    tft.loadFont(AA_FONT_SMALL);
    tft.fillRectHGradient(50, 50, 250, 150, getColorSchemeOne(), getColorSchemeTwo());

    tft.setCursor(70,70);
    tft.println("YOU FOUND A SECRET!");
    tft.setCursor(70,100);
    if(number == 1)
        tft.println("flag:{FLAGHIDDENMENU}");
    if(number == 2)
        tft.println("flag:{FLAGNoDilophosaurusHere}");
    if(number == 3)
        tft.println("flag:{WIFIMASTER}");
    if(number == 4){
        tft.println("CONGRATULATIONS! You WIN!");
        tft.setCursor(70,120);
        tft.println("flag:{WIFIMASTER}");
    }

    delay(6000);
    tft.unloadFont(); // Remove the font to recover memory used
    RT_Menu();
}

void RT_Battery_Warning(){
    tft.loadFont(AA_FONT_SMALL);
    tft.fillRectHGradient(50, 50, 250, 150, getColorSchemeOne(), getColorSchemeTwo());

    tft.setCursor(70,70);
    tft.println("Your battery is running low");
    tft.setCursor(70,100);
    tft.println("Please replace batteries");

    delay(3000);
    tft.unloadFont(); // Remove the font to recover memory used
}

void RT_BossQuiz(int bossNum){
    tft.fillScreen(TFT_BLACK);

    tft.fillRectHGradient(20, 10, 235, 230, getColorSchemeOne(), getColorSchemeTwo());

    tft.loadFont(AA_FONT_SMALL);
    tft.setCursor(30,20);

    // J is which part of the question to display
    int row = 0;
    int modNumber = 26;
    int charCount = 0;
    int answers[3] = {1,1,1};
    int answer1 = -1;
    int answer2 = -1;
    int answer3 = -1;
    bool NextQuestion = false;
    int questionIndex = 1;

    int QuestionOffset = 0;
    if(bossNum == 2)
        QuestionOffset = 12;
    if(bossNum == 3)
        QuestionOffset = 24;

    Serial.println("NOW! Lets see what you learned");

    //List each question
    for(int Question = 1; Question < 4; Question++){  
        bool answered = false;

        //List each answer
        for(int j = ((Question -1) * 4 + 1); j < (4 * Question + 1); j++){
            Serial.print("Printing question ");
            Serial.print(Question);
            Serial.print(" part ");
            Serial.println(j);
            // Line Wrap
            for(int i=0; i<  QuizQ[j + QuestionOffset].size(); i++){
                tft.print(QuizQ[j + QuestionOffset][i]);
                    if(charCount == modNumber){
                        if(QuizQ[j + QuestionOffset][i+1] != ' ' || QuizQ[j + QuestionOffset][i+2] != ' ')
                            tft.print("-");
                        row++;
                        charCount = 0;
                        tft.setCursor(30,20*row +15);
                    }

                charCount++;
            }
            charCount = 0;
            row++;
            tft.setCursor(30,20*row +20);
        }

        int QaX = 265;
        int QaY = 200;
        int QM = 1;


        //while(getA() != 0){
        while(!answered){
            tft.fillTriangle(QaX-2,QaY,QaX+52,QaY-17,QaX+52,QaY+17,TFT_BLACK);

            QaY = 70 + 35 * QM;
            tft.drawTriangle(QaX,QaY,QaX+50,QaY-15,QaX+50,QaY+15,TFT_WHITE);
            tft.fillTriangle(QaX+2,QaY,QaX+49,QaY-13,QaX+49,QaY+13,TFT_RED);

            if(getUp() == 0){
                while(getUp() == 0){
                    delay(50);
                }
                if(QM == 1)
                    QM = 3;
                else
                    QM--;
            }
            else if(getDown() == 0){
                while(getDown() == 0){
                    delay(50);
                }
                if(QM == 3)
                    QM = 1;
                else
                    QM++;

            }
            if (getA() == 0) {
                answered = true;
                while(getA() == 0){
                    while(getA() == 0){
                        delay(20);
                    }
                    delay(200);
                }
            }

            delay(300);
        }
        
        
      // delay(600);
        row = 0;
        tft.setCursor(30,20);
        //WHY ON GODS GREEN EARTH DOES THIS MAKE THE QUESTION LOOP GO BEYOND 3?!
        //answers[Question] = QM;

        if(Question == 1 ){
            answer1 = QM;
        }
        if(Question == 2 ){
            answer2 = QM;
        }
        if(Question == 3 ){
            answer3 = QM;
        }

        Serial.print("You answered ");
        Serial.print(QM);
        //Serial.print(" . The correct answer was ");
        //Serial.println(getQuizAnswer(bossNum,Question));

        if(QM == getQuizAnswer(bossNum,Question)){
            tft.fillScreen(TFT_GREEN);
            Happy();
            delay(50);
        }
        else{
            tft.fillScreen(TFT_RED);
            Sad();
            delay(50);
        }
        tft.fillScreen(TFT_BLACK);
        tft.fillRectHGradient(20, 10, 235, 230, getColorSchemeOne(), getColorSchemeTwo());
    }

    if(answer1 == getQuizAnswer(bossNum, 1) && 
    answer2 == getQuizAnswer(bossNum,2) &&
    answer3 == getQuizAnswer(bossNum,3)){
        tft.fillScreen(TFT_GREEN);
        delay(100);

        Success();
        tft.fillRectHGradient(20, 10, 235, 230, getColorSchemeOne(), getColorSchemeTwo());
        tft.setCursor(60,30);
        tft.println("WELL DONE!");
        tft.setCursor(50,50);
        tft.println("All answers correct!");

        if(bossNum == 1 && get_Boss_status() == 0b000)
            set_Boss_status(0b001);
        else if(bossNum == 1 && get_Boss_status() == 0b010)
            set_Boss_status(0b011);
        else if(bossNum == 1 && get_Boss_status() == 0b100)
            set_Boss_status(0b101);
        else if(bossNum == 1 && get_Boss_status() == 0b110)
            set_Boss_status(0b111);

        else if(bossNum == 2 && get_Boss_status() == 0b000)
            set_Boss_status(0b010);
        else if(bossNum == 2 && get_Boss_status() == 0b001)
            set_Boss_status(0b011);
        else if(bossNum == 2 && get_Boss_status() == 0b100)
            set_Boss_status(0b110);
        else if(bossNum == 2 && get_Boss_status() == 0b101)
            set_Boss_status(0b111);

        else if(bossNum == 3 && get_Boss_status() == 0b000)
            set_Boss_status(0b100);
        else if(bossNum == 3 && get_Boss_status() == 0b010)
            set_Boss_status(0b110);
        else if(bossNum == 3 && get_Boss_status() == 0b001)
            set_Boss_status(0b101);
        else if(bossNum == 3 && get_Boss_status() == 0b011)
            set_Boss_status(0b111);

        delay(6000);

        if(get_Boss_status() == 0b111){
            RT_Secret(4);
        }
    }
    else {
        tft.fillScreen(TFT_RED);
        delay(100);

        Failure();
        tft.fillRectHGradient(20, 10, 235, 230, getColorSchemeOne(), getColorSchemeTwo());
        tft.setCursor(60,30);
        tft.println("Try again");
        tft.setCursor(50,50);
        tft.println("One or more answers incorrect");

        delay(6000);
    }

    tft.fillScreen(TFT_BLACK);
    Draw_Room(get_Room_Number());
    guy.pushSprite(x,y);
}

//unsigned long lastChange;
int currentFace = 1;

void RT_Conversation(int number){
    tft.fillScreen(TFT_BLACK);
    Serial.println("In Conversation");
    unsigned long currentTime = millis();

    RT_Talking(0);
    RT_Convo_Text_Wrap(number);

    while(getA() != 0){
        if(millis() - startFaceTime >= 400){
            if(currentFace == 1){
                currentFace = 2;
                RT_Talking(1);
            }
            else if(currentFace == 2){
                currentFace = 1;
                RT_Talking(2);
            }
            startFaceTime = millis();
            }
    } 

            tft.fillScreen(TFT_CYAN);
            tft.fillScreen(TFT_BLACK);
            Draw_Room(get_Room_Number());
            guy.pushSprite(x,y);
}

//Function to take care of flapping the lips
void RT_Talking(int stage){
    if(stage == 0) {
        background.pushImage(0,0,126,240,TalkBackground);
    }
    else if(stage == 1) {
        talkingGuy.pushImage(0,0,14,9,Open);
        talkingGuy.pushToSprite(&background,38,103);
        background.pushSprite(0,0);
    }
    else if(stage == 2) {
        talkingGuy.pushImage(0,0,14,9,Closed);
        talkingGuy.pushToSprite(&background,38,103);
        background.pushSprite(0,0);
    }
}

void RT_Convo_Text_Wrap(int s){
    tft.loadFont(AA_FONT_SMALL);
    int row = 0;
    int modNumber = 24;
    int charCount = 0;
    tft.setCursor(125,15);

    if(s == -1){
        s = 1;
        for(int i=0; i<Intro[s].size(); i++){
            tft.print(Intro[s][i]);
            if(charCount == modNumber){
                row++;
                charCount = 0;
                tft.setCursor(125,20*row +15);
            }
            else if(charCount == (modNumber-1) && Intro[s][i+1] == ' '){
                row++;
                charCount = 0;
                tft.setCursor(125,20*row +15);
            }
            else if(charCount == (modNumber-2) && Intro[s][i+1] == ' '){
                row++;
                charCount = 0;
                tft.setCursor(125,20*row +15);
            }
            else if(charCount == (modNumber-3) && Intro[s][i+1] == ' '){
                row++;
                charCount = 0;
                tft.setCursor(125,20*row +15);
            }
            else if(charCount == (modNumber-4) && Intro[s][i+1] == ' '){
                row++;
                charCount = 0;
                tft.setCursor(125,20*row +15);
            }
            if(Intro[s][i+1] == '/'){
                i++;
                row++;
                tft.setCursor(125,20*row +15);
            }
            charCount++;
        }
    }
    else {
        for(int i=0; i<Lessons[s].size(); i++){
            tft.print(Lessons[s][i]);
            if(charCount == modNumber){
                row++;
                charCount = 0;
                tft.setCursor(125,20*row +15);
            }
            else if(charCount == (modNumber-1) && Lessons[s][i+1] == ' '){
                row++;
                charCount = 0;
                tft.setCursor(125,20*row +15);
            }
            else if(charCount == (modNumber-2) && Lessons[s][i+1] == ' '){
                row++;
                charCount = 0;
                tft.setCursor(125,20*row +15);
            }
            else if(charCount == (modNumber-3) && Lessons[s][i+1] == ' '){
                row++;
                charCount = 0;
                tft.setCursor(125,20*row +15);
            }
            else if(charCount == (modNumber-4) && Lessons[s][i+1] == ' '){
                row++;
                charCount = 0;
                tft.setCursor(125,20*row +15);
            }

            if(Lessons[s][i+1] == '/'){
                i++;
                row++;
                tft.setCursor(125,20*row +15);
            }
            charCount++;
        }
    }
    tft.unloadFont();
}

void GameSetup(){

}


void drawHexagonGrid(int startX, int startY, int offsetX, int offsetY, int numColumns, int numRows) {
  int sideLength = 10; // Customize the side length of the hexagons as needed

  int x, y;
  for (int row = 0; row < numRows; row++) {
    for (int col = 0; col < numColumns; col++) {
     // x = startX + col * (sideLength * 3 / 2) + (row % 2) * (sideLength * 3 / 4);
     // y = startY + row * (sideLength * 2) * 3 / 4;
      x = startX + col * (offsetX + sideLength * 3);
      y = startY + row * (offsetY + sideLength * 2) + (col % 2) * (offsetY + sideLength);

      drawHexagon(x, y, sideLength);
    }
  }
}

void drawHexagon(int x, int y, int sideLength) {
  int x0, y0, x1, y1;

  for (int i = 0; i < 6; i++) {
    x0 = x + sideLength * cos(2 * PI / 6 * i);
    y0 = y + sideLength * sin(2 * PI / 6 * i);
    x1 = x + sideLength * cos(2 * PI / 6 * (i + 1));
    y1 = y + sideLength * sin(2 * PI / 6 * (i + 1));

    tft.drawLine(x0, y0, x1, y1, TFT_GREENYELLOW);
  }
}

void dont(){
    if(get_Boot_Presses() < 2){
        while(bootButtonRead() == 0){}
        setBootSpecial(false);
        enableBootInturrupt();

        tft.loadFont(AA_FONT_SMALL);
        tft.fillRectHGradient(50, 50, 250, 150, getColorSchemeOne(), getColorSchemeTwo());

        tft.setCursor(70,70);
        tft.println("HEY! DONT TOUCH ME THERE!");

        tft.unloadFont();

        set_Boot_Presses(get_Boot_Presses()+1);

        delay(2000);

        tft.fillScreen(TFT_BLACK);
        Draw_Room(get_Room_Number());
        guy.pushSprite(x,y);
    }
    else {
        while(bootButtonRead() == 0){}
        setBootSpecial(false);
        enableBootInturrupt();
        /*
        tft.loadFont(AA_FONT_SMALL);
        tft.fillRectHGradient(50, 50, 250, 150, getColorSchemeOne(), getColorSchemeTwo());

        tft.setCursor(70,70);
        tft.println("YOU ASKED FOR IT");

        tft.unloadFont();
        */
       if(touchRead(4) > 100000 && touchRead(3) > 100000){
            RT_Secret(2);
            set_Boot_Presses(0);
       }
       else{
        bool change = false;
        bool down = false;
        int j = 0;

            tft.pushImage(130,90,96,77,Nah);

            for(int i =0; i<80; i++){
                Siren();
                set_Siren_Count(1);
                Serial.print("Siren is ");
                Serial.print(i);
                Serial.println(" out of 100");
                j++;
                if(j == 2){
                    j = 0;
                    if(down){
                        down = false;
                        change = true;
                    }
                    else{
                        down = true;
                        change = true;
                    }
                }

                if(change){
                    if(down)
                        tft.pushImage(130,90,90,76,NahDown);
                    else
                        tft.pushImage(130,90,90,77,Nah);
                }
                delay(400);
            }
            if(get_Siren_Count() >= 500){
                set_Boot_Presses(0);
                }
        }

        shutUp();
        tft.fillScreen(TFT_BLACK);
        Draw_Room(get_Room_Number());
        guy.pushSprite(x,y);
    
    }
}

void Pedagogy(){
    tft.fillScreen(TFT_BLACK);
    Draw_Room(get_Room_Number());

//    int x = 100;
//    int y = 101;
    int oldX = 0;
    int oldY = 0;
    bool redraw = false;

    guy.pushImage(0, 0, 30, 38, Guy_30x38);
    guy.pushSprite(x,y);

    unsigned long sleepTimer = millis();
    unsigned long currentTime = millis();
    bool movement = true;
    bool beepOnce = true;

    while(true){
        currentTime = millis();
        if((currentTime - sleepTimer)  >= 30000 && movement == false){
            Serial.println("Going to sleep");
            setLightsOut(true);
            PowerLed_on();
            Backlight_off();
            setLightsOut(false);
            powerBreathe();
            WifiCycle();
            if(beepOnce){
                Success();
                Sad();
                beepOnce = false;
            }

            if(getA() == 0 || getB() ==0 || getUp() == 0 || getDown() == 0){
                setLightsOut(false);
                Backlight_on();
                PowerLed_on();
                beepOnce = true;

            }
        }
        if(movement){
            sleepTimer = millis();
            movement = false;
        }

        if(getNeedMenu()){
            setNeedMenu(false);
            RT_Menu();
        }

        if(getBootSpecial()){
            setBootSpecial(false);
            dont();
        }

        //guy.pushImage(0, 0, 30, 38, Guy_30x38);
        //guy.pushToSprite(&background, x, y);    
        //background.pushSprite(0,0);
        if(getUp() == 0){
            movement = true;
            if(y>52){
                if(y<65 && get_Room_North(get_Room_Number()) != -1){
                    Serial.println("Moving North");
                    set_Room_Number(get_Room_North(get_Room_Number()));
                    x = 145;
                    y = 220;
                    redraw = true;
                }

                Step_Chirp();
                tft.fillRect(x, y ,30, 38, TFT_BLACK);
                y -= 10;
                if(is_on_info(x, y) && !learned){
                    if(get_Room_Number() == 6)
                        RT_BossQuiz(1);
                    else if(get_Room_Number() == 14)
                        RT_BossQuiz(2);
                    else if(get_Room_Number() == 20)
                        RT_BossQuiz(3);
                    else
                        RT_Conversation(get_Room_Number());
                    int x = 100;
                    int y = 101;
                    learned = true;
                    if(get_IsBossRoom(get_Room_Number()) == 0){
                        tft.pushImage(145,105,30,30,information);
                    }
                    //else{
                    //    RT_BossQuiz(get_Room_Number());
                    //}
                }
                if(get_IsBossRoom(get_Room_Number()) == 0){
                    tft.pushImage(145,105,30,30,information);
                }
                guy.pushSprite(x,y);
            }
        }
        else if(getRight() == 0){
            movement = true;
            if(x<284){
                if(x>264 && get_Room_East(get_Room_Number()) != -1){
                    Serial.println("Moving East");
                    set_Room_Number(get_Room_East(get_Room_Number()));
                    x = 30;
                    y = 101;
                    redraw = true;
                }
                tft.fillRect(x, y ,30, 38, TFT_BLACK);
                Step_Chirp();
                x += 10;
                if(is_on_info(x, y) && !learned){
                    if(get_Room_Number() == 6)
                        RT_BossQuiz(1);
                    else if(get_Room_Number() == 14)
                        RT_BossQuiz(2);
                    else if(get_Room_Number() == 20)
                        RT_BossQuiz(3);
                    else
                        RT_Conversation(get_Room_Number());
                    int x = 100;
                    int y = 101;
                    learned = true;
                    if(get_IsBossRoom(get_Room_Number()) == 0){
                        tft.pushImage(145,105,30,30,information);
                    }
                }
                if(get_IsBossRoom(get_Room_Number()) == 0){
                    tft.pushImage(145,105,30,30,information);
                }
                guy.pushSprite(x,y);
            }
        }
        else if(getDown() == 0){
            movement = true;
            if(y<175){
                if(y>160 && get_Room_South(get_Room_Number()) != -1){
                    Serial.println("Moving South");
                    set_Room_Number(get_Room_South(get_Room_Number()));
                    x = 145;
                    y = 40;
                    redraw = true;
                }
                tft.fillRect(x, y ,30, 38, TFT_BLACK);
                Step_Chirp();
                y += 10;
                if(is_on_info(x, y) && !learned){
                    if(get_Room_Number() == 6)
                        RT_BossQuiz(1);
                    else if(get_Room_Number() == 14)
                        RT_BossQuiz(2);
                    else if(get_Room_Number() == 20)
                        RT_BossQuiz(3);
                    else
                        RT_Conversation(get_Room_Number());
                    int x = 100;
                    int y = 101;
                    learned = true;
                    if(get_IsBossRoom(get_Room_Number()) == 0){
                        tft.pushImage(145,105,30,30,information);
                    }
                }
                if(get_IsBossRoom(get_Room_Number()) == 0){
                    tft.pushImage(145,105,30,30,information);
                }
                guy.pushSprite(x,y);
            }
        }
        else if(getLeft() == 0){
            movement = true;
            if(x>20){
                if(x<40 && get_Room_West(get_Room_Number()) != -1){
                    Serial.println("Moving West");
                    set_Room_Number(get_Room_West(get_Room_Number()));
                    x = 280;
                    y = 101;
                    redraw = true;
                }
                tft.fillRect(x, y ,30, 38, TFT_BLACK);
                Step_Chirp();
                x -= 10;
                if(is_on_info(x, y) && !learned){
                    if(get_Room_Number() == 6)
                        RT_BossQuiz(1);
                    else if(get_Room_Number() == 14)
                        RT_BossQuiz(2);
                    else if(get_Room_Number() == 20)
                        RT_BossQuiz(3);
                    else
                        RT_Conversation(get_Room_Number());
                    int x = 100;
                    int y = 101;
                    learned = true;
                }
                if(get_IsBossRoom(get_Room_Number()) == 0){
                    tft.pushImage(145,105,30,30,information);
                }
                guy.pushSprite(x,y);
            }
        }
        if(redraw){
            tft.fillScreen(TFT_RED);
            tft.fillScreen(TFT_BLACK);
            Draw_Room(get_Room_Number());
            guy.pushSprite(x,y);
            redraw = false;
            learned = false;
        }
        delay(200);
    }
}

void Draw_Room(int RoomNumber){
    //background.pushImage(0,0,320,240,CyberFloor);
    //drawHexagonGrid(5, 5, -14, -2, 25, 16); // Customize parameters as needed

    int RoomLayout = get_RoomLayout(RoomNumber);

    int NorthMask = 1 << 3;
    int EastMask = 1 << 2;
    int SouthMask = 1 << 1;
    int WestMask = 1;

    int NorthBit = (RoomLayout & NorthMask) ? 1 : 0;
    int EastBit = (RoomLayout & EastMask) ? 1 : 0;
    int SouthBit = (RoomLayout & SouthMask) ? 1 : 0;
    int WestBit = (RoomLayout & WestMask) ? 1 : 0;

    Serial.print("You are currently in room number: ");
    Serial.println(RoomNumber);
    Serial.print("The exits are to the ");
    if(NorthBit == 1){
        Serial.print("North ");
        tft.pushImage(0,0,119,42,BrickTopLeft);
        tft.pushImage(199,0,119,42,BrickTopRight);
    }
    else
        tft.pushImage(0,0,320,36,BrikTop);
    if(EastBit == 1){
        Serial.print("East ");
        tft.pushImage(304,36,16,51,BrickSideTop);
        tft.pushImage(304,157,18,58,BrickSideBottom);
    }
    else
        tft.pushImage(304,36,18,182,BrickSide);
    if(SouthBit == 1){
        Serial.print("South ");
        tft.pushImage(0,214,120,25,BrickBottomLeft);
        tft.pushImage(200,214,120,25,BrickBottomRight);
    }
    else
        tft.pushImage(0,214,320,42,BrickBottom);
    if(WestBit == 1){
        Serial.print("West ");
        tft.pushImage(0,36,16,51,BrickSideTop);
        tft.pushImage(0,157,18,58,BrickSideBottom);
    }
    else
        tft.pushImage(0,36,18,182,BrickSide);
    
    Serial.println("");

    if(get_IsBossRoom(RoomNumber) == 0){
        tft.pushImage(145,105,30,30,information);
    }

    if(RoomNumber == 6){
        tft.pushImage(136,95,48,40,OpenBoss);
    }
    else if(RoomNumber == 8){
        tft.pushImage(136,95,35,81,MissingNo);
        delay(5000);
        
    }
    else if(RoomNumber == 14){
        tft.pushImage( 133, 85, 53, 70, WEPBoss);
    }
    else if(RoomNumber == 20){
        tft.pushImage( 125, 100, 70, 111, WPABoss);
    }

    //brickBottom.pushImage(0,0,320,36,BrickBottom);
    //talkingGuy.pushToSprite(&background,0,204);
    //background.pushSprite(0,0);
}

//=========================================v==========================================
//                                      pngDraw
//====================================================================================
// This next function will be called during decoding of the png file to
// render each image line to the TFT.  If you use a different TFT library
// you will need to adapt this function to suit.
// Callback function to draw pixels to the display
void pngDraw(PNGDRAW *pDraw) {
  uint16_t lineBuffer[MAX_IMAGE_WIDTH];
  png.getLineAsRGB565(pDraw, lineBuffer, PNG_RGB565_BIG_ENDIAN, 0xffffffff);
  tft.pushImage(xpos, ypos + pDraw->y, pDraw->iWidth, 1, lineBuffer);
}












