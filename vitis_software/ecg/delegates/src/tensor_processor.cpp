/*
 * tensor_processor.cpp
 *
 *  Created on: July 31st, 2021
 *      Author: Yarib Nevarez
 */

#include "tensor_processor.h"
#include "tensorflow/lite/kernels/internal/common.h"

#include "conv_vtbl.h"
#include "dma_vtbl.h"
#include "memory_manager.h"
#include "miscellaneous.h"
#include "custom_float.h"

#include "xparameters.h"

TensorProcessor::ProcessorArray TensorProcessor::singleton_tp_array_ = { 0 };

TensorProcessor::TensorProcessor()
{

}

TensorProcessor::~TensorProcessor()
{

}

int TensorProcessor::initialize(ProcessingUnit::Profile * profile)
{
  return ProcessingUnit::initialize (profile);
}

TensorProcessor::Task TensorProcessor::createTask (const tflite::ConvParams& params,
                             const tflite::RuntimeShape& input_shape,
                             const float* input_data,
                             const tflite::RuntimeShape& filter_shape,
                             const float* filter_data,
                             const tflite::RuntimeShape& bias_shape,
                             const float* bias_data,
                             const tflite::RuntimeShape& output_shape,
                             float* output_data,
                             Event * parent)
{
  ConvProfile conv_profile = { 0 };

  conv_profile.parameters_.stride_.height_ = params.stride_height;
  conv_profile.parameters_.stride_.width_ = params.stride_width;

  conv_profile.parameters_.dilation_.height_ = params.dilation_height_factor;
  conv_profile.parameters_.dilation_.width_ = params.dilation_width_factor;

  conv_profile.parameters_.padding_.height_ = params.padding_values.height;
  conv_profile.parameters_.padding_.width_ = params.padding_values.width;

  conv_profile.parameters_.offset_.input_ = 0;
  conv_profile.parameters_.offset_.output_ = 0;

  conv_profile.parameters_.activation_.max_ = params.float_activation_max;
  conv_profile.parameters_.activation_.min_ = params.float_activation_min;

  conv_profile.parameters_.depth_multiplier_ = 0;
  conv_profile.parameters_.type_ = CONV_2D;

  TFLITE_DCHECK_EQ(input_shape.DimensionsCount (), 4);
  TFLITE_DCHECK_EQ(filter_shape.DimensionsCount (), 4);
  TFLITE_DCHECK_EQ(output_shape.DimensionsCount (), 4);

  for (int i = 0; i < 4; i++)
  {
    conv_profile.input_shape_.dims_[i] = input_shape.Dims (i);
    conv_profile.filter_shape_.dims_[i] = filter_shape.Dims (i);
    conv_profile.output_shape_.dims_[i] = output_shape.Dims (i);
  }

  TFLITE_DCHECK_EQ(bias_shape.DimensionsCount (), 1);
  conv_profile.bias_shape_.dims_[0] = bias_shape.Dims (0);
  conv_profile.bias_shape_.dims_[1] = 1;
  conv_profile.bias_shape_.dims_[2] = 1;
  conv_profile.bias_shape_.dims_[3] = 1;

  return createTask (conv_profile, input_data, filter_data, bias_data, output_data, parent);
}

TensorProcessor::Task TensorProcessor::createTask (const tflite::ConvParams& params,
       const int32_t* output_multiplier,
       const int32_t* output_shift,
       const tflite::RuntimeShape& input_shape, const int8_t* input_data,
       const tflite::RuntimeShape& filter_shape, const int8_t* filter_data,
       const tflite::RuntimeShape& bias_shape, const int32_t* bias_data,
       const tflite::RuntimeShape& output_shape, int8_t* output_data,
       Event * parent)
{
  ConvProfile conv_profile = { 0 };

  conv_profile.parameters_.stride_.height_ = params.stride_height;
  conv_profile.parameters_.stride_.width_ = params.stride_width;

  conv_profile.parameters_.dilation_.height_ = params.dilation_height_factor;
  conv_profile.parameters_.dilation_.width_ = params.dilation_width_factor;

  conv_profile.parameters_.padding_.height_ = params.padding_values.height;
  conv_profile.parameters_.padding_.width_ = params.padding_values.width;

  conv_profile.parameters_.offset_.input_ = params.input_offset;
  conv_profile.parameters_.offset_.filter_ = params.weights_offset;
  conv_profile.parameters_.offset_.output_ = params.output_offset;

  conv_profile.parameters_.activation_.max_ = 0;
  conv_profile.parameters_.activation_.min_ = 0;

  conv_profile.parameters_.quantized_activation_.max_ = params.quantized_activation_max;
  conv_profile.parameters_.quantized_activation_.min_ = params.quantized_activation_min;
  TFLITE_DCHECK_LE(params.quantized_activation_min, params.quantized_activation_max);

  conv_profile.parameters_.depth_multiplier_ = 0;
  conv_profile.parameters_.type_ = CONV_2D;

  TFLITE_DCHECK_EQ(input_shape.DimensionsCount (), 4);
  TFLITE_DCHECK_EQ(filter_shape.DimensionsCount (), 4);
  TFLITE_DCHECK_EQ(output_shape.DimensionsCount (), 4);

  for (int i = 0; i < 4; i++)
  {
    conv_profile.input_shape_.dims_[i] = input_shape.Dims (i);
    conv_profile.filter_shape_.dims_[i] = filter_shape.Dims (i);
    conv_profile.output_shape_.dims_[i] = output_shape.Dims (i);
  }

  TFLITE_DCHECK_EQ(bias_shape.DimensionsCount (), 1);
  conv_profile.bias_shape_.dims_[0] = bias_shape.Dims (0);
  conv_profile.bias_shape_.dims_[1] = 1;
  conv_profile.bias_shape_.dims_[2] = 1;
  conv_profile.bias_shape_.dims_[3] = 1;

  return createTask (conv_profile,
                     input_data,
                     filter_data,
                     bias_data,
                     output_data,
                     output_multiplier,
                     output_shift,
                     bias_shape.Dims (0),
                     parent);
}

TensorProcessor::Task TensorProcessor::createTask (const tflite::DepthwiseParams& params,
                             const tflite::RuntimeShape& input_shape,
                             const float* input_data,
                             const tflite::RuntimeShape& filter_shape,
                             const float* filter_data,
                             const tflite::RuntimeShape& bias_shape,
                             const float* bias_data,
                             const tflite::RuntimeShape& output_shape,
                             float* output_data,
                             Event * parent)
{
  ConvProfile conv_profile = { 0 };

  conv_profile.parameters_.stride_.height_ = params.stride_height;
  conv_profile.parameters_.stride_.width_ = params.stride_width;

  conv_profile.parameters_.dilation_.height_ = params.dilation_height_factor;
  conv_profile.parameters_.dilation_.width_ = params.dilation_width_factor;

  conv_profile.parameters_.padding_.height_ = params.padding_values.height;
  conv_profile.parameters_.padding_.width_ = params.padding_values.width;

  conv_profile.parameters_.offset_.input_ = 0;
  conv_profile.parameters_.offset_.output_ = 0;

  conv_profile.parameters_.activation_.max_ = params.float_activation_max;
  conv_profile.parameters_.activation_.min_ = params.float_activation_min;

  conv_profile.parameters_.depth_multiplier_ = params.depth_multiplier;
  conv_profile.parameters_.type_ = DEPTHWISE_CONV_2D;

  TFLITE_DCHECK_EQ(input_shape.DimensionsCount (), 4);
  TFLITE_DCHECK_EQ(filter_shape.DimensionsCount (), 4);
  TFLITE_DCHECK_EQ(output_shape.DimensionsCount (), 4);

  for (int i = 0; i < 4; i++)
  {
    conv_profile.input_shape_.dims_[i] = input_shape.Dims (i);
    conv_profile.filter_shape_.dims_[i] = filter_shape.Dims (i);
    conv_profile.output_shape_.dims_[i] = output_shape.Dims (i);
  }

  TFLITE_DCHECK_EQ(bias_shape.DimensionsCount (), 1);
  conv_profile.bias_shape_.dims_[0] = bias_shape.Dims (0);
  conv_profile.bias_shape_.dims_[1] = 1;
  conv_profile.bias_shape_.dims_[2] = 1;
  conv_profile.bias_shape_.dims_[3] = 1;

  return createTask (conv_profile, input_data, filter_data, bias_data, output_data, parent);
}

TensorProcessor::Task TensorProcessor::createTask (const tflite::DepthwiseParams& params,
       const int32_t* output_multiplier,
       const int32_t* output_shift,
       const tflite::RuntimeShape& input_shape, const int8_t* input_data,
       const tflite::RuntimeShape& filter_shape, const int8_t* filter_data,
       const tflite::RuntimeShape& bias_shape, const int32_t* bias_data,
       const tflite::RuntimeShape& output_shape, int8_t* output_data,
       Event * parent)
{
  ConvProfile conv_profile = { 0 };

  conv_profile.parameters_.stride_.height_ = params.stride_height;
  conv_profile.parameters_.stride_.width_ = params.stride_width;

  conv_profile.parameters_.dilation_.height_ = params.dilation_height_factor;
  conv_profile.parameters_.dilation_.width_ = params.dilation_width_factor;

  conv_profile.parameters_.padding_.height_ = params.padding_values.height;
  conv_profile.parameters_.padding_.width_ = params.padding_values.width;

  conv_profile.parameters_.offset_.input_ = params.input_offset;
  conv_profile.parameters_.offset_.output_ = params.output_offset;

  conv_profile.parameters_.quantized_activation_.max_ = params.quantized_activation_max;
  conv_profile.parameters_.quantized_activation_.min_ = params.quantized_activation_min;

  conv_profile.parameters_.depth_multiplier_ = params.depth_multiplier;
  conv_profile.parameters_.type_ = DEPTHWISE_CONV_2D;

  TFLITE_DCHECK_EQ(input_shape.DimensionsCount (), 4);
  TFLITE_DCHECK_EQ(filter_shape.DimensionsCount (), 4);
  TFLITE_DCHECK_EQ(output_shape.DimensionsCount (), 4);

  for (int i = 0; i < 4; i++)
  {
    conv_profile.input_shape_.dims_[i] = input_shape.Dims (i);
    conv_profile.filter_shape_.dims_[i] = filter_shape.Dims (i);
    conv_profile.output_shape_.dims_[i] = output_shape.Dims (i);
  }

  TFLITE_DCHECK_EQ(bias_shape.DimensionsCount (), 1);
  conv_profile.bias_shape_.dims_[0] = bias_shape.Dims (0);
  conv_profile.bias_shape_.dims_[1] = 1;
  conv_profile.bias_shape_.dims_[2] = 1;
  conv_profile.bias_shape_.dims_[3] = 1;

  return createTask (conv_profile,
                     input_data,
                     filter_data,
                     bias_data,
                     output_data,
                     output_multiplier,
                     output_shift,
                     bias_shape.Dims (0),
                     parent);
}

void TensorProcessor::setExecutionFlags (TensorProcessor::Task & task, int flags)
{
  ProcessingUnit::setTransactionFlags (task.compute, flags);
}

int TensorProcessor::getExecutionFlags (TensorProcessor::Task & task)
{
  return ProcessingUnit::getTransactionFlags (task.compute);
}

TensorProcessor::Task TensorProcessor::createTask (const ConvProfile& profile,
                                                     const float* input_data,
                                                     const float* filter_data,
                                                     const float* bias_data,
                                                     float* output_data,
                                                     Event * parent)
{
  Task task = { 0 };

  ConvProfile * conv_profile = nullptr;
  float * filter = nullptr;
  float * bias = nullptr;

  size_t input_size = sizeof(float)
                      * profile.input_shape_.dims_[0]
                      * profile.input_shape_.dims_[1]
                      * profile.input_shape_.dims_[2]
                      * profile.input_shape_.dims_[3];

  size_t filter_size = sizeof(float)
                      * profile.filter_shape_.dims_[0]
                      * profile.filter_shape_.dims_[1]
                      * profile.filter_shape_.dims_[2]
                      * profile.filter_shape_.dims_[3];

  size_t bias_size = sizeof(float)
                    * profile.bias_shape_.dims_[0]
                    * profile.bias_shape_.dims_[1]
                    * profile.bias_shape_.dims_[2]
                    * profile.bias_shape_.dims_[3];

  size_t output_size = sizeof(float)
                      * profile.output_shape_.dims_[0]
                      * profile.output_shape_.dims_[1]
                      * profile.output_shape_.dims_[2]
                      * profile.output_shape_.dims_[3];

  size_t txBufferSize = sizeof(ConvProfile) + filter_size + bias_size;

  void * txBufferPtr = MemoryBlock_alloc (&profile_->ddrMem, txBufferSize);
  memset (txBufferPtr, 0, txBufferSize);
  Xil_DCacheFlushRange ((UINTPTR) txBufferPtr, txBufferSize);

  task.setup.mode = CONV_SETUP;
  task.setup.flags = BLOCKING_IN_OUT | RX_CACHE_FETCH | TX_CACHE_FUSH;
  task.setup.txBufferPtr = txBufferPtr;
  task.setup.txBufferSize = txBufferSize;
  task.setup.rxBufferPtr = nullptr;
  task.setup.rxBufferSize = 0;

  conv_profile = (ConvProfile *) txBufferPtr;
  filter = (float *) &conv_profile[1];
  bias = &filter[filter_size / sizeof(float)];

  memcpy (conv_profile, &profile, sizeof(ConvProfile));
  memcpy (filter, filter_data, filter_size);
  memcpy (bias, bias_data, bias_size);

  task.compute.mode = CONV_EXECUTION;
  task.compute.flags = BLOCKING_IN_OUT | RX_CACHE_FETCH | TX_CACHE_FUSH;
  task.compute.txBufferPtr = (void *) input_data;
  task.compute.txBufferSize = input_size;
  task.compute.rxBufferPtr = (void *) output_data;
  task.compute.rxBufferSize = output_size;

  task.delegate_event = Event_new (parent, EVENT_DELEGATE, (void *) "DELEGATE");
  task.hardware_event = Event_new (task.delegate_event, EVENT_HARDWARE, (void *) "HARDWARE");

  return task;
}

TensorProcessor::Task TensorProcessor::createTask (const ConvProfile& profile,
                         const int8_t* input_data,
                         const int8_t* filter_data,
                         const int32_t* bias_data,
                         int8_t* output_data,
                         const int32_t* quant_output_multiplier,
                         const int32_t* quant_output_shift,
                         const int32_t  quant_len,
                         Event * parent)
{
  Task task = { 0 };

  ConvProfile * conv_profile = nullptr;
  int8_t * filter = nullptr;
  int8_t * bias = nullptr;

  int32_t* output_multiplier_vector = nullptr;
  int32_t* output_shift_vector = nullptr;

  size_t input_size = sizeof(int8_t)
                      * profile.input_shape_.dims_[0]
                      * profile.input_shape_.dims_[1]
                      * profile.input_shape_.dims_[2]
                      * profile.input_shape_.dims_[3];
  if (input_size % 4)
  {
    input_size += 4 - input_size % 4;
  }

  size_t filter_size = sizeof(int8_t)
                      * profile.filter_shape_.dims_[0]
                      * profile.filter_shape_.dims_[1]
                      * profile.filter_shape_.dims_[2]
                      * profile.filter_shape_.dims_[3];
  if (filter_size % 4)
  {
    filter_size += 4 - filter_size % 4;
  }

  size_t bias_size = sizeof(int32_t)
                    * profile.bias_shape_.dims_[0]
                    * profile.bias_shape_.dims_[1]
                    * profile.bias_shape_.dims_[2]
                    * profile.bias_shape_.dims_[3];

  size_t output_size = sizeof(int8_t)
                      * profile.output_shape_.dims_[0]
                      * profile.output_shape_.dims_[1]
                      * profile.output_shape_.dims_[2]
                      * profile.output_shape_.dims_[3];
  if (output_size % 4)
  {
    output_size += 4 - output_size % 4;
  }

  size_t txBufferSize = sizeof(ConvProfile) + filter_size + bias_size + 2 * sizeof(int32_t) * quant_len;

  ASSERT(txBufferSize % 4 == 0);

  void * txBufferPtr = MemoryBlock_alloc (&profile_->ddrMem, txBufferSize);
  memset (txBufferPtr, 0, txBufferSize);
  Xil_DCacheFlushRange ((UINTPTR) txBufferPtr, txBufferSize);

  task.setup.mode = CONV_SETUP;
  task.setup.flags = BLOCKING_IN_OUT | RX_CACHE_FETCH | TX_CACHE_FUSH;
  task.setup.txBufferPtr = txBufferPtr;
  task.setup.txBufferSize = txBufferSize;
  task.setup.rxBufferPtr = nullptr;
  task.setup.rxBufferSize = 0;

  conv_profile = (ConvProfile *) txBufferPtr;
  filter = (int8_t *) &conv_profile[1];
  bias = &filter[filter_size / sizeof(int8_t)];
  output_multiplier_vector = (int32_t *) &((int8_t*) txBufferPtr)[sizeof(ConvProfile) + filter_size + bias_size];
  output_shift_vector = (int32_t *) &((int8_t*) txBufferPtr)[sizeof(ConvProfile) + filter_size + bias_size + sizeof(int32_t) * quant_len];

  memcpy (conv_profile, &profile, sizeof(ConvProfile));
  memcpy (filter, filter_data, filter_size);
  memcpy (bias, bias_data, bias_size);
  memcpy (output_multiplier_vector, quant_output_multiplier, sizeof(int32_t) * quant_len);
  memcpy (output_shift_vector, quant_output_shift, sizeof(int32_t) * quant_len);

  task.compute.mode = CONV_EXECUTION;
  task.compute.flags = BLOCKING_IN_OUT | RX_CACHE_FETCH | TX_CACHE_FUSH;
  task.compute.txBufferPtr = (void *) input_data;
  task.compute.txBufferSize = input_size;
  task.compute.rxBufferPtr = (void *) output_data;
  task.compute.rxBufferSize = output_size;

  task.delegate_event = Event_new (parent, EVENT_DELEGATE, (void *) "DELEGATE");
  task.hardware_event = Event_new (task.delegate_event, EVENT_HARDWARE, (void *) "HARDWARE");

  return task;
}

bool TensorProcessor::isValid(Task & task)
{
  return (((0 < task.setup.txBufferSize)
          && (task.setup.txBufferPtr != nullptr))
          || ((0 < task.setup.rxBufferSize)
              && (task.setup.rxBufferPtr != nullptr)));
}

int TensorProcessor::execute(Task & task)
{
  int status = XST_FAILURE;

  if (isValid (task))
  {
    current_task_ = &task;

    status_ = CONFIGURATION;
    Event_start (task.delegate_event);
    status = ProcessingUnit::execute(&task.setup);
    ASSERT(status == XST_SUCCESS);

    status_ = EXECUTION;
    Event_start (task.hardware_event);
    status = ProcessingUnit::execute(&task.compute);
    ASSERT(status == XST_SUCCESS);

    Event_stop (task.delegate_event);
  }

  return status;
}

TensorProcessor::Status TensorProcessor::getStatus (void)
{
  return status_;
}

TensorProcessor::ProcessorArray * TensorProcessor::getSingletonTPArray (void)
{
  if (singleton_tp_array_.len == 0)
  {
    instatiateProcessors();
  }

  ASSERT(0 < singleton_tp_array_.len);
  ASSERT(singleton_tp_array_.tp != nullptr);

  return & singleton_tp_array_;
}

TensorProcessor::ProcessorArray * TensorProcessor::instatiateProcessors (void)
{
  static ProcessingUnit::Profile pu_profile_array[] =
  {
#ifdef XPAR_CONV_0_DEVICE_ID
    {
      .hwVtbl        = &HardwareVtbl_Conv_,
      .dmaVtbl       = &DMAHardwareVtbl_,
      .hwDeviceID    = XPAR_CONV_0_DEVICE_ID,
      .dmaDeviceID   = XPAR_AXI_DMA_0_DEVICE_ID,
      .hwIntVecID    = XPAR_FABRIC_CONV_0_INTERRUPT_INTR,
      .dmaTxIntVecID = XPAR_FABRIC_AXI_DMA_0_MM2S_INTROUT_INTR,
      .dmaRxIntVecID = XPAR_FABRIC_AXI_DMA_0_S2MM_INTROUT_INTR,
      .channelSize   = 4,
      .ddrMem =
      { .baseAddress = XPAR_PS7_DDR_0_S_AXI_BASEADDR + 0x1A000000,
        .highAddress = XPAR_PS7_DDR_0_S_AXI_BASEADDR + 0x1AFFFFFF,
        .blockIndex  = 0
      }
    },
#endif
#ifdef XPAR_CONV_1_DEVICE_ID
    {
      .hwVtbl        = &HardwareVtbl_Conv_,
      .dmaVtbl       = &DMAHardwareVtbl_,
      .hwDeviceID    = XPAR_CONV_1_DEVICE_ID,
      .dmaDeviceID   = XPAR_AXI_DMA_1_DEVICE_ID,
      .hwIntVecID    = XPAR_FABRIC_CONV_1_INTERRUPT_INTR,
      .dmaTxIntVecID = XPAR_FABRIC_AXI_DMA_1_MM2S_INTROUT_INTR,
      .dmaRxIntVecID = XPAR_FABRIC_AXI_DMA_1_S2MM_INTROUT_INTR,
      .channelSize   = 4,
      .ddrMem =
      { .baseAddress = XPAR_PS7_DDR_0_S_AXI_BASEADDR + 0x1B000000,
        .highAddress = XPAR_PS7_DDR_0_S_AXI_BASEADDR + 0x1BFFFFFF,
        .blockIndex  = 0
      }
    },
#endif
#ifdef XPAR_CONV_2_DEVICE_ID
    {
      .hwVtbl        = &HardwareVtbl_Conv_,
      .dmaVtbl       = &DMAHardwareVtbl_,
      .hwDeviceID    = XPAR_CONV_2_DEVICE_ID,
      .dmaDeviceID   = XPAR_AXI_DMA_2_DEVICE_ID,
      .hwIntVecID    = XPAR_FABRIC_CONV_2_INTERRUPT_INTR,
      .dmaTxIntVecID = XPAR_FABRIC_AXI_DMA_2_MM2S_INTROUT_INTR,
      .dmaRxIntVecID = XPAR_FABRIC_AXI_DMA_2_S2MM_INTROUT_INTR,
      .channelSize   = 4,
      .ddrMem =
      { .baseAddress = XPAR_PS7_DDR_0_S_AXI_BASEADDR + 0x1C000000,
        .highAddress = XPAR_PS7_DDR_0_S_AXI_BASEADDR + 0x1CFFFFFF,
        .blockIndex  = 0
      }
    }
#endif
#ifdef XPAR_CONV_3_DEVICE_ID
    {
      .hwVtbl        = &HardwareVtbl_Conv_,
      .dmaVtbl       = &DMAHardwareVtbl_,
      .hwDeviceID    = XPAR_CONV_3_DEVICE_ID,
      .dmaDeviceID   = XPAR_AXI_DMA_3_DEVICE_ID,
      .hwIntVecID    = XPAR_FABRIC_CONV_3_INTERRUPT_INTR,
      .dmaTxIntVecID = XPAR_FABRIC_AXI_DMA_3_MM2S_INTROUT_INTR,
      .dmaRxIntVecID = XPAR_FABRIC_AXI_DMA_3_S2MM_INTROUT_INTR,
      .channelSize   = 4,
      .ddrMem =
      { .baseAddress = XPAR_PS7_DDR_0_S_AXI_BASEADDR + 0x1D000000,
        .highAddress = XPAR_PS7_DDR_0_S_AXI_BASEADDR + 0x1DFFFFFF,
        .blockIndex  = 0
      }
    }
#endif
  };

  int num_processors = sizeof(pu_profile_array) / sizeof(ProcessingUnit::Profile);

  ASSERT(0 < num_processors);

  singleton_tp_array_.tp = new TensorProcessor[num_processors];

  ASSERT(singleton_tp_array_.tp != nullptr);

  if (singleton_tp_array_.tp != nullptr)
  {
    singleton_tp_array_.len = num_processors;
    for (int i = 0; i < num_processors; i++)
    {
      singleton_tp_array_.tp[i].initialize(&pu_profile_array[i]);
    }
  }

  return &singleton_tp_array_;
}

void TensorProcessor::onDone_ip (void)
{
  if ((current_task_ != nullptr) && (status_  == EXECUTION))
  {
    Event_stop (current_task_->hardware_event);
    status_ = IDLE;
    current_task_ = nullptr;
  }
}

void TensorProcessor::onDone_dmaRx (void)
{

}

void TensorProcessor::onDone_dmaTx (void)
{

}
