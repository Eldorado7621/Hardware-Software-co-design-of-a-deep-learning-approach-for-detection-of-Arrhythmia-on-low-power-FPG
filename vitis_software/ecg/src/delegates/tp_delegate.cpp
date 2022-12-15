#include "tp_delegate.h"
#include "miscellaneous.h"

TPDelegate::TPDelegate ()
{

}

TPDelegate::~TPDelegate ()
{

}

int TPDelegate::initialize (void)
{
  singleton_tp_array_ = TensorProcessor::instatiateProcessors();

  ASSERT(singleton_tp_array_ != nullptr);
  ASSERT(singleton_tp_array_->tp != nullptr);
  ASSERT(0 < singleton_tp_array_->len);

  return (singleton_tp_array_ != nullptr)
      && (singleton_tp_array_->tp != nullptr)
      && (0 < singleton_tp_array_->len) ?
          XST_SUCCESS : XST_FAILURE;
}

TPDelegate::Job TPDelegate::createJob (const tflite::ConvParams& params,
      const tflite::RuntimeShape& input_shape, const float* input_data,
      const tflite::RuntimeShape& filter_shape, const float* filter_data,
      const tflite::RuntimeShape& bias_shape, const float* bias_data,
      const tflite::RuntimeShape& output_shape, float* output_data,
      Event * parent)
{
  Job job = { 0 };

  int partition_input_tensor_size = input_shape.FlatSize () / singleton_tp_array_->len;
  int partition_output_tensor_size = output_shape.FlatSize () / singleton_tp_array_->len;

  for (int i = 0; i < singleton_tp_array_->len; i++)
  {
    tflite::ConvParams partition_params = params;
    tflite::RuntimeShape partition_input_shape (input_shape);
    tflite::RuntimeShape partition_output_shape (output_shape);

    int correction_shape_h = (1 < singleton_tp_array_->len) ? (i == 0) : 0;

    partition_input_shape.SetDim (1, input_shape.Dims (1) / singleton_tp_array_->len + correction_shape_h);
    partition_output_shape.SetDim (1, output_shape.Dims (1) / singleton_tp_array_->len);

    float * partition_input_data = (float *) input_data + i * partition_input_tensor_size;
    float * partition_output_data = (float *) output_data + i * partition_output_tensor_size;

    job.task_array[i] = singleton_tp_array_->tp[i].createTask (partition_params,
                                                               partition_input_shape,
                                                               partition_input_data,
                                                               filter_shape,
                                                               filter_data,
                                                               bias_shape,
                                                               bias_data,
                                                               partition_output_shape,
                                                               partition_output_data,
                                                               parent);

    TensorProcessor::setExecutionFlags(job.task_array[i],
                                         ProcessingUnit::TX_CACHE_FUSH
                                         | ProcessingUnit::RX_CACHE_FETCH);
  }

  job.task_array_len = singleton_tp_array_->len;

  return job;
}

TPDelegate::Job TPDelegate::createJob (const tflite::ConvParams& params,
       const int32_t* output_multiplier,
       const int32_t* output_shift,
       const tflite::RuntimeShape& input_shape, const int8_t* input_data,
       const tflite::RuntimeShape& filter_shape, const int8_t* filter_data,
       const tflite::RuntimeShape& bias_shape, const int32_t* bias_data,
       const tflite::RuntimeShape& output_shape, int8_t* output_data,
       Event * parent)
{

}

TPDelegate::Job TPDelegate::createJob (const tflite::DepthwiseParams& params,
      const tflite::RuntimeShape& input_shape, const float* input_data,
      const tflite::RuntimeShape& filter_shape, const float* filter_data,
      const tflite::RuntimeShape& bias_shape, const float* bias_data,
      const tflite::RuntimeShape& output_shape, float* output_data,
      Event * parent)
{ /////////////////////////////////////////////////// TODO: Check this operator engine
  Job job = { 0 };

  int partition_input_tensor_size = input_shape.FlatSize () / singleton_tp_array_->len;
  int partition_output_tensor_size = output_shape.FlatSize () / singleton_tp_array_->len;

  for (int i = 0; i < singleton_tp_array_->len; i++)
  {
    tflite::DepthwiseParams partition_params = params;
    tflite::RuntimeShape partition_input_shape (input_shape);
    tflite::RuntimeShape partition_output_shape (output_shape);

    int correction_shape_h = (1 < singleton_tp_array_->len) ? (i == 0) : 0;

    partition_input_shape.SetDim (1, input_shape.Dims (1) / singleton_tp_array_->len + correction_shape_h);
    partition_output_shape.SetDim (1, output_shape.Dims (1) / singleton_tp_array_->len);

    float * partition_input_data = (float *) input_data + i * partition_input_tensor_size;
    float * partition_output_data = (float *) output_data + i * partition_output_tensor_size;

    job.task_array[i] = singleton_tp_array_->tp[i].createTask (partition_params,
                                                               partition_input_shape,
                                                               partition_input_data,
                                                               filter_shape,
                                                               filter_data,
                                                               bias_shape,
                                                               bias_data,
                                                               partition_output_shape,
                                                               partition_output_data,
                                                               parent);

    TensorProcessor::setExecutionFlags(job.task_array[i],
                                         ProcessingUnit::TX_CACHE_FUSH
                                         | ProcessingUnit::RX_CACHE_FETCH);
  }

  job.task_array_len = singleton_tp_array_->len;

  return job;
}

TPDelegate::Job TPDelegate::createJob (const tflite::DepthwiseParams& params,
       const int32_t* output_multiplier,
       const int32_t* output_shift,
       const tflite::RuntimeShape& input_shape, const int8_t* input_data,
       const tflite::RuntimeShape& filter_shape, const int8_t* filter_data,
       const tflite::RuntimeShape& bias_shape, const int32_t* bias_data,
       const tflite::RuntimeShape& output_shape, int8_t* output_data,
       Event * parent)
{

}

bool TPDelegate::isValid (TPDelegate::Job & job)
{
  bool result = (0 < job.task_array_len);
  for (int i = 0; result && (i < job.task_array_len); i++)
  {
    result = TensorProcessor::isValid (job.task_array[i]);
  }
  return result;
}

int TPDelegate::execute (TPDelegate::Job & job)
{
  for (int i = 0; i < singleton_tp_array_->len; i++)
    if (singleton_tp_array_->tp[i].getStatus () != TensorProcessor::IDLE)
      return XST_FAILURE;

  for (int i = 0; i < singleton_tp_array_->len; i++)
    singleton_tp_array_->tp[i].execute (job.task_array[i]);

  for (int i = 0; i < singleton_tp_array_->len; i++)
    while (singleton_tp_array_->tp[i].getStatus() != TensorProcessor::IDLE)
      ;

  return XST_SUCCESS;
}
