#include "uart.h"
#include "qbuffer.h"
#include "cli.h"
#if HW_USE_CDC == 1
#include "cdc.h"
#endif

#ifdef _USE_HW_UART


#define UART_RX_BUF_LENGTH        1024



typedef struct
{
  const char *p_msg;
  uint32_t    h_uart;
  uint32_t    h_dma_rx;
  dma_channel_enum h_dma_rx_ch;
  bool        is_rs485;
} uart_hw_t;

typedef struct
{
  bool is_open;
  uint32_t baud;

  uint8_t  rx_buf[UART_RX_BUF_LENGTH];
  qbuffer_t qbuffer;

  uint32_t rx_cnt;
  uint32_t tx_cnt;

  const uart_hw_t *p_hw;
} uart_tbl_t;


static bool uartInitHw(uint8_t ch);
#if CLI_USE(HW_UART)
static void cliUart(cli_args_t *args);
#endif


static bool is_init = false;

__attribute__((section(".non_cache")))
static uart_tbl_t uart_tbl[UART_MAX_CH];


const static uart_hw_t uart_hw_tbl[UART_MAX_CH] =
{
  {"USART1 SWD   ", USART0, DMA0, DMA_CH0, false},
#if HW_UART_MAX_CH >= 2
  {"USB CD       ", NULL,   NULL,    false},
#endif
};





bool uartInit(void)
{
  for (int i=0; i<UART_MAX_CH; i++)
  {
    uart_tbl[i].is_open = false;
    uart_tbl[i].baud    = 57600;
    uart_tbl[i].rx_cnt  = 0;
    uart_tbl[i].tx_cnt  = 0;
    uart_tbl[i].p_hw    = &uart_hw_tbl[i];
  }

  is_init = true;

#if CLI_USE(HW_UART)
  cliAdd("uart", cliUart);
#endif
  return true;
}

bool uartDeInit(void)
{
  return true;
}

bool uartIsInit(void)
{
  return is_init;
}

bool uartOpen(uint8_t ch, uint32_t baud)
{
  bool ret = false;


  if (ch >= UART_MAX_CH) return false;

  if (uart_tbl[ch].is_open == true && uart_tbl[ch].baud == baud)
  {
    return true;
  }


  switch(ch)
  {
    case _DEF_UART1:
      uart_tbl[ch].baud = baud;


      qbufferCreate(&uart_tbl[ch].qbuffer, &uart_tbl[ch].rx_buf[0], UART_RX_BUF_LENGTH);
      
      rcu_periph_clock_enable(RCU_USART0);

      usart_deinit(uart_tbl[ch].p_hw->h_uart);
      usart_word_length_set(uart_tbl[ch].p_hw->h_uart, USART_WL_8BIT);
      usart_stop_bit_set(uart_tbl[ch].p_hw->h_uart, USART_STB_1BIT);
      usart_parity_config(uart_tbl[ch].p_hw->h_uart, USART_PM_NONE);
      usart_baudrate_set(uart_tbl[ch].p_hw->h_uart, baud);
      usart_receive_config(uart_tbl[ch].p_hw->h_uart, USART_RECEIVE_ENABLE);
      usart_transmit_config(uart_tbl[ch].p_hw->h_uart, USART_TRANSMIT_ENABLE);
      usart_enable(uart_tbl[ch].p_hw->h_uart);


      ret = uartInitHw(ch);
      if (ret)
      {
        uart_tbl[ch].is_open = true;

        uint32_t dma_cnt;

        dma_cnt = dma_transfer_number_get(uart_tbl[ch].p_hw->h_dma_rx, uart_tbl[ch].p_hw->h_dma_rx_ch);

        uart_tbl[ch].qbuffer.in  = uart_tbl[ch].qbuffer.len - dma_cnt;
        uart_tbl[ch].qbuffer.out = uart_tbl[ch].qbuffer.in;
      }
      break;

    case _DEF_UART2:
      uart_tbl[ch].baud    = baud;
      uart_tbl[ch].is_open = true;
      ret = true;
      break;      
  }

  return ret;
}

bool uartClose(uint8_t ch)
{
  if (ch >= UART_MAX_CH) return false;

  uart_tbl[ch].is_open = false;

  return true;
}

bool uartInitHw(uint8_t ch)
{
  if (ch == _DEF_UART1)
  {
    dma_single_data_parameter_struct dma_init_struct;

    rcu_periph_clock_enable(RCU_GPIOA);


    gpio_af_set(GPIOA, GPIO_AF_7, GPIO_PIN_9);
    gpio_af_set(GPIOA, GPIO_AF_7, GPIO_PIN_10);

    gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_9);
    gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_100_220MHZ, GPIO_PIN_9);

    gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_10);
    gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_100_220MHZ, GPIO_PIN_10);


    /* enable DMA clock */
    rcu_periph_clock_enable(RCU_DMA0);
    /* enable DMAMUX clock */
    rcu_periph_clock_enable(RCU_DMAMUX);

    dma_deinit(uart_tbl[ch].p_hw->h_dma_rx, uart_tbl[ch].p_hw->h_dma_rx_ch);
    dma_single_data_para_struct_init(&dma_init_struct);
    dma_init_struct.request             = DMA_REQUEST_USART0_RX;
    dma_init_struct.direction           = DMA_PERIPH_TO_MEMORY;
    dma_init_struct.memory0_addr        = (uint32_t)&uart_tbl[ch].rx_buf[0];
    dma_init_struct.memory_inc          = DMA_MEMORY_INCREASE_ENABLE;
    dma_init_struct.periph_memory_width = DMA_PERIPH_WIDTH_8BIT;
    dma_init_struct.number              = sizeof(uart_tbl[ch].rx_buf);
    dma_init_struct.periph_addr         = (uint32_t)&USART_RDATA(uart_tbl[ch].p_hw->h_uart);
    dma_init_struct.periph_inc          = DMA_PERIPH_INCREASE_DISABLE;
    dma_init_struct.priority            = DMA_PRIORITY_LOW;
    dma_single_data_mode_init(uart_tbl[ch].p_hw->h_dma_rx, uart_tbl[ch].p_hw->h_dma_rx_ch, &dma_init_struct);

    /* configure DMA mode */
    dma_circulation_enable(uart_tbl[ch].p_hw->h_dma_rx, uart_tbl[ch].p_hw->h_dma_rx_ch);
    /* enable DMA channel 1 */
    dma_channel_enable(uart_tbl[ch].p_hw->h_dma_rx, uart_tbl[ch].p_hw->h_dma_rx_ch);
    /* USART DMA enable for reception */
    usart_dma_receive_config(uart_tbl[ch].p_hw->h_uart, USART_RECEIVE_DMA_ENABLE);
  }

  return true;
}

uint32_t uartAvailable(uint8_t ch)
{
  uint32_t ret = 0;
  uint32_t dma_cnt;


  switch(ch)
  {
    case _DEF_UART1:
      dma_cnt = dma_transfer_number_get(uart_tbl[ch].p_hw->h_dma_rx, uart_tbl[ch].p_hw->h_dma_rx_ch);
      uart_tbl[ch].qbuffer.in = (uart_tbl[ch].qbuffer.len - dma_cnt);
      ret = qbufferAvailable(&uart_tbl[ch].qbuffer);      
      break;

    case _DEF_UART2:
      #if HW_USE_CDC == 1
      ret = cdcAvailable();
      #endif
      break;      
  }

  return ret;
}

bool uartFlush(uint8_t ch)
{
  uint32_t pre_time;


  pre_time = millis();
  while(uartAvailable(ch))
  {
    if (millis()-pre_time >= 10)
    {
      break;
    }
    uartRead(ch);
  }

  return true;
}

uint8_t uartRead(uint8_t ch)
{
  uint8_t ret = 0;


  switch(ch)
  {
    case _DEF_UART1:
      qbufferRead(&uart_tbl[ch].qbuffer, &ret, 1);
      break;

    case _DEF_UART2:
      #if HW_USE_CDC == 1
      ret = cdcRead();
      #endif
      break;      
  }
  uart_tbl[ch].rx_cnt++;

  return ret;
}

uint32_t uartWrite(uint8_t ch, uint8_t *p_data, uint32_t length)
{
  uint32_t ret = 0;
  uint32_t tx_cnt = 0;
  uint32_t pre_time;


  pre_time = millis();
  switch(ch)
  {
    case _DEF_UART1:
      while (tx_cnt < length)
      {        
        if(usart_flag_get(uart_tbl[ch].p_hw->h_uart, USART_FLAG_TBE))
        {          
          usart_data_transmit(uart_tbl[ch].p_hw->h_uart, p_data[tx_cnt++]);
        }
        if (millis()-pre_time > 100)
        {
          break;
        }
      }
      ret = tx_cnt;
      break;

    case _DEF_UART2:
      #if HW_USE_CDC == 1
      ret = cdcWrite(p_data, length);
      #endif
      break;      
  }
  uart_tbl[ch].tx_cnt += ret;

  return ret;
}

uint32_t uartPrintf(uint8_t ch, const char *fmt, ...)
{
  char buf[256];
  va_list args;
  int len;
  uint32_t ret;

  va_start(args, fmt);
  len = vsnprintf(buf, 256, fmt, args);

  ret = uartWrite(ch, (uint8_t *)buf, len);

  va_end(args);


  return ret;
}

uint32_t uartGetBaud(uint8_t ch)
{
  uint32_t ret = 0;


  if (ch >= UART_MAX_CH) return 0;

  #if HW_USE_CDC == 1
  if (ch == HW_UART_CH_USB)
    ret = cdcGetBaud();
  else
    ret = uart_tbl[ch].baud;
  #else
  ret = uart_tbl[ch].baud;
  #endif
  
  return ret;
}

uint32_t uartGetRxCnt(uint8_t ch)
{
  if (ch >= UART_MAX_CH) return 0;

  return uart_tbl[ch].rx_cnt;
}

uint32_t uartGetTxCnt(uint8_t ch)
{
  if (ch >= UART_MAX_CH) return 0;

  return uart_tbl[ch].tx_cnt;
}

#if CLI_USE(HW_UART)
void cliUart(cli_args_t *args)
{
  bool ret = false;


  if (args->argc == 1 && args->isStr(0, "info"))
  {
    for (int i=0; i<UART_MAX_CH; i++)
    {
      cliPrintf("_DEF_UART%d : %s, %d bps\n", i+1, uart_hw_tbl[i].p_msg, uartGetBaud(i));
    }
    ret = true;
  }

  if (args->argc == 2 && args->isStr(0, "test"))
  {
    uint8_t uart_ch;

    uart_ch = constrain(args->getData(1), 1, UART_MAX_CH) - 1;

    if (uart_ch != cliGetPort())
    {
      uint8_t rx_data;

      while(1)
      {
        if (uartAvailable(uart_ch) > 0)
        {
          rx_data = uartRead(uart_ch);
          cliPrintf("<- _DEF_UART%d RX : 0x%X\n", uart_ch + 1, rx_data);
        }

        if (cliAvailable() > 0)
        {
          rx_data = cliRead();
          if (rx_data == 'q')
          {
            break;
          }
          else
          {
            uartWrite(uart_ch, &rx_data, 1);
            cliPrintf("-> _DEF_UART%d TX : 0x%X\n", uart_ch + 1, rx_data);            
          }
        }
      }
    }
    else
    {
      cliPrintf("This is cliPort\n");
    }
    ret = true;
  }

  if (ret == false)
  {
    cliPrintf("uart info\n");
    cliPrintf("uart test ch[1~%d]\n", HW_UART_MAX_CH);
  }
}
#endif


#endif

