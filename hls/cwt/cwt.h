#ifndef __CWT_H__
#define __CWT_H__

#include <cmath>
#include <complex>
#include <iostream>

#include <ap_axi_sdata.h>
#include <ap_int.h>
#include <hls_stream.h>

using namespace std;

typedef float ecgDatatype;

typedef ap_axis<32,2,5,6> dataFmt;

struct axis_data{
	ecgDatatype data;
	ap_uint<1> last;

};

union fpint {
		int ival;		// integer alias
		float fval;		// floating-point alias
	};

//int cwt(float data[data_length],float cwt_output[SCALES][data_length]);

int cwt(hls::stream<dataFmt>&dataInStream,hls::stream<dataFmt>&cwtOuptutStream);


#endif
