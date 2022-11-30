#include "fftifft.h"

//this function computes the fourier transform of the wavelet
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
