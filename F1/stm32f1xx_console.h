#ifndef __SERIAL_SHELL_H__
#define __SERIAL_SHELL_H__

#include "stm32f1xx_serial.h"

#if (SERIAL_OS)   // from stm32f4xx_serial.h ，是否带操作系统
	#include "FreeRTOS.h"
#else 
	#include "heaplib.h"
#endif 
#define f1s_malloc(x)   pvPortMalloc(x)
#define f1s_free(x)     vPortFree(x)


void f1shell_puts(const char * buf,uint16_t len) ;
void serial_console_init(char * info);
void serial_console_deinit(void) ;
void shell_iap_command(void * arg);


extern serial_t * ttyconsole ;


#endif


