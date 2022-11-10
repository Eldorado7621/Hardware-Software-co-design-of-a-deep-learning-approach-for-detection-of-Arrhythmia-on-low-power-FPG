#include <stdio.h>      /* printf */
#include <math.h>
#include <algorithm>
#include "fftifft.h"
#include "cwt.h"


void wavelet_ft(float wft[SCALES][data_length*2],float wav_scales[SCALES])
{

	int padded_length=data_length*2;
	float omega[data_length*2];
#pragma HLS ARRAY_PARTITION variable=omega type=complete
	//sample the frequency vector of the fourier transform of the wavelet
	omega[0]=0;
	/*
       for (int i=1;i<padded_length;i++)
       {
          omega[i]=i*2*3.142/(padded_length);
       }
	 */
	for (int i=1;i<=data_length;i++)
	{
#pragma HLS UNROLL

		omega[i]=i*2*3.142/(padded_length);
	}
	for (int i=data_length+1;i<padded_length;i++)
	{
#pragma HLS PIPELINE
		omega[i]=-omega[padded_length-i];
	}

	//multiply the mother wavelet by scales
	for (int i=0;i<SCALES;i++)
	{
		for (int j=0;j<padded_length;j++)
		{      //multiply the scales by the daughter wavelet
			//the daughter wavelet is obtained by multiplying the mother wavelet and a scale factor
			wft[i][j]=74.61*pow(wav_scales[i],0.5)*exp(-(pow(6+omega[j],2))/2);
		}
	}
}

void getScales(float wav_scales[SCALES])
{
	int nv=10;
	float ds=1/nv;
	float a0=pow(2,ds);
	for(int i=0;i<=SCALES;i++)
	{
		wav_scales[i]=2*pow(a0,i);
	}

}

//int cwt(float data_in[data_length],float cwt_output[SCALES][data_length])
int cwt(hls::stream<dataFmt> &dataInStream,hls::stream<dataFmt> &cwtOuptutStream)
{

	//#pragma HLS INTERFACE ap_ctrl_none port=return
#pragma HLS INTERFACE axis register both port=dataInStream
#pragma HLS INTERFACE axis register both port=cwtOuptutStream
#pragma HLS INTERFACE s_axilite port=return bundle=CTRL_BUS

	dataFmt input_stream,output_stream;
	ecgDatatype data_in[data_length],cwt_output[SCALES][data_length],cwt_output_ss[data_length];
	//cout << "cwt strted: "  << endl;
	float sum=0.0;
	float wav_scales[SCALES];
	data_comp fft_output[2*data_length];
	float dataOut[SCALES][data_length];
	float wft[SCALES][data_length*2];

	//data_comp fft_mul[SCALES][data_length*2];
	data_comp fft_mul2[data_length*2];
	float padded_array2[2*data_length]={0};

	data_comp padded_array[2*data_length]={0};
	fpint idata;
	fpint odata;

	//#pragma HLS ARRAY_PARTITION variable=wft type=complete
	//#pragma HLS ARRAY_PARTITION variable=fft_output type=complete

	//read the data and normalize the data by dividing by the mean
	for(int i=0;i<data_length;i++)
	{
#pragma HLS PIPELINE

	    input_stream=dataInStream.read();
   	    idata.ival=input_stream.data;
	    data_in[i]=idata.fval;
        sum += data_in[i];

	}
	float average=sum/data_length;

	for(int i=0;i<data_length;i++)
	{
		data_in[i] = data_in[i]-average;
	}

	//pad the data symmetrically such that the new array is pad_aary[padindex datalenght padindex]
	//the padded values are reversed of the data from index zero to padindex
	int padindex=data_length/2;
	//move the data to the new array starting from index padindex


	for(int i=0;i<data_length;i++)
	{
#pragma HLS PIPELINE
		padded_array[padindex+i] = data_in[i];
		if (i<padindex)
		{

			padded_array[i] = data_in[padindex-i-1];
			padded_array[data_length+padindex+i] = data_in[data_length-i-1];
		}
	}

	//cout << "padding ended: "  << endl;
	//calculate the scales
	float s0=2.0/SAMPLING_FREQUENCY;
	float a0=pow(2.0, 0.1);

	for (int i=0; i<=SCALES;i++)
	{
#pragma HLS UNROLL
		wav_scales[i]=s0*pow(a0,i);
	}


	//cout << "fft strted: "  << endl;
	//compute the fft of the signal
	fft_sw(fft_output,padded_array);



	//cout << "ft ended: "  << endl;
	wavelet_ft(wft, wav_scales);

	//cout << "wavfft ends: "  << endl;
	//cwt
	//first multiply the FT of the wavelet and the FT of the signal
	for (int i=0;i<SCALES;i++)
	{

		for (int j=0;j<data_length*2;j++)
		{
#pragma HLS PIPELINE
			//#pragma HLS UNROLL
			fft_mul2[j]=fft_output[j]*wft[i][j];
		}
		//ifft_sw(cwt_output, fft_mul2,i);
		ifft_sw_ss(cwt_output_ss, fft_mul2);

		for (int col=0;col<data_length;col++)
		{
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










