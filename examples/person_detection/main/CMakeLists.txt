
#
# Main component of TF Micro project 'person_detection'.
#

idf_component_register(
    SRCS
        "detection_responder.cc"
        "image_provider.cc"
        "main.cc"
        "main_functions.cc"
        "model_settings.cc"
        "person_detect_model_data.cc"
        "app_camera_esp.c"
        "esp_cli.c"
        "rtc_wake_stub_pir_debounce.c"
        "webserver.c"

    PRIV_REQUIRES console spi_flash esp-tls esp_http_server nvs_flash esp_netif esp_wifi protocol_examples_common
    INCLUDE_DIRS "")
