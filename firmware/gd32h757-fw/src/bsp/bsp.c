#include "bsp.h"
#include "hw_def.h"


static volatile uint32_t counter_ms = 0;




bool bspInit(void)
{
  bool ret = true;

  #ifdef _USE_HW_CACHE
  SCB_EnableICache();
  SCB_EnableDCache();
  #endif  

  SysTick_Config(SystemCoreClock / 1000U);
  NVIC_SetPriority(SysTick_IRQn, 0x00U);

  return ret;
}

void delay(uint32_t ms)
{
#ifdef _USE_HW_RTOS
  if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)
  {
    osDelay(ms);
  }
  else
  {
    uint32_t pre_time = systick_counter;

    while(systick_counter-pre_time < ms);
  }
#else
  uint32_t pre_time = counter_ms;

  while(counter_ms-pre_time < ms);
#endif
}

void HAL_IncTick(void)
{
  counter_ms++;
}

uint32_t millis(void)
{
  return counter_ms;
}

uint32_t micros(void)
{
  uint32_t          m0  = millis();
  volatile uint32_t u0  = SysTick->VAL;
  uint32_t          m1  = millis();
  volatile uint32_t u1  = SysTick->VAL;
  const uint32_t    tms = SysTick->LOAD + 1;

  if (m1 != m0)
  {
    return (m1 * 1000 + ((tms - u1) * 1000) / tms);
  }
  else
  {
    return (m0 * 1000 + ((tms - u0) * 1000) / tms);
  }
}

