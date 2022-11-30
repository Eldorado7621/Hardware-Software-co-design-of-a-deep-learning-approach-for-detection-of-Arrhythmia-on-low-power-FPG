#include "helper.h"
#include <iostream>
using namespace std;


void FFT0(int FFT_stage,int pass_check,int index_shift,int pass_shift,data_comp data_IN[PADDED_LENGTH], data_comp data_OUT[PADDED_LENGTH],data_comp W[PADDED_LENGTH/2]){

	int butterfly_span=0,butterfly_pass=0;
	FFT_label1: for (int i = 0; i < PADDED_LENGTH/2; i++) {
		int index = butterfly_span << index_shift;
		int Ulimit = butterfly_span + (butterfly_pass<<pass_shift);
		int Llimit = Ulimit + FFT_stage;
		data_comp Product = W[index] * data_IN[Llimit];//calculate the product
		data_OUT[Llimit] = data_IN[Ulimit]-Product;
		data_OUT[Ulimit] = data_IN[Ulimit]+Product;
		if (butterfly_span<FFT_stage-1){
			butterfly_span++;
		} else if (butterfly_pass<pass_check-1) {
			butterfly_span = 0;	butterfly_pass++;
		} else {
			butterfly_span = 0;	butterfly_pass=0;
		}
	}
}

void fft_sw(data_comp data_OUT_SW[PADDED_LENGTH], data_comp FFT_input[PADDED_LENGTH])
{
	//cout << "fft strted: "  << endl;

	static data_comp data_OUT0[PADDED_LENGTH];
	static data_comp data_OUT1[PADDED_LENGTH];
	static data_comp W[PADDED_LENGTH/2];
	int FFT_int_stage[PADDED_LENGTH];
	//calculate the bit reversal
	//#pragma HLS dataflow
	bit_reversal(FFT_input, data_OUT1,FFT_int_stage);

	//calculate the twiddle factor
	twiddle_factor(W);

	//calculate the intermediate FFT stages
	for (int i=0;i<NO_STAGES;i=i+2)
	{

		FFT0(FFT_int_stage[NO_STAGES-(i+1)],FFT_int_stage[i],NO_STAGES-(i+1),i+1,data_OUT1,data_OUT0,W);
		if(i+1<NO_STAGES)
		{
			FFT0(FFT_int_stage[NO_STAGES-(i+2)],FFT_int_stage[i+1],NO_STAGES-(i+2),i+2,data_OUT0,data_OUT1,W);
		}

	}
	if(NO_STAGES%2==0)
	{
		for (int i=0;i<PADDED_LENGTH;i++)
		{
			data_OUT_SW[i]=data_OUT1[i];
		}
	}
	else
	{
		for (int i=0;i<PADDED_LENGTH;i++)
		{
			data_OUT_SW[i]=data_OUT0[i];
		}
	}
	//cout << "fft ended: "  << endl;

}


void ifft_sw(float data_OUT_SW[SCALES][PADDED_LENGTH/2], data_comp IFFT_input[PADDED_LENGTH],const int output_row)
{

	static data_comp data_OUT0[PADDED_LENGTH];
	static data_comp W[PADDED_LENGTH/2];
	int FFT_int_stage[PADDED_LENGTH];
	static data_comp data_OUT1[PADDED_LENGTH];

	bit_reversal(IFFT_input, data_OUT1,FFT_int_stage);

	twiddle_factor_ifft(W);

	//calculate the intermediate FFT stages
	for (int i=0;i<NO_STAGES;i=i+2)
	{

		FFT0(FFT_int_stage[NO_STAGES-(i+1)],FFT_int_stage[i],NO_STAGES-(i+1),i+1,data_OUT1,data_OUT0,W);
		if(i+1<NO_STAGES)
		{
			FFT0(FFT_int_stage[NO_STAGES-(i+2)],FFT_int_stage[i+1],NO_STAGES-(i+2),i+2,data_OUT0,data_OUT1,W);
		}

	}
	if(NO_STAGES%2==0)
	{
		for (int i=0;i<PADDED_LENGTH/2;i++)
		{
			data_OUT_SW[output_row][i]=real(data_OUT1[i+(PADDED_LENGTH/4)])/PADDED_LENGTH;
		}
	}
	else
	{
		for (int i=0;i<PADDED_LENGTH/2;i++)
		{
			data_OUT_SW[output_row][i]=real(data_OUT0[i+(PADDED_LENGTH/4)])/PADDED_LENGTH;

		}
	}


}


void ifft_sw_ss(float data_OUT_SW[PADDED_LENGTH/2], data_comp IFFT_input[PADDED_LENGTH])
{
	//cout << "ifft strted: "  << endl;
	static data_comp data_OUT0[PADDED_LENGTH];
	static data_comp W[PADDED_LENGTH/2];
	int FFT_int_stage[PADDED_LENGTH];
	static data_comp data_OUT1[PADDED_LENGTH];

	bit_reversal(IFFT_input, data_OUT1,FFT_int_stage);

	twiddle_factor_ifft(W);

	//calculate the intermediate FFT stages
	for (int i=0;i<NO_STAGES;i=i+2)
	{

		FFT0(FFT_int_stage[NO_STAGES-(i+1)],FFT_int_stage[i],NO_STAGES-(i+1),i+1,data_OUT1,data_OUT0,W);
		if(i+1<NO_STAGES)
		{
			FFT0(FFT_int_stage[NO_STAGES-(i+2)],FFT_int_stage[i+1],NO_STAGES-(i+2),i+2,data_OUT0,data_OUT1,W);
		}

	}
	if(NO_STAGES%2==0)
	{
		for (int i=0;i<PADDED_LENGTH/2;i++)
		{
			data_OUT_SW[i]=real(data_OUT1[i+(PADDED_LENGTH/4)])/PADDED_LENGTH;
		}
	}
	else
	{
		for (int i=0;i<PADDED_LENGTH/2;i++)
		{
			data_OUT_SW[i]=real(data_OUT0[i+(PADDED_LENGTH/4)])/PADDED_LENGTH;
		}
	}


}















