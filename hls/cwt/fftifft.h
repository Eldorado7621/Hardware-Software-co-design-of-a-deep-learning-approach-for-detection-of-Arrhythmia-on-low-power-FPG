#ifndef __MAIN_H__
#define __MAIN_H__

#include <cmath>
#include <complex>

using namespace std;
#define FFT_LENGTH 32
#define NO_STAGES 5

#define data_length 16
#define SAMPLING_FREQUENCY 16
#define SCALES 32


typedef float data_t;
typedef complex<data_t> data_comp;

void fft_sw(data_comp data_IN[FFT_LENGTH], data_comp data_OUT[FFT_LENGTH]);
void ifft_sw(float data_OUT_SW[FFT_LENGTH], data_comp IFFT_input[FFT_LENGTH]);
void ifft_sw(float data_OUT_SW[SCALES][data_length], data_comp IFFT_input[FFT_LENGTH],const int output_row);
void ifft_sw_ss(float data_OUT_SW[FFT_LENGTH/2], data_comp IFFT_input[FFT_LENGTH]);

//void wavelet_ft(float wft[SCALES][data_length*2],float wav_scales[SCALES]);
#endif
