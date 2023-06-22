#include <Arduino.h>
#include <TFT_eSPI.h>
#include "SPI.h"
#include "Free_Fonts.h"

TFT_eSPI tft = TFT_eSPI();

unsigned long drawTime = 0;



// put function declarations here:
int myFunction(int, int);
void header(const char*, uint16_t );

void setup() {
  // put your setup code here, to run once:
  int result = myFunction(2, 3);

  tft.begin();

  tft.setRotation(0);

}

void loop() {
  int xpos = 0;
  int ypos = 40;

  const char headerText[] = "Using print() method";
  header(headerText, TFT_NAVY);

  tft.setTextColor(TFT_YELLOW);
  tft.setCursor(xpos, ypos);

    tft.setFreeFont(TT1);     // Select the orginal small TomThumb font
  tft.println();             // Move cursor down a line
  tft.print("The really tiny TomThumb font");    // Print the font name onto the TFT screen
  tft.println();
  tft.println();

  tft.setFreeFont(FSB9);   // Select Free Serif 9 point font, could use:
  // tft.setFreeFont(&FreeSerif9pt7b);
  tft.println();          // Free fonts plot with the baseline (imaginary line the letter A would sit on)
  // as the datum, so we must move the cursor down a line from the 0,0 position
  tft.print("HI Vallen");  // Print the font name onto the TFT screen

  tft.setFreeFont(FSB12);       // Select Free Serif 12 point font
  tft.println();                // Move cursor down a line
  tft.print("I've finally started"); // Print the font name onto the TFT screen

  tft.setFreeFont(FSB18);       // Select Free Serif 12 point font
  tft.println();                // Move cursor down a line
  tft.print("Getting display to work"); // Print the font name onto the TFT screen

  tft.setFreeFont(FSB24);       // Select Free Serif 24 point font
  tft.println();                // Move cursor down a line
  tft.print("WAHOO"); // Print the font name onto the TFT screen


  delay(4000);

  
}

// put function definitions here:
int myFunction(int x, int y) {
  return x + y;
}

// Print the header for a display screen
void header(const char *string, uint16_t color)
{
  tft.fillScreen(color);
  tft.setTextSize(1);
  tft.setTextColor(TFT_MAGENTA, TFT_BLUE);
  tft.fillRect(0, 0, 320, 30, TFT_BLUE);
  tft.setTextDatum(TC_DATUM);
  tft.drawString(string, 160, 2, 4); // Font 4 for fast drawing with background
}