#ifndef __MAIN_H__
#define __MAIN_H__

#include <cmath>
#include <complex>

using namespace std;
#define PADDED_LENGTH 256
#define NO_STAGES 8

#define data_length 128
#define SCALES 64


typedef float data_t;
typedef complex<data_t> data_comp;

void fft_sw(data_comp data_IN[PADDED_LENGTH], data_comp data_OUT[PADDED_LENGTH]);
void ifft_sw(float data_OUT_SW[PADDED_LENGTH], data_comp IFFT_input[PADDED_LENGTH]);
void ifft_sw(float data_OUT_SW[SCALES][data_length], data_comp IFFT_input[PADDED_LENGTH],const int output_row);
void ifft_sw_ss(float data_OUT_SW[PADDED_LENGTH/2], data_comp IFFT_input[PADDED_LENGTH]);

//void wavelet_ft(float wft[SCALES][data_length*2],float wav_scales[SCALES]);
#endif
