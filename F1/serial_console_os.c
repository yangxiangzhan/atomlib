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

#include "serial_hal.h"
#include "serial_console.h"
#include "iap_hal.h"

#include "containerof.h"
#include "shell.h"
#include "vim.h"
//--------------------相关宏定义及结构体定乿-------------------
osThreadId SerialConsoleTaskHandle;
osSemaphoreId osSerialRxSemHandle;


static struct serial_iap
{
	uint32_t addr;
	uint32_t size;
}
iap;

const static char iap_logo[]=
"\r\n\
 ____   ___   ____\r\n\
|_  _| / _ \\ |  _ \\\r\n\
 _||_ |  _  ||  __/don't press any key now\r\n\
|____||_| |_||_|   ";

static const char division [] = "\r\n----------------------------\r\n";

//------------------------------相关函数声明------------------------------



//------------------------------华丽的分割线------------------------------

/*
	* for freertos;
	configUSE_STATS_FORMATTING_FUNCTIONS	1
	portCONFIGURE_TIMER_FOR_RUN_TIME_STATS	1
	configUSE_TRACE_FACILITY 1
	
	STM32CUBEMX : RUN_TIME_STATS

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

	char * tasklistbuf = (char *)pvPortMalloc(1024);

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

	char * tasklistbuf = (char *)pvPortMalloc(1024);

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
	* @brief iap_gets  
	*        iap 升级任务，获取数据流并写入flash
	*        注：由于在写 flash 的时候，单片机会停止读取 flash，即
	*        代码不会运行。如果在写 flash 的时候有中断产生，单片机
	*        可能会死机。写 flash 的时候不妨碍 dma 的传输，所以 dma
	*        接收缓冲要大一些，要在下一包数据接收完写完当前包数据
	* @param void
	* @return NULL
*/
static void iap_gets(struct shell_input * shell ,char * buf , uint32_t len)
{
	uint32_t * value = (uint32_t*)buf;
	
	shell->apparg = (void*)MODIFY_MASK;

	for (iap.size = iap.addr + len ; iap.addr < iap.size ; iap.addr += 4)// f4 可以以 word 写入
		iap_write_flash(iap.addr,*value++); 
	
	if ((iap.addr & (FLASH_PAGE_SIZE-1)) == 0)//清空下一页
		iap_erase_flash(iap.addr ,1) ;
	else
		printl(".",1);//打印一个点以示清白
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
	
	shell->gets = iap_gets;//串口数据流获取至 iap_gets

	argc = cmdline_param((char*)arg,&erasesize,1);

	iap.addr = (SCB->VTOR == FLASH_BASE) ? APP_ADDR : IAP_ADDR;
	iap.size = (argc == 1) ? erasesize : 1 ;

	//由于要写完最后一包数据才能上锁，所以上锁不放在这
	iap_unlock_flash();
	iap_erase_flash(iap.addr , iap.size);
	color_printk(light_green,"\033[2J\033[%d;%dH%s",0,0,iap_logo);//清屏
	serial_recv_reset(HAL_RX_BUF_SIZE/2);
}



void task_SerialConsole(void const * argument)
{
	struct shell_input serial_shell;
	char *info ;
	uint16_t len ;
	uint32_t wait;
	
	SHELL_INPUT_INIT(&serial_shell,serial_puts);
	
	for(;;)
	{
		// 等待信号量时间，在 iap 模式中等待超过 1.6 s 的时间视为 iap 接收结束
		wait = serial_shell.apparg == (void*)MODIFY_MASK ? 1600 : osWaitForever;
		
		if (osOK == osSemaphoreWait(osSerialRxSemHandle,wait))
		{
			while(serial_rxpkt_queue_out(&info,&len))
				shell_input(&serial_shell,info,len);
		}
		else
		{
			iap_lock_flash();
			uint32_t filesize = (SCB->VTOR == FLASH_BASE) ? (iap.addr-APP_ADDR):(iap.addr-IAP_ADDR);
			printk("\r\nupdate completed!\r\nupdate package size:%d byte\r\n",filesize);
			serial_shell.gets = cmdline_gets ;
			serial_shell.apparg = NULL;
		}
	}
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
	hal_serial_init();

	printk("\r\n");
	color_printk(purple,"%s",info);//打印开机信息或者控制台信息

	shell_register_command("top"   ,task_list);
	shell_register_command("ps"    ,task_runtime);
	shell_register_command("reboot",shell_reboot_command);
	shell_register_command("syscfg",_shell_edit_syscfg);
	shell_register_command("flash-erase",shell_erase_flash);
	
	if (SCB->VTOR == FLASH_BASE)
		shell_register_command("update-app",shell_iap_command);	
	else
		shell_register_confirm("update-iap",shell_iap_command,"sure to update iap?");

	osSemaphoreDef(osSerialRxSem);
	osSerialRxSemHandle = osSemaphoreCreate(osSemaphore(osSerialRxSem), 1); //创建中断信号釿
  
	osThreadDef(SerialConsole, task_SerialConsole, osPriorityNormal, 0, 168+(sizeof(struct shell_input)/4));
	SerialConsoleTaskHandle = osThreadCreate(osThread(SerialConsole), NULL);
}






