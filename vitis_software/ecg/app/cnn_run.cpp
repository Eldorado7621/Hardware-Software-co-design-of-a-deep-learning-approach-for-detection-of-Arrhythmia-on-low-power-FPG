/* Copyright 2019 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#include <TensorFlowLite.h>

#include "main_functions.h"
#include "detection_responder.h"
#include "image_provider.h"
#include "model_settings.h"
#include "person_detect_model_data.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/micro/system_setup.h"
#include "tensorflow/lite/schema/schema_generated.h"

#include "generate_spectogram.h"
#include "model.h"
#include "cwt_inp.h"
#include <xtime_l.h>

#include "xparameters.h"

////////////////////////////////////////////////////////////////////
// Xilinx libraries
#include "ff.h"
#include "xstatus.h"
#include "miscellaneous.h"
#include "cwt.h"
//#include "data.h"

// Globals, used for compatibility with Arduino-style sketches.
namespace {
tflite::ErrorReporter* error_reporter = nullptr;
const tflite::Model* model = nullptr;
tflite::MicroInterpreter* interpreter = nullptr;
TfLiteTensor* input = nullptr;

// In order to use optimized tensorflow lite kernels, a signed int8_t quantized
// model is preferred over the legacy unsigned model format. This means that
// throughout this project, input images must be converted from unisgned to
// signed format. The easiest and quickest way to convert from unsigned to
// signed 8-bit integers is to subtract 128 from the unsigned value to get a
// signed value.

// An area of memory to use for input, output, and intermediate arrays.
constexpr int kTensorArenaSize = 256 * 1024 * 1024;
static uint8_t tensor_arena[kTensorArenaSize];
}  // namespace

static FATFS fatfs;
static FRESULT File_initializeSD (void)
{
	TCHAR *path = "0:/"; /* Logical drive number is 0 */
	/* Register volume work area, initialize device */
	return f_mount (&fatfs, path, 0);
}

static FRESULT File_readData (const char * file_name, void * model, size_t model_size)
{
	FIL fil; /* File object */
	FRESULT rc;
	size_t read_result = 0;

	rc = f_open (&fil, file_name, FA_READ);
	ASSERT(rc == FR_OK);
	if (rc == FR_OK)
	{
		rc = f_read (&fil, model, model_size, &read_result);
		ASSERT(rc == FR_OK);

		rc = f_close (&fil);
		ASSERT(rc == FR_OK);
	}

	return rc;
}

static FRESULT File_writeData (const char * file_name, const void * data, size_t size)
{
	FIL fil; /* File object */
	FRESULT rc;
	size_t result = 0;

	rc = f_open (&fil, file_name, FA_WRITE | FA_CREATE_NEW);
	ASSERT(rc == FR_OK);
	if (rc == FR_OK)
	{
		rc = f_write (&fil, data, size, &result);
		ASSERT(rc == FR_OK);
		ASSERT(size == result);

		rc = f_close (&fil);
		ASSERT(rc == FR_OK);
	}

	return rc;
}


unsigned char model_data[4966272];

unsigned char labels[10000];

// The name of this function is important for Arduino compatibility.
void setup ()
{
	FRESULT rc;
	tflite::InitializeTarget ();

	rc = File_initializeSD ();
	ASSERT(rc == FR_OK);

	//rc = File_readData ("sconvi8", model_data, 1168880);
	//rc = File_readData ("vgg6_f32", model_data, 2207464); // [Acc 71.8%, 143] [Acc 70.6%, 152c]
	//rc = File_readData ("vgg6_i8", model_data, 573792);
	//rc = File_readData ("PERSON", model_data, 300568);
	//rc = File_readData ("vgg4_f32", model_data, 3739612); // [Acc 63.09, 143] [Acc 46.51, 150]
	//rc = File_readData ("mobile5", model_data, 214588); // [Acc 31.44, 143] [Acc 66.67, 154] [Acc 12.58, 152]
	//rc = File_readData ("models/qvgg", model_data, 2486324); // Logarithmic quantization *************

	//rc = File_readData ("models/mob_f32", model_data, 214648);
	//rc = File_readData ("models/mob_i8",  model_data,  65656);

	//rc = File_readData ("models/vgg_f32", model_data, 3739612);
	//rc = File_readData ("models/vgg_i8",  model_data,  958768);

	//rc = File_readData ("models/qvgg_q0", model_data, 1377772);
	//rc = File_readData ("models/qvgg_q1", model_data, 1377772);
	//rc = File_readData ("models/qvgg_q2", model_data, 1377772);
	//rc = File_readData ("models/tvgg_nq", model_data, 1377772);
	//rc = File_readData ("models/tvgg_nq2", model_data, 2127696);
	//rc = File_readData ("models/vgg_m2", model_data, 2127696);
	//rc = File_readData ("models/tvgg_n", model_data, 1167348);
	//rc = File_readData ("models/tvggn60", model_data, 1457320);
	//rc = File_readData ("models/t2vggn60", model_data, 4352316);
	//rc = File_readData ("models/t2vggn60", model_data, 1375660);
	//rc = File_readData ("models/cnn_f32", model_data, 1284928);
	//rc = File_readData ("models/scnn_f8", model_data, 979748);
	//rc = File_readData ("models/scnn_l5", model_data, 979748);
	//rc = File_readData ("models/cnn_log4", model_data, 1284928);
	//rc = File_readData ("models/d_cnn", model_data, 621856);
	//rc = File_readData ("models/vgg120", model_data, 1114844);

	//rc = File_readData ("models/CNN.cc", model_data, 434472);

	//rc = File_readData ("models/SCNN", model_data, 129292);

	ASSERT(rc == FR_OK);

	// Set up logging. Google style is to avoid globals or statics because of
	// lifetime uncertainty, but since this has a trivial destructor it's okay.
	// NOLINTNEXTLINE(runtime-global-variables)
	static tflite::MicroErrorReporter micro_error_reporter;
	error_reporter = &micro_error_reporter;

	// Map the model into a usable data structure. This doesn't involve any
	// copying or parsing, it's a very lightweight operation.
	//model = tflite::GetModel (model_data);

	model = tflite::GetModel (ecgFloatn32modelNQ_tflite);


	if (model->version () != TFLITE_SCHEMA_VERSION)
	{
		TF_LITE_REPORT_ERROR(error_reporter,
				"Model provided is schema version %d not equal "
				"to supported version . %d.",
				model->version (), TFLITE_SCHEMA_VERSION);
		return;
	}

	static tflite::AllOpsResolver micro_op_resolver;

	// Build an interpreter to run the model with.
	// NOLINTNEXTLINE(runtime-global-variables)
	static tflite::MicroInterpreter static_interpreter (model, micro_op_resolver,
			tensor_arena,
			kTensorArenaSize,
			error_reporter);
	interpreter = &static_interpreter;

	// Allocate memory from the tensor_arena for the model's tensors.
	TfLiteStatus allocate_status = interpreter->AllocateTensors ();
	if (allocate_status != kTfLiteOk)
	{
		TF_LITE_REPORT_ERROR(error_reporter, "AllocateTensors() failed");
		return;
	}

	interpreter->enable_delegate(false); //////////////////////////////

	// Get information about the memory area to use for the model's input.
	input = interpreter->input (0);

	TF_LITE_REPORT_ERROR(error_reporter, "input->dims->size = %d",
			input->dims->size);
	for (int i = 0; i < input->dims->size; i++)
	{
		TF_LITE_REPORT_ERROR(error_reporter, "input->dims->data[%d] = %d", i,
				input->dims->data[i]);
	}
	TF_LITE_REPORT_ERROR(error_reporter, "input->type = 0x%d", input->type);

	//////////////////////////////////////////////////////////////////////////////////
	TfLiteTensor* output = interpreter->output (0);

	TF_LITE_REPORT_ERROR(error_reporter, "output->dims->size = %d",
			output->dims->size);
	for (int i = 0; i < output->dims->size; i++)
	{
		TF_LITE_REPORT_ERROR(error_reporter, "output->dims->data[%d] = %d", i,
				output->dims->data[i]);
	}
	TF_LITE_REPORT_ERROR(error_reporter, "input->type = 0x%d", output->type);

	// rc = File_readData ("/CIFAR/labels", labels, 10000);
}


// The name of this function is important for Arduino compatibility.
void loop ()
{
	char img_name[32] = { 0 };
	float cwt_output[SCALE][DATA_LENGTH]={0};
	XTime hw_processor_start, hw_processor_stop,hw_processor_start1, hw_processor_stop1;
	FRESULT rc;
	int status_cwt;

	TfLiteStatus status;
	static int image_index = 0;

	/*
  sprintf(img_name, "/CIFAR/%d", image_index);

  rc = File_readData (img_name, input->data.data, 12288);
  ASSERT(rc == FR_OK);
	 */

	//input->data.data=cwt_input_12;



	//memcpy (input->data.data, cwt_input, sizeof(cwt_input));

	//get spectogram software

	XTime_GetTime(&hw_processor_start);
	cwt_sw( cwt_output);
	XTime_GetTime(&hw_processor_stop);


/*	XTime_GetTime(&hw_processor_start);
	status_cwt=generate_spectogram( cwt_output);
	XTime_GetTime(&hw_processor_stop);

	if(status_cwt!=0)
	{
		TF_LITE_REPORT_ERROR(error_reporter, "cwt operation failed.");
	}*/

	memcpy (input->data.data, cwt_output, sizeof(cwt_output));

	if (status != kTfLiteOk)
	{
		TF_LITE_REPORT_ERROR(error_reporter, "Image capture failed.");
	}
	XTime_GetTime(&hw_processor_start1);

	status = interpreter->Invoke ();

	XTime_GetTime(&hw_processor_stop1);
	if (status != kTfLiteOk)
	{
		TF_LITE_REPORT_ERROR(error_reporter, "Invoke failed.");
	}

	float hw_processing_time=1000000.0*(hw_processor_stop-hw_processor_start)/(COUNTS_PER_SECOND);
	float hw_processing_time1=1000000.0*(hw_processor_stop1-hw_processor_start1)/(COUNTS_PER_SECOND);

	interpreter->get_eventLog ();

	TfLiteTensor* output = interpreter->output (0);

	int index[3] = { 0 };

	float temp = 0;
	for (int i = 0; i < output->dims->data[1]; i++)
	{
		TF_LITE_REPORT_ERROR(error_reporter, "score = %f",
				output->data.f[i]);
		if (temp < output->data.f[i])
		{
			temp = output->data.f[i];
			index[2] = index[1];
			index[1] = index[0];
			index[0] = i;
		}
	}
	printf ("\Time: %f \n ",hw_processing_time1);

	TF_LITE_REPORT_ERROR(error_reporter, "predicted output = %d",
			index[0]);

	//RespondToDetection (error_reporter, output, labels[image_index]);

	/*  //image_index++;

  if (image_index == 10000)
  {
    printf ("\nDone!\n");
    while (true)
      ;
  }*/
}
