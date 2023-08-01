#include <Arduino.h>
#include <TFT_eSPI.h>
#include "RT_Disp.h"
#include "RT_Button.h"
#include "RT_Led.h"
#include "RT_wifi.h"



void setup() {
  Serial.begin(115200);

  RT_Disp_init();
  RT_Button_init();
  RT_Led_init();

  Backlight_on();
  PowerLed_on();
  RT_Button_Interrupt_init();

 // RT_Wifi_Scan();


 //RT_Disp_Splash();
//delay(3000);

RT_Sprite_init();
//RT_Conversation(1);
//delay(6000);

}

bool once = true;
int button = 0;

void loop() {

 // int button = RT_Button_Scan();
  /*
if(once == true)
  once =  RT_Conversation(button);
else
*/
  //RT_background_refresh(button);

WifiCycle();
 // RT_Menu();
//text_Box_out(10, 50, 300, 200, TFT_RED, TFT_PINK);

if(getNeedMenu()){
  setNeedMenu(false);
  RT_Menu();
}


}


