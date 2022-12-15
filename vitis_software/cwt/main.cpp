#include <stdio.h>
#include <complex.h>
#include <math.h>
#include <xtime_l.h>
#include "xil_printf.h"
#include "xaxidma.h"
#include "xparameters.h"
#include "data.h"
#include "xcwt.h"

#define FFT_LENGTH 128

XAxiDma axiDma;
XCwt cwt;


int initDma()
{
	printf("Initializing DMA ... \n");
	XAxiDma_Config *DmaCfgPtr;
	DmaCfgPtr=XAxiDma_LookupConfig(XPAR_AXI_DMA_0_DEVICE_ID);

	if(!DmaCfgPtr)
	{
		xil_printf("no config found for %d\r\n",XPAR_AXI_DMA_0_DEVICE_ID);
		return XST_FAILURE;
	}
	int status=XAxiDma_CfgInitialize(&axiDma,DmaCfgPtr);

	if(status!=XST_SUCCESS)
	{
			printf("Error INITIALIAZING DMA \n");
			return XST_FAILURE;
		}
	XAxiDma_IntrDisable(&axiDma,XAXIDMA_IRQ_ALL_MASK,XAXIDMA_DEVICE_TO_DMA);
	XAxiDma_IntrDisable(&axiDma,XAXIDMA_IRQ_ALL_MASK,XAXIDMA_DMA_TO_DEVICE);
	return XST_SUCCESS;

}

int init_cwt()
{
	printf("Initializing CWT ... \n");

	XCwt_Config *cwt_config;

	cwt_config=XCwt_LookupConfig(XPAR_CWT_0_DEVICE_ID);
	if(cwt_config)
	{
		int status=XCwt_CfgInitialize(&cwt,cwt_config);
		if(status!=XST_SUCCESS)
		{
			printf("error Initializing CWT ... \n");
		}
	}
	return XST_SUCCESS;
}
u32 checkIdle(u32 baseAddress,u32 offset)
{
	u32 status;
	status=(XAxiDma_ReadReg(baseAddress, offset)) & XAXIDMA_IDLE_MASK;

	return status;

}

int main()
{
	print("entering main function\n\r");

	initDma();
	init_cwt();
	XCwt_Start(&cwt);

	//int RX_PNTR[N];
	float  RX_PNTR[SCALE][N];

	 //DATA TRANSFER
	int status_tf;

	//Dma transfer
	XTime hw_processor_start, hw_processor_stop;
	//software computation
	Xil_DCacheFlushRange((UINTPTR)RX_PNTR,(sizeof(int )*SCALE*N));
	Xil_DCacheFlushRange((UINTPTR)FFT_input,(sizeof(float )*N));


	XTime_GetTime(&hw_processor_start);

	// xil_printf("\nDMA status before transfer to device: %d",checkIdle(XPAR_AXI_DMA_0_BASEADDR,0x4));

	status_tf=XAxiDma_SimpleTransfer(&axiDma,(UINTPTR)FFT_input,(sizeof(float )*N),XAXIDMA_DMA_TO_DEVICE);
	if(status_tf!=XST_SUCCESS)
	{
		printf("WRITING DATA FROM DDR TO FIFO VIA DMA FAILED  \n");
	}
	status_tf= XAxiDma_SimpleTransfer(&axiDma,(UINTPTR)RX_PNTR,(sizeof(int )*SCALE*N),XAXIDMA_DEVICE_TO_DMA);
	if(status_tf!=XST_SUCCESS)
	{
		printf("\n WRITING DATA FROM IP TO DMA FAILED  \n");
	}

	// xil_printf("\nDMA status before transfer to device: %d",checkIdle(XPAR_AXI_DMA_0_BASEADDR,0x34));
	status_tf=(XAxiDma_ReadReg(XPAR_AXI_DMA_0_BASEADDR, 0x04)) & 0x00000002;
	while(status_tf != 0x00000002)
	{
		status_tf=(XAxiDma_ReadReg(XPAR_AXI_DMA_0_BASEADDR, 0x04)) & 0x00000002;
	}
	Xil_DCacheInvalidateRange((u32)RX_PNTR,SCALE*N*sizeof(float));
	while(!XCwt_IsDone(&cwt));
	XTime_GetTime(&hw_processor_stop);
	printf("calc complete \n");

	/*		 int status=checkIdle(XPAR_AXI_DMA_0_BASEADDR,0x4);
		  while(status!=2)
		  {
		 	status=checkIdle(XPAR_AXI_DMA_0_BASEADDR,0x4);
		 }

		 status=checkIdle(XPAR_AXI_DMA_0_BASEADDR,0x34);
		  while(status!=2)
		 	 {
		 		 status=checkIdle(XPAR_AXI_DMA_0_BASEADDR,0x34);
		 	}*/


	xil_printf("\n printing FFT output on windowed");
	float hw_processing_time=1000000.0*(hw_processor_stop-hw_processor_start)/(COUNTS_PER_SECOND);
	printf(" hardware processing time: %f\n",hw_processing_time);

	//print out the fft of the windowed signal
	int error_count=0;
	for (int row=0;row<SCALE;row++)
	{
		for(int col=0;col<N;col++)
		{
			if((expected_output[row][col]-RX_PNTR[row][col])>0.001)
			{
				printf("\n PL output: %f, expected output: %f  at index %d,%d \n",expected_output[row][col],
						RX_PNTR[row][col],row,col);
				error_count++;
			}

		}

	}
	printf("\n total error= %d",error_count);


    return 0;
}
