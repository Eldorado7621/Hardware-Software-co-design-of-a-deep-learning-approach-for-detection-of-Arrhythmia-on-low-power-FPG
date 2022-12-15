/*
 * tp_delegate.h
 *
 *  Created on: October 14th, 2021
 *      Author: Yarib Nevarez
 */
#ifndef TP_DELEGATE_H_
#define TP_DELEGATE_H_

#include "tensorflow/lite/kernels/internal/common.h"
#include "tensorflow/lite/kernels/internal/types.h"

#include "tensor_processor.h"
#include "conv_hls.h"
#include "event.h"

class TPDelegate
{
public:

  typedef struct
  {
    TensorProcessor::Task task_array[16];
    int task_array_len;
  } Job;

  TPDelegate ();
  ~TPDelegate ();

  virtual int initialize (void);

  virtual Job createJob (const tflite::ConvParams& params,
        const tflite::RuntimeShape& input_shape, const float* input_data,
        const tflite::RuntimeShape& filter_shape, const float* filter_data,
        const tflite::RuntimeShape& bias_shape, const float* bias_data,
        const tflite::RuntimeShape& output_shape, float* output_data,
        Event * parent = nullptr);

  virtual Job createJob (const tflite::ConvParams& params,
         const int32_t* output_multiplier,
         const int32_t* output_shift,
         const tflite::RuntimeShape& input_shape, const int8_t* input_data,
         const tflite::RuntimeShape& filter_shape, const int8_t* filter_data,
         const tflite::RuntimeShape& bias_shape, const int32_t* bias_data,
         const tflite::RuntimeShape& output_shape, int8_t* output_data,
         Event * parent = nullptr);

  virtual Job createJob (const tflite::DepthwiseParams& params,
        const tflite::RuntimeShape& input_shape, const float* input_data,
        const tflite::RuntimeShape& filter_shape, const float* filter_data,
        const tflite::RuntimeShape& bias_shape, const float* bias_data,
        const tflite::RuntimeShape& output_shape, float* output_data,
        Event * parent = nullptr);

  virtual Job createJob (const tflite::DepthwiseParams& params,
         const int32_t* output_multiplier,
         const int32_t* output_shift,
         const tflite::RuntimeShape& input_shape, const int8_t* input_data,
         const tflite::RuntimeShape& filter_shape, const int8_t* filter_data,
         const tflite::RuntimeShape& bias_shape, const int32_t* bias_data,
         const tflite::RuntimeShape& output_shape, int8_t* output_data,
         Event * parent = nullptr);

  static bool isValid (Job &);

  virtual int execute (Job &);

protected:

  TensorProcessor::ProcessorArray * singleton_tp_array_ = nullptr;
};

#endif // TP_DELEGATE_H_
