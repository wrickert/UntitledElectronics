#include <Arduino.h>
#include <PNGdec.h>
#include <TFT_eSPI.h>
#include "Assets/RedTeamVillage.h"
#include "Assets/CyberFloor.h"
#include "RT_Disp.h"
#include "RT_Button.h"
#include "RT_wifi.h"
#include "Assets/Guy_30x38.h"
#include "Assets/circle.h"
#include "Assets/arrow.h"
#include "Assets/Guy_smile_50x63.h"
#include "Assets/Guy_open_50x63.h"
#include "Assets/NotoSansBold15.h"

#define ENbtnPin 10

PNG png; // PNG decoder inatance

#define MAX_IMAGE_WIDTH 320 // Adjust for your images
#define AA_FONT_SMALL NotoSansBold15

int16_t xpos = 0;
int16_t ypos = 0;
int16_t Gxpos = 0;
int16_t Gypos = 0;
unsigned long startFaceTime = millis();


TFT_eSPI tft = TFT_eSPI();         // Invoke custom library.
TFT_eSprite background = TFT_eSprite(&tft);
TFT_eSprite guy = TFT_eSprite(&tft);  // Create Sprite object "img" with pointer to "tft" object
                                      // the pointer is used by pushSprite() to push it onto the TFT
TFT_eSprite talkingGuyS = TFT_eSprite(&tft);
TFT_eSprite talkingGuyO = TFT_eSprite(&tft);
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
}

void RT_Sprite_init(){
    tft.init();
    background.createSprite(320, 240);
    background.setSwapBytes(true);
    guy.createSprite(30,38);
    guy.setSwapBytes(true);
    talkingGuyS.createSprite(50,63);
    talkingGuyS.setSwapBytes(true);
    talkingGuyO.createSprite(50,63);
    talkingGuyO.setSwapBytes(true);
 //   text.createSprite(250,240);
  //  text.setSwapBytes(true);
}

int x=20;
int y=20;

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
    background.pushImage(0,0,320,240,CyberFloor);

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
    tft.fillScreen(TFT_BLACK);

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

    enableEncoderInturrupt();

    switch(M){
        //Game
        case 0:
            RT_background_refresh(1);
            break;
        //WiFi
        case 1:
            RT_Wifi_Scan();
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
            RT_Secret();
            break;

    }
}

void RT_Settings(){

}

void RT_About(){

}

void RT_Secret(){
    
}

//unsigned long lastChange;
int currentFace = 1;

bool RT_Conversation(int button){
    unsigned long currentTime = millis();
 //   tft.fillScreen(TFT_RED);
    if(currentFace == 1){

        talkingGuyS.pushImage(0, 0, 50, 63, Guy_smile_50x63);
        talkingGuyS.pushSprite(10,88);
    }
    if(currentFace == 2){
        talkingGuyO.pushImage(0, 0, 50, 63, Guy_open_50x63);
        talkingGuyO.pushSprite(10,88);
    }

    if(currentTime - startFaceTime >= 400){
        if(currentFace == 1)
            currentFace = 2;
        else if(currentFace == 2)
            currentFace = 1;
        startFaceTime = currentTime;
    } 

    tft.setCursor(70,0);

    tft.loadFont(AA_FONT_SMALL);
    //tft.setTextWrap(true);
//    tft.println("WELCOME!");
    tft.println( "Welcome to the Red Team ");
    tft.setCursor(70,15);
    tft.println( "Village badge! We're ");
    tft.setCursor(70,30);
    tft.println( "thrilled to have you here. ");
    tft.setCursor(70,45);
    tft.println( "This badge is all about ");
    tft.setCursor(70,60);
    tft.println( "WiFi hacking, an exciting ");
    tft.setCursor(70,75);
    tft.println( "and vital skill for ");
    tft.setCursor(70,90);
    tft.println( "cybersecurity professionals. ");
    tft.setCursor(70,105);
    tft.println( "Get ready to delve into the ");
    tft.setCursor(70,120);
    tft.println( "fascinating world of wireless ");
    tft.setCursor(70,135);
    tft.println( "network security. Let's empower ");
    tft.setCursor(70,150);
    tft.println( "you with the knowledge to ");
    tft.setCursor(70,165);
    tft.println( "secure networks and defend ");
    tft.setCursor(70,180);
    tft.println( "against potential threats. ");
    /*
    tft.setCursor(70,195);
    tft.println( "Together, we'll explore the ");
    tft.setCursor(70,220);
    tft.println( "intricacies of WiFi hacking ");
    tft.setCursor(70,45);
    tft.println( "and equip you with invaluable ");
    tft.setCursor(70,45);
    tft.println( "skills. Prepare to unlock new ");
    tft.setCursor(70,45);
    tft.println( "horizons in cybersecurity with ");
    tft.setCursor(70,45);
    tft.println( "the Red Team Village badge.");
    tft.setCursor(70,45);

    int textX = 70;
    int textY = 0;
    int wrapWidth = 320;  // Width of the wrap area
    int wrapHeight = 100; // Height of the wrap area
    int lineHeight = 16;  // Height of each line of text

*/
    //text.pushSprite(70,0);

  //  background.pushSprite(0,0);

 //   tft.unloadFont();
if(button == 5){
    return false;
    tft.unloadFont();
    tft.fillScreen(TFT_BLACK);
    Serial.println("Found An A");
}
else
return true;

  delay(3000);
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












