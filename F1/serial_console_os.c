/**
  ******************************************************************************
  * @file           serial_console.c
  * @author         ��ô��
  * @brief          serial_console file
                    ���ڿ���̨�ļ����ļ���ֱ�Ӳ���Ӳ�������dserial_hal
  ******************************************************************************
  *
  * COPYRIGHT(c) 2018 GoodMorning
  *
  ******************************************************************************
  */
/* Includes ---------------------------------------------------*/
#include <string.h>
#include <stdarg.h>
#include <stdint.h> //�����˺ܶ���������

#include "stm32f1xx_hal.h" //for SCB->VTOR,FLASH_PAGE_SIZE

#include "cmsis_os.h" // ���� freertos

#include "serial_hal.h"
#include "serial_console.h"
#include "iap_hal.h"

#include "kernel.h"
#include "shell.h"
#include "vim.h"
//--------------------��غ궨�弰�ṹ�嶨�v-------------------
osThreadId SerialConsoleTaskHandle;
osSemaphoreId osSerialRxSemHandle;
static char task_list_buf[600];

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

//------------------------------��غ�������------------------------------



//------------------------------�����ķָ���------------------------------

/*
	* for freertos;
	configUSE_STATS_FORMATTING_FUNCTIONS    1
	portCONFIGURE_TIMER_FOR_RUN_TIME_STATS  1
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
	* @brief iap_gets  iap �������񣬻�ȡ��������д��flash
	* @param void
	* @return NULL
*/
static void iap_gets(struct shell_input * shell ,char * buf , uint32_t len)
{
	uint32_t * value = (uint32_t*)buf;
	
	shell->apparg = (void*)MODIFY_MASK;

	for (iap.size = iap.addr + len ; iap.addr < iap.size ; iap.addr += 4)// f4 ������ word д��
		iap_write_flash(iap.addr,*value++); 
	
	if ((iap.addr & (FLASH_PAGE_SIZE-1)) == 0)//�����һҳ
		iap_erase_flash(iap.addr ,1) ;
	else
		printl(".",1);//��ӡһ������ʾ���
}



/**
	* @brief    shell_iap_command
	*           ��������Ӧ����
	* @param    arg  : �������ڴ�ָ��
	* @return   void
*/
void shell_iap_command(void * arg)
{
	int argc , erasesize ;

	struct shell_input * shell = container_of(arg, struct shell_input, cmdline);
	shell->gets = iap_gets;//������������ȡ�� iap_gets

	argc = cmdline_param((char*)arg,&erasesize,1);

	iap.addr = (SCB->VTOR == FLASH_BASE) ? APP_ADDR : IAP_ADDR;
	iap.size = (argc == 1) ? erasesize : 1 ;

	//����Ҫд�����һ�����ݲ�������������������������
	iap_unlock_flash();
	iap_erase_flash(iap.addr , iap.size);
	color_printk(light_green,"\033[2J\033[%d;%dH%s",0,0,iap_logo);//����
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
		// �ȴ��ź���ʱ�䣬�� iap ģʽ�еȴ����� 1s ��ʱ����Ϊ iap ���ս���
		wait = serial_shell.apparg == (void*)MODIFY_MASK ? 1000 : osWaitForever;
		
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
	*          ��ȡ syscfg ��Ϣ���� shell_into_edit ����
	* @param
	* @return   �ɹ� ����VIM_FILE_OK
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
	*          ���� syscfg ��Ϣ���� shell_into_edit ����
	* @param
	* @return   void
*/
void _syscfg_fputs(char * fpath, char * fdata,uint32_t fsize)
{
	syscfg_write(fdata,fsize);
}



/**
	* @brief    _shell_edit_syscfg
	*           �����б༭ syscfg ��Ϣ
	* @param    arg  �������ڴ�
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
	color_printk(purple,"%s",info);//��ӡ������Ϣ���߿���̨��Ϣ

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
	osSerialRxSemHandle = osSemaphoreCreate(osSemaphore(osSerialRxSem), 1); //�����ж��ź��Y
  
	osThreadDef(SerialConsole, task_SerialConsole, osPriorityNormal, 0, 168+(sizeof(struct shell_input)/4));
	SerialConsoleTaskHandle = osThreadCreate(osThread(SerialConsole), NULL);
}






