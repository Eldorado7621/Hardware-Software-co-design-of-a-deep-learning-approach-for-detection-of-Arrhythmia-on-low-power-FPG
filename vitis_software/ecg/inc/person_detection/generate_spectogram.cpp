#include "xcwt.h"
#include "xil_printf.h"
#include "xaxidma.h"
#include "xparameters.h"
#include "data.h"
#include "generate_spectogram.h"



XAxiDma axiDma;
XCwt cwt;

int initDma()
{
	//printf("Initializing DMA ... \n");
	XAxiDma_Config *DmaCfgPtr;
	DmaCfgPtr=XAxiDma_LookupConfig(XPAR_AXI_DMA_1_DEVICE_ID);

	if(!DmaCfgPtr)
	{
		xil_printf("no config found for %d\r\n",XPAR_AXI_DMA_1_DEVICE_ID);
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
	//printf("Initializing CWT ... \n");

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

int generate_spectogram(float cwt_output[SCALE][DATA_LENGTH])
{
	//print("entering generating spectogram function\n\r");

	initDma();
	init_cwt();
	XCwt_Start(&cwt);
	int status_tf;


	status_tf=XAxiDma_SimpleTransfer(&axiDma,(UINTPTR)input_data,(sizeof(float )*DATA_LENGTH),XAXIDMA_DMA_TO_DEVICE);
	if(status_tf!=XST_SUCCESS)
	{
		printf("WRITING DATA FROM DDR TO FIFO VIA DMA FAILED  \n");
	}
	status_tf= XAxiDma_SimpleTransfer(&axiDma,(UINTPTR)cwt_output,(sizeof(int )*SCALE*DATA_LENGTH),XAXIDMA_DEVICE_TO_DMA);
	if(status_tf!=XST_SUCCESS)
	{
		printf("\n WRITING DATA FROM IP TO DMA FAILED  \n");
	}

	// xil_printf("\nDMA status before transfer to device: %d",checkIdle(XPAR_AXI_DMA_0_BASEADDR,0x34));
	//check the status reg of the dma
	status_tf=(XAxiDma_ReadReg(XPAR_AXI_DMA_1_BASEADDR, 0x04)) & 0x00000002;
	while(status_tf != 0x00000002)
	{
		status_tf=(XAxiDma_ReadReg(XPAR_AXI_DMA_1_BASEADDR, 0x04)) & 0x00000002;
	}
	Xil_DCacheInvalidateRange((u32)cwt_output,SCALE*DATA_LENGTH*sizeof(float));
	while(!XCwt_IsDone(&cwt));

	return 0;

}
