#include <Arduino.h>
#include <PNGdec.h>
#include <TFT_eSPI.h>
#include "Assets/RedTeamVillage.h"
#include "Assets/CyberFloor.h"
#include "RT_Disp.h"
#include "Assets/Guy_30x38.h"
#include "Assets/circle.h"
#include "Assets/arrow.h"
#include "Assets/Guy_smile_50x63.h"
#include "Assets/Guy_open_50x63.h"
#include "Assets/NotoSansBold15.h"


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












