/* Copyright 2019 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#include "main_functions.h"
#include "esp_log.h"
#include "esp_system.h"
#include "webserver.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_main.h"

#if CLI_ONLY_INFERENCE
#include "esp_cli.h"
#endif

void tf_main(void) {
#if CLI_ONLY_INFERENCE
  setup();
  esp_cli_start();
  vTaskDelay(portMAX_DELAY);
#elif defined(PRODUCTION)
  #ifndef PRODUCTION_V2
  wakeup();
  #endif

  // Get task handle for current (main) task
  TaskHandle_t mainTaskHandle = xTaskGetCurrentTaskHandle();

  // Create cam_setup task on core 0 and pass in main task handle for notification
  // Runs slower when assigned to core 1
  xTaskCreatePinnedToCore((TaskFunction_t)&cam_capture_frame, "cam_capture_frame", 4 * 1024, (void *)mainTaskHandle, 10, NULL, 0);
  
  // Create setup task on core 1
  xTaskCreatePinnedToCore((TaskFunction_t)&setup, "setup_task", 4 * 1024, NULL, 10, NULL, 1);

  // Wait for the notification from both tasks
  ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

  loop();

  // Used during development to avoid being unable to flash new program due to deep sleep.
  //vTaskDelay(pdMS_TO_TICKS(5000)); // Stay on for 5 seconds
  
  // Infinite loop to avoid entering deep sleep. 
  //while(true){
  //  vTaskDelay(pdMS_TO_TICKS(1000)); // Delay for 1 second
  //}

  #ifndef PRODUCTION_V2
  deep_sleep_start_with_wake_stub();
  #endif
#else
  setup();
  //xTaskCreatePinnedToCore((TaskFunction_t)&start_event_loop, "wifi_event_loop", 7 * 1024 * 1024, NULL, 10, NULL, 0);
  start_event_loop(); //will not return control
  while (true){
    loop();
    vTaskDelay(pdMS_TO_TICKS(1000)); // Delay for 1 second
  }
#endif
}

extern "C" void app_main() {
  xTaskCreate((TaskFunction_t)&tf_main, "tf_main",  512 * 1024, NULL, 8, NULL);
  vTaskDelete(NULL);
}
