/**
  ******************************************************************************
  * @file           serial_console.c
  * @author         古么宁
  * @brief          serial_console file
                    串口控制台文件。文件不直接操作硬件，依赖 serial_hal.c 和 iap_hal.c
  ******************************************************************************
  *
  * COPYRIGHT(c) 2018 GoodMorning
  *
  ******************************************************************************
  */
/* Includes ---------------------------------------------------*/
#include <string.h>
#include <stdarg.h>
#include <stdint.h> //定义了很多数据类型

#include "tasklib.h"
#include "iap_hal.h"

#include "shell.h"
#include "serial_hal.h"
#include "serial_console.h"

#include "stm32f1xx_hal.h" //for SCB->VTOR

//--------------------相关宏定义及结构体定义--------------------


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

static ros_task_t iap_timeout_task;
static ros_task_t serial_console_task;


static struct shell_input serial_shell;
//------------------------------相关函数声明------------------------------




//------------------------------华丽的分割线------------------------------



/**
	* @brief iap_check_complete iap 升级超时判断任务
	* @param void
	* @return NULL
*/
static int iap_check_complete(void * arg)
{
	struct shell_input * shell;
	uint32_t filesize ;

	TASK_BEGIN();//任务开始
	
	printk("loading");
	
	task_cond_wait(OS_current_time - iap.timestamp > 1600) ;//超时 1.6 s
	
	iap_lock_flash();   //由于要写完最后一包数据才能上锁，所以上锁放在 iap_check_complete 中
	
	serial_recv_reset(COMMANDLINE_MAX_LEN); //重置串口接收包长
	
	filesize = (SCB->VTOR == FLASH_BASE) ? (iap.addr-APP_ADDR):(iap.addr-IAP_ADDR);
	
	printk("\r\nupdate completed!\r\nupdate package size:%d byte\r\n",filesize);

	shell = (struct shell_input*)arg;
	shell->gets = cmdline_gets;       //恢复串口命令行模式

	TASK_END();
}






/** 
	* @brief iap_gets  iap 升级任务，获取数据流并写入flash
	* @param void
	* @return NULL
*/
static void iap_gets(struct shell_input * shell ,char * buf , uint32_t len)
{
	uint32_t * value = (uint32_t*)buf;
	
	for (iap.size = iap.addr + len ; iap.addr < iap.size ; iap.addr += 4)// f4 可以以 word 写入
		iap_write_flash(iap.addr,*value++); 

	iap.timestamp = OS_current_time;//更新时间戳
	
	if ((iap.addr & USART_IAP_ADDR_MASK) == 0)//清空下一页
		iap_erase_flash(iap.addr ,1) ;

	if (task_is_exited(&iap_timeout_task))//开始接收后创建超时携程，判断接收结束
		task_create(&iap_timeout_task,NULL,iap_check_complete,shell);
	else
		printl(".",1);//打印一个点以示清白
}



/**
	* @brief    iap_option_gets
	*           iap 键盘数据流入,因为擦除 iap 区域有可能会变砖，所以要有信息确认
	* @param    
	* @return   void
*/
static void iap_option_gets(struct shell_input * shell ,char * buf , uint32_t len)
{
	char * option = shell->apparg;
	
	if (0 == *option) //先输入 [Y/y/N/n] ，其他按键无效
	{
		if (*buf == 'Y' || *buf == 'y' || *buf == 'N' || *buf == 'n')
		{
			*option = *buf;
			printl(buf,1);
		}
	}
	else
	if (* buf == KEYCODE_BACKSPACE) //回退键
	{
		printk("\b \b");
		*option = 0;
	}
	else
	if (* buf == '\r' || * buf == '\n') //按回车确定
	{
		if ( *option == 'Y' || *option == 'y') //确定更新 iap
		{
			//由于要写完最后一包数据才能上锁，所以上锁放在 iap_check_complete 中
			iap_unlock_flash();
			iap_erase_flash(iap.addr , iap.size);
			color_printk(light_green,"\033[2J\033[%d;%dH%s",0,0,iap_logo);//清屏
			serial_recv_reset(UASRT_IAP_BUF_SIZE); //重置串口缓冲包大小
			shell->gets = iap_gets;		           //串口数据流获取至 iap_gets
		}
		else
		{
			printk("\r\ncancel update\r\n%s",shell_input_sign);
			shell->gets = cmdline_gets;        //串口数据回归为命令行模式
		}
	}
}




/** 
	* @brief serial_console_recv  串口控制台处理任务，不直接调用
	* @param void
	* @return int 
*/
static int serial_console_recv(void * arg)
{
	char  *  packet;
	uint16_t pktlen ;

	TASK_BEGIN();//任务开始
	
	while(1)
	{
		task_cond_wait(serial_rxpkt_queue_out(&packet,&pktlen));//等待串口接收

		shell_input(&serial_shell,packet,pktlen);//数据帧传入应用层
	}
	
	TASK_END();
}




/**
	* @brief    shell_iap_command
	*           命令行响应函数
	* @param    
	* @return   void
*/

void shell_iap_command(void * arg)
{
	int argc = 0;
	int argv[4];
	
	struct shell_input * this_input = container_of(arg, struct shell_input, buf);

	argc = shell_cmdparam((char*)arg,argv);

	if (SCB->VTOR == FLASH_BASE)//如果目前所在是 iap 模式，擦除 app 区域
	{
		iap.addr = APP_ADDR;
		iap.size = (argc == 1) ? argv[0] : 1 ;
		
		//由于要写完最后一包数据才能上锁，所以上锁放在 iap_check_complete 中
		iap_unlock_flash();
		iap_erase_flash(iap.addr , 1);
		color_printk(light_green,"\033[2J\033[%d;%dH%s",0,0,iap_logo);//清屏
		serial_recv_reset(UASRT_IAP_BUF_SIZE); //重置串口缓冲包大小
		this_input->gets = iap_gets;           //串口数据流获取至 iap_gets
	}	
 	else //如果目前所在是 app 模式，需要先提示信息
 	{ 
		iap.addr = IAP_ADDR; //iap 地址在 0x8000000,删除扇区0数据
		iap.size = (argc == 1) ? argv[0] : (1) ;
		
		printk("\r\nSure to update IAP?[Y/N] "); //需要输入确认
		this_input->gets = iap_option_gets;          //串口数据流获取至 iap_option_gets
		this_input->apparg = this_input->buf;
		this_input->buf[0] = 0;	
 	}	
}






void shell_erase_flash(void * arg)
{
	static const char tips[] = "flash-erase (addr) [page-size]\r\n";
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
		uint32_t size = (argc == 1)? 1:argv[1];//默认擦除一个扇区
		
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
	*           命令行响应函数,列出所有 protothread
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
	*           命令行响应函数,删除某个 protothread
	* @param
	* @return   void
*/
void shell_kill_protothread(void * arg)
{
	int argc;
	int argv[4];
	int searchID ;

	struct list_head * search_list;
	struct protothread * pthread;

	argc = shell_cmdparam((char*)arg,argv);

	if (argc ==  1)
		searchID = argv[0];
	else
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


/**
	* @brief    serial_console_init
	*           初始化串口控制台
	* @param
	* @return   void
*/
void serial_console_init(char * info)
{
	hal_serial_init(); //先初始化硬件层
	
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

	#ifdef OS_USE_ID_AND_NAME
		shell_register_command("top",shell_show_protothread);
		shell_register_command("kill",shell_kill_protothread);
	#endif


	task_create(&serial_console_task,NULL,serial_console_recv,NULL);
	
	color_printk(purple,"%s",info);//打印开机信息或者控制台信息
	
	while(serial_busy()); //等待打印结束
}







