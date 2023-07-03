#include <Arduino.h>
#include <TFT_eSPI.h>
#include <lvgl.h>
#include "RT_input.h"
#include "RT_disp.h"




#if USE_LV_LOG != 0
/* Serial debugging */
void my_print(lv_log_level_t level, const char * file, uint32_t line, const char * dsc)
{

    Serial.printf("%s@%d->%s\r\n", file, line, dsc);
    Serial.flush();
}
#endif



void setup() {

    Serial.begin(115200); /* prepare for possible serial debug */

    lv_init();

#if USE_LV_LOG != 0
    lv_log_register_print_cb(my_print); /* register print function for debugging */
#endif

    RT_display_init();

    //Initialize the inputs
    RT_input_init();

    /* Okay, this works now lets add our stuff
    lv_obj_t *label = lv_label_create(lv_scr_act(), NULL);
    lv_label_set_text(label, "Hello Arduino! (V7.0.X)");
    lv_obj_align(label, NULL, LV_ALIGN_CENTER, 0, 0);
    */

    lv_obj_t *screen = lv_scr_act(); // Get the active screen

    lv_obj_set_style_local_bg_color(screen, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_BLACK);


}

void loop() {

    lv_task_handler(); /* let the GUI do its work */
    delay(5);
}
