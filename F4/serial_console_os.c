/**
  ******************************************************************************
  * @file           serial_console.c
  * @author         杨翔湿
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

#include "cmsis_os.h" // 启用 freertos

#include "shell.h"
#include "serial_hal.h"
#include "serial_console.h"
#include "iap_hal.h"
//--------------------相关宏定义及结构体定乿-------------------
osThreadId SerialConsoleTaskHandle;
osSemaphoreId osSerialRxSemHandle;
static char task_list_buf[512];
//------------------------------相关函数声明------------------------------



//------------------------------华丽的分割线------------------------------

/*
	* for freertos;
	configUSE_STATS_FORMATTING_FUNCTIONS    1
	portCONFIGURE_TIMER_FOR_RUN_TIME_STATS  1
	configUSE_TRACE_FACILITY 1
	
	#ifndef portCONFIGURE_TIMER_FOR_RUN_TIME_STATS
		extern uint32_t iThreadRuntime;
		#define portCONFIGURE_TIMER_FOR_RUN_TIME_STATS() (iThreadRuntime = 0)
		#define portGET_RUN_TIME_COUNTER_VALUE() (iThreadRuntime)
	#endif
*/
void task_list(void * arg)
{
	static const char acTaskInfo[] = "\r\nthread\t\tstate\tPrior\tstack\tID\r\n----------------------------\r\n";
	static const char acTaskInfoDescri[] = "\r\n----------------------------\r\nB(block),R(ready),D(delete),S(suspended)\r\n";

	osThreadList((uint8_t *)task_list_buf);
	printk("%s%s%s",acTaskInfo,task_list_buf,acTaskInfoDescri);
}


void task_runtime(void * arg)
{
	static const char acTaskInfo[] = "\r\nthread\t\ttime\t\t%CPU\r\n----------------------------\r\n";
	static const char acTaskInfoDescri[] = "\r\n----------------------------\r\n";

	vTaskGetRunTimeStats(task_list_buf);
	printk("%s%s%s",acTaskInfo,task_list_buf,acTaskInfoDescri);
}




void shell_erase_flash(void * arg)
{
	static const char msg[] = "flash-erase (addr) [size]\r\n";
	int argc ;
	int argv[5];

	argc = shell_cmdparam((char*)arg,argv);

	printk("erase flash ",argc);

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




void task_SerialConsole(void const * argument)
{
	struct shell_input serial_shell;
	char *info ;
	uint16_t len ;

	SHELL_INPUT_INIT(&serial_shell,serial_puts);
	
	for(;;)
	{
		if (osOK == osSemaphoreWait(osSerialRxSemHandle,osWaitForever))
		{
			while(serial_rxpkt_queue_out(&info,&len))
				shell_input(&serial_shell,info,len);
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
	struct shell_input * shellin = container_of(arg, struct shell_input, buf);
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
	

	osSemaphoreDef(osSerialRxSem);
	osSerialRxSemHandle = osSemaphoreCreate(osSemaphore(osSerialRxSem), 1); //创建中断信号釿
  
	osThreadDef(SerialConsole, task_SerialConsole, osPriorityNormal, 0, 256);
	SerialConsoleTaskHandle = osThreadCreate(osThread(SerialConsole), NULL);
}






