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

#ifndef TENSORFLOW_LITE_MICRO_EXAMPLES_PERSON_DETECTION_MAIN_FUNCTIONS_H_
#define TENSORFLOW_LITE_MICRO_EXAMPLES_PERSON_DETECTION_MAIN_FUNCTIONS_H_

// Uncomment this line to enable PRODUCTION mode
// In default mode, PD is run on a loop with no deep sleep enabled
// #define PRODUCTION

// Uncomment this line to enable PRODUCTION_V2 mode
// This version uses two XIAOs to complete the detection hierarchy
// #define PRODUCTION_V2

// Expose a C friendly interface for main functions.
#ifdef __cplusplus
extern "C" {
#endif

// Initializes all data needed for the example. The name is important, and needs
// to be setup() for Arduino compatibility.
void setup();

// Initializes the camera and captures one frame of image
void cam_capture_frame(void *pvParameters);

// Runs one iteration of data gathering and inference. This should be called
// repeatedly from the application code. The name needs to be loop() for Arduino
// compatibility.
void loop();

// Runs during PRODUCTION mode to enable deep sleep with PIR sensor wakeup.
void deep_sleep_start();
void deep_sleep_start_with_wake_stub();

// Runs during PRODUCTION mode to display information when waking up from deep sleep.
// Performs PIR initialization on intial boot.
void wakeup();

#ifdef __cplusplus
}
#endif

#endif  // TENSORFLOW_LITE_MICRO_EXAMPLES_PERSON_DETECTION_MAIN_FUNCTIONS_H_
