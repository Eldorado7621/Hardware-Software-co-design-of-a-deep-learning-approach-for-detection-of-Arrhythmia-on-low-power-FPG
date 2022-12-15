#include <stdio.h>      /* printf */
#include <math.h>
#include <algorithm>

#include "fftifft.h"
#include "cwt.h"
#include "datasw.h"

void wavelet_ft(float wft[PADDED_LENGTH],float wav_scale,float omega[PADDED_LENGTH])
{
	//multiply the mother wavelet by scales
	int gc=6;
	int mul=2;


		for (int j=0;j<PADDED_LENGTH;j++)
		{      //multiply the scales by the daughter wavelet
			//the daughter wavelet is obtained by multiplying the mother wavelet and a scale factor
			//wft[j]=74.61*pow(wav_scales,0.5)*exp(-(pow(6+omega[j],2))/2);
			if(omega[j]>0)
				wft[j]=mul*exp(-(pow((wav_scale*omega[j]-gc),2)/2));
			else
				wft[j]=0.0;
		}

}

void cal_omega(float omega[PADDED_LENGTH])
{

#pragma HLS ARRAY_PARTITION variable=omega type=complete
	//sample the frequency vector of the fourier transform of the wavelet
	omega[0]=0;

	for (int i=1;i<=data_length;i++)
	{
#pragma HLS UNROLL

		omega[i]=i*2*3.142/(PADDED_LENGTH);
	}
	for (int i=data_length+1;i<PADDED_LENGTH;i++)
	{
#pragma HLS PIPELINE
		omega[i]=-omega[PADDED_LENGTH-i];
	}


}


//int cwt(float data_in[data_length],float cwt_output[SCALES][data_length])
void  cwt_sw(float cwt_output[SCALESS][data_lengtth])
{
	//cwt_output[SCALES][data_length],
	//cout << "cwt strted: "  << endl;
	float sum=0.0;
	float wav_scales[SCALES];
	data_comp fft_output[PADDED_LENGTH];
	float wft[PADDED_LENGTH];
	int padindex=data_length/2;
	float omega[PADDED_LENGTH]={0};

	//data_comp fft_mul[SCALES][data_length*2];
	data_comp fft_mul2[PADDED_LENGTH];
	data_comp padded_array[PADDED_LENGTH]={0};

	for(int i=0;i<data_length;i++)
	{
   	    padded_array[padindex+i]=dataIn[i];
	}


	int i=0;
	while(i<padindex)
	{
		padded_array[i] =padded_array[data_length-i-1];
		padded_array[data_length+padindex+i] = padded_array[padindex+data_length-i-1];
		i++;
	}
	/*for (int i=0; i<data_length*2;i++)
		cout << "ivff: " << padded_array[i]<< endl;*/

	//calculate the scales
	//float s0=2.0/data_length;
	float s0=2.0;
	float a0=1.07177; //pow(2.0, 0.1);

	for (int i=0; i<=SCALES;i++)
	{
		wav_scales[i]=s0*pow(a0,i);
	}


	//cout << "fft strted: "  << endl;
	//compute the fft of the signal
	fft_sw(fft_output,padded_array);



	//cout << "ft ended: "  << endl;
	//wavelet_ft(wft, wav_scales);
	cal_omega(omega);

	//cout << "wavfft ends: "  << endl;
	//cwt
	//first multiply the FT of the wavelet and the FT of the signal

	for (int i=0;i<SCALES;i++)
	{

		wavelet_ft(wft, wav_scales[i],omega);
		for (int j=0;j<data_length*2;j++)
		{
			fft_mul2[j]=fft_output[j]*wft[j];

		}
		ifft_sw(cwt_output, fft_mul2,i);
		//ifft_sw_ss(cwt_output_ss, fft_mul2);


	}






}










