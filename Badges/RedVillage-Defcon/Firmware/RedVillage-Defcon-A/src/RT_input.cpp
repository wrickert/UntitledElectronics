#include "RT_input.h"
#include <lvgl.h>
#include <MD_REncoder.h>

#define wheelUp 16
#define wheelDn 9 
#define wheelIn 10


// Okay... So I dont know how to make the encoder work. Onto the next problem

//TODO Fix encoder
int32_t RotaryCount = 0; //used to track rotary position
MD_REncoder encoderObj = MD_REncoder(wheelUp, wheelDn);

void RT_input_init(){
    /*Initialize the (dummy) input device driver*/
    lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_ENCODER;
    indev_drv.read_cb = read_encoder;
    lv_indev_drv_register(&indev_drv);

    encoderObj.begin();
}

/* Reading input device (simulated encoder here) */
bool read_encoder(lv_indev_drv_t * indev, lv_indev_data_t * data)
{
    rotaryRead();

    static int32_t last_diff = 0;
    int32_t diff = RotaryCount; /* Dummy - no movement */
    int btn_state = LV_INDEV_STATE_REL; /* Dummy - no press */

    data->enc_diff = diff - last_diff;;
    data->state = btn_state;

    last_diff = diff;

    return false;
}

void rotaryRead(){
      uint8_t x = encoderObj.read();
  if (x)
  {
    if (x == DIR_CW ) {
      Serial.print("NEXT ");
      ++RotaryCount;
      Serial.println(RotaryCount);
    }
    else
    {
      Serial.print("PREV ");
      --RotaryCount;
      Serial.println(RotaryCount);
    }
  }
}