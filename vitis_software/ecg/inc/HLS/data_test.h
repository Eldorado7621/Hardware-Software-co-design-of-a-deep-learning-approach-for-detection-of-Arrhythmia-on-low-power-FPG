#ifndef DATA_TEST
#define DATA_TEST

namespace conv_test {
namespace float32 {
namespace conv_2d {
extern unsigned int transaction_setup [];
extern unsigned int transaction_setup_len;

extern unsigned int input_tensor [];
extern unsigned int input_tensor_len;

extern unsigned int output_tensor [];
extern unsigned int output_tensor_len;
} // conv_2d

namespace depthwise_conv_2d {
extern unsigned int transaction_setup [];
extern unsigned int transaction_setup_len;

extern unsigned int input_tensor [];
extern unsigned int input_tensor_len;

extern unsigned int output_tensor [];
extern unsigned int output_tensor_len;
} // depthwise_conv_2d
}

namespace int8 {
namespace conv_2d {
extern unsigned int transaction_setup [];
extern unsigned int transaction_setup_len;

extern unsigned int input_tensor [];
extern unsigned int input_tensor_len;

extern unsigned int output_tensor [];
extern unsigned int output_tensor_len;
} // conv_2d

namespace depthwise_conv_2d {
extern unsigned int transaction_setup [];
extern unsigned int transaction_setup_len;

extern unsigned int input_tensor [];
extern unsigned int input_tensor_len;

extern unsigned int output_tensor [];
extern unsigned int output_tensor_len;
} // depthwise_conv_2d
}
} // conv_test
#endif //DATA_TEST
