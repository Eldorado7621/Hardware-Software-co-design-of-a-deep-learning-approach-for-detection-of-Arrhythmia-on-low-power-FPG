#include <stdio.h>
#include <complex.h>
#include <math.h>
#include <xtime_l.h>
#include "xil_printf.h"
#include "xaxidma.h"
#include "xparameters.h"
#include "xcwt.h"
#include "data.h"


XAxiDma axiDma;
XCwt cwt;

#define MEM_BASE_ADDR 0x1000000
#define TX_BUFFER_BASE (MEM_BASE_ADDR + 0x00100000 )
#define RX_BUFFER_BASE (MEM_BASE_ADDR + 0x00300000 )
#define SCALE 32

//float  FFT_Input[N]

float rx[SCALE][N]={0};



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

	int *m_dma_buffer_TX=(int*) TX_BUFFER_BASE;
	int *m_dma_buffer_RX=(int*) RX_BUFFER_BASE;
	XTime hw_processor_start, hw_processor_stop;


	//DATA TRANSFER
	int status_tf;



	//printf("choose gain:");
	//scanf("%d",&gain);

	XCwt_Start(&cwt);

	Xil_DCacheFlushRange((u32)FFT_input,(sizeof(int )*N));
	Xil_DCacheFlushRange((u32)rx,(sizeof(int )*N*SCALE));

	XTime_GetTime(&hw_processor_start);
	status_tf=XAxiDma_SimpleTransfer(&axiDma,(u32)FFT_input,(sizeof(int )*N),XAXIDMA_DMA_TO_DEVICE);
	status_tf=XAxiDma_SimpleTransfer(&axiDma,(u32)rx,(sizeof(int )*N*SCALE),XAXIDMA_DEVICE_TO_DMA);

	while(XAxiDma_Busy(&axiDma,XAXIDMA_DEVICE_TO_DMA));
	XTime_GetTime(&hw_processor_stop);
	Xil_DCacheInvalidateRange((u32)rx,SCALE*N*sizeof(int));

	while(!XCwt_IsDone(&cwt));
	float hw_processing_time=1000000.0*(hw_processor_stop-hw_processor_start)/(COUNTS_PER_SECOND);
	printf(" hardware processing time: %f\n",hw_processing_time);

	for (int j=0;j<10;j++)
	{
		printf("\n id: %d input: %f, Pl output: %f \n",j, FFT_input[j],rx[0][j]);


	}





	return 0;
}
