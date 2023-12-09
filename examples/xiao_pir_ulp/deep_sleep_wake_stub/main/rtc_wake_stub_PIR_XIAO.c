/*
 * SPDX-FileCopyrightText: 2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include <inttypes.h>
#include "esp_sleep.h"
#include "esp_cpu.h"
#include "esp_rom_sys.h"
#include "esp_wake_stub.h"
#include "driver/rtc_io.h"
#include "sdkconfig.h"

// GPIO macro definition
#define PIR_GPIO     GPIO_NUM_1
#define PD_GPIO      GPIO_NUM_2
#define PD_done_GPIO GPIO_NUM_3

// wake up stub function stored in RTC memory
void wake_stub_PIR_XIAO(void)
{
    #ifdef DEVELOPMENT
    rtc_gpio_init(GPIO_NUM_21);
    rtc_gpio_set_level(GPIO_NUM_21, 0);
    #endif

    // Wakeup triggered by PIR
    if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT0) {
        // Enable power delivery to PD_XIAO
        rtc_gpio_init(PD_GPIO);
        rtc_gpio_set_direction_in_sleep(PD_GPIO, RTC_GPIO_MODE_OUTPUT_ONLY);
        rtc_gpio_set_level(PD_GPIO, 1);
        rtc_gpio_hold_en(PD_GPIO);

        // Enable wakeup on PD_done
        const int ext_wakeup_pin = PD_done_GPIO;
        ESP_ERROR_CHECK(esp_sleep_enable_ext1_wakeup(1ULL << ext_wakeup_pin, ESP_EXT1_WAKEUP_ANY_HIGH));
        ESP_ERROR_CHECK(rtc_gpio_pullup_dis(PD_done_GPIO));
        ESP_ERROR_CHECK(rtc_gpio_pulldown_en(PD_done_GPIO));
    }

    // Wakeup triggered by PD_done
    else if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT1) {
        rtc_gpio_hold_dis(PD_GPIO);
        rtc_gpio_set_level(PD_GPIO, 0);
        rtc_gpio_deinit(PD_GPIO);

        // Enable 5-second timer sleep to debounce PIR
        ESP_ERROR_CHECK(esp_sleep_enable_timer_wakeup(5 * 1000000));
    }

    // Wakeup triggered by timer
    else if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_TIMER) {
        rtc_gpio_init(PIR_GPIO);
        rtc_gpio_set_direction_in_sleep(PIR_GPIO, RTC_GPIO_MODE_INPUT_ONLY);
        ESP_ERROR_CHECK(rtc_gpio_pullup_dis(PIR_GPIO));
        ESP_ERROR_CHECK(rtc_gpio_pulldown_en(PIR_GPIO));

        if (rtc_gpio_get_level(PIR_GPIO) == 1){
            // Enable power delivery to PD_XIAO
            rtc_gpio_init(PD_GPIO);
            rtc_gpio_set_direction_in_sleep(PD_GPIO, RTC_GPIO_MODE_OUTPUT_ONLY);
            rtc_gpio_set_level(PD_GPIO, 1);
            rtc_gpio_hold_en(PD_GPIO);

            // Enable wakeup on PD_done
            const int ext_wakeup_pin = PD_done_GPIO;
            ESP_ERROR_CHECK(esp_sleep_enable_ext1_wakeup(1ULL << ext_wakeup_pin, ESP_EXT1_WAKEUP_ANY_HIGH));
            ESP_ERROR_CHECK(rtc_gpio_pullup_dis(PD_done_GPIO));
            ESP_ERROR_CHECK(rtc_gpio_pulldown_en(PD_done_GPIO));
        }

        // Enable deep sleep on PIR
        else {
            ESP_ERROR_CHECK(esp_sleep_enable_ext0_wakeup(PIR_GPIO, 1));
            ESP_ERROR_CHECK(rtc_gpio_pullup_dis(PIR_GPIO));
            ESP_ERROR_CHECK(rtc_gpio_pulldown_en(PIR_GPIO));
        }

    }

    #ifdef DEVELOPMENT
    rtc_gpio_set_level(GPIO_NUM_21, 1);
    rtc_gpio_deinit(GPIO_NUM_21);
    #endif

    // Return to deep sleep
    ESP_RTC_LOGI("wake stub: going to deep sleep");
    esp_wake_stub_sleep(&wake_stub_PIR_XIAO);
}
