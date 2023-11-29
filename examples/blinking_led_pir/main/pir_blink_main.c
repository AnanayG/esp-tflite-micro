/* 
PIR example + LED
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "led_strip.h"
#include "sdkconfig.h"

static const char *TAG_LED = "LED";
static const char *TAG_PIR = "PIR";


#define BLINK_GPIO CONFIG_BLINK_GPIO
#define PIR_GPIO CONFIG_PIR_GPIO

int val = 0;
bool motionState = false; //Start with no motion is detection

#ifdef CONFIG_BLINK_LED_RMT


#elif CONFIG_BLINK_LED_GPIO

static void blink_led(void)
{

    // Read out the pirPin and store as val:
    val = gpio_get_level((gpio_num_t)PIR_GPIO);

    // If motion is detected (pirPin = HIGH), do the following:
    if (val == 1) {
        gpio_set_level(BLINK_GPIO, 0); // Turn on the on-board LED.

        // Change the motion state to true (motion detected):
        if (motionState == false) {
            ESP_LOGI(TAG_PIR, "Motion detected!");
            motionState = true;
        }
    }

    // If no motion is detected (pirPin = LOW), do the following:
    else {
        gpio_set_level(BLINK_GPIO, 1); // Turn off the on-board LED.

        // Change the motion state to false (no motion):
        if (motionState == true) {
            ESP_LOGI(TAG_PIR, "Motion ended!");
            motionState = false;
        }
    }
    //vTaskDelay(pdMS_TO_TICKS(1)); //Delay for 1ms
}

static void configure_pins(void)
{
    ESP_LOGI(TAG_LED, "Example configured to blink GPIO LED!");
    gpio_reset_pin(BLINK_GPIO);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIR_GPIO, GPIO_MODE_INPUT);
}

#endif

void main_loop_task(void) {
    configure_pins();
    while(true) {
        blink_led();
        vTaskDelay(pdMS_TO_TICKS(1)); //Delay for 1ms
    }
}

void app_main(void)
{
    xTaskCreatePinnedToCore((TaskFunction_t)&main_loop_task, "main_pir_loop", 4 * 1024, NULL, tskIDLE_PRIORITY, NULL, 0);
    vTaskDelete(NULL);
}
