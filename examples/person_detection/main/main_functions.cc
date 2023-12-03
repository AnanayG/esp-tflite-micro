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

#include "detection_responder.h"
#include "image_provider.h"
#include "model_settings.h"
#include "person_detect_model_data.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_log.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/schema/schema_generated.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <esp_heap_caps.h>
#include <esp_timer.h>
#include <esp_log.h>
#include "esp_main.h"

// For measuring execution time
#include <chrono>
using namespace std::chrono;

// For displaying PD status via built-in LED
#include "driver/gpio.h"

// For deep sleep
#include "esp_sleep.h"
#include "esp_wake_stub.h"
#include "driver/rtc_io.h"

// Globals, used for compatibility with Arduino-style sketches.
namespace {
  // For deep sleep
  static RTC_DATA_ATTR struct timeval sleep_enter_time;
  static RTC_DATA_ATTR int boot_count = 0;
  static RTC_RODATA_ATTR const int max_boot_count = 20;

  const tflite::Model* model = nullptr;
  tflite::MicroInterpreter* interpreter = nullptr;
  TfLiteTensor* input = nullptr;

  // In order to use optimized tensorflow lite kernels, a signed int8_t quantized
  // model is preferred over the legacy unsigned model format. This means that
  // throughout this project, input images must be converted from unisgned to
  // signed format. The easiest and quickest way to convert from unsigned to
  // signed 8-bit integers is to subtract 128 from the unsigned value to get a
  // signed value.

  #ifdef CONFIG_IDF_TARGET_ESP32S3
  constexpr int scratchBufSize = 39 * 1024;
  #else
  constexpr int scratchBufSize = 0;
  #endif
  // An area of memory to use for input, output, and intermediate arrays.
  constexpr int kTensorArenaSize = 81 * 1024 + scratchBufSize;
  static uint8_t *tensor_arena;//[kTensorArenaSize]; // Maybe we should move this to external
}  // namespace

// The name of this function is important for Arduino compatibility.
void setup() { 
  // Set up on-board LED for displaying PD status
  gpio_reset_pin(LED_BUILTIN_GPIO);
  gpio_set_direction(LED_BUILTIN_GPIO, GPIO_MODE_OUTPUT);
  gpio_set_level(LED_BUILTIN_GPIO, 1);

  // Map the model into a usable data structure. This doesn't involve any
  // copying or parsing, it's a very lightweight operation.
  model = tflite::GetModel(g_person_detect_model_data);
  if (model->version() != TFLITE_SCHEMA_VERSION) {
    MicroPrintf("Model provided is schema version %d not equal to supported "
                "version %d.", model->version(), TFLITE_SCHEMA_VERSION);
    return;
  }

  if (tensor_arena == NULL) {
    tensor_arena = (uint8_t *) heap_caps_malloc(kTensorArenaSize, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  }
  if (tensor_arena == NULL) {
    printf("Couldn't allocate memory of %d bytes\n", kTensorArenaSize);
    return;
  }

  // Pull in only the operation implementations we need.
  // This relies on a complete list of all the ops needed by this graph.
  // An easier approach is to just use the AllOpsResolver, but this will
  // incur some penalty in code space for op implementations that are not
  // needed by this graph.
  //
  // tflite::AllOpsResolver resolver;
  // NOLINTNEXTLINE(runtime-global-variables)
  static tflite::MicroMutableOpResolver<5> micro_op_resolver;
  micro_op_resolver.AddAveragePool2D();
  micro_op_resolver.AddConv2D();
  micro_op_resolver.AddDepthwiseConv2D();
  micro_op_resolver.AddReshape();
  micro_op_resolver.AddSoftmax();

  // Build an interpreter to run the model with.
  // NOLINTNEXTLINE(runtime-global-variables)
  static tflite::MicroInterpreter static_interpreter(
      model, micro_op_resolver, tensor_arena, kTensorArenaSize);
  interpreter = &static_interpreter;

  // Allocate memory from the tensor_arena for the model's tensors.
  TfLiteStatus allocate_status = interpreter->AllocateTensors();
  if (allocate_status != kTfLiteOk) {
    MicroPrintf("AllocateTensors() failed");
    return;
  }

  // Get information about the memory area to use for the model's input.
  input = interpreter->input(0);

  #ifdef PRODUCTION
  ESP_LOGI("setup", "Done");
  vTaskDelete(NULL);
  #else
  #ifndef CLI_ONLY_INFERENCE
  // Initialize Camera
  TfLiteStatus init_status = InitCamera();
  if (init_status != kTfLiteOk) {
    MicroPrintf("InitCamera failed\n");
    return;
  }
  #endif
  #endif
}

void cam_capture_frame(void *pvParameters){
  #ifndef CLI_ONLY_INFERENCE
  // Initialize Camera
  TfLiteStatus init_status = InitCamera();
  if (init_status != kTfLiteOk) {
    MicroPrintf("InitCamera failed\n");
    return;
  }
  #endif
  
  // Get image from provider.
  if (kTfLiteOk != GetImage(kNumCols, kNumRows, kNumChannels, input->data.int8)) {
    MicroPrintf("Image capture failed.");
  }

  ESP_LOGI("cam_capture_frame", "Done");
  xTaskNotifyGive((TaskHandle_t)pvParameters); // Notify main task
  vTaskDelete(NULL);
}


#ifndef CLI_ONLY_INFERENCE
// The name of this function is important for Arduino compatibility.
void loop() {
  // Measure PD_total time
  int64_t start_time_PD = esp_timer_get_time();

  #ifndef PRODUCTION
   // Get image from provider.
  if (kTfLiteOk != GetImage(kNumCols, kNumRows, kNumChannels, input->data.int8)) {
    MicroPrintf("Image capture failed.");
  }
  #endif

  // Run the model on this input and make sure it succeeds.
  if (kTfLiteOk != interpreter->Invoke()) {
    MicroPrintf("Invoke failed.");
  }

  TfLiteTensor* output = interpreter->output(0);

  // Process the inference results.
  int8_t person_score = output->data.uint8[kPersonIndex];
  int8_t no_person_score = output->data.uint8[kNotAPersonIndex];

  float person_score_f = (person_score - output->params.zero_point) * output->params.scale;
  float no_person_score_f = (no_person_score - output->params.zero_point) * output->params.scale;

  // Respond to detection
  RespondToDetection(person_score_f, no_person_score_f);

  // Measure PD_total time
  int64_t end_time_PD = esp_timer_get_time();
  MicroPrintf("Total PD time taken: %lldms\n", end_time_PD - start_time_PD); // Time in microseconds
  
  // vTaskDelay(1); // to avoid watchdog trigger when running in an infinite loop
}
#endif

#if defined(COLLECT_CPU_STATS)
  long long total_time = 0;
  long long start_time = 0;
  extern long long softmax_total_time;
  extern long long dc_total_time;
  extern long long conv_total_time;
  extern long long fc_total_time;
  extern long long pooling_total_time;
  extern long long add_total_time;
  extern long long mul_total_time;
#endif

void run_inference(void *ptr) {
  /* Convert from uint8 picture data to int8 */
  for (int i = 0; i < kNumCols * kNumRows; i++) {
    input->data.int8[i] = ((uint8_t *) ptr)[i] ^ 0x80;
  }

#if defined(COLLECT_CPU_STATS)
  long long start_time = esp_timer_get_time();
#endif
  // Run the model on this input and make sure it succeeds.
  if (kTfLiteOk != interpreter->Invoke()) {
    MicroPrintf("Invoke failed.");
  }

#if defined(COLLECT_CPU_STATS)
  long long total_time = (esp_timer_get_time() - start_time);
  printf("Total time = %lld\n", total_time / 1000);
  //printf("Softmax time = %lld\n", softmax_total_time / 1000);
  printf("FC time = %lld\n", fc_total_time / 1000);
  printf("DC time = %lld\n", dc_total_time / 1000);
  printf("conv time = %lld\n", conv_total_time / 1000);
  printf("Pooling time = %lld\n", pooling_total_time / 1000);
  printf("add time = %lld\n", add_total_time / 1000);
  printf("mul time = %lld\n", mul_total_time / 1000);

  /* Reset times */
  total_time = 0;
  //softmax_total_time = 0;
  dc_total_time = 0;
  conv_total_time = 0;
  fc_total_time = 0;
  pooling_total_time = 0;
  add_total_time = 0;
  mul_total_time = 0;
#endif

  TfLiteTensor* output = interpreter->output(0);

  // Process the inference results.
  int8_t person_score = output->data.uint8[kPersonIndex];
  int8_t no_person_score = output->data.uint8[kNotAPersonIndex];

  float person_score_f =
      (person_score - output->params.zero_point) * output->params.scale;
  float no_person_score_f =
      (no_person_score - output->params.zero_point) * output->params.scale;
  RespondToDetection(person_score_f, no_person_score_f);
}



void wakeup(){
  /*if (boot_count == 0) {
    ESP_LOGI("BOOT", "Fresh boot, waiting for PIR sensor to initialize...");
    vTaskDelay(pdMS_TO_TICKS(10000)); // Wait for 10 seconds for PIR sensor to stabilize
    boot_count++;
    deep_sleep_start();
  } else {*/
    boot_count++;
    
    struct timeval now;
    gettimeofday(&now, NULL);
    int sleep_time_s = now.tv_sec - sleep_enter_time.tv_sec;
    ESP_LOGI("BOOT", "Waken up from deep sleep. Time spent in deep sleep: %dms", sleep_time_s);

    //gpio_dump_io_configuration() // Does not work
  //}
}



void RTC_IRAM_ATTR wake_stub_block(){
  // Increment the counter.
  boot_count++;

  // Print the counter value and wakeup cause.
  ESP_RTC_LOGI("Wakeup stub: wakeup boot count is %d, blocking PIR wakeup", boot_count);

  // boot_count is < max_boot_count, go back to deep sleep.
  if (boot_count >= max_boot_count) {
      // Reset boot_count
      boot_count = 1;

      // Set the default wake stub.
      // There is a default version of this function provided in esp-idf.
      esp_default_wake_deep_sleep();

      // Return from the wake stub function to continue
      // booting the firmware.
      return;
  }

  // Print status.
  ESP_RTC_LOGI("Wakeup stub: returning to deep sleep");

  // Set stub entry, then going to deep sleep again.
  esp_wake_stub_sleep(&wake_stub_block);
}



void deep_sleep_start(void)
{
  // Isolate all pins to Sense board, likely already done by default
  gpio_num_t sense_gpio_pins[] = {
    // SD card
    GPIO_NUM_3, 
    GPIO_NUM_7, 
    GPIO_NUM_8, 
    GPIO_NUM_9,
    // Camera
    //GPIO_NUM_10, // XMCLK pin, causes camera init to fail after wakeup
    GPIO_NUM_11, 
    GPIO_NUM_12, 
    GPIO_NUM_13,
    GPIO_NUM_14, 
    GPIO_NUM_15, 
    GPIO_NUM_16, 
    GPIO_NUM_17,
    GPIO_NUM_18
  };

  int num_pins = sizeof(sense_gpio_pins) / sizeof(sense_gpio_pins[0]);
  for (int i = 0; i < num_pins; ++i) {
    rtc_gpio_isolate(sense_gpio_pins[i]);
  }

  #if CONFIG_EXAMPLE_EXT0_WAKEUP
  // Enable deep sleep EXT0 wakeup (on HIGH)
  ESP_ERROR_CHECK(esp_sleep_enable_ext0_wakeup((gpio_num_t)CONFIG_EXAMPLE_EXT0_WAKEUP_PIN, 1));
  ESP_ERROR_CHECK(rtc_gpio_pullup_dis((gpio_num_t)CONFIG_EXAMPLE_EXT0_WAKEUP_PIN));  // Disable pullup
  ESP_ERROR_CHECK(rtc_gpio_pulldown_en((gpio_num_t)CONFIG_EXAMPLE_EXT0_WAKEUP_PIN)); // Enable  pulldown
  #endif // CONFIG_EXAMPLE_EXT0_WAKEUP

  #if CONFIG_EXAMPLE_EXT1_WAKEUP
  const int ext_wakeup_pin = CONFIG_EXAMPLE_EXT1_WAKEUP_PIN;
  const uint64_t ext_wakeup_pin_1_mask = 1ULL << ext_wakeup_pin;
  const esp_sleep_ext1_wakeup_mode_t ext_wakeup_mode = (esp_sleep_ext1_wakeup_mode_t)CONFIG_EXAMPLE_EXT1_WAKEUP_MODE;

  ESP_ERROR_CHECK(esp_sleep_enable_ext1_wakeup(ext_wakeup_pin_1_mask, ext_wakeup_mode));

  //ESP_ERROR_CHECK(rtc_gpio_hold_en(ext_wakeup_pin));
  //ESP_ERROR_CHECK(esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF));

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

  // Enable wakeup stub
  esp_set_deep_sleep_wake_stub(&wake_stub_block);
  
  // Enter deep sleep
  gettimeofday(&sleep_enter_time, NULL); // Get deep sleep enter time
  ESP_LOGI("BOOT", "Entering deep sleep now");
  esp_deep_sleep_start();
}