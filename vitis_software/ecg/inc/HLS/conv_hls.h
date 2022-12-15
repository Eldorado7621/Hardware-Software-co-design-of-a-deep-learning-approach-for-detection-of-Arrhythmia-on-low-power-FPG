#ifndef CONV_HLS_H_
#define CONV_HLS_H_

#define DMA_CHANNEL_WIDTH 32

typedef enum
{
  CONV_SETUP,
  CONV_EXECUTION
} ConvExecutionMode;

typedef union
{
  unsigned int u32;
  float f32;
} Data;

#define MAX_TENSOR_SIZE 4
typedef struct
{
  int dims_[MAX_TENSOR_SIZE];
} TensorShape;

typedef struct
{
  Data * data_;
  TensorShape shape_;
} Tensor;

typedef struct
{
  int height_;
  int width_;
} ConvStride;

typedef struct
{
  int height_;
  int width_;
} ConvDilation;

typedef struct
{
  int height_;
  int width_;
} ConvPadding;

typedef struct
{
  int input_;
  int filter_;
  int output_;
} ConvOffset;

typedef struct
{
  float max_;
  float min_;
} ConvActivation;

typedef struct
{
  int max_;
  int min_;
} ConvQantActivation;

typedef enum
{
  CONV_2D,
  DEPTHWISE_CONV_2D
} OperatorType;

typedef struct
{
  ConvStride          stride_;
  ConvDilation        dilation_;
  ConvPadding         padding_;
  ConvOffset          offset_;
  ConvActivation      activation_;
  ConvQantActivation  quantized_activation_;
  int                 depth_multiplier_;
  OperatorType        type_;
} ConvParameters;


typedef struct
{
  ConvParameters  parameters_;
  TensorShape     input_shape_;
  TensorShape     filter_shape_;
  TensorShape     bias_shape_;
  TensorShape     output_shape_;
} ConvProfile;


#endif // CONV_HLS_H_

