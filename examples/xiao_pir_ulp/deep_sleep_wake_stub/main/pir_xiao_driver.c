/*
 * SPDX-FileCopyrightText: 2020-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_sleep.h"
#include "esp_wake_stub.h"
#include "driver/gpio.h"
#include "driver/rtc_io.h"

// In RTC RAM
static RTC_DATA_ATTR int boot_count = 0;

// GPIO macro definition
#define PIR_GPIO     GPIO_NUM_1
#define PD_done_GPIO GPIO_NUM_2
#define PD_GPIO      GPIO_NUM_3

#define DEVELOPMENT



void deep_sleep_PD_done_wakeup(){
    const int ext1_wakeup_pin = PD_done_GPIO;
    esp_sleep_enable_ext1_wakeup(1ULL << ext1_wakeup_pin, ESP_EXT1_WAKEUP_ANY_HIGH);
    rtc_gpio_pullup_dis(PD_done_GPIO);
    rtc_gpio_pulldown_en(PD_done_GPIO);
}

void app_main(void)
{   
    #ifdef DEVELOPMENT
    gpio_reset_pin(GPIO_NUM_21);
    gpio_set_direction(GPIO_NUM_21, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_NUM_21, 0);
    #endif

    if (boot_count == 0){
        ++boot_count;
        ESP_LOGI("BOOT", "Fresh boot");

        // Sleep for 10 seconds for debouncing PIR
        esp_sleep_enable_timer_wakeup(10 * 1000000);

        // Pull P-MOSFET gate pin to HIGH
        rtc_gpio_init(PD_GPIO);
        rtc_gpio_set_direction(PD_GPIO, RTC_GPIO_MODE_OUTPUT_ONLY);
        rtc_gpio_set_level(PD_GPIO, 1);

        esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
        ESP_LOGI("BOOT", "Entering deep sleep now, wakeup in 10 seconds to initialize PIR");
    }
    else{
        // Wakeup triggered by PIR
        if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT0) {
            // Enable power delivery to PD_XIAO
            rtc_gpio_init(PD_GPIO);
            rtc_gpio_set_direction(PD_GPIO, RTC_GPIO_MODE_INPUT_ONLY);
            rtc_gpio_pulldown_en(PD_GPIO);
            rtc_gpio_pullup_dis(PD_GPIO);
            //rtc_gpio_set_level(PD_GPIO, 0);

            // Delay needed for GPIO output to stabilize when not using an RC delay circuit
            //vTaskDelay(pdMS_TO_TICKS(1000));

            deep_sleep_PD_done_wakeup();
            ESP_LOGI("BOOT", "Entering deep sleep now, wakeup on PD_done");
        }

        // Wakeup triggered by PD_done
        else if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT1) {
            // Disable power delivery to PD_XIAO
            rtc_gpio_init(PD_GPIO);
            rtc_gpio_set_direction(PD_GPIO, RTC_GPIO_MODE_OUTPUT_ONLY);
            rtc_gpio_set_level(PD_GPIO, 1);

            // Enable 5-second timer sleep to debounce PIR
            esp_sleep_enable_timer_wakeup(5 * 1000000);
            esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
            ESP_LOGI("BOOT", "Entering deep sleep now, wakeup in 5 seconds to debounce PIR");
        }

        // Wakeup triggered by timer
        else if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_TIMER) {
            gpio_reset_pin(PIR_GPIO);
            gpio_set_direction(PIR_GPIO, GPIO_MODE_INPUT);
            gpio_pullup_dis(PIR_GPIO);
            gpio_pulldown_en(PIR_GPIO);

            if (gpio_get_level(PIR_GPIO) == 1){
                // Enable power delivery to PD_XIAO
                rtc_gpio_init(PD_GPIO);
                //rtc_gpio_set_direction(PD_GPIO, RTC_GPIO_MODE_OUTPUT_ONLY);
                //rtc_gpio_set_level(PD_GPIO, 0);
                rtc_gpio_set_direction(PD_GPIO, RTC_GPIO_MODE_INPUT_ONLY);
                rtc_gpio_pulldown_en(PD_GPIO);
                rtc_gpio_pullup_dis(PD_GPIO);
                
                // Delay needed for GPIO output to stabilize when not using an RC delay circuit
                //vTaskDelay(pdMS_TO_TICKS(1000));

                deep_sleep_PD_done_wakeup();
                ESP_LOGI("BOOT", "Entering deep sleep now, wakeup on PD_done");
            }

            // Enable deep sleep on PIR
            else {
                rtc_gpio_init(PD_GPIO);
                rtc_gpio_set_direction(PD_GPIO, RTC_GPIO_MODE_OUTPUT_ONLY);
                rtc_gpio_set_level(PD_GPIO, 1);

                esp_sleep_enable_ext0_wakeup(PIR_GPIO, 1);
                rtc_gpio_pullup_dis(PIR_GPIO);
                rtc_gpio_pulldown_en(PIR_GPIO);
                ESP_LOGI("BOOT", "Entering deep sleep now, wakeup on PIR");
            }
        }
    }

    #ifdef DEVELOPMENT
    //vTaskDelay(pdMS_TO_TICKS(3000));
    gpio_set_level(GPIO_NUM_21, 1);
    ESP_LOGI("BOOT", "Final message before deep sleep!");
    #endif

    esp_deep_sleep_start();
}
