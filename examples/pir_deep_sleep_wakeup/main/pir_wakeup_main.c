/* 
PIR example + LED
*/
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include "sdkconfig.h"
#include "soc/soc_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_sleep.h"
#include "esp_log.h"
#include <esp_timer.h>
#include "driver/rtc_io.h"
#include "driver/gpio.h"

#if SOC_RTC_FAST_MEM_SUPPORTED
static RTC_DATA_ATTR struct timeval sleep_enter_time;
#else
static struct timeval sleep_enter_time;
#endif

static const char *TAG_LED = "LED";
static const char *TAG_PIR = "PIR";
static const char *TAG_BOOT = "BOOT";


#define BLINK_GPIO CONFIG_BLINK_GPIO
#define PIR_GPIO CONFIG_PIR_GPIO

#if CONFIG_EXAMPLE_EXT0_WAKEUP
#if CONFIG_IDF_TARGET_ESP32
const int ext_wakeup_pin_0 = 25;
#else
const int ext_wakeup_pin_0 = 3;
#endif

void example_deep_sleep_register_ext0_wakeup(void)
{
    printf("Enabling EXT0 wakeup on pin GPIO%d\n", ext_wakeup_pin_0);
    ESP_ERROR_CHECK(esp_sleep_enable_ext0_wakeup(ext_wakeup_pin_0, 1));

    // Configure pullup/downs via RTCIO to tie wakeup pins to inactive level during deepsleep.
    // EXT0 resides in the same power domain (RTC_PERIPH) as the RTC IO pullup/downs.
    // No need to keep that power domain explicitly, unlike EXT1.
    ESP_ERROR_CHECK(rtc_gpio_pullup_dis(ext_wakeup_pin_0));
    ESP_ERROR_CHECK(rtc_gpio_pulldown_en(ext_wakeup_pin_0));
}
#endif // CONFIG_EXAMPLE_EXT0_WAKEUP

int val = 0;
bool motionState = false; //Start with no motion is detection
int MAX_ON_TIME = 10000000; //10 sec
RTC_DATA_ATTR int bootCount = 0;

#ifdef CONFIG_BLINK_LED_RMT


#elif CONFIG_BLINK_LED_GPIO

/*
Method to print the reason by which ESP32
has been awaken from sleep
*/
void print_wakeup_reason(){
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch(wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 : printf("Wakeup caused by external signal using EXT0"); break;
    case ESP_SLEEP_WAKEUP_EXT1 : printf("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER : printf("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : printf("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP : printf("Wakeup caused by ULP program"); break;
    default : printf("Wakeup was not caused by deep sleep: %d\n",wakeup_reason); break;
  }
}

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
    int64_t start_time = esp_timer_get_time();
    int64_t duration = 0, end_time = 0;

    vTaskDelay(pdMS_TO_TICKS(1000)); // wait for 1000 ms

    //Increment boot number and print it every reboot
    ++bootCount;
    // ESP_LOGI(TAG_BOOT, "Boot Number: " + char*(bootCount));

    //Print the wakeup reason for ESP32
    print_wakeup_reason();

    configure_pins();
    while(duration <= MAX_ON_TIME) {
        blink_led();

        // Get current ON time
        end_time = esp_timer_get_time();
        duration = end_time - start_time; // Time in microseconds

        // ESP_LOGI(TAG_BOOT, "Total execution time: " + char*(duration) + " microseconds\n");

        vTaskDelay(pdMS_TO_TICKS(1)); //Delay for 1ms
    }

    #if CONFIG_EXAMPLE_EXT0_WAKEUP
        /* Enable wakeup from deep sleep by ext0 */
        example_deep_sleep_register_ext0_wakeup();
    #endif

    ESP_LOGI(TAG_BOOT, "Going to sleep now");
    esp_deep_sleep_start();
    ESP_LOGI(TAG_BOOT, "This will never be printed!");
}

void app_main(void)
{
    xTaskCreate((TaskFunction_t)&main_loop_task, "main_pir_loop", 4 * 1024, NULL, tskIDLE_PRIORITY, NULL);
    vTaskDelete(NULL);
}
