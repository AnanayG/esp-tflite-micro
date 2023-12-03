#include "wifi_connection.h"

#include "esp_camera.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "nvs_flash.h"

static const char *WIFI_TAG = "WIFI";
// #include "app_camera_esp.h"

// ===========================
// Enter your WiFi credentials
// ===========================
#define STA_SSID CONFIG_STA_SSID
#define STA_PASS CONFIG_STA_PASSWORD

void initialize_nvs(void) {
    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
}

void wifi_connect(void) {
    initialize_nvs();

    wifi_init_config_t wifi_config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_config));

    wifi_config_t sta_config = {
        .sta = {
            .ssid = STA_SSID,
            .password = STA_PASS
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta_config));
    //WiFi.setSleep(false);
    ESP_ERROR_CHECK(esp_wifi_start()); //start wifi

    ESP_ERROR_CHECK(esp_wifi_connect());
    //while (WiFi.status() != WL_CONNECTED) {
    //    vTaskDelay(pdMS_TO_TICKS(500)); // Delay
    //    ESP_ERROR_CHECK(esp_wifi_connect());
    //    Serial.print(".");
    //}
    vTaskDelay(pdMS_TO_TICKS(5000)); // Delay
    ESP_LOGI(WIFI_TAG, "WiFi connected");

    // startCameraServer();
    // ESP_LOGI(WIFI_TAG, "Camera Ready! Use 'http://%d.%d.%d.%d' to connect" , WiFi.localIP());
    vTaskDelay(pdMS_TO_TICKS(5000)); // Delay
}