/* Includes ------------------------------------------------------------------*/
#include "bsp.h"
#include "fault.h"
#include "gd32h73x_75x_it.h"


extern void HAL_IncTick(void);

/******************************************************************************/
/*           Cortex Processor Interruption and Exception Handlers          */
/******************************************************************************/
/**
  * @brief This function handles Non maskable interrupt.
  */
void NMI_Handler(void)
{
  while (1)
  {
  }
}

/**
  * @brief This function handles Hard fault interrupt.
  */
void HardFault_Handler_C(uint32_t *p_stack)
{
  #ifdef _USE_HW_FAULT
  faultReset("HardFault", p_stack);
  #endif
  while (1)
  {
  }
}

/**
  * @brief This function handles Memory management fault.
  */
void MemManage_Handler_C(uint32_t *p_stack)
{
  #ifdef _USE_HW_FAULT
  faultReset("MemManage", p_stack);
  #endif
  while (1)
  {
  }
}

/**
  * @brief This function handles Pre-fetch fault, memory access fault.
  */
void BusFault_Handler_C(uint32_t *p_stack)
{
  #ifdef _USE_HW_FAULT
  faultReset("BusFault", p_stack);
  #endif
  while (1)
  {
  }
}

/**
  * @brief This function handles Undefined instruction or illegal state.
  */
void UsageFault_Handler_C(uint32_t *p_stack)
{
  // faultReset("UsageFault", p_stack);
  while (1)
  {
  }
}

/**
  * @brief This function handles System service call via SWI instruction.
  */
#ifndef _USE_HW_RTOS
void SVC_Handler(void)
{
}
#endif

/**
  * @brief This function handles Debug monitor.
  */
void DebugMon_Handler(void)
{
}

void FPU_IRQHandler(void)
{
    /* if FPU exception occurs, go to infinite loop */
    while(1) {
    }
}

/**
  * @brief This function handles Pendable request for system service.
  */
#ifndef _USE_HW_RTOS
void PendSV_Handler(void)
{
}
#endif

#ifdef _USE_HW_RTOS
extern void osSystickHandler(void);

void SysTick_Handler(void)
{
  osSystickHandler();
}
#else

/**
  * @brief This function handles System tick timer.
  */
void SysTick_Handler(void)
{
  HAL_IncTick();
}
#endif
