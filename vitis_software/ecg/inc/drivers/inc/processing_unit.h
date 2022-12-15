/*
 * processing_unit.h
 *
 *  Created on: July 31st, 2021
 *      Author: Yarib Nevarez
 */
#ifndef PROCESSING_UNIT_H_
#define PROCESSING_UNIT_H_

/***************************** Include Files *********************************/
#include <stdint.h>
#include <stddef.h>

#include "memory_manager.h"

#include "gic.h"

#include "xil_types.h"
#include "xstatus.h"
#include "xaxidma.h"
/***************** Macros (Inline Functions) Definitions *********************/


/**************************** Type Definitions *******************************/
typedef enum
{
  MEMORY_TO_HARDWARE = XAXIDMA_DMA_TO_DEVICE,
  HARDWARE_TO_MEMORY = XAXIDMA_DEVICE_TO_DMA
} DMATransferDirection;

typedef enum
{
  DMA_IRQ_IOC   = XAXIDMA_IRQ_IOC_MASK,   /* Completion */
  DMA_IRQ_DELAY = XAXIDMA_IRQ_DELAY_MASK, /* Delay      */
  DMA_IRQ_ERROR = XAXIDMA_IRQ_ERROR_MASK, /* Error      */
  DMA_IRQ_ALL   = XAXIDMA_IRQ_ALL_MASK    /* All        */
} DMAIRQMask;

typedef struct
{
  void *    (*new_)              (void);
  void      (*delete_)           (void ** instance_ptr);

  int       (*Initialize)       (void * instance, u16 deviceId);

  uint32_t  (*Move)             (void * instance,
                                 void * BuffAddr,
                                 uint32_t Length,
                                 DMATransferDirection direction);

  void      (*InterruptEnable)  (void * instance,
                                 DMAIRQMask mask,
                                 DMATransferDirection direction);

  void      (*InterruptDisable) (void * instance,
                                 DMAIRQMask mask,
                                 DMATransferDirection direction);

  void        (*InterruptClear)       (void * instance,
                                       DMAIRQMask mask,
                                       DMATransferDirection direction);

  DMAIRQMask  (*InterruptGetEnabled)  (void * instance,
                                       DMATransferDirection direction);

  DMAIRQMask  (*InterruptGetStatus)   (void * instance,
                                       DMATransferDirection direction);

  void        (*Reset)                (void * instance);

  int         (*ResetIsDone)          (void * instance);

  uint32_t    (*InterruptSetHandler)  (void *InstancePtr,
                                       uint32_t ID,
                                       ARM_GIC_InterruptHandler handler,
                                       void * data);
} DMAHardwareVtbl;


typedef enum
{
  HW_INITIALIZE,
  HW_INFERENCE
} HardwareMode;

typedef struct
{
  void *    (*new_)(void);
  void      (*delete_)(void ** InstancePtr);

  int       (*Initialize) (void *InstancePtr, u16 deviceId);
  void      (*Start)      (void *InstancePtr);
  uint32_t  (*IsDone)     (void *InstancePtr);
  uint32_t  (*IsIdle)     (void *InstancePtr);
  uint32_t  (*IsReady)    (void *InstancePtr);
  void      (*EnableAutoRestart) (void *InstancePtr);
  void      (*DisableAutoRestart) (void *InstancePtr);
  uint32_t  (*Get_return) (void *InstancePtr);

  void      (*Set_mode)    (void *InstancePtr, uint32_t Data);
  uint32_t  (*Get_mode)    (void *InstancePtr);
  uint32_t  (*Get_debug)   (void *InstancePtr);

  void      (*InterruptGlobalEnable)  (void *InstancePtr);
  void      (*InterruptGlobalDisable) (void *InstancePtr);
  void      (*InterruptEnable)        (void *InstancePtr, uint32_t Mask);
  void      (*InterruptDisable)       (void *InstancePtr, uint32_t Mask);
  void      (*InterruptClear)         (void *InstancePtr, uint32_t Mask);
  uint32_t  (*InterruptGetEnabled)    (void *InstancePtr);
  uint32_t  (*InterruptGetStatus)     (void *InstancePtr);

  uint32_t  (*InterruptSetHandler)    (void *InstancePtr,
                                       uint32_t ID,
                                       ARM_GIC_InterruptHandler handler,
                                       void * data);
} HardwareVtbl;

class ProcessingUnit
{
public:
  typedef struct
  {
    HardwareVtbl *      hwVtbl;
    void *              hwInstance;
    DMAHardwareVtbl *   dmaVtbl;
    void *              dmaInstance;
    uint32_t            hwDeviceID;
    uint32_t            dmaDeviceID;
    uint32_t            hwIntVecID;
    uint32_t            dmaTxIntVecID;
    uint32_t            dmaRxIntVecID;
    size_t              channelSize;
    MemoryBlock         ddrMem;
  } Profile;

  typedef enum
  {
    TX_CACHE_FUSH   = 1<<0,
    RX_CACHE_FETCH  = 1<<1,
    BLOCKING_IN_OUT = 1<<2,
    HW_IP_DONE      = 1<<3,
    DMA_TX_DONE     = 1<<4,
    DMA_RX_DONE     = 1<<5
  } TransactionFlags;

  typedef struct
  {
    int    mode;
    int    flags;
    void * txBufferPtr;
    size_t txBufferSize;
    void * rxBufferPtr;
    size_t rxBufferSize;
  } Transaction;

  ProcessingUnit (void);
  ~ProcessingUnit (void);

  virtual int initialize (Profile *);

  virtual int execute (Transaction * transaction);

  static void setTransactionFlags (Transaction &, int);

  static int getTransactionFlags (Transaction &);

protected:
  virtual int initialize_ip (void);
  virtual int initialize_dma (void);

  virtual void onDone_ip (void);
  virtual void onDone_dmaTx (void);
  virtual void onDone_dmaRx (void);

  Profile * profile_ = nullptr;
  Transaction * transaction_ = nullptr;

private:
  static void hardwareInterruptHandler (void *);
  static void dmaTxInterruptHandler (void *);
  static void dmaRxInterruptHandler (void *);
};

/************************** Constant Definitions *****************************/

/************************** Variable Definitions *****************************/

/************************** Function Prototypes ******************************/

#endif /* PROCESSING_UNIT_H_ */
