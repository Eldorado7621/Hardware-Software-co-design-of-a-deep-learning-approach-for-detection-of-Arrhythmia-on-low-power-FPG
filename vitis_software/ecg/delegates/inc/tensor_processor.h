/*
 * tensor_processor.h
 *
 *  Created on: July 31st, 2021
 *      Author: Yarib Nevarez
 */
#ifndef TENSOR_PROCESSOR_H_
#define TENSOR_PROCESSOR_H_

#include "tensorflow/lite/kernels/internal/common.h"
#include "tensorflow/lite/kernels/internal/types.h"

#include "processing_unit.h"
#include "conv_hls.h"
#include "event.h"

class TensorProcessor: protected ProcessingUnit
{
public:

  typedef struct
  {
    TensorProcessor * tp;
    int               len;
  } ProcessorArray;

  typedef enum
  {
    IDLE,
    CONFIGURATION,
    EXECUTION
  } Status;

  typedef struct
  {
    ProcessingUnit::Transaction setup;
    ProcessingUnit::Transaction compute;
    Event * delegate_event;
    Event * hardware_event;
  } Task;

  TensorProcessor ();
  ~TensorProcessor ();

  virtual int initialize (ProcessingUnit::Profile * profile);

  virtual Task createTask (const tflite::ConvParams& params,
        const tflite::RuntimeShape& input_shape, const float* input_data,
        const tflite::RuntimeShape& filter_shape, const float* filter_data,
        const tflite::RuntimeShape& bias_shape, const float* bias_data,
        const tflite::RuntimeShape& output_shape, float* output_data,
        Event * parent = nullptr);

  virtual Task createTask (const tflite::ConvParams& params,
         const int32_t* output_multiplier,
         const int32_t* output_shift,
         const tflite::RuntimeShape& input_shape, const int8_t* input_data,
         const tflite::RuntimeShape& filter_shape, const int8_t* filter_data,
         const tflite::RuntimeShape& bias_shape, const int32_t* bias_data,
         const tflite::RuntimeShape& output_shape, int8_t* output_data,
         Event * parent = nullptr);

  virtual Task createTask (const tflite::DepthwiseParams& params,
        const tflite::RuntimeShape& input_shape, const float* input_data,
        const tflite::RuntimeShape& filter_shape, const float* filter_data,
        const tflite::RuntimeShape& bias_shape, const float* bias_data,
        const tflite::RuntimeShape& output_shape, float* output_data,
        Event * parent = nullptr);

  virtual Task createTask (const tflite::DepthwiseParams& params,
         const int32_t* output_multiplier,
         const int32_t* output_shift,
         const tflite::RuntimeShape& input_shape, const int8_t* input_data,
         const tflite::RuntimeShape& filter_shape, const int8_t* filter_data,
         const tflite::RuntimeShape& bias_shape, const int32_t* bias_data,
         const tflite::RuntimeShape& output_shape, int8_t* output_data,
         Event * parent = nullptr);

  static void setExecutionFlags (Task &, int);

  static int getExecutionFlags (Task &);

  static bool isValid (Task &);

  virtual int execute (Task &);

  virtual Status getStatus (void);

  static TensorProcessor::ProcessorArray * instatiateProcessors (void);

  static ProcessorArray * getSingletonTPArray (void);

protected:

  virtual Task createTask (const ConvProfile& profile,
                           const float* input_data,
                           const float* filter_data,
                           const float* bias_data,
                           float* output_data,
                           Event * parent = nullptr);

  virtual Task createTask (const ConvProfile& profile,
                           const int8_t* input_data,
                           const int8_t* filter_data,
                           const int32_t* bias_data,
                           int8_t* output_data,
                           const int32_t* quant_output_multiplier,
                           const int32_t* quant_output_shift,
                           const int32_t  quant_len,
                           Event * parent = nullptr);

  virtual void onDone_ip (void);
  virtual void onDone_dmaTx (void);
  virtual void onDone_dmaRx (void);

  static ProcessorArray singleton_tp_array_;

  Task * current_task_ = nullptr;

  Status status_ = IDLE;
};

#endif // TENSOR_PROCESSOR_H_
