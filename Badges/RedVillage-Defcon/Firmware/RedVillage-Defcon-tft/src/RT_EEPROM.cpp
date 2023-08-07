#include <EEPROM.h>
#include <Preferences.h>

#define EEPROM_SIZE 2

Preferences preferences;

void InitalizeEEPROM(){
    Serial.println("EEPROM Initalization done.");
    preferences.begin("my-app", false);
}

void resetAllGameData(){
    preferences.begin("my-app", false);
	preferences.putUInt("wifi",0);


    preferences.putUInt("wifiNum",0);
    preferences.putUInt("StoryLoc",0);
    preferences.putUInt("BootPress",0);
    preferences.putUInt("SirenCount",0);
    preferences.putUInt("RecRoom",0);
    preferences.putUInt("BossList",0);

    Serial.println("All game data reset.");


}

int get_Wifi_eeprom(){
    preferences.begin("my-app", false);
	return preferences.getUInt("wifi",0);
}

// Sets WiFi mode. 0 is normal operation, 1 is WiFi operation
void set_Wifi_eeprom(int value){
    preferences.begin("my-app", false);
	preferences.putUInt("wifi",value);
	Serial.println(preferences.getUInt("wifi"));

	preferences.end();
}

int get_Wifi_number(){
    preferences.begin("my-app", false);
    return preferences.getUInt("wifiNum",0);
}

void set_Wifi_number(int number){
    int current = get_Wifi_number();
    current += number;
    preferences.putUInt("wifiNum",current);
    Serial.print("The current number of seen WiFi is: ");
    Serial.println(current);
}

int get_Story_Location(){
    preferences.begin("my-app", false);
    return preferences.getUInt("StoryLoc",0);
}

//Number in this case is added to the Story Loc. 
void set_Story_Location(int number){
    int current = get_Story_Location();
    current += number;
    preferences.putUInt("StoryLoc",current);
    Serial.print("The current Story Location is: ");
    Serial.println(current);
}

int get_Boot_Presses(){
    preferences.begin("my-app", false);
    return preferences.getUInt("BootPress",0);
}

void set_Boot_Presses(int number){
    int n = preferences.getUInt("BootPress",0);
    preferences.putUInt("BootPress",( number));
}

int get_Siren_Count(){
    preferences.begin("my-app", false);
    return preferences.getUInt("SirenCount",0);
}

void set_Siren_Count(int number){
    int n = preferences.getUInt("SirenCount",0);
    preferences.putUInt("SirenCount",(n + number));
}

int get_Room_Number(){
    preferences.begin("my-app", false);
    return preferences.getUInt("RecRoom",0);
}

//Number in this case is added to the Story Loc. 
void set_Room_Number(int number){
    preferences.putUInt("RecRoom",number);
    Serial.print("Setting room number to: ");
    Serial.println(number);
}

//0 means not defeated yet. 0b000 is inital state
int get_Boss_status(){
    preferences.begin("my-app", false);
    return preferences.getUInt("BossList",0);
}

void set_Boss_status(int number){
    preferences.putUInt("BossList",number);
    Serial.print("Setting boss state to ");
    Serial.println(number);

}

void LastTimeOnDragonBallZ(){
    Serial.println("Welcome Back.");
    Serial.print("Your Story Location is: ");
    Serial.println(get_Story_Location());

    Serial.print("Your current room number is: ");
    Serial.println(get_Room_Number());

    Serial.print("You have found ");
    Serial.print(get_Wifi_number());
    Serial.println(" WiFi Networks");
}