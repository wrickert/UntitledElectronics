//Use this website to decode image files http://www.rinkydinkelectronics.com/t_imageconverter565.php


#include <PNGdec.h>
#include <TFT_eSPI.h>


void RT_Disp_init();
void RT_Disp_Splash();
void pngDraw(PNGDRAW *pDraw);
void RT_Playground_setup();

void Hackio(int x, int y);

void RT_Sprite_init();
void RT_background_refresh(int);
void RT_Conversation(int number);

void setX(int);
void setY(int);
int getX();
int getY();
TFT_eSPI RT_GetTFT();

void text_Box_in(int x, int y, int l, int w, u_int32_t color1, u_int32_t color2);
void text_Box_out(int x, int y, int l, int w, u_int32_t color1, u_int32_t color2);

void blackScreen();
int getColorSchemeOne();
int getColorSchemeTwo();
void RT_Menu();
void RT_Settings();
void RT_About();
void RT_Secret(int number);
void Wifi_Warning();
void Wifi_Results(int n );
void RT_Talking(int stage);
void RT_Convo_Text_Wrap(int s);
void RT_Battery_Warning();
void drawHexagon(int x, int y, int sideLength) ;
void drawHexagonGrid(int startX, int startY, int offsetX, int offsetY, int numColumns, int numRows) ;
void Pedagogy();
void Draw_Room(int RoomNumber);
void dont();
void RT_Party();