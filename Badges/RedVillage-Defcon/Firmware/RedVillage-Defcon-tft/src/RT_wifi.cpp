#include <WiFi.h>
#include "RT_wifi.h"
#include <TFT_eSPI.h>
#include "RT_Disp.h"
#include "RT_Led.h"
#include "Assets/NotoSansBold15.h"
#include "esp32-hal-cpu.h"

#define AA_FONT_SMALL NotoSansBold15
#define AbtnPin 14

void RT_Wifi_Scan(){

    TFT_eSPI tft = RT_GetTFT();

    tft.fillScreen(TFT_BLACK);

    //Display is 320x240
    tft.fillRectHGradient(10,10,300,220,TFT_RED,TFT_ORANGE);
    tft.loadFont(AA_FONT_SMALL);
    tft.setCursor(20,20);
    tft.println("Preparing to Scan WiFi.");
    tft.setCursor(20,50);
    tft.println("Scanning is battery intensive, the");
    tft.setCursor(20,65);
    tft.println("badge will appear to sleep while ");
    tft.setCursor(20,80);
    tft.println("the local Wifi is scanned.");
    tft.setCursor(70,180);
    tft.println("Press 'A' to continue.");

    //while(digitalRead(AbtnPin) != 0){}
    delay(2000);

    //Low power on the badge and scan for wifi.
    setLightsOut(true);
    PowerLed_on();
    Backlight_off();
    Serial.print("CPU Speed is: ");
    Serial.println(getCpuFrequencyMhz());
    setCpuFrequencyMhz(80);
    Serial.print("Changed to 80Mhz. Verifying: ");
    Serial.println(getCpuFrequencyMhz());
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    Serial.println("Wifi Setup Done.");
    Serial.println("Starting Wifi Scan.");

    WiFi.mode(WIFI_OFF);

    setLightsOut(false);
    Backlight_on();
    PowerLed_on();

    int n = WiFi.scanNetworks();

    Serial.print("Found ");
    Serial.print(n);
    Serial.println(" wifi networks.");

    tft.fillScreen(TFT_BLACK);

    
    tft.fillRect(0,0,340,20,TFT_GREEN);
    tft.setCursor(20,0);
    tft.printf("Found %d WiFi Networks", n);

    for(int i = 0; i <= n; i++){
        //text_Box_in(50, 50, 250, 170, getColorSchemeOne(), getColorSchemeTwo());
        tft.fillRectHGradient(50, 50, 250, 150, getColorSchemeOne(), getColorSchemeTwo());
        tft.setCursor(60,50);
        tft.printf("WiFi %d of %d",i,n);
        tft.setCursor(60,70);
        tft.printf("SSID: %s",WiFi.SSID(i));

        tft.setCursor(60,90);
        tft.print("Encryption Type: ");
        if(WiFi.encryptionType(i) == 2)
            tft.print("TKIP (WPA)");
        else if(WiFi.encryptionType(i) == 5)
            tft.print("WEP");
        else if(WiFi.encryptionType(i) == 4)
            tft.print("CCMP (WPA)");
        else if(WiFi.encryptionType(i) == 7)
            tft.print("NONE!");
        else if(WiFi.encryptionType(i) == 8)
            tft.print("AUTO");
        else{
            tft.printf("Encryption Type returned something weird: %d",WiFi.encryptionType(i));
        }

        //while(digitalRead(AbtnPin) != 0){}
        delay(3000);

        //text_Box_out(50, 50, 250, 170, getColorSchemeOne(), getColorSchemeTwo());
        tft.fillRect(50,50,250,170, TFT_BLACK);
    }
}
