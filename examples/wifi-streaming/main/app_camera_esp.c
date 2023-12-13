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

#include "app_camera_esp.h"
//#include "sdkconfig.h"

static const char *TAG = "app_camera";

int app_camera_init() {

#if (CONFIG_TFLITE_USE_BSP)
  bsp_i2c_init();
  camera_config_t config = BSP_CAMERA_DEFAULT_CONFIG;

#else // CONFIG_TFLITE_USE_BSP
  camera_config_t config;
  config.ledc_channel =  LEDC_CHANNEL_0;
  config.ledc_timer =    LEDC_TIMER_0;
  config.pin_d0 =        CAMERA_PIN_D0;
  config.pin_d1 =        CAMERA_PIN_D1;
  config.pin_d2 =        CAMERA_PIN_D2;
  config.pin_d3 =        CAMERA_PIN_D3;
  config.pin_d4 =        CAMERA_PIN_D4;
  config.pin_d5 =        CAMERA_PIN_D5;
  config.pin_d6 =        CAMERA_PIN_D6;
  config.pin_d7 =        CAMERA_PIN_D7;
  config.pin_xclk =      CAMERA_PIN_XCLK;
  config.pin_pclk =      CAMERA_PIN_PCLK;
  config.pin_vsync =     CAMERA_PIN_VSYNC;
  config.pin_href =      CAMERA_PIN_HREF;
  config.pin_sscb_sda =  CAMERA_PIN_SIOD;
  config.pin_sscb_scl =  CAMERA_PIN_SIOC;
  config.pin_pwdn =      CAMERA_PIN_PWDN;
  config.pin_reset =     CAMERA_PIN_RESET;
  config.xclk_freq_hz =  XCLK_FREQ_HZ;
  // config.jpeg_quality =  10; // For streaming
  config.fb_count =      1; // fb = 1 seems to work now but didn't work at one point.
  config.fb_location =   CAMERA_FB_IN_PSRAM;
  config.grab_mode =     CAMERA_GRAB_LATEST; // CAMERA_GRAB_LATEST is needed for the PD to be run on the latest image (with fb_count = 2)
#endif // CONFIG_TFLITE_USE_BSP

  // Pixel format and frame size are specific configurations options for this application.
  // Frame size must be 96x96 pixels to match the trained model.
  // Pixel format defaults to grayscale to match the trained model.
  // With display support enabled, the pixel format is RGB565 to match the display. The frame is converted to grayscale before it is passed to the trained model.
  //config.pixel_format =  CAMERA_PIXEL_FORMAT;
  config.pixel_format =  PIXFORMAT_JPEG;
  config.jpeg_quality =  12;
  config.frame_size =    FRAMESIZE_UXGA;
  //config.frame_size =    FRAMESIZE_240X240;
  //config.frame_size =    CAMERA_FRAME_SIZE; // Used to initialize the frame size

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Camera init failed with error 0x%x", err);
    return -1;
  }

  // Adjust camera settings
  sensor_t *s = esp_camera_sensor_get();
  if (s->id.PID == OV2640_PID){
    s->set_gain_ctrl(s, 1);     // Auto gain on (0 or 1)
    s->set_exposure_ctrl(s, 1); // Auto exposure on (0 or 1)
    s->set_awb_gain(s, 1);      // Auto White Balance enable (0 or 1)
    s->set_brightness(s, 0);    // Set brightness (-2 to 2) 
  }
  else if (s->id.PID == OV3660_PID) {
    s->set_vflip(s, 1);         // Flip sensor (initial sensors are flipped vertically and colors are a bit saturated)
    s->set_brightness(s, 1);    // Up the blightness just a bit
    s->set_saturation(s, -2);   // Lower the saturation
  }

  // Adjust frame size post camera initialization
  // s->set_framesize(s, CAMERA_FRAME_SIZE);

  return 0;
}
