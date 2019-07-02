/**
  ******************************************************************************
  * @file           serial_console.c
  * @author         古么宁
  * @brief          serial_console file
                    串口控制台文件。文件不直接操作硬件，依赿serial_hal
  ******************************************************************************
  *
  * COPYRIGHT(c) 2018 GoodMorning
  *
  ******************************************************************************
  */
/* Includes ---------------------------------------------------*/
#include <string.h>
#include <stdarg.h>
#include <stdint.h> //定义了很多数据类垿

#include "stm32f1xx_hal.h" //for SCB->VTOR,FLASH_PAGE_SIZE

#include "cmsis_os.h" // 启用 freertos


#include "iap_hal.h"

#include "containerof.h"
#include "shell.h"
#include "vim.h"

#include "stm32f1xx_serial.h"
#include "stm32f1xx_console.h"

//--------------------相关宏定义及结构体定乿-------------------
osThreadId SerialConsoleTaskHandle; 


static struct serial_iap
{
	uint32_t addr;
	uint32_t size;
	char *   databuf ;
	uint32_t bufsize;
}
iap;

const static char iap_logo[]=
"\r\n\
 ____   ___   ____\r\n\
|_  _| / _ \\ |  _ \\\r\n\
 _||_ |  _  ||  __/don't press any key now\r\n\
|____||_| |_||_|   ";

static const char division [] = "\r\n----------------------------\r\n";

// 正在进行串口升级 iap
static volatile char console_iap = 0 ;

static struct shell_input f1shell;

serial_t * ttyconsole = NULL;
//------------------------------相关函数声明------------------------------

void task_SerialConsole(void const * argument);


//------------------------------华丽的分割线------------------------------

/*
	* for freertos;
	configUSE_STATS_FORMATTING_FUNCTIONS	1
	portCONFIGURE_TIMER_FOR_RUN_TIME_STATS	1
	configUSE_TRACE_FACILITY 1
	
	STM32CUBEMX : CONFIGURE_TIMER_FOR_RUN_TIME_STATS , USE_TRACE_FACILITY ,USE_STATS_FORMATTING_FUNCTIONS

#ifndef portCONFIGURE_TIMER_FOR_RUN_TIME_STATS
		extern uint32_t iThreadRuntime;
	#define portCONFIGURE_TIMER_FOR_RUN_TIME_STATS() (iThreadRuntime = 0)
	#define portGET_RUN_TIME_COUNTER_VALUE() (iThreadRuntime)
#endif
*/
void task_list(void * arg)
{
	static const char title[] = "\r\nthread\t\tstate\tPrior\tstack\tID";
	static const char descri[] = "B(block),R(ready),D(delete),S(suspended)\r\n";

	char * tasklistbuf = (char *)pvPortMalloc(2048);

	osThreadList((uint8_t *)tasklistbuf);

	printl((char*)title,sizeof(title)-1);
	printl((char*)division,sizeof(division)-1);
	printl(tasklistbuf,strlen(tasklistbuf));
	printl((char*)division,sizeof(division)-1);
	printl((char*)descri,sizeof(descri)-1);

	vPortFree(tasklistbuf);
}


void task_runtime(void * arg)
{
	static const char title[] = "\r\nthread\t\ttime\t\t%CPU";

	char * tasklistbuf = (char *)pvPortMalloc(2048);

	vTaskGetRunTimeStats(tasklistbuf);

	printl((char*)title,sizeof(title)-1);
	printl((char*)division,sizeof(division)-1);
	printl(tasklistbuf,strlen(tasklistbuf));
	printl((char*)division,sizeof(division)-1);

	vPortFree(tasklistbuf);
}

volatile unsigned long lThreadRuntime = 0;
void configureTimerForRunTimeStats(void)
{
	lThreadRuntime = 0;
}

unsigned long getRunTimeCounterValue(void)
{
	return lThreadRuntime;
}


void shell_erase_flash(void * arg)
{
	static const char msg[] = "flash-erase (addr) [size]\r\n";
	int argc ;
	int argv[2];

	argc = cmdline_param((char*)arg,argv,2);

	printk("erase flash ");

	if (argc < 1)
	{
		printl((char*)msg,sizeof(msg)-1);
	}
	else
	{
		uint32_t addr = argv[0];
		uint32_t size = (argc == 1)? 2:argv[1];
		
		if (iap_unlock_flash())
		{
			color_printk(light_red,"error,cannot unlock flash\r\n");
			return ;
		}

		if (iap_erase_flash(addr,size))
		{
			color_printk(light_red,"error,cannot erase flash\r\n");
			return ;
		}
		
		if (iap_lock_flash())
		{
			color_printk(light_red,"error,cannot lock flash\r\n");
			return ;
		}
		
		printk("(0x%x)done\r\n",addr);
	}
}



/**
	* @brief    shell_iap_command
	*           命令行响应函数
	* @param    arg  : 命令行内存指针
	* @return   void
*/
void shell_iap_command(void * arg)
{
	int argc , erasesize ;

	struct shell_input * shell = container_of(arg, struct shell_input, cmdline);	
	
	if (shell != &f1shell) {
		printk("cannot update at this channel.\r\n");
		return ;
	}

	argc = cmdline_param((char*)arg,&erasesize,1);

	iap.addr = (SCB->VTOR == FLASH_BASE) ? APP_ADDR : IAP_ADDR;
	iap.size = (argc == 1) ? erasesize : 1 ;
	console_iap = 1 ;
}



/**
	* @brief    hal_usart_puts console 硬件层输出
	* @param    空
	* @return   空
*/
void f1shell_puts(const char * buf,uint16_t len)
{
	serial_write(ttyconsole,buf,len,O_NOBLOCK); 
}




void task_SerialConsole(void const * argument)
{
	char *databuf ;
	int   datalen ,timeout = 0, time = 0;
	#define TIMEOUT_NO_RECV    15000   // 等待数据包超时
	#define TIMEOUT_RECV       1600    // 最后一包数据超时判断

	for ( ; ; ) {
		if (0 == console_iap) {   // 正常的接收模式
			datalen = serial_gets(ttyconsole,&databuf,O_BLOCK);
			if (datalen > 0)
				shell_input(&f1shell,databuf,datalen);//数据帧传入应用层
			else 
			if (datalen == 0)
				osDelay(5);
			else 
				break;
		}
		else 
		if (1 == console_iap) {   // 命令行输入 update 命令后进入 iap 模式
			char * newbuf = f1s_malloc(FLASH_PAGE_SIZE * 2);
			if (!newbuf) {
				printk("%s():cannot malloc.\r\n",__FUNCTION__);
				console_iap = 0;
			}
			else {
				char ** ttybuf = (char **)&ttyconsole->rxbuf ;      // 记录原接收缓存区
				uint16_t * ttymax = (uint16_t *)&ttyconsole->rxmax ;// 原接收缓存大小
				iap.databuf = *ttybuf ;
				iap.bufsize = *ttymax;
				serial_close(ttyconsole);               // 关闭设备，重新打开
				*ttybuf = newbuf;                       // 重定义设备接收缓存
				*ttymax = FLASH_PAGE_SIZE ;             // 重定义设备接收缓存大小
				serial_open(ttyconsole,115200,8,'N',1); // 重新打开设备
				iap_unlock_flash();
				iap_erase_flash(iap.addr , iap.size);
				color_printk(light_green,"\033[2J\033[%d;%dH%s",0,0,iap_logo);//清屏
				timeout = TIMEOUT_NO_RECV; // 设置超时为 15s ，15s 内无数据接收则认为超时
				console_iap = 2;           // 进入 iap 模式接收串口数据
			}
		}
		else 
		if (2 == console_iap) {   // iap 模式接收数据包
			datalen = serial_gets(ttyconsole,&databuf,O_NOBLOCK); // 非阻塞模式接收串口数据
			if (datalen > 0 ) {
				uint32_t * value = (uint32_t*)databuf;
				if (TIMEOUT_NO_RECV == timeout) { 
					timeout = TIMEOUT_RECV;      // 接收到第一包数据，超时改为 1.6s ，即接收到最后一包后 1.6 内无数据则认为更新结束
					serial_write(ttyconsole,"loading",7,O_NOBLOCK);
				}
				time = 0 ;      // 清空超时计数。
				iap.size = iap.addr + datalen ;
				for ( ; iap.addr < iap.size ; iap.addr += 4) // f4 可以以 word 写入
					iap_write_flash(iap.addr,*value++); 

				if ((iap.addr & (FLASH_PAGE_SIZE-1)) == 0){   // 清空下一页
					iap_erase_flash(iap.addr ,1) ;
					serial_write(ttyconsole,".",1,O_NOBLOCK);
				}
				else { 
					console_iap = 3; // 当接收到的包长不为 FLASH_PAGE_SIZE 则认为是最后一包数据
				}
			}
			else                  // 0 == datalen，无接收数据
			if (++time > timeout) // 超时
				console_iap = 3;
			else 
				osDelay(1);       // 1ms 后再读数据
		}
		else { // 3 == console_iap , iap 收尾工作
			char ** ttybuf    = (char **)&ttyconsole->rxbuf ;   // 记录原接收缓存区
			uint16_t * ttymax = (uint16_t *)&ttyconsole->rxmax ;// 原接收缓存大小
			uint32_t filesize = (SCB->VTOR == FLASH_BASE) ? (iap.addr-APP_ADDR):(iap.addr-IAP_ADDR);
			
			serial_close(ttyconsole);               // 关闭设备，重新打开
			f1s_free(*ttybuf) ;                     // 释放在 第二步 申请的内存
			*ttybuf = iap.databuf;                  // 恢复内存
			*ttymax = iap.bufsize;                  // 恢复大小
			serial_open(ttyconsole,115200,8,'N',1); // 重新打开设备

			if (filesize)
				printk("\r\nupdate completed!\r\nupdate package size:%d byte\r\n",filesize);
			else 
				printk("iap:recv timeout.\r\n");
			iap_lock_flash();
			console_iap = 0;
		}
	}

	printk("%s() quit.\r\n",__FUNCTION__);
	vTaskDelete(NULL);
}


/**
	* @brief    _syscfg_fgets
	*          获取 syscfg 信息，由 shell_into_edit 调用
	* @param
	* @return   成功 返回VIM_FILE_OK
*/
uint32_t _syscfg_fgets(char * fpath, char * fdata,uint16_t * fsize)
{
	uint32_t len = 0;
	uint32_t addr = syscfg_addr();
	
	if (0 == addr)
		return VIM_FILE_ERROR;

	for (char * syscfg = (char*)addr ; *syscfg ; ++len)
		*fdata++ = *syscfg++ ;

	*fsize = len;

	return VIM_FILE_OK;
}

/**
	* @brief    _syscfg_fputs
	*          更新 syscfg 信息，由 shell_into_edit 调用
	* @param
	* @return   void
*/
void _syscfg_fputs(char * fpath, char * fdata,uint32_t fsize)
{
	syscfg_write(fdata,fsize);
}



/**
	* @brief    _shell_edit_syscfg
	*           命令行编辑 syscfg 信息
	* @param    arg  命令行内存
	* @return   void
*/
void _shell_edit_syscfg(void * arg)
{
	struct shell_input * shellin = container_of(arg, struct shell_input, cmdline);
	shell_into_edit(shellin , _syscfg_fgets , _syscfg_fputs );
}


void serial_console_init(char * info)
{
	ttyconsole = &ttyS1 ; // 控制台输出设备

	serial_open(ttyconsole,115200,8,'N',1);
	shell_input_init(&f1shell,f1shell_puts);

	//printk("\r\n");
	//color_printk(purple,"%s",info);//打印开机信息或者控制台信息

	shell_register_command("ps"   ,task_list);
	shell_register_command("top"    ,task_runtime);
	shell_register_command("reboot",shell_reboot_command);
	shell_register_command("syscfg",_shell_edit_syscfg);
	shell_register_command("flash-erase",shell_erase_flash);

	if (ttyconsole->flag & FLAG_RX_DMA) { // 当接收为 DMA 模式下，注册在线升级命令
		if (SCB->VTOR == FLASH_BASE)
			shell_register_command("update-app",shell_iap_command);	
		else
			shell_register_confirm("update-iap",shell_iap_command,"sure to update iap?");
	}

	osThreadDef(SerialConsole, task_SerialConsole, osPriorityNormal, 0, 168);
	SerialConsoleTaskHandle = osThreadCreate(osThread(SerialConsole), &f1shell);
}



void serial_console_deinit(void)
{
	serial_close(ttyconsole);
}


