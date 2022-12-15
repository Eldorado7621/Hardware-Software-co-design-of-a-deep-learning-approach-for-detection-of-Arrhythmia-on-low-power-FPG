/*
 * conv_vtbl.c
 *
 *  Created on: July 31st, 2021
 *      Author: Yarib Nevarez
 */
/***************************** Include Files *********************************/
#include "conv_vtbl.h"
#include "stdio.h"
#include "stdlib.h"

#include "miscellaneous.h"

/***************** Macros (Inline Functions) Definitions *********************/

/**************************** Type Definitions *******************************/

/************************** Constant Definitions *****************************/

/************************** Variable Definitions *****************************/

/************************** Function Prototypes ******************************/

/************************** Function Definitions******************************/

static void * hardware_new (void)
{
  return malloc (sizeof(XConv));
}

static void hardware_delete (void ** InstancePtr)
{
  if (InstancePtr && *InstancePtr)
  {
    free (*InstancePtr);
    *InstancePtr = NULL;
  }
}

static uint32_t  hardware_SetInterruptHandler (void *instance,
                                                             uint32_t ID,
                                                             ARM_GIC_InterruptHandler handler,
                                                             void * data)
{
  uint32_t status = ARM_GIC_connect (ID, handler, data);
  ASSERT (status == XST_SUCCESS);
  return status;
}

HardwareVtbl HardwareVtbl_Conv_ =
{
  .new_ =    hardware_new,
  .delete_ = hardware_delete,

  .Initialize =         (int (*)(void *, u16))  XConv_Initialize,
  .Start =              (void (*)(void *))      XConv_Start,
  .IsDone =             (uint32_t(*)(void *))   XConv_IsDone,
  .IsIdle =             (uint32_t(*) (void *))  XConv_IsIdle,
  .IsReady =            (uint32_t(*) (void *))  XConv_IsReady,
  .EnableAutoRestart =  (void (*) (void *))     XConv_EnableAutoRestart,
  .DisableAutoRestart = (void (*) (void *))     XConv_DisableAutoRestart,
  .Get_return =         (uint32_t(*) (void *))  NULL,

  .Set_mode =           (void (*) (void *, uint32_t )) XConv_Set_mode,
  .Get_mode =           (uint32_t (*) (void *))        XConv_Get_mode,
  .Get_debug =          (uint32_t (*) (void *))        XConv_Get_debug,

  .InterruptGlobalEnable =  (void (*) (void *))             XConv_InterruptGlobalEnable,
  .InterruptGlobalDisable = (void (*) (void *))             XConv_InterruptGlobalDisable,
  .InterruptEnable =        (void (*) (void *, uint32_t ))  XConv_InterruptEnable,
  .InterruptDisable =       (void (*) (void *, uint32_t ))  XConv_InterruptDisable,
  .InterruptClear =         (void (*) (void *, uint32_t ))  XConv_InterruptClear,
  .InterruptGetEnabled =    (uint32_t(*) (void *))          XConv_InterruptGetEnabled,
  .InterruptGetStatus =     (uint32_t(*) (void *))          XConv_InterruptGetStatus,

  .InterruptSetHandler = hardware_SetInterruptHandler
};
