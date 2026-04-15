#ifndef HW_H_
#define HW_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "hw_def.h"


#include "led.h"
#include "uart.h"
#include "cli.h"
#include "log.h"
#include "fault.h"
#include "swtimer.h"
#include "button.h"
#include "reset.h"
#include "rtc.h"
#include "buzzer.h"
#include "i2c.h"
#include "lcd.h"
#include "gpio.h"
#include "spi.h"
#include "spi_flash.h"
#include "rf_remote.h"
#include "micros.h"
#include "pwm.h"
#include "esp.h"
#include "fs.h"
#include "nvs.h"
#include "qbuffer.h"
#include "lora.h"

#include "osal/thread.h"


bool hwInit(void);


#ifdef __cplusplus
}
#endif

#endif