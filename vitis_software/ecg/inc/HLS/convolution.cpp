#include "convolution.h"
#include "../libs/utilities/inc/custom_float.h"
#include <stdint.h>
#include <algorithm>

//#define CONV_FILTER_BUFFER_SIZE     512*1024
//#define CONV_BIAS_BUFFER_SIZE       512
//
//#define CONV_INPUT_BUFFER_SIZE      4*1024
//
//#define FIXED_POINT                 true
//
//#define HYBRID_LOGARITHMIC          false
//#define CUSTOM_SIGN_BIT             1
//#define CUSTOM_EXPONENT_BIT_WIDTH   4
//#define CUSTOM_MANTISSA_BIT_WIDTH   3
//
//// Set to 0 for normalized numbers [0 - 1), All exponents are on the left side, so they are stored without sign
//#define CUSTOM_EXPONENT_SIGN_BIT    1

#define CUSTOM_GET_SIGN(x)            ((1 << ((CUSTOM_EXPONENT_BIT_WIDTH)+(CUSTOM_MANTISSA_BIT_WIDTH))) & (x))
#define CUSTOM_GET_EXPONENT(x)        (((1 << (CUSTOM_EXPONENT_BIT_WIDTH)) - 1) & ((x) >> (CUSTOM_MANTISSA_BIT_WIDTH)))
#define CUSTOM_GET_EXPONENT_SIGN(x)   (CUSTOM_GET_EXPONENT(x) & (1 << ((CUSTOM_EXPONENT_BIT_WIDTH) - 1)))
#define CUSTOM_GET_MANTISSA(x)        (((1 << (CUSTOM_MANTISSA_BIT_WIDTH)) | (((1 << (CUSTOM_MANTISSA_BIT_WIDTH)) - 1) & (x))) << (23 - (CUSTOM_MANTISSA_BIT_WIDTH)))


#define CUSTOM_FORMAT_BIT_WIDTH   (CUSTOM_SIGN_BIT + CUSTOM_EXPONENT_BIT_WIDTH + CUSTOM_MANTISSA_BIT_WIDTH)
#define BUILD_CUSTOM(s, exponent, mantissa) (((s!=0) << (CUSTOM_EXPONENT_BIT_WIDTH + CUSTOM_MANTISSA_BIT_WIDTH)) | ((((1 << CUSTOM_EXPONENT_BIT_WIDTH) - 1) & (exponent)) << CUSTOM_MANTISSA_BIT_WIDTH) | (((mantissa) >> (23 - CUSTOM_MANTISSA_BIT_WIDTH)) & ((1 << CUSTOM_MANTISSA_BIT_WIDTH) - 1)))


#if FIXED_POINT

  typedef int8_t          InputFormat;
  typedef int8_t          CustomFormat;
  typedef int8_t          OutputFormat;
  typedef int32_t         BiasFormat;
  typedef int32_t         AccumulatorFormat;

#else

  typedef float           InputFormat;
  typedef float           OutputFormat;

  #if HYBRID_LOGARITHMIC
    typedef ap_uint<CUSTOM_FORMAT_BIT_WIDTH> CustomFormat;
    typedef int64_t         MagnitudeFormat;
    typedef int8_t          ExponentFormat;
    typedef bool            SignFormat;
  #else
    typedef float           CustomFormat;
  #endif

  typedef CustomFormat    BiasFormat;
  typedef float           AccumulatorFormat;

#endif

#define DMA_CHANNEL_BYTE_WIDTH    (DMA_CHANNEL_WIDTH/8)
#define DMA_CHANNEL_FLOAT_WIDTH   (DMA_CHANNEL_BYTE_WIDTH/sizeof(float))


static StreamChannel channel_in;
static StreamChannel channel_out;

// Correctly-rounded-to-nearest division by a power-of-two.
// Also known as a rounding arithmetic right shift.
inline int RoundingDivideByPOT (int x, int exponent)
{
  const int mask = (1ll << exponent) - 1;
  const int remainder = x & mask;
  const int threshold = (mask >> 1) + (((x < 0) ? ~(0) : 0) & 1);
  return ((x >> exponent) + (((remainder > threshold) ? ~(0) : 0) & 1));
}

inline int SaturatingRoundingDoublingHighMul (int32_t a, int32_t b)
{
  bool overflow = a == b && a == 0x80000000;
  int64_t a_64(a);
  int64_t b_64(b);
  int64_t ab_64 = a_64 * b_64;
  int32_t nudge = ab_64 >= 0 ? (1 << 30) : (1 - (1 << 30));
  //int32_t ab_x2_high32 = (int32_t) ((ab_64 + nudge) / (1ll << 31));
  int32_t ab_x2_high32 = (int32_t) ((ab_64 + nudge) >> 31);
  return overflow ? 0x7fffffff : ab_x2_high32;
}

inline int32_t MultiplyByQuantizedMultiplier (int32_t x,
                                              int32_t quantized_multiplier,
                                              int shift)
{
  int left_shift = shift > 0 ? shift : 0;
  int right_shift = shift > 0 ? 0 : -shift;
  return RoundingDivideByPOT (SaturatingRoundingDoublingHighMul (x  << left_shift, quantized_multiplier), right_shift);
}


template <typename T>
inline T ActivationFunctionWithMinMax(T x, T output_activation_min,
                                      T output_activation_max) {
  using std::max;
  using std::min;
  return min(max(x, output_activation_min), output_activation_max);
}

#define FlatSize(shape)               ((shape)->dims_[0] * (shape)->dims_[1] * (shape)->dims_[2] * (shape)->dims_[3])

#define Offset(shape, i0, i1, i2, i3) ((((i0) * (shape)->dims_[1] + i1) * (shape)->dims_[2] + (i2)) * (shape)->dims_[3] + (i3))

#if HYBRID_LOGARITHMIC
inline MagnitudeFormat Float_denormalize (float value)
{
  Data data;
  SignFormat sign = 0;
  ExponentFormat exponent = 0;
  MagnitudeFormat mantissa = 0;
  MagnitudeFormat magnitude = 0;

  data.f32 = value;

  if (data.u32)
  {
    sign = DATA32_GET_SIGN(data.u32);
    exponent = DATA32_GET_EXPONENT(data.u32);
    mantissa = DATA32_GET_MANTISSA(data.u32);
    magnitude =
        (0 < exponent) ? (mantissa << exponent) : (mantissa >> -exponent);

    if (32 < exponent)
      magnitude = (((MagnitudeFormat)(1)) << 63) - 1;

    if (sign)
      magnitude = ~magnitude + 1;
  }

  return magnitude;
}

inline float Float_normalize (MagnitudeFormat magnitude)
{
  SignFormat        sign = 0;
  ExponentFormat    exponent = 0;
  float             value;

  if (magnitude != 0)
  {
    sign = magnitude < 0;
    if (sign)
    {
      magnitude = ~magnitude + 1;
    }

    TOTAL_NORMALIZATION_LOOP: for (exponent = 0; !(0x80000000000000 & magnitude); exponent++)
    { // Normalize
#pragma HLS pipeline
      magnitude <<= 1;
    }

    exponent -= 32;
    magnitude >>= 32;

    *(uint32_t*) (&value) = BUILD_FLOAT(sign, -exponent, magnitude);
  }
  else
  {
    *(uint32_t*) (&value) = 0;
  }

  return value;
}

inline MagnitudeFormat Custom_denormalize (CustomFormat value)
{
  SignFormat sign = 0;
  ExponentFormat exponent = 0;
  MagnitudeFormat mantissa = 0;
  MagnitudeFormat magnitude = 0;

  if (value)
  {
    sign = CUSTOM_GET_SIGN(value);
#if CUSTOM_EXPONENT_SIGN_BIT
    exponent = (ap_int<CUSTOM_EXPONENT_BIT_WIDTH>)CUSTOM_GET_EXPONENT(value);
#else
    exponent = -(ap_uint<CUSTOM_EXPONENT_BIT_WIDTH>)CUSTOM_GET_EXPONENT(value);
#endif
    mantissa = CUSTOM_GET_MANTISSA(value);

    magnitude =
        (0 < exponent) ? (mantissa << exponent) : (mantissa >> -exponent);

    if (sign)
      magnitude = ~magnitude + 1;
  }

  return magnitude;
}

inline MagnitudeFormat ActivationFunctionWithMinMaxMagnitude (MagnitudeFormat x,
                                       MagnitudeFormat output_activation_min,
                                       MagnitudeFormat output_activation_max)
{

  return (x < output_activation_min) ? output_activation_min :
         (output_activation_max < x) ? output_activation_max : x;
}

inline void DotProduct_logarithmic (MagnitudeFormat & Total_magnitude,
                                    float & input_value,
                                    CustomFormat & filter_value)
{
  Data      input_data;

  SignFormat        f_s;
  ExponentFormat    f_e;
  MagnitudeFormat   f_m;

  SignFormat        i_s;
  ExponentFormat    i_e;
  MagnitudeFormat   i_m;

  SignFormat        p_s;
  ExponentFormat    p_e;
  MagnitudeFormat   p_m;

  MagnitudeFormat   p_magnitude;

  input_data.f32 = input_value;

  if (input_data.u32 == 0 || filter_value == 0)
    return;

  i_s = DATA32_GET_SIGN(input_data.u32);
  i_e = DATA32_GET_EXPONENT(input_data.u32);
  i_m = DATA32_GET_MANTISSA(input_data.u32);

  f_s = CUSTOM_GET_SIGN(filter_value);
#if CUSTOM_EXPONENT_SIGN_BIT
  f_e = (ap_int<CUSTOM_EXPONENT_BIT_WIDTH>)CUSTOM_GET_EXPONENT(filter_value);
#else
  f_e = -(ap_uint<CUSTOM_EXPONENT_BIT_WIDTH>)CUSTOM_GET_EXPONENT(filter_value);
#endif
  f_m = CUSTOM_GET_MANTISSA(filter_value);

  p_s = i_s != f_s;
  p_e = i_e + f_e;
  p_m = (i_m * f_m) >> 23;

  if (p_m & 0x01000000)
  {
    p_e++;
    p_m >>= 1;
  }

  p_magnitude = (0 < p_e) ? (p_m << p_e) : (p_m >> -p_e);

  if (p_s)
    p_magnitude = ~p_magnitude + 1;

  Total_magnitude += p_magnitude;
}
#endif

///////////////////////////////////////////////////////////////////////////////
static InputFormat StreamPeripheral_inputBuffer[CONV_INPUT_BUFFER_SIZE] = { 0 };
static int StreamPeripheral_yOffset = 0;
static int StreamPeripheral_lookupTable[32] = { 0 };
static int StreamPeripheral_lookupTableRows[32] = { 0 };
static int StreamPeripheral_lookupIndex = 0;
#if DYNAMIC_FILTER_SHAPE
static int StreamPeripheral_lookupLength = 0;
#else
const int StreamPeripheral_lookupLength = FILTER_HEIGHT;
#endif
static int StreamPeripheral_rowCount = 0;
static int StreamPeripheral_rowSize = 0;
static TensorShape StreamPeripheral_inputShape = { 0 };
///////////////////////////////////////////////////////////////////////////////
static int AXIStream_index = 0;
static float AXIStream_outputBuffer[2] = { 0 };
static int AXIStreamOut_index = 0;
static int AXIStreamOut_indexLast = 0;
///////////////////////////////////////////////////////////////////////////////

inline void StreamPeripheral_readRowBuffer (hls::stream<StreamChannel> &stream_in,
                                            int input_index)
{
  const int data_ratio = (DMA_CHANNEL_WIDTH / (8 * sizeof(InputFormat)));

  READ_INPUT_TENSOR_LINE_ROW:
  for (int j = 0; j < StreamPeripheral_rowSize; j += data_ratio)
  {
#pragma HLS pipeline
    channel_in = stream_in.read ();
#if FIXED_POINT
    FIXED_POINT_UNROLL: for (int i = 0; i < data_ratio; i++)
    {
#pragma HLS unroll
      StreamPeripheral_inputBuffer[input_index + j + i] = channel_in.data
          >> (8 * sizeof(InputFormat) * i);
    }
#else
    Data temp;
    temp.u32 = channel_in.data;
    StreamPeripheral_inputBuffer[input_index + j] = temp.f32;
#endif
  }
}

inline void StreamPeripheral_initialize ( hls::stream<StreamChannel> &stream_in,
                                   TensorShape * input_shape,
#if DYNAMIC_FILTER_SHAPE
                                   TensorShape * filter_shape,
#endif
                                   TensorShape * output_shape)
{
  AXIStream_index = 0;

  AXIStreamOut_index = 0;

  AXIStreamOut_indexLast = FlatSize(output_shape);

#if DYNAMIC_FILTER_SHAPE
  StreamPeripheral_lookupLength = filter_shape->dims_[1]; // Filter height
#endif

  StreamPeripheral_lookupIndex = StreamPeripheral_lookupLength - 1;

  StreamPeripheral_rowCount = 0;

  // input_width * input_depth;
  StreamPeripheral_rowSize = input_shape->dims_[2] * input_shape->dims_[3];

  StreamPeripheral_inputShape = *input_shape;

  INITIALIZE_INPUT_TENSOR_LINE_ROWS:
  for (int row = 0; row < StreamPeripheral_lookupIndex; row++)
  {
#pragma HLS pipeline
    StreamPeripheral_lookupTable[row] = AXIStream_index;
    StreamPeripheral_lookupTableRows[row] = StreamPeripheral_rowCount++;

    StreamPeripheral_readRowBuffer (stream_in, AXIStream_index);

    AXIStream_index += StreamPeripheral_rowSize;
  }

  StreamPeripheral_lookupTable[StreamPeripheral_lookupIndex] = AXIStream_index;
}

inline void StreamPeripheral_pushRowBuffer(hls::stream<StreamChannel> &stream_in,
                                           const int in_y_origin)
{
  if (0 <= in_y_origin && StreamPeripheral_rowCount < StreamPeripheral_inputShape.dims_[1])
  {
    int i = StreamPeripheral_lookupTable[StreamPeripheral_lookupIndex];

    StreamPeripheral_yOffset = in_y_origin;

    StreamPeripheral_lookupTableRows[StreamPeripheral_lookupIndex] = StreamPeripheral_rowCount++;

    StreamPeripheral_readRowBuffer (stream_in, i);

    AXIStream_index += StreamPeripheral_rowSize;

    if (StreamPeripheral_lookupIndex + 1 < StreamPeripheral_lookupLength)
    {
      StreamPeripheral_lookupIndex++;
    }
    else
    {
      StreamPeripheral_lookupIndex = 0;
    }
  }
}

inline InputFormat StreamPeripheral_read (const int in_y,
                                          const int in_x,
                                          const int in_channel)
{
  int lookupIndex;
  int i;

  if (StreamPeripheral_lookupIndex < in_y)
  {
    lookupIndex = in_y - StreamPeripheral_yOffset
        + StreamPeripheral_lookupIndex;
  }
  else
  {
    lookupIndex = in_y;
  }

  if (lookupIndex > StreamPeripheral_lookupLength)
    lookupIndex -= StreamPeripheral_lookupLength;

  i = StreamPeripheral_lookupTable[lookupIndex]
      + StreamPeripheral_inputShape.dims_[3] * in_x
      + in_channel;

  return StreamPeripheral_inputBuffer[i];
}

inline void StreamPeripheral_output (hls::stream<StreamChannel> &stream_out,
                                     OutputFormat  data)
{
  channel_out.last = (AXIStreamOut_index + 1) == AXIStreamOut_indexLast;
#if FIXED_POINT
  const int data_ratio = (DMA_CHANNEL_WIDTH / (8 * sizeof(OutputFormat)));

  channel_out.data = (channel_out.data & ~((ap_uint<DMA_CHANNEL_WIDTH> ) 0xFF << (8 * (AXIStreamOut_index % data_ratio))))
      | (((ap_uint<DMA_CHANNEL_WIDTH> ) data) << (8 * (AXIStreamOut_index % data_ratio)));

  if (  ((AXIStreamOut_index % data_ratio) == (data_ratio - 1))
     || ((AXIStreamOut_index + 1)          == AXIStreamOut_indexLast))
  {
    stream_out.write (channel_out);
  }
#else
  Data data_;
  data_.f32 = data;
  channel_out.data = data_.u32;
  stream_out.write (channel_out);
#endif
  AXIStreamOut_index ++;
}


inline int Convolution_execution (hls::stream<StreamChannel> &stream_in,
                                  hls::stream<StreamChannel> &stream_out,
                                  ConvProfile &  Conv_profile,
                                  CustomFormat * Conv_filter,
                                  BiasFormat *   Conv_bias
#if FIXED_POINT
                                  ,
                                  BiasFormat * Conv_quant_output_multiplier,
                                  BiasFormat * Conv_quant_output_shift
#endif
                           )
{
#if HYBRID_LOGARITHMIC
  MagnitudeFormat   Total_magnitude = 0;
  ///////////////////////////////////////////////////////////////////////////////
  MagnitudeFormat   Activation_max_magnitude = 0;
  MagnitudeFormat   Activation_min_magnitude = 0;
#endif
  ///////////////////////////////////////////////////////////////////////////////

  ConvProfile * profile = &Conv_profile;

  TensorShape * input_shape = &profile->input_shape_;
  volatile int batches = input_shape->dims_[0];
  int input_height = input_shape->dims_[1];
  int input_width = input_shape->dims_[2];
  volatile int input_depth = input_shape->dims_[3];

  TensorShape * filter_shape = &profile->filter_shape_;
#if DYNAMIC_FILTER_SHAPE
  volatile int filter_height = filter_shape->dims_[1];
  volatile int filter_width = filter_shape->dims_[2];
#else
  const int filter_height = FILTER_HEIGHT;
  const int filter_width = FILTER_WIDTH;
#endif

  TensorShape * output_shape = &profile->output_shape_;
  volatile int output_height = output_shape->dims_[1];
  volatile int output_width = output_shape->dims_[2];
  volatile int output_depth = output_shape->dims_[3];

  TensorShape * bias_shape = &profile->bias_shape_;

  bool bias_data_enable = (bias_shape->dims_[0] == output_depth);

  int stride_height = profile->parameters_.stride_.height_;
  int stride_width = profile->parameters_.stride_.width_;

  int pad_height = profile->parameters_.padding_.height_;
  int pad_width = profile->parameters_.padding_.width_;

  int dilation_height_factor = profile->parameters_.dilation_.height_;
  int dilation_width_factor = profile->parameters_.dilation_.height_;

#if FIXED_POINT
  const int output_activation_max = Conv_profile.parameters_.quantized_activation_.max_;
  const int output_activation_min = Conv_profile.parameters_.quantized_activation_.min_;

  const int32_t input_offset = Conv_profile.parameters_.offset_.input_;
  const int32_t output_offset = Conv_profile.parameters_.offset_.output_;
#else
  const float output_activation_max = Conv_profile.parameters_.activation_.max_;
  const float output_activation_min = Conv_profile.parameters_.activation_.min_;
#endif

  OutputFormat activation_output = 0;

  ///////////////////////////////////////////////////////////////////////////////
#if HYBRID_LOGARITHMIC
  Activation_max_magnitude = Float_denormalize (output_activation_max);
  Activation_min_magnitude = Float_denormalize (output_activation_min);
#endif
  ///////////////////////////////////////////////////////////////////////////////
#if DYNAMIC_FILTER_SHAPE
  StreamPeripheral_initialize (stream_in, input_shape, filter_shape, output_shape);
#else
  StreamPeripheral_initialize (stream_in, input_shape, output_shape);
#endif
  CONV_OUTPUT_BATCH: for (int batch = 0; batch < batches; ++batch)
  {
#pragma HLS pipeline
    CONV_OUTPUT_ROW: for (int out_y = - pad_height; out_y < output_height - pad_height; ++out_y)
    {
#pragma HLS pipeline
      StreamPeripheral_pushRowBuffer(stream_in, out_y);

      CONV_OUTPUT_COL: for (int out_x = - pad_width; out_x < output_width - pad_width; ++out_x)
      {
#pragma HLS pipeline
        CONV_OUTPUT_CHANNEL: for (int out_channel = 0; out_channel < output_depth; ++out_channel)
        {
#pragma HLS pipeline
          AccumulatorFormat total = 0;
#if HYBRID_LOGARITHMIC
          Total_magnitude = 0;
#endif
          CONV_FILTER_ROW: for (int in_y = out_y; in_y < filter_height + out_y; ++in_y)
          {
#pragma HLS pipeline
            CONV_FILTER_COL: for (int in_x = out_x; in_x < filter_width + out_x; ++in_x)
            {
#pragma HLS pipeline
              // Zero padding by omitting the areas outside the image.
              if ((in_x >= 0) && (in_x < input_width) && (in_y >= 0)
                  && (in_y < input_height))
              {
                CONV_FILTER_CHANNEL: for (int in_channel = 0; in_channel < input_depth; ++in_channel)
                {
  #pragma HLS pipeline
                  InputFormat input_value = StreamPeripheral_read (in_y, in_x, in_channel);

  #if DYNAMIC_FILTER_SHAPE
                  CustomFormat filter_value = Conv_filter[Offset (filter_shape,
                                                           out_channel, in_y - out_y,
                                                           in_x - out_x, in_channel)];
  #else
                  CustomFormat filter_value = Conv_filter[((((out_channel) * FILTER_HEIGHT + (in_y - out_y)) * FILTER_WIDTH + (in_x - out_x)) * (filter_shape)->dims_[3] + (in_channel))];
  #endif

  #if FIXED_POINT
                  total += ((AccumulatorFormat) filter_value)
                        * (((AccumulatorFormat) input_value) + input_offset);
  #else
  #if HYBRID_LOGARITHMIC
                  DotProduct_logarithmic (Total_magnitude, input_value, filter_value);
  #else
                  total += (input_value * filter_value);
  #endif
  #endif
                }
              }
            }
          }
#if FIXED_POINT
            if (bias_data_enable)
            {
              total += Conv_bias[out_channel];
            }
            total = MultiplyByQuantizedMultiplier (total, Conv_quant_output_multiplier[out_channel], Conv_quant_output_shift[out_channel]) + output_offset;


            activation_output = (total < output_activation_min) ? output_activation_min :
                (output_activation_max < total) ? output_activation_max : total;
#else
#if HYBRID_LOGARITHMIC
          if (bias_data_enable)
          {
              Total_magnitude += Custom_denormalize (Conv_bias[out_channel]);
          }

          Total_magnitude = ActivationFunctionWithMinMaxMagnitude (Total_magnitude,
                                                            Activation_min_magnitude,
                                                            Activation_max_magnitude);

          activation_output = Float_normalize (Total_magnitude);
#else
          BiasFormat bias_value = 0.0f;
          if (bias_data_enable)
          {
            bias_value = Conv_bias[out_channel];
          }
          total += bias_value;

          activation_output = ActivationFunctionWithMinMax (total,
                                                            output_activation_min,
                                                            output_activation_max);
#endif
#endif
          StreamPeripheral_output (stream_out, activation_output);
        }
      }
    }
  }

  return 0;
}

inline void Convolution_loadProfile (hls::stream<StreamChannel> &stream_in,
                                     ConvProfile  & Conv_profile)
{
  Data temp;

  // stride_
  Conv_profile.parameters_.stride_.height_   = stream_in.read ().data;
  Conv_profile.parameters_.stride_.width_    = stream_in.read ().data;

  // dilation_
  Conv_profile.parameters_.dilation_.height_ = stream_in.read ().data;
  Conv_profile.parameters_.dilation_.width_  = stream_in.read ().data;

  // padding_
  Conv_profile.parameters_.padding_.height_  = stream_in.read ().data;
  Conv_profile.parameters_.padding_.width_   = stream_in.read ().data;

  // offset_
  Conv_profile.parameters_.offset_.input_   = stream_in.read ().data;
  Conv_profile.parameters_.offset_.filter_  = stream_in.read ().data;
  Conv_profile.parameters_.offset_.output_  = stream_in.read ().data;

  // activation_
  temp.u32 = stream_in.read ().data;
  Conv_profile.parameters_.activation_.max_ = temp.f32;
  temp.u32 = stream_in.read ().data;
  Conv_profile.parameters_.activation_.min_  = temp.f32;

  // quantized_activation_
  Conv_profile.parameters_.quantized_activation_.max_ = stream_in.read ().data;
  Conv_profile.parameters_.quantized_activation_.min_  = stream_in.read ().data;

  // depth_multiplier_ for DEPTHWISE_CONV_2D and OperatorType
  Conv_profile.parameters_.depth_multiplier_ = stream_in.read ().data;
  *((uint32_t*) &Conv_profile.parameters_.type_) = stream_in.read ().data;

  // input_shape_
  Conv_profile.input_shape_.dims_[0] = stream_in.read ().data;
  Conv_profile.input_shape_.dims_[1] = stream_in.read ().data;

  Conv_profile.input_shape_.dims_[2] = stream_in.read ().data;
  Conv_profile.input_shape_.dims_[3] = stream_in.read ().data;

  // filter_shape_
  Conv_profile.filter_shape_.dims_[0] = stream_in.read ().data;
  Conv_profile.filter_shape_.dims_[1] = stream_in.read ().data;

  Conv_profile.filter_shape_.dims_[2] = stream_in.read ().data;
  Conv_profile.filter_shape_.dims_[3] = stream_in.read ().data;

  // bias_shape_
  Conv_profile.bias_shape_.dims_[0] = stream_in.read ().data;
  Conv_profile.bias_shape_.dims_[1] = stream_in.read ().data;

  Conv_profile.bias_shape_.dims_[2] = stream_in.read ().data;
  Conv_profile.bias_shape_.dims_[3] = stream_in.read ().data;

  // output_shape_
  Conv_profile.output_shape_.dims_[0] = stream_in.read ().data;
  Conv_profile.output_shape_.dims_[1] = stream_in.read ().data;

  Conv_profile.output_shape_.dims_[2] = stream_in.read ().data;
  Conv_profile.output_shape_.dims_[3] = stream_in.read ().data;
}

#if FIXED_POINT

template <typename T>
inline void Convolution_loadTensor (hls::stream<StreamChannel> &stream_in,
                                    TensorShape * tensorShape,
                                    T * tensor)
{
  ap_int<DMA_CHANNEL_WIDTH> data;
  volatile int tensor_size = FlatSize(tensorShape);
  const int data_ratio = (DMA_CHANNEL_WIDTH / (8 * sizeof(T)));

  // CONV_LOAD_FILTER_LOOP
  LOAD_TENSOR_INT8_LOOP: for (int i = 0; i < tensor_size; i += data_ratio)
  {
#pragma HLS pipeline
    data = stream_in.read ().data;

    for (int j = 0; j < data_ratio; j++)
    {
#pragma HLS unroll
      tensor[i + j] = data >> (8 * j);
    }
  }
}

#else

inline void Convolution_loadTensor (hls::stream<StreamChannel> &stream_in,
                                    TensorShape * tensorShape,
                                    CustomFormat * tensor)
{
#if HYBRID_LOGARITHMIC
  SignFormat         sign[DMA_CHANNEL_FLOAT_WIDTH];
  ExponentFormat     exponent[DMA_CHANNEL_FLOAT_WIDTH];
  MagnitudeFormat    mantissa[DMA_CHANNEL_FLOAT_WIDTH];
#endif

  Data temp[DMA_CHANNEL_FLOAT_WIDTH];

  volatile int tensor_size = FlatSize (tensorShape);

  // CONV_LOAD_FILTER_LOOP
  CONV_LOAD_TENSOR_LOOP: for (int i = 0; i < tensor_size; i += DMA_CHANNEL_FLOAT_WIDTH)
  {
#pragma HLS pipeline
    channel_in = stream_in.read ();

    for (int j = 0; j < DMA_CHANNEL_FLOAT_WIDTH; j++)
    {
#pragma HLS unroll
      temp[j].u32 = channel_in.data >> (8 * sizeof(float) * j);

#if HYBRID_LOGARITHMIC
      sign[j] = DATA32_GET_SIGN(temp[j].u32);
      exponent[j] = DATA32_GET_EXPONENT(temp[j].u32);
      mantissa[j] = DATA32_GET_MANTISSA(temp[j].u32);

#if CORRECTION
#if CUSTOM_MANTISSA_BIT_WIDTH == 0
      if (0x400000 < (0x7FFFFF & mantissa[j]))
      {
        exponent[j]++;
      }
#else
      if ((0x400000 >> CUSTOM_MANTISSA_BIT_WIDTH) < ((0x7FFFFF >> CUSTOM_MANTISSA_BIT_WIDTH) & mantissa[j]))
      {
        MagnitudeFormat mantissa_temp = (0x7FFFFF & mantissa[j]) + (0x400000 >> CUSTOM_MANTISSA_BIT_WIDTH);

        if (0x800000 & mantissa_temp)
        {
          exponent[j]++;
        }
        mantissa[j] = mantissa_temp;
      }
#endif
#endif

      if (exponent[j] < - ((1 << (CUSTOM_EXPONENT_BIT_WIDTH - CUSTOM_EXPONENT_SIGN_BIT)) - 1))
      {
        tensor[i + j] = 0;
      }
      else if (exponent[j] > ((1 << (CUSTOM_EXPONENT_BIT_WIDTH - CUSTOM_EXPONENT_SIGN_BIT)) - 1))
      {
        tensor[i + j] = BUILD_CUSTOM(sign[j], ((1 << (CUSTOM_EXPONENT_BIT_WIDTH - CUSTOM_EXPONENT_SIGN_BIT)) - 1), 0xFFFFFFFF);
      }
      else
      {
#if CUSTOM_EXPONENT_SIGN_BIT
        tensor[i + j] = BUILD_CUSTOM(sign[j], exponent[j], mantissa[j]);
#else
        tensor[i + j] = BUILD_CUSTOM(sign[j], -exponent[j], mantissa[j]);
#endif
      }

#else
      tensor[i + j] = temp[j].f32;
#endif
    }
  }
}

#endif

inline void DepthwiseConv (hls::stream<StreamChannel> &stream_in,
                           hls::stream<StreamChannel> &stream_out,
                           ConvProfile & Conv_profile,
                           CustomFormat * Conv_filter,
                           BiasFormat * Conv_bias
#if FIXED_POINT
                           ,
                           BiasFormat * Conv_quant_output_multiplier,
                           BiasFormat * Conv_quant_output_shift
#endif
                            )
{
  const int stride_height = Conv_profile.parameters_.stride_.height_;
  const int stride_width = Conv_profile.parameters_.stride_.width_;

  const int dilation_height_factor = Conv_profile.parameters_.dilation_.height_;
  const int dilation_width_factor = Conv_profile.parameters_.dilation_.height_;

  const int pad_height = Conv_profile.parameters_.padding_.height_;
  const int pad_width = Conv_profile.parameters_.padding_.width_;

  const int depth_multiplier = Conv_profile.parameters_.depth_multiplier_;

#if FIXED_POINT
  const int output_activation_max = Conv_profile.parameters_.quantized_activation_.max_;
  const int output_activation_min = Conv_profile.parameters_.quantized_activation_.min_;
#else
  const float output_activation_max = Conv_profile.parameters_.activation_.max_;
  const float output_activation_min = Conv_profile.parameters_.activation_.min_;
#endif

  TensorShape & input_shape = Conv_profile.input_shape_;
  const int input_height = input_shape.dims_[1];
  const int input_width = input_shape.dims_[2];
  const int input_depth = input_shape.dims_[3];

  TensorShape & filter_shape = Conv_profile.filter_shape_;
#if DYNAMIC_FILTER_SHAPE
  volatile int filter_height = filter_shape.dims_[1];
  volatile int filter_width = filter_shape.dims_[2];
#else
  const int filter_height = FILTER_HEIGHT;
  const int filter_width = FILTER_WIDTH;
#endif

  TensorShape & output_shape = Conv_profile.output_shape_;
  const int batches = output_shape.dims_[0];
  const int output_depth = output_shape.dims_[3];
  const int output_height = output_shape.dims_[1];
  const int output_width = output_shape.dims_[2];

  bool bias_data_enable = (Conv_profile.bias_shape_.dims_[0] == output_depth);
  AccumulatorFormat total = 0;
  BiasFormat bias_value = 0.0f;
  OutputFormat activation_output = 0;

#if FIXED_POINT
  const int32_t input_offset = Conv_profile.parameters_.offset_.input_;
  const int32_t output_offset = Conv_profile.parameters_.offset_.output_;
#endif

#if HYBRID_LOGARITHMIC
  MagnitudeFormat   Total_magnitude = 0;
  MagnitudeFormat   Activation_max_magnitude = Float_denormalize (output_activation_max);
  MagnitudeFormat   Activation_min_magnitude = Float_denormalize (output_activation_min);
#endif

#if DYNAMIC_FILTER_SHAPE
  StreamPeripheral_initialize (stream_in, &input_shape, &filter_shape, &output_shape);
#else
  StreamPeripheral_initialize (stream_in, &input_shape, &output_shape);
#endif

  for (int b = 0; b < batches; ++b)
  {
#pragma HLS pipeline
    for (int out_y = - pad_height; out_y < output_height - pad_height; ++out_y)
    {
#pragma HLS pipeline
      StreamPeripheral_pushRowBuffer(stream_in, out_y);

      for (int out_x = - pad_width; out_x < output_width - pad_width; ++out_x)
      {
#pragma HLS pipeline
        for (int ic = 0; ic < input_depth; ++ic)
        {
#pragma HLS pipeline
          for (int m = 0; m < depth_multiplier; m++)
          {
#pragma HLS pipeline
            const int oc = m + ic * depth_multiplier;
            total = 0;
#if HYBRID_LOGARITHMIC
            Total_magnitude = 0;
#endif
            for (int in_y = out_y; in_y < filter_height + out_y; ++in_y)
            {
#pragma HLS pipeline
              for (int in_x = out_x; in_x < filter_width + out_x; ++in_x)
              {
#pragma HLS pipeline
                // If the location is outside the bounds of the input image,
                // use zero as a default value.
                if ((in_x >= 0) && (in_x < input_width) && (in_y >= 0)
                    && (in_y < input_height))
                {
                  InputFormat input_value = StreamPeripheral_read (in_y, in_x, ic);

#if DYNAMIC_FILTER_SHAPE
                  CustomFormat filter_value = Conv_filter[Offset(&filter_shape,
                                                                 0, in_y - out_y,
                                                                 in_x - out_x, oc)];
#else
                CustomFormat filter_value = Conv_filter[((((0) * FILTER_HEIGHT + (in_y - out_y)) * FILTER_WIDTH + (in_x - out_x)) * (&filter_shape)->dims_[3] + (oc))];
#endif
#if FIXED_POINT
                  total += ((AccumulatorFormat) filter_value)
                        * (((AccumulatorFormat) input_value) + input_offset);
#else

#if HYBRID_LOGARITHMIC
                  DotProduct_logarithmic (Total_magnitude, input_value, filter_value);
#else
                  total += (input_value * filter_value);
#endif

#endif
                }
              }
            }
#if FIXED_POINT
            if (bias_data_enable)
            {
              total += Conv_bias[oc];
            }
            total = MultiplyByQuantizedMultiplier (total, Conv_quant_output_multiplier[oc], Conv_quant_output_shift[oc]) + output_offset;


            activation_output = (total < output_activation_min) ? output_activation_min :
                (output_activation_max < total) ? output_activation_max : total;
#else
#if HYBRID_LOGARITHMIC
          if (bias_data_enable)
          {
              Total_magnitude += Custom_denormalize (Conv_bias[oc]);
          }

          Total_magnitude = ActivationFunctionWithMinMaxMagnitude (Total_magnitude,
                                                                   Activation_min_magnitude,
                                                                   Activation_max_magnitude);

          activation_output = Float_normalize (Total_magnitude);
#else
            bias_value = 0.0f;
            if (bias_data_enable) {
              bias_value = Conv_bias[oc];
            }
            activation_output = ActivationFunctionWithMinMax(total + bias_value,
                                                             output_activation_min,
                                                             output_activation_max);
#endif
#endif
            StreamPeripheral_output (stream_out, activation_output);
          }
        }
      }
    }
  }
}

int conv (ConvExecutionMode mode,
          hls::stream<StreamChannel> &stream_in,
          hls::stream<StreamChannel> &stream_out,
          int * debug)
{
#pragma HLS INTERFACE s_axilite port=mode  bundle=CRTL_BUS
#pragma HLS INTERFACE s_axilite port=debug bundle=CRTL_BUS

#pragma HLS INTERFACE axis      port=stream_in
#pragma HLS INTERFACE axis      port=stream_out

#pragma HLS INTERFACE s_axilite port=return bundle=CRTL_BUS

  ///////////////////////////////////////////////////////////////////////////////
  static ConvProfile  Conv_profile;
  static CustomFormat Conv_filter[CONV_FILTER_BUFFER_SIZE];
  static BiasFormat   Conv_bias[CONV_BIAS_BUFFER_SIZE];
#if FIXED_POINT
  static BiasFormat   Conv_quant_output_multiplier[CONV_BIAS_BUFFER_SIZE];
  static BiasFormat   Conv_quant_output_shift[CONV_BIAS_BUFFER_SIZE];
#endif
  ///////////////////////////////////////////////////////////////////////////////

//#pragma HLS DATAFLOW
//#pragma HLS INLINE region // bring loops in sub-functions to this DATAFLOW region

  int rc = 0;
  channel_out.keep = -1;
  channel_out.strb = -1;

  switch (mode)
  {
    case CONV_SETUP:
      Convolution_loadProfile  (stream_in, Conv_profile);
      Convolution_loadTensor   (stream_in, &Conv_profile.filter_shape_, Conv_filter);
      Convolution_loadTensor   (stream_in, &Conv_profile.bias_shape_, Conv_bias);
#if FIXED_POINT
      Convolution_loadTensor   (stream_in, &Conv_profile.bias_shape_, Conv_quant_output_multiplier);
      Convolution_loadTensor   (stream_in, &Conv_profile.bias_shape_, Conv_quant_output_shift);
#endif
      break;
    case CONV_EXECUTION:
      switch(Conv_profile.parameters_.type_)
      {
        case CONV_2D:
#if FIXED_POINT
        Convolution_execution (stream_in,
                               stream_out,
                               Conv_profile,
                               Conv_filter,
                               Conv_bias,
                               Conv_quant_output_multiplier,
                               Conv_quant_output_shift);
#else
          Convolution_execution (stream_in, stream_out, Conv_profile, Conv_filter, Conv_bias);
#endif
          break;
#if DEPTHWISE_CONV_ENGINE
        case DEPTHWISE_CONV_2D:
#if FIXED_POINT
          DepthwiseConv (stream_in,
                         stream_out,
                         Conv_profile,
                         Conv_filter,
                         Conv_bias,
                         Conv_quant_output_multiplier,
                         Conv_quant_output_shift);
#else
          DepthwiseConv (stream_in, stream_out, Conv_profile, Conv_filter, Conv_bias);
#endif
          break;
#endif
        default:;
      }
      break;
    default:
      rc = -1;
  }

  channel_out.last = 0;

  return rc;
}

