#include <Arduino.h>
#include <TFT_eSPI.h>
#include "RT_Disp.h"
#include "RT_Button.h"
#include "RT_Led.h"
#include "RT_wifi.h"
#include "RT_wifi_only.h"
#include <EEPROM.h>
#include <rom/rtc.h>
#include "RT_EEPROM.h"
#include "RT_Buzz.h"


void setup() {
    Serial.begin(115200);

    Serial.println("\n\nWelcome To the Red Team Village Badge! \n");
    LastTimeOnDragonBallZ();

    

    Serial.print("Boot Value is: ");
    int bootVal = get_Wifi_eeprom();
    Serial.println(bootVal);


    int rebootReason = rtc_get_reset_reason(0);
    Serial.print("Most recent reboot reason: ");
    Serial.println(rebootReason);

    if(rebootReason == 15){
        Restore_Normal_Mode();
        RT_Battery_Warning();
    }

    if(bootVal == 0){
        Serial.println("Booting Normal Mode");
        RT_Disp_init();
        RT_Button_init();
        RT_Led_init();
        RT_Buzzer_init();

        Backlight_on();
        PowerLed_on();
        RT_Button_Interrupt_init();

        //Welcome_Buzz();
        Buzz_Spin();
        RT_Disp_Splash();

        RT_Sprite_init();
        
        if(get_Story_Location() == 0){
            RT_Conversation(-1);
            set_Story_Location(1);
            blackScreen();
        }

        if(get_Boot_Presses() >= 3)
          dont();

        RT_Menu();
       // RT_Conversation();
        //Pedagogy();
    }
    else if(bootVal == 1){
        RT_Wifi_only_init();
    }
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

blackScreen();
if(getNeedMenu()){
  setNeedMenu(false);
  RT_Menu();
}


}


