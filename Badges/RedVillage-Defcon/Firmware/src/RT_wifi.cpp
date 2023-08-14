#include <WiFi.h>
#include "RT_wifi.h"
#include <TFT_eSPI.h>
#include "RT_wifi_only.h"
#include "RT_Disp.h"
#include "RT_Led.h"
#include "RT_EEPROM.h"
//#include "Assets/NotoSansBold15.h"
//#include "esp32-hal-cpu.h"

//#define AA_FONT_SMALL NotoSansBold15
#define AbtnPin 14


//EEPROM address 0 Value 1 is boot into Wifi mode
void InitalizeWifi(){
	Serial.println("Going to reboot into WiFi Mode");
	set_Wifi_eeprom(1);
	ESP.restart();
}

void Restore_Normal_Mode(){
	set_Wifi_eeprom(0);
	ESP.restart();
}


void RT_Wifi_Scan(){
    Wifi_Warning();

    //while(digitalRead(AbtnPin) != 0){}
    delay(2000);

    //Low power on the badge and scan for wifi.
    setLightsOut(true);
    PowerLed_on();
    Backlight_off();
    /*
    Serial.print("CPU Speed is: ");
    Serial.println(getCpuFrequencyMhz());
    setCpuFrequencyMhz(80);
    delay(100);
    Serial.print("Changed to 80Mhz. Verifying: ");
    Serial.println(getCpuFrequencyMhz());
    */
    WiFi.mode(WIFI_STA);
    delay(100);
    WiFi.disconnect();

    Serial.println("Wifi Setup Done.");
    Serial.println("Starting Wifi Scan.");

    WiFi.mode(WIFI_OFF);
    delay(100);

    setLightsOut(false);
    Backlight_on();
    PowerLed_on();

    int n = WiFi.scanNetworks();

    Serial.print("Found ");
    Serial.print(n);
    Serial.println(" wifi networks.");


    Wifi_Results(n);
	Restore_Normal_Mode();
}
