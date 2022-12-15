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

#include "image_provider.h"

#include "model_settings.h"

TfLiteStatus GetImage (tflite::ErrorReporter* error_reporter,
                       int image_index,
                       int image_width,
                       int image_height,
                       int channels,
                       float * image_data)
{
  const size_t image_size = image_width * image_height * channels * sizeof(char);

  TF_LITE_REPORT_ERROR(error_reporter, "Image: %s", CifarClassLabels[image_index]);

  for (size_t i = 0; i < image_size; ++i)
  {
    image_data[i] = 1 / 255.0;
  }

  return kTfLiteOk;
}
