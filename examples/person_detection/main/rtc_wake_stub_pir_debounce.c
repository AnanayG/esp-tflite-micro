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
#include "sdkconfig.h"
#include "driver/rtc_io.h"

/*
 * Deep sleep wake stub function is a piece of code that will be loaded into 'RTC Fast Memory'.
 * The first way is to use the RTC_IRAM_ATTR attribute to place a function into RTC memory,
 * The second way is to place the function into any source file whose name starts with rtc_wake_stub.
 * Files names rtc_wake_stub* have their contents automatically put into RTC memory by the linker.
 *
 * First, call esp_set_deep_sleep_wake_stub to set the wake stub function as the RTC stub entry,
 * The wake stub function runs immediately as soon as the chip wakes up - before any normal
 * initialisation, bootloader, or ESP-IDF code has run. After the wake stub runs, the SoC
 * can go back to sleep or continue to start ESP-IDF normally.
 *
 * Wake stub code must be carefully written, there are some rules for wake stub:
 * 1) The wake stub code can only access data loaded in RTC memory.
 * 2) The wake stub code can only call functions implemented in ROM or loaded into RTC Fast Memory.
 * 3) RTC memory must include any read-only data (.rodata) used by the wake stub.
 */

// Configure PIR wakeup pin
#if CONFIG_EXAMPLE_EXT0_WAKEUP
#define PIR_SENSOR_PIN CONFIG_EXAMPLE_EXT0_WAKEUP_PIN
#else
#define PIR_SENSOR_PIN CONFIG_EXAMPLE_EXT1_WAKEUP_PIN
#endif // CONFIG_EXAMPLE_EXT0_WAKEUP

// wake up stub function stored in RTC memory
void wake_stub_PIR_debounce(void)
{
    // esp_default_wake_deep_sleep();
    for (int i = 0; i < 10000; i++){
        ESP_RTC_LOGI("BOOT", "Executing PIR debounce wake stub");
    }

    // Initialize RTC GPIO pin connected to PIR output
    rtc_gpio_init(PIR_SENSOR_PIN);
    rtc_gpio_set_direction(PIR_SENSOR_PIN, RTC_GPIO_MODE_INPUT_ONLY);

    // Check PIR sensor state
    if (rtc_gpio_get_level(PIR_SENSOR_PIN) == 0) {
        //#if CONFIG_EXAMPLE_EXT0_WAKEUP
        // Enable deep sleep EXT0 wakeup (on HIGH)
        ESP_ERROR_CHECK(esp_sleep_enable_ext0_wakeup((gpio_num_t)CONFIG_EXAMPLE_EXT0_WAKEUP_PIN, 1));
        ESP_ERROR_CHECK(rtc_gpio_pullup_dis((gpio_num_t)CONFIG_EXAMPLE_EXT0_WAKEUP_PIN));  // Disable pullup
        ESP_ERROR_CHECK(rtc_gpio_pulldown_en((gpio_num_t)CONFIG_EXAMPLE_EXT0_WAKEUP_PIN)); // Enable  pulldown
        //#endif // CONFIG_EXAMPLE_EXT0_WAKEUP

        #if CONFIG_EXAMPLE_EXT1_WAKEUP
        const int ext_wakeup_pin = CONFIG_EXAMPLE_EXT1_WAKEUP_PIN;
        const uint64_t ext_wakeup_pin_1_mask = 1ULL << ext_wakeup_pin;
        const esp_sleep_ext1_wakeup_mode_t ext_wakeup_mode = (esp_sleep_ext1_wakeup_mode_t)CONFIG_EXAMPLE_EXT1_WAKEUP_MODE;

        ESP_ERROR_CHECK(esp_sleep_enable_ext1_wakeup(ext_wakeup_pin_1_mask, ext_wakeup_mode));

        /* If there are no external pull-up/downs, tie wakeup pins to inactive level with internal pull-up/downs via RTC IO
        * during deepsleep. However, RTC IO relies on the RTC_PERIPH power domain. Keeping this power domain on will
        * increase some power comsumption. However, if we turn off the RTC_PERIPH domain or if certain chips lack the RTC_PERIPH
        * domain, we will use the HOLD feature to maintain the pull-up and pull-down on the pins during sleep.*/
        #if CONFIG_EXAMPLE_EXT1_USE_INTERNAL_PULLUPS
        #if SOC_RTCIO_INPUT_OUTPUT_SUPPORTED
            if (ext_wakeup_mode) {
                ESP_ERROR_CHECK(rtc_gpio_pullup_dis((gpio_num_t)CONFIG_EXAMPLE_EXT1_WAKEUP_MODE));
                ESP_ERROR_CHECK(rtc_gpio_pulldown_en((gpio_num_t)CONFIG_EXAMPLE_EXT1_WAKEUP_MODE));
            } else {
                ESP_ERROR_CHECK(rtc_gpio_pulldown_dis((gpio_num_t)CONFIG_EXAMPLE_EXT1_WAKEUP_MODE));
                ESP_ERROR_CHECK(rtc_gpio_pullup_en((gpio_num_t)CONFIG_EXAMPLE_EXT1_WAKEUP_MODE));
            }
        #else
            if (ext_wakeup_mode) {
                ESP_ERROR_CHECK(gpio_pullup_dis((gpio_num_t)CONFIG_EXAMPLE_EXT1_WAKEUP_MODE));
                ESP_ERROR_CHECK(gpio_pulldown_en((gpio_num_t)CONFIG_EXAMPLE_EXT1_WAKEUP_MODE));
            } else {
                ESP_ERROR_CHECK(gpio_pulldown_dis((gpio_num_t)CONFIG_EXAMPLE_EXT1_WAKEUP_MODE));
                ESP_ERROR_CHECK(gpio_pullup_en((gpio_num_t)CONFIG_EXAMPLE_EXT1_WAKEUP_MODE));
            }
        #endif // SOC_RTCIO_INPUT_OUTPUT_SUPPORTED
        #endif // CONFIG_EXAMPLE_EXT1_USE_INTERNAL_PULLUPS
        #endif // CONFIG_EXAMPLE_EXT0_WAKEUP

        // Enter deep sleep
        ESP_RTC_LOGI("BOOT", "Returning to deep sleep now");
        esp_wake_stub_sleep(&wake_stub_PIR_debounce);
    }
    else {
        // Wakes up the system
        esp_default_wake_deep_sleep();
        return;
    }
}
