set(requires "")
idf_build_get_property(target IDF_TARGET)

if(${target} STREQUAL "linux")
    list(APPEND requires esp_stubs esp-tls esp_http_server protocol_examples_common nvs_flash esp32-camera esp_bsp_generic esp_timer)
endif()
idf_component_register(
            SRCS
                "main.c"
                "app_camera_esp.c"
            INCLUDE_DIRS "."
            REQUIRES ${requires})
