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
#include "rtc_wake_stub_PIR_XIAO.h"

#ifdef DEVELOPMENT
#include "driver/gpio.h"
#endif

void app_main(void)
{
    ESP_LOGI("BOOT", "Fresh boot");

    #ifdef DEVELOPMENT
    gpio_reset_pin(GPIO_NUM_21);
    gpio_set_direction(GPIO_NUM_21, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_NUM_21, 0);
    vTaskDelay(pdMS_TO_TICKS(3000));
    #endif

    // Sleep for 10 seconds for debouncing PIR
    ESP_ERROR_CHECK(esp_sleep_enable_timer_wakeup(10 * 1000000));
    esp_set_deep_sleep_wake_stub(&wake_stub_PIR_XIAO);
    ESP_LOGI("BOOT", "Entering deep sleep now");

    #ifdef DEVELOPMENT
    gpio_set_level(GPIO_NUM_21, 1);
    #endif

    esp_deep_sleep_start();
}
