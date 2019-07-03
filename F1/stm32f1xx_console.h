#ifndef __SERIAL_SHELL_H__
#define __SERIAL_SHELL_H__

#include "stm32f1xx_serial.h"

/* 
#include <stdlib.h>
#define f1s_malloc(x)   malloc(x)
#define f1s_free(x)     free(x)
*/

#include "freertos.h"
//#include "heaplib.h"
#define f1s_malloc(x)   pvPortMalloc(x)
#define f1s_free(x)     vPortFree(x)


void shell_iap_command(void * arg);
void f1shell_puts(const char * buf,uint16_t len) ;
void serial_console_init(char * info);
void serial_console_deinit(void) ;


extern serial_t * ttyconsole ;


#endif


