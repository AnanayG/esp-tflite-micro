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

#ifndef TENSORFLOW_LITE_MICRO_EXAMPLES_PERSON_DETECTION_ESP_APP_CAMERA_ESP_H_
#define TENSORFLOW_LITE_MICRO_EXAMPLES_PERSON_DETECTION_ESP_APP_CAMERA_ESP_H_

#include "sensor.h"
#include "esp_camera.h"
#include "esp_log.h"
#include "esp_system.h"

/**
 * PIXFORMAT_RGB565,    // 2BPP/RGB565
 * PIXFORMAT_YUV422,    // 2BPP/YUV422
 * PIXFORMAT_GRAYSCALE, // 1BPP/GRAYSCALE
 * PIXFORMAT_JPEG,      // JPEG/COMPRESSED
 * PIXFORMAT_RGB888,    // 3BPP/RGB888
 */
#if defined DISPLAY_SUPPORT
#define CAMERA_PIXEL_FORMAT PIXFORMAT_RGB565
#else
#define CAMERA_PIXEL_FORMAT PIXFORMAT_GRAYSCALE
#endif
/*
 * FRAMESIZE_96X96,    // 96x96
 * FRAMESIZE_QQVGA,    // 160x120
 * FRAMESIZE_QQVGA2,   // 128x160
 * FRAMESIZE_QCIF,     // 176x144
 * FRAMESIZE_HQVGA,    // 240x176
 * FRAMESIZE_QVGA,     // 320x240
 * FRAMESIZE_CIF,      // 400x296
 * FRAMESIZE_VGA,      // 640x480
 * FRAMESIZE_SVGA,     // 800x600
 * FRAMESIZE_XGA,      // 1024x768
 * FRAMESIZE_SXGA,     // 1280x1024
 * FRAMESIZE_UXGA,     // 1600x1200
 */
#define CAMERA_FRAME_SIZE FRAMESIZE_96X96

#define CAMERA_MODULE_NAME "XIAO_ESP32S3"
#define CAMERA_PIN_PWDN   -1
#define CAMERA_PIN_RESET  -1
#define CAMERA_PIN_XCLK   10
#define CAMERA_PIN_SIOD   40
#define CAMERA_PIN_SIOC   39

#define CAMERA_PIN_D7     48
#define CAMERA_PIN_D6     11
#define CAMERA_PIN_D5     12
#define CAMERA_PIN_D4     14
#define CAMERA_PIN_D3     16
#define CAMERA_PIN_D2     18
#define CAMERA_PIN_D1     17
#define CAMERA_PIN_D0     15
#define CAMERA_PIN_VSYNC  38
#define CAMERA_PIN_HREF   47
#define CAMERA_PIN_PCLK   13

#define XCLK_FREQ_HZ 10000000 // 20 MHz default; 16 MHz to enable EDMA mode (has extra delay on first capture)

#ifdef __cplusplus
extern "C" {
#endif

int app_camera_init();

#ifdef __cplusplus
}
#endif

#endif  // TENSORFLOW_LITE_MICRO_EXAMPLES_PERSON_DETECTION_ESP_APP_CAMERA_ESP_H_
