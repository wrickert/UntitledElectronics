#include <stdio.h>
#include <Arduino.h>
#include <TFT_eSPI.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_system.h"


#define POWER_RED_PIN GPIO_NUM_34
#define POWER_GREEN_PIN GPIO_NUM_35
#define POWER_BLUE_PIN GPIO_NUM_33

#define WIFI_RED_PIN GPIO_NUM_38
#define WIFI_GREEN_PIN GPIO_NUM_36
#define WIFI_BLUE_PIN GPIO_NUM_37

void hello_task(void *pvParameter)
{
	while(1)
	{
	    printf("Hello world!\n");
	    vTaskDelay(100);
	}
}

void power_toggle_task(void *pvParameters) {
    gpio_reset_pin(POWER_RED_PIN);
    gpio_reset_pin(POWER_BLUE_PIN);
    gpio_reset_pin(POWER_GREEN_PIN);

    gpio_set_direction(POWER_RED_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(POWER_BLUE_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(POWER_GREEN_PIN, GPIO_MODE_OUTPUT);

    while(1) {
        gpio_set_level(POWER_RED_PIN, 1);   // Set pin high
        gpio_set_level(POWER_BLUE_PIN, 0);   // Set pin high
        gpio_set_level(POWER_GREEN_PIN, 0);   // Set pin high
        vTaskDelay(pdMS_TO_TICKS(1000)); // Delay for 1 second
        gpio_set_level(POWER_RED_PIN, 0);   // Set pin high
        gpio_set_level(POWER_BLUE_PIN, 1);   // Set pin high
        gpio_set_level(POWER_GREEN_PIN, 0);   // Set pin high
        vTaskDelay(pdMS_TO_TICKS(1000)); // Delay for 1 second
        gpio_set_level(POWER_RED_PIN, 0);   // Set pin high
        gpio_set_level(POWER_BLUE_PIN, 0);   // Set pin high
        gpio_set_level(POWER_GREEN_PIN, 1);   // Set pin high
        vTaskDelay(pdMS_TO_TICKS(1000)); // Delay for 1 second
    }
}

void wifi_toggle_task(void *pvParameters) {
    gpio_reset_pin(WIFI_RED_PIN);
    gpio_reset_pin(WIFI_BLUE_PIN);
    gpio_reset_pin(WIFI_GREEN_PIN);

    gpio_set_direction(WIFI_RED_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(WIFI_BLUE_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(WIFI_GREEN_PIN, GPIO_MODE_OUTPUT);

    while(1) {
        gpio_set_level(WIFI_RED_PIN, 0);   // Set pin high
        gpio_set_level(WIFI_BLUE_PIN, 0);   // Set pin high
        gpio_set_level(WIFI_GREEN_PIN, 0);   // Set pin high
        vTaskDelay(pdMS_TO_TICKS(1000)); // Delay for 1 second
        gpio_set_level(WIFI_RED_PIN, 0);   // Set pin high
        gpio_set_level(WIFI_BLUE_PIN, 1);   // Set pin high
        gpio_set_level(WIFI_GREEN_PIN, 0);   // Set pin high
        vTaskDelay(pdMS_TO_TICKS(1000)); // Delay for 1 second
        gpio_set_level(WIFI_RED_PIN, 0);   // Set pin high
        gpio_set_level(WIFI_BLUE_PIN, 0);   // Set pin high
        gpio_set_level(WIFI_GREEN_PIN, 1);   // Set pin high
        vTaskDelay(pdMS_TO_TICKS(1000)); // Delay for 1 second
    }
}

void app_main() {
    xTaskCreate(&hello_task, "hello_task", 2048, NULL, 5, NULL);
    xTaskCreate(&power_toggle_task, "toggle_task", 2048, NULL, 5, NULL);
    xTaskCreate(&wifi_toggle_task, "toggle_task", 2048, NULL, 5, NULL);
}


//I dont think the code ever gets here
//void app_main() {}