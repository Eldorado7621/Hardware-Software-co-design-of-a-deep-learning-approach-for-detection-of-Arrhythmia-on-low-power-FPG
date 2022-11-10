#include <iostream>
#include <stdlib.h>
#include <fstream>
#include <complex>
#include "fftifft.h"
#include "cwt.h"

#define TOL 0.01
#define NORM(x) ((x) * (x)).real()
#define SCALES 32

int main()
{
	int result=0;

		dataFmt tb_input_stream,tb_output_stream;
		   hls::stream<dataFmt>tb_dataInStream,tb_dataOutStream;
		data_comp data_in[FFT_LENGTH];
		data_comp data_out[FFT_LENGTH];
		float ifft_out[FFT_LENGTH];
		float cwt_in[FFT_LENGTH/2];
		float cwt_output[SCALES][FFT_LENGTH/2];
		fpint t;
		data_comp exp_out[FFT_LENGTH];
		ifstream FFTfileIN("cwt_inp.txt");  //reading cwt input to the fft
/*		ifstream FFTfileIN("out_cpp.txt");
		ifstream FFTfileOUT("inp_cpp.txt");  //expected fft
		if (FFTfileIN.fail() || FFTfileOUT.fail()) {
			std::cerr << "Failed to open file." << endl;
			exit(-1);
		}*/
		float temp1,temp2,temp3,temp4;
/*		data_t temp1,temp2,temp3,temp4;

		for(int i=0; i<FFT_LENGTH; i++){
			FFTfileIN>>temp1>>temp2;
			data_in[i]=data_comp(temp1,temp2);
		}*/

		//______________________cwt__________________________


		for(int i=0; i<FFT_LENGTH/2; i++){
			FFTfileIN>>temp1;
			cwt_in[i]=temp1;
		}
		for(int i=0; i<FFT_LENGTH/2; i++){
			t.fval=cwt_in[i];
			tb_input_stream.data=t.ival;

			//tb_input_stream.data=cwt_in[i];

			if(i==(FFT_LENGTH/2)-1)
				tb_input_stream.last=1;
			else
				tb_input_stream.last=0;
			tb_dataInStream.write(tb_input_stream);

		}

		FFTfileIN.close();
	cwt(tb_dataInStream,tb_dataOutStream);

	    for (int row=0; row<SCALES;row++)
	    {
	    	for (int col=0;col<data_length;col++)
	    	{
	    		tb_output_stream=tb_dataOutStream.read();
	    		//cwt_output[row][col]=tb_output_stream.data;

	    		t.ival=tb_output_stream.data;
	    		cwt_output[row][col]=t.fval;

	    		//cwt_output[row][col]=tb_output_stream.data;
	    	}
	    }
		for(int k=0;k<FFT_LENGTH/2;k++){

			cout << "id:"<<k<< " tb- Output: " << cwt_output[0][k] << endl;


		}




	//----------block for fft test ----------------------------------
/*
		for(int j=0; j<FFT_LENGTH;j++){
			FFTfileOUT >> temp3 >> ws >> temp4;
			exp_out[j]=data_comp(temp3,temp4);
		}
		FFTfileOUT.close();
		for(int k=0;k<FFT_LENGTH;k++){
			data_t n = NORM(exp_out[k]-data_out[k]);
			cout << "Exp: " << exp_out[k] << " \t- Got: " << data_out[k] << " \t- Inp: " << data_in[k] << " \t- Norm: " << n << endl;
//			cout << data_out[k].real() * 256 << ", " << data_out[k].imag() * 256 << endl;
			if (n>TOL) {
				result ++;
			}

*/
		//----------block for ifft test ----------------------------------
/*		ifstream FFTfileIN("out_cpp.txt");
		ifstream FFTfileOUT("inp_cpp.txt");  //expected fft
		if (FFTfileIN.fail() || FFTfileOUT.fail()) {
			std::cerr << "Failed to open file." << endl;
			exit(-1);
		}



		for(int j=0; j<FFT_LENGTH;j++){
			FFTfileOUT >> temp3>> temp4;
			exp_out[j]=data_comp(temp3,temp4);
		}
		FFTfileOUT.close();
		for(int i=0; i<FFT_LENGTH; i++){
			FFTfileIN>>temp1>>temp2;
			data_in[i]=data_comp(temp1,temp2);
		}
		ifft_sw(ifft_out,data_in);
		for(int k=0;k<FFT_LENGTH;k++){
			data_t n = NORM(exp_out[k]-data_out[k]);
			cout << "Exp: " << exp_out[k] << " \t- Got: " << ifft_out[k] << " \t- Inp: " << data_in[k] << " \t- Norm: " << n << endl;

			if (n>TOL) {
				result ++;
			}
	}*/
	if (result == 0) {
		cout << "PASS" << endl;
	} else {
		cout << "FAIL: " << result << " errors" << endl;
	}
	return result;


}
