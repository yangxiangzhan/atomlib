#ifndef __SERIAL_SHELL_H__
#define __SERIAL_SHELL_H__

#include "stm32f4xx_serial.h"

#if (SERIAL_OS)   // from stm32f4xx_serial.h ，是否带操作系统
	#include "FreeRTOS.h"
#else 
	#include "heaplib.h"
#endif 
#define f4s_malloc(x)   pvPortMalloc(x)
#define f4s_free(x)     vPortFree(x)


void f4shell_puts(const char * buf,uint16_t len) ;
void serial_console_init(char * info);
void serial_console_deinit(void) ;


extern serial_t * ttyconsole ;

#endif


