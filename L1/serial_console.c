/**
  ******************************************************************************
  * @file           serial_console.c
  * @author         ��ô��
  * @brief          serial_console file
                    ���ڿ���̨�ļ����ļ���ֱ�Ӳ���Ӳ�������� serial_hal.c �� iap_hal.c
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

#include "AtomROS.h"
#include "iap_hal.h"

#include "shell.h"
#include "serial_hal.h"
#include "serial_console.h"

#include "stm32l1xx_hal.h" //for SCB->VTOR

//--------------------��غ궨�弰�ṹ�嶨��--------------------


#define UASRT_IAP_BUF_SIZE  1024
#define USART_IAP_ADDR_MASK (FLASH_PAGE_SIZE -1)


const static char iap_logo[]=
"\r\n\
 ____   ___   ____\r\n\
|_  _| / _ \\ |  _ \\\r\n\
 _||_ |  _  ||  __/don't press any key now\r\n\
|____||_| |_||_|   ";



static struct serial_iap
{
	uint32_t timestamp;
	uint32_t addr;
	uint32_t size;
}
iap;

//ros_semaphore_t rosSerialRxSem;

static ros_task_t iap_task;
static ros_task_t iap_timeout_task;
static ros_task_t serial_console_task;


static struct shell_input serial_shell;
//------------------------------��غ�������------------------------------




//------------------------------�����ķָ���------------------------------


/**
	* @brief iap_check_complete iap ������ʱ�ж�����
	* @param void
	* @return NULL
*/
int iap_check_complete(void * arg)
{
	TASK_BEGIN();//����ʼ
	
	printk("loading");
	
	task_cond_wait(OS_current_time - iap.timestamp > 1600) ;//��ʱ 1.6 s
	
	task_cancel(&iap_task); // ɾ�� iap ����
	
	iap_lock_flash();   //����Ҫд�����һ�����ݲ��������������������� iap_check_complete ��

	serial_recv_reset(COMMANDLINE_MAX_LEN); //���ô��ڽ��հ���
	
	uint32_t filesize = (SCB->VTOR == FLASH_BASE) ? (iap.addr-APP_ADDR):(iap.addr-IAP_ADDR);
	
	printk("\r\nupdate completed!\r\nupdate package size:%d byte\r\n",filesize);

	TASK_END();
}


/** 
	* @brief iap_recv_process  iap ��������
	* @param void
	* @return NULL
*/
int iap_recv_process(void * arg)
{
	uint32_t * value;
	char   * pktdata;
	uint16_t pktsize;
	
	TASK_BEGIN();//����ʼ
	
	iap_unlock_flash();//����Ҫд�����һ�����ݲ��������������������� iap_check_complete ��
	
	if( iap_erase_flash(iap.addr ,1))
	{
		Error_Here();//����������	
		task_exit();
	}

	color_printk(light_green,"\033[2J\033[%d;%dH%s",0,0,iap_logo);//����
	
	while(1)
	{
		task_cond_wait(serial_rxpkt_queue_out(&pktdata,&pktsize));//�ȴ����յ�һ������

		value = (uint32_t*)pktdata;

		for (iap.size = iap.addr + pktsize ; iap.addr < iap.size ; iap.addr += 4)// f4 ������ word д��
			iap_write_flash(iap.addr,*value++); 

		iap.timestamp = OS_current_time;//����ʱ���

		if ((iap.addr & USART_IAP_ADDR_MASK) == 0)//�����һҳ
			iap_erase_flash(iap.addr ,1) ;

		if (task_is_exited(&iap_timeout_task))
			task_create(&iap_timeout_task,NULL,iap_check_complete,NULL);
		else
			printl(".",1);

	}
	
	TASK_END();
}




/** 
	* @brief serial_console_recv  ���ڿ���̨����
	* @param void
	* @return NULL
*/
int serial_console_recv(void * arg)
{
	char  *  packet;
	uint16_t pktlen ;

	TASK_BEGIN();//����ʼ
	
	while(1)
	{
		task_cond_wait(serial_rxpkt_queue_out(&packet,&pktlen));

		shell_input(&serial_shell,packet,pktlen);//����֡����Ӧ�ò�
		
		task_join(&iap_task); //��������ʱ�������� iap ������
	}
	
	TASK_END();
}



/** 
	* @brief shell_iap_command  iap ��������
	* @param void
	* @return NULL
*/
void shell_iap_command(void * arg)
{
	iap.addr = (SCB->VTOR == FLASH_BASE) ? APP_ADDR : IAP_ADDR;//����� iap ģʽ������ app ����

	task_create(&iap_task,NULL,iap_recv_process,NULL);
	
	serial_recv_reset(UASRT_IAP_BUF_SIZE);
}






void shell_erase_flash(void * arg)
{
	static const char tips[] = "flash-erase (addr) [size]\r\n";
	int argc ;
	int argv[5];

	argc = shell_cmdparam((char*)arg,argv);

	printk("erase flash ",argc);

	if (argc < 1)
	{
		printl((char*)tips,sizeof(tips)-1);
	}
	else
	{
		uint32_t addr = argv[0];
		uint32_t size = (argc == 1)? 1:argv[1];//Ĭ�ϲ���һ������
		
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




void serial_console_init(char * info)
{
	hal_serial_init(); //�ȳ�ʼ��Ӳ����
	
	SHELL_INPUT_INIT(&serial_shell,serial_puts);

	if (SCB->VTOR != FLASH_BASE)
	{
		shell_register_command("update-iap",shell_iap_command);	
	}
	else
	{
		shell_register_command("update-app",shell_iap_command);	
		shell_register_command("jump-app",shell_jump_command);
	}

	shell_register_command("reboot"  ,shell_reboot_command);
	shell_register_command("flash-erase",shell_erase_flash);
	
	task_create(&serial_console_task,NULL,serial_console_recv,NULL);
	
	color_printk(purple,"%s",info);//��ӡ������Ϣ���߿���̨��Ϣ
	
	while(serial_busy()); //�ȴ���ӡ����
}







