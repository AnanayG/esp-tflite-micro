
#include "esp_event.h"
#include <esp_http_server.h>
#include "esp_camera.h"
#include "image_provider.h"

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

static httpd_handle_t start_webserver(void);
static esp_err_t stop_webserver(httpd_handle_t server);
static void connect_handler(void* arg, esp_event_base_t event_base,
                            int32_t event_id, void* event_data);
static void disconnect_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data);
void start_event_loop(bmpframe_t * trigger_img);

#ifdef __cplusplus
}
#endif