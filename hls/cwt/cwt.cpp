#include <stdio.h>      /* printf */
#include <math.h>
#include <algorithm>
#include "cwt.h"
#include "wavelet.h"


/*
The project entails design an IP that computes the Continuous wavelet transform
using the morlet wavelet - The wavelet transform is computed by multiply the fourier
transform of the morlet wavelet and the fourier transform of the 1D time series
 signal and then converting it back to time domain by computing the inverse fourier
 transform*/

//This is the top function
int cwt(hls::stream<dataFmt> &dataInStream,hls::stream<dataFmt> &cwtOuptutStream)
{

#pragma HLS INTERFACE axis register both port=dataInStream
#pragma HLS INTERFACE axis register both port=cwtOuptutStream
#pragma HLS INTERFACE s_axilite port=return bundle=CTRL_BUS




	dataFmt input_stream,output_stream;
	ecgDatatype data_in[data_length],cwt_output_ss[data_length];
	data_comp fft_output[PADDED_LENGTH];

	float sum=0.0;
	float wav_scales[SCALES];

	float wft[PADDED_LENGTH];
	int padindex=data_length/2;
	float omega[PADDED_LENGTH]={0};
	data_comp fft_mul2[PADDED_LENGTH];
	data_comp padded_array[PADDED_LENGTH]={0};
	fpint idata,odata;


	for(int i=0;i<data_length;i++)
	{
#pragma HLS PIPELINE

		input_stream=dataInStream.read();
		idata.ival=input_stream.data;
		padded_array[padindex+i]=idata.fval;

	}

	int i=0;
	while(i<padindex)
	{
		padded_array[i] =padded_array[data_length-i-1];
		padded_array[data_length+padindex+i] = padded_array[padindex+data_length-i-1];
		i++;
	}

	float s0=2.0;
	float a0=1.07177; //pow(2.0, 0.1);

	for (int i=0; i<=SCALES;i++)
	{
#pragma HLS UNROLL
		wav_scales[i]=s0*pow(a0,i);
	}

	//compute the fft of the signal
	fft_sw(fft_output,padded_array);

	cal_omega(omega);

	for (int i=0;i<SCALES;i++)
	{
		/*calculate the wavelet for each scale,find the fourier transform
		  and then multiply it by the input signal in frequency domain*/
		wavelet_ft(wft, wav_scales[i],omega);
		for (int j=0;j<data_length*2;j++)
		{
			fft_mul2[j]=fft_output[j]*wft[j];

		}

		//calculate the IFFT of the product
		ifft_sw_ss(cwt_output_ss, fft_mul2);

		for (int col=0;col<data_length;col++)
		{
#pragma HLS UNROLL

			odata.fval=cwt_output_ss[col];
			output_stream.data=odata.ival;

			output_stream.keep=input_stream.keep;

			output_stream.strb=input_stream.strb;

			output_stream.user=input_stream.user;

			output_stream.id=input_stream.id;

			output_stream.dest=input_stream.dest;

			if((i==SCALES-1)&&(col==data_length-1))
				output_stream.last=1;
			else
				output_stream.last=0;

			cwtOuptutStream.write(output_stream);
		}
	}

	return 0;


}










