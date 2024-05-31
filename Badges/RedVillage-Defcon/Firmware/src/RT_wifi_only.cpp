#include <TFT_eSPI.h>
#include <WiFi.h>
#include "RT_wifi.h"
#include "Assets/NotoSansBold15.h"



#define MAX_IMAGE_WIDTH 320 // Adjust for your images
#define AA_FONT_SMALL NotoSansBold15


TFT_eSPI tft2 = TFT_eSPI();         // Invoke custom library.

void RT_Wifi_only_init(){
    tft2.begin();
    tft2.setRotation(3);
    tft2.fillScreen(TFT_BLACK);
	
    RT_Wifi_Scan();
}

void Wifi_Warning(){
    tft2.fillScreen(TFT_BLACK);

    //Display is 320x240
    tft2.fillRectHGradient(10,10,300,220,TFT_RED,TFT_ORANGE);
    tft2.loadFont(AA_FONT_SMALL);
    tft2.setCursor(20,20);
    tft2.println("Preparing to Scan WiFi.");
    tft2.setCursor(20,50);
    tft2.println("Scanning is battery intensive, the");
    tft2.setCursor(20,65);
    tft2.println("badge will appear to sleep while ");
    tft2.setCursor(20,80);
    tft2.println("the local Wifi is scanned.");
    //tft.setCursor(70,180);
    //tft.println("Press 'A' to continue.");


    tft2.unloadFont(); // Remove the font to recover memory used

}

void Wifi_Results(int n ){
    Serial.println(WiFi.SSID(1));
    tft2.fillScreen(TFT_BLACK);

    
    tft2.loadFont(AA_FONT_SMALL);
    tft2.fillRect(0,0,340,20,TFT_GREEN);
    tft2.setCursor(20,0);
    tft2.printf("Found %d WiFi Networks", n);

    for(int i = 0; i < n; i++){
        //text_Box_in(50, 50, 250, 170, getColorSchemeOne(), getColorSchemeTwo());
        tft2.fillRectHGradient(50, 50, 250, 150, TFT_RED, TFT_ORANGE);
        tft2.setCursor(60,50);
        tft2.printf("WiFi %d of %d",i,n);
        tft2.setCursor(60,70);
        tft2.printf("SSID: %s",WiFi.SSID(i));

        tft2.setCursor(60,90);
        tft2.print("Encryption Type: ");
        if(WiFi.encryptionType(i) == 2)
            tft2.print("TKIP (WPA)");
        else if(WiFi.encryptionType(i) == 5)
            tft2.print("WEP");
        else if(WiFi.encryptionType(i) == 4)
            tft2.print("CCMP (WPA)");
        else if(WiFi.encryptionType(i) == 7)
            tft2.print("NONE!");
        else if(WiFi.encryptionType(i) == 8)
            tft2.print("AUTO");
        else{
            tft2.printf("Encryption Type returned something weird: %d",WiFi.encryptionType(i));
            tft2.println(WiFi.encryptionType(i));
        }

        //while(digitalRead(AbtnPin) != 0){}
        delay(3000);

        //text_Box_out(50, 50, 250, 170, getColorSchemeOne(), getColorSchemeTwo());
        tft2.fillRect(50,50,250,170, TFT_BLACK);
    }
        tft2.unloadFont();
}
