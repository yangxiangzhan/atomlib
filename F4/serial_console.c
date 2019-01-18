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

#include "tasklib.h"
#include "iap_hal.h"

#include "shell.h"
#include "vim.h"
#include "serial_hal.h"
#include "serial_console.h"

#include "stm32f429xx.h" //for SCB->VTOR


/* Private macro ------------------------------------------------------------*/

#define SYSTEM_CONFIG_FILE 

#define UASRT_IAP_BUF_SIZE  1024

/* Private variables ------------------------------------------------------------*/

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

static ros_task_t iap_timeout_task;
static ros_task_t serial_console_task;


static struct shell_input serial_shell;

/* Global variables ------------------------------------------------------------*/


/* Private function prototypes -----------------------------------------------*/



/* Gorgeous Split-line �����ķָ���------------------------------------*/



/**
	* @brief iap_check_complete iap ������ʱ�ж�����
	* @param void
	* @return NULL
*/
static int iap_check_complete(void * arg)
{
	struct shell_input * shell;
	uint32_t filesize ;

	TASK_BEGIN();//����ʼ
	
	printk("loading");
	
	task_cond_wait(OS_current_time - iap.timestamp > 1600) ;//��ʱ 1.6 s
	
	iap_lock_flash();   //����Ҫд�����һ�����ݲ��������������������� iap_check_complete ��
	
	serial_recv_reset(COMMANDLINE_MAX_LEN); //���ô��ڽ��հ���
	
	filesize = (SCB->VTOR == FLASH_BASE) ? (iap.addr-APP_ADDR):(iap.addr-IAP_ADDR);
	
	printk("\r\nupdate completed!\r\nupdate package size:%d byte\r\n",filesize);

	shell = (struct shell_input*)arg;
	shell->gets = cmdline_gets;       //�ָ�����������ģʽ

	TASK_END();
}




/** 
	* @brief serial_console_recv  ���ڿ���̨�������񣬲�ֱ�ӵ���
	* @param void
	* @return int 
*/
static int serial_console_recv(void * arg)
{
	char  *  packet;
	uint16_t pktlen ;

	TASK_BEGIN();//����ʼ
	
	while(1)
	{
		task_cond_wait(serial_rxpkt_queue_out(&packet,&pktlen));//�ȴ����ڽ���

		shell_input(&serial_shell,packet,pktlen);//����֡����Ӧ�ò�
	}
	
	TASK_END();
}





/** 
	* @brief iap_gets  iap �������񣬻�ȡ��������д��flash
	* @param void
	* @return NULL
*/
static void iap_gets(struct shell_input * shell ,char * buf , uint32_t len)
{
	uint32_t * value = (uint32_t*)buf;
	
	for (iap.size = iap.addr + len ; iap.addr < iap.size ; iap.addr += 4)// f4 ������ word д��
		iap_write_flash(iap.addr,*value++); 

	iap.timestamp = OS_current_time;//����ʱ���

	if (task_is_exited(&iap_timeout_task))//��ʼ���պ󴴽���ʱЯ�̣��жϽ��ս���
		task_create(&iap_timeout_task,NULL,iap_check_complete,shell);
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

	//����Ҫд�����һ�����ݲ��������������������� iap_check_complete ��
	iap_unlock_flash();
	iap_erase_flash(iap.addr , iap.size);
	color_printk(light_green,"\033[2J\033[%d;%dH%s",0,0,iap_logo);//����
}








static void shell_erase_flash(void * arg)
{
	static const char tips[] = "flash-erase (addr) [size]\r\n";
	int argc ;
	int argv[2];

	argc = cmdline_param((char*)arg,argv,2);

	printk("erase flash ");

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


#ifdef OS_USE_ID_AND_NAME
/**
	* @brief    shell_show_protothread
	*           ��������Ӧ����,�г����� protothread
	* @param
	* @return   void
*/
void shell_show_protothread(void * arg)
{
	struct list_head * TaskListNode;
	struct protothread * pthread;

	if (list_empty(&OS_scheduler_list)) return;

	printk("\r\n\tID\t\tThread\r\n");

	list_for_each(TaskListNode,&OS_scheduler_list)
	{
		pthread = list_entry(TaskListNode,struct protothread,list_node);
		printk("\t%d\t\t%s\r\n",pthread->ID, pthread->name);
	}
}


/**
	* @brief    shell_kill_protothread
	*           ��������Ӧ����,ɾ��ĳ�� protothread
	* @param
	* @return   void
*/
void shell_kill_protothread(void * arg)
{
	int searchID ;

	struct list_head * search_list;
	struct protothread * pthread;

	if (1 != cmdline_param((char*)arg,&searchID,1))
		return ;

	list_for_each(search_list, &OS_scheduler_list)
	{
		pthread = list_entry(search_list,struct protothread,list_node);

		if (searchID == pthread->ID)
		{
			task_cancel(pthread);
			printk("\r\nKill %s\r\n",pthread->name);
		}
	}
}

#endif


#ifdef SYSTEM_CONFIG_FILE

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


/**
	* @brief    _shell_rm_syscfg
	*           ������ɾ�� syscfg ��Ϣ
	* @param    arg  �������ڴ�
	* @return   void
*/
void _shell_rm_syscfg(void * arg)
{
	syscfg_erase();
}

#endif //#ifdef SYSTEM_CONFIG_FILE


/**
	* @brief    serial_console_init
	*           ���ڿ���̨��ʼ��
	* @param    info:  ��ʼ����Ϣ
	* @return   void
*/
void serial_console_init(char * info)
{
	hal_serial_init(); //�ȳ�ʼ��Ӳ����
	
	SHELL_INPUT_INIT(&serial_shell,serial_puts);//�½����������Ϊ�������

	shell_register_command("reboot"  ,shell_reboot_command);
	shell_register_command("flash-erase",shell_erase_flash);

	//���� SCB->VTOR �жϵ�ǰ�����еĴ���Ϊ app ���� iap.��ͬ����ע�᲻һ��������
	if (SCB->VTOR == FLASH_BASE)
		shell_register_command("update-app",shell_iap_command);
	else
		shell_register_confirm("update-iap",shell_iap_command,"sure to update iap?");
	
	#ifdef SYSTEM_CONFIG_FILE
		shell_register_command("syscfg",_shell_edit_syscfg);
	#endif

	#ifdef OS_USE_ID_AND_NAME
		shell_register_command("top",shell_show_protothread);
		shell_register_command("kill",shell_kill_protothread);
	#endif

	//����һ�����ڽ������񣬴��ڽ��յ�һ������ serial_console_recv 
	task_create(&serial_console_task,NULL,serial_console_recv,NULL);
	
	serial_puts(info,strlen(info));//��ӡ������Ϣ���߿���̨��Ϣ
	
	while(serial_busy()); //�ȴ���ӡ����
}






