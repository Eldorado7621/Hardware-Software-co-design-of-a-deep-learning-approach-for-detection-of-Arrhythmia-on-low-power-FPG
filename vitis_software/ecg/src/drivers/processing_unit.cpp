/*
 * processing_unit.cpp
 *
 *  Created on: July 31st, 2021
 *      Author: Yarib Nevarez
 */
#include "processing_unit.h"

#include "xil_cache.h"
#include "miscellaneous.h"
#include "event.h"
#include <limits>

#include "dma_vtbl.h"
#include "conv_vtbl.h"

ProcessingUnit::ProcessingUnit ()
{

}

ProcessingUnit::~ProcessingUnit ()
{

}

int ProcessingUnit::execute (Transaction * transaction)
{
  int status = XST_FAILURE;

  ASSERT(profile_ != nullptr);
  ASSERT(transaction != nullptr);

  if ((profile_ != nullptr) && (transaction != nullptr))
  {
    DMAHardwareVtbl * dmaVtbl = nullptr;
    HardwareVtbl *    hwVtbl = nullptr;
    void * dmaInstance = nullptr;
    void * hwInstance = nullptr;

    void * tx_buffer = transaction->txBufferPtr;
    size_t tx_buffer_size = transaction->txBufferSize;
    void * rx_buffer = transaction->rxBufferPtr;
    size_t rx_buffer_size = transaction->rxBufferSize;
    volatile int * flags = &transaction->flags;

    transaction_ = transaction;

    ASSERT(profile_->dmaVtbl != nullptr);
    ASSERT(profile_->dmaInstance != nullptr);

    dmaVtbl = profile_->dmaVtbl;
    dmaInstance = profile_->dmaInstance;

    ASSERT(profile_->hwVtbl != nullptr);
    ASSERT(profile_->hwInstance != nullptr);

    hwVtbl = profile_->hwVtbl;
    hwInstance = profile_->hwInstance;

    while (!hwVtbl->IsReady (hwInstance));

    hwVtbl->Set_mode (hwInstance, transaction->mode);

    *flags &= ~HW_IP_DONE;

    hwVtbl->Start (hwInstance);

    if (tx_buffer != nullptr && 0 < tx_buffer_size)
    {
      if (*flags & TX_CACHE_FUSH)
        Xil_DCacheFlushRange ((UINTPTR) tx_buffer, tx_buffer_size);

      *flags &= ~DMA_TX_DONE;

      status = dmaVtbl->Move (dmaInstance, (void *) tx_buffer, tx_buffer_size,
                              MEMORY_TO_HARDWARE);
      ASSERT(status == XST_SUCCESS);
    }

    if (rx_buffer != nullptr && 0 < rx_buffer_size)
    {
      *flags &= ~DMA_RX_DONE;

      status = dmaVtbl->Move (dmaInstance, (void *) rx_buffer, rx_buffer_size,
                              HARDWARE_TO_MEMORY);
      ASSERT(status == XST_SUCCESS);
    }

    if (*flags & BLOCKING_IN_OUT)
    {
      while (!hwVtbl->IsDone (hwInstance));

      if (rx_buffer != nullptr && 0 < rx_buffer_size)
        while (!((*flags) & DMA_RX_DONE));
    }
  }

  return status;
}

void ProcessingUnit::setTransactionFlags (Transaction & transaction, int flags)
{
  transaction.flags = flags;
}

int ProcessingUnit::getTransactionFlags (Transaction & transaction)
{
  return transaction.flags;
}

void ProcessingUnit::onDone_ip (void)
{

}

void ProcessingUnit::onDone_dmaRx (void)
{

}

void ProcessingUnit::onDone_dmaTx (void)
{

}

///////////////////////////////////////////////////////////////////////////////

#define ACCELERATOR_DMA_RESET_TIMEOUT 1000

void ProcessingUnit::dmaTxInterruptHandler (void * data)
{
  ASSERT(data != NULL);
  if (data)
  {
    ProcessingUnit * pu = reinterpret_cast<ProcessingUnit *> (data);
    Profile * profile = pu->profile_;
    DMAHardwareVtbl * dmaVtbl = nullptr;
    void * dmaInstance = nullptr;
    DMAIRQMask irq_status;

    ASSERT(profile != nullptr);
    ASSERT(profile->dmaVtbl != nullptr);

    dmaVtbl = profile->dmaVtbl;
    dmaInstance = profile->dmaInstance;

    irq_status = dmaVtbl->InterruptGetStatus (dmaInstance, MEMORY_TO_HARDWARE);
    dmaVtbl->InterruptClear (dmaInstance, irq_status, MEMORY_TO_HARDWARE);

    if (!(irq_status & DMA_IRQ_ALL))
      return;

    if (irq_status & DMA_IRQ_DELAY)
      return;

    if (irq_status & DMA_IRQ_ERROR)
    {
      int TimeOut;

      dmaVtbl->Reset (dmaInstance);

      for (TimeOut = ACCELERATOR_DMA_RESET_TIMEOUT; 0 < TimeOut; TimeOut--)
        if (dmaVtbl->ResetIsDone (dmaInstance))
          break;

      ASSERT(0);
      return;
    }

    if (irq_status & DMA_IRQ_IOC)
    {
      ASSERT(pu->transaction_ != nullptr);
      pu->transaction_->flags |= DMA_TX_DONE;
      pu->onDone_dmaTx ();
    }
  }
}

void ProcessingUnit::dmaRxInterruptHandler (void * data)
{
  ASSERT(data != NULL);
  if (data)
  {
    ProcessingUnit * pu = reinterpret_cast<ProcessingUnit *> (data);
    Profile * profile = pu->profile_;
    DMAHardwareVtbl * dmaVtbl = nullptr;
    void * dmaInstance = nullptr;
    DMAIRQMask irq_status;

    ASSERT(profile != nullptr);
    ASSERT(profile->dmaVtbl != nullptr);

    dmaVtbl = profile->dmaVtbl;
    dmaInstance = profile->dmaInstance;

    irq_status = dmaVtbl->InterruptGetStatus (dmaInstance, HARDWARE_TO_MEMORY);

    dmaVtbl->InterruptClear (dmaInstance, irq_status, HARDWARE_TO_MEMORY);

    if (!(irq_status & DMA_IRQ_ALL))
      return;

    if (irq_status & DMA_IRQ_DELAY)
      return;

    if (irq_status & DMA_IRQ_ERROR)
    {
      int TimeOut;

      dmaVtbl->Reset (dmaInstance);

      for (TimeOut = ACCELERATOR_DMA_RESET_TIMEOUT; 0 < TimeOut; TimeOut--)
        if (dmaVtbl->ResetIsDone (dmaInstance))
          break;

      ASSERT(0);
      return;
    }

    if (irq_status & DMA_IRQ_IOC)
    {
      Transaction * transaction = pu->transaction_;
      ASSERT(transaction != nullptr);
      if ((transaction != nullptr) && (transaction->flags & RX_CACHE_FETCH))
      {
        Xil_DCacheInvalidateRange ((INTPTR) transaction->rxBufferPtr,
                                   transaction->rxBufferSize);
      }

      transaction->flags |= DMA_RX_DONE;

      pu->onDone_dmaRx ();
    }
  }
}

void ProcessingUnit::hardwareInterruptHandler (void * data)
{
  ASSERT(data != NULL);
  if (data)
  {
    ProcessingUnit * pu = reinterpret_cast<ProcessingUnit *> (data);
    Profile * profile = pu->profile_;
    HardwareVtbl * hwVtbl = nullptr;
    void * hwInstance = nullptr;
    uint32_t status;

    ASSERT(profile != nullptr);
    ASSERT(profile->hwVtbl != nullptr);

    hwVtbl = profile->hwVtbl;
    hwInstance = profile->hwInstance;

    status = hwVtbl->InterruptGetStatus (hwInstance);
    hwVtbl->InterruptClear (hwInstance, status);

    /*!! Clear profile BEFORE making the accelerator ready !!*/
    if (status & 1)
    {
      ASSERT(pu->transaction_ != nullptr);
      pu->transaction_->flags |= HW_IP_DONE;
      pu->onDone_ip ();
    }
  }
}

int ProcessingUnit::initialize_ip ()
{
  ARM_GIC_InterruptHandler interruptHandler = hardwareInterruptHandler;
  HardwareVtbl * hwVtbl = nullptr;
  void * hwInstance = nullptr;
  int hwDeviceID;
  int hwIntVecID;
  int status;

  ASSERT (profile_ != nullptr);
  ASSERT (profile_->dmaVtbl != nullptr);

  if ((profile_ == nullptr) || (profile_->dmaVtbl == nullptr))
    return XST_FAILURE;

  hwVtbl = profile_->hwVtbl;
  hwDeviceID = profile_->hwDeviceID;
  hwIntVecID = profile_->hwIntVecID;

  hwInstance = hwVtbl->new_ ();
  ASSERT(hwInstance != nullptr);

  if (hwInstance == nullptr)
    return XST_FAILURE;

  profile_->hwInstance = hwInstance;

  status = hwVtbl->Initialize (hwInstance, hwDeviceID);
  ASSERT (status == XST_SUCCESS);

  if (status != XST_SUCCESS)
    return status;

  if (hwIntVecID)
  {
    hwVtbl->InterruptGlobalEnable (hwInstance);
    hwVtbl->InterruptEnable (hwInstance, 1);
    hwVtbl->InterruptSetHandler (hwInstance, hwIntVecID,
                                 interruptHandler, (void *) this);
  }

  return status;
}

int ProcessingUnit::initialize_dma (void)
{
  ARM_GIC_InterruptHandler txInterruptHandler = dmaTxInterruptHandler;
  ARM_GIC_InterruptHandler rxInterruptHandler = dmaRxInterruptHandler;
  DMAHardwareVtbl * dmaVtbl = nullptr;
  void * dmaInstance = nullptr;
  int dma_device_id;
  int dmaTxIntVecID;
  int dmaRxIntVecID;
  int status;

  ASSERT (profile_ != nullptr);
  ASSERT (profile_->dmaVtbl != nullptr);

  if ((profile_ == nullptr) || (profile_->dmaVtbl == nullptr))
    return XST_FAILURE;

  dmaVtbl = profile_->dmaVtbl;
  dma_device_id = profile_->dmaDeviceID;
  dmaTxIntVecID = profile_->dmaTxIntVecID;
  dmaRxIntVecID = profile_->dmaRxIntVecID;

  dmaInstance = dmaVtbl->new_ ();
  ASSERT (dmaInstance != nullptr);

  profile_->dmaInstance = dmaInstance;

  status = dmaVtbl->Initialize (dmaInstance, dma_device_id);


  if (dmaTxIntVecID)
  {
    ASSERT(txInterruptHandler != nullptr);

    dmaVtbl->InterruptEnable (dmaInstance, DMA_IRQ_ALL, MEMORY_TO_HARDWARE);

    status = dmaVtbl->InterruptSetHandler (dmaInstance, dmaTxIntVecID,
                                           txInterruptHandler,
                                           (void *) this);
    ASSERT(status == XST_SUCCESS);

    if (status != XST_SUCCESS)
      return status;
  }

  if (dmaRxIntVecID)
  {
    ASSERT(rxInterruptHandler != nullptr);

    dmaVtbl->InterruptEnable (dmaInstance, DMA_IRQ_ALL, HARDWARE_TO_MEMORY);

    status = dmaVtbl->InterruptSetHandler (dmaInstance, dmaRxIntVecID,
                                           rxInterruptHandler,
                                           (void *) this);
    ASSERT(status == XST_SUCCESS);

    if (status != XST_SUCCESS)
      return status;
  }

  return status;
}

int ProcessingUnit::initialize (Profile * profile)
{
  int status;

  ASSERT (profile != nullptr);
  ASSERT (profile->hwVtbl != nullptr);
  ASSERT (profile->dmaVtbl != nullptr);

  if ((profile == nullptr)
      && (profile->hwVtbl != nullptr)
      && (profile->dmaVtbl != nullptr))
    return XST_FAILURE;

  profile_ = profile;

  ARM_GIC_initialize ();

  status = initialize_ip ();
  ASSERT(status == XST_SUCCESS);

  if (status != XST_SUCCESS)
    return status;

  status = initialize_dma ();
  ASSERT(status == XST_SUCCESS);

  return status;
}
