/**
  ******************************************************************************
  * @file           shell.h
  * @author         古么宁
  * @brief          命令解释器头文件
  * 使用步骤：
  *    0.初始化硬件部分。
  *    1.编写硬件对应的void puts(char * buf , uint16_t len) 发送函数。
  *    2.shell_init(sign,puts) 初始化输入标志和默认输出。
  *    3.新建一个  shellinput_t shellx , 初始化输出 SHELL_INPUT_INIT(&shellx,puts);
  *    4.接收到一包数据后，调用 shell_input(shellx,buf,len)
  ******************************************************************************
  *
  * COPYRIGHT(c) 2018 GoodMorning
  *
  ******************************************************************************
*/
#ifndef __SHELL_H__
#define __SHELL_H__

// 以下为 shell 所依赖的基本库
#include "ustdio.h"


/* Public macro (共有宏)------------------------------------------------------------*/

/* option (配置项) */

//命令数超过30条时可以考虑用平衡二叉树进行查找匹配
//#define USE_AVL_TREE

//命令带上参数的字符串输入最长记录长度
#define COMMANDLINE_MAX_LEN       36     

//控制台记录条目数，设为 0 时不记录
#define COMMANDLINE_MAX_RECORD    4     



#ifdef USE_AVL_TREE
	#include "avltree.h"//命令索引用avl树进行查找匹配
#endif


#define KEYCODE_END               35
#define KEYCODE_HOME              36
#define KEYCODE_CTRL_C            0x03
#define KEYCODE_BACKSPACE         0x08   //键盘的回退键
#define KEYCODE_TAB               '\t'   //键盘的tab键
#define KEYCODE_NEWLINE           0x0A
#define KEYCODE_ENTER             0x0D   //键盘的回车键
#define KEYCODE_ESC               0x1b



/*
-----------------------------------------------------------------------
	调用宏 shell_register_command(pstr,pfunc) 注册命令
	注册一个命令号的同时会新建一个与命令对应的控制块
	在 shell 注册的函数类型统一为 void(*CmdFuncDef)(void * arg); 
	arg 为控制台输入命令后所跟的参数输入
-----------------------------------------------------------------------
*/
#define shell_register_command(pstr,pfunc)\
	do{\
		static struct shell_cmd newcmd = {0};\
		_shell_register(pstr,pfunc,&newcmd); \
	}while(0)


//初始化一个 shell 交互，默认输入为 cmdline_gets
#define SHELL_INPUT_INIT(shellinput,shellputs) \
	do{\
		(shellinput)->gets    = cmdline_gets;\
		(shellinput)->puts	  = shellputs;\
		(shellinput)->buftail = 0;		  \
		(shellinput)->edit	  = 0;		  \
	}while(0)


//获取一个命令的长度
#define SHELL_CMD_LEN(pCommand)  (((pCommand)->ID >> 21) & 0x0000001F)


//shell 入口对应出口，从哪里输入则从对应的地方输出
#define shell_input(shell,buf,len) \
	do{\
		current_puts = (shell)->puts;      \
		(shell)->gets((shell),(buf),(len));\
		current_puts = default_puts;       \
	}while(0)


/* Public types ------------------------------------------------------------*/

enum INPUT_PARAMETER
{
	PARAMETER_ERROR = -2,
	PARAMETER_HELP = -1,
};


// 命令对应的函数类型，至于为什么输入设计为 void *,我不记得了
typedef void (*cmd_fn_t)(void * arg);


// 单链表节点，用来串命令
typedef struct shell_list
{
	struct shell_list * next;
}
shell_list_t ;

//命令结构体，用于注册匹配命令
typedef struct shell_cmd
{
	uint32_t	  ID;	//命令标识码
	char *		  name; //记录每条命令字符串的内存地址
	cmd_fn_t	  Func; //记录命令函数指针
	
	#ifdef USE_AVL_TREE
		struct avl_node cmd_node;//avl树节点
	#else
		struct shell_list cmd_node;//链表节点
	#endif
}
shellcmd_t;


// 交互结构体，数据的输入输出不一定
typedef struct shell_input
{
	//指定数据流输入,初始化默认为 cmdline_gets() ,即命令行
	void (*gets)(struct shell_input * , char * ,uint32_t );

	//指定数据流对应的输出接口，串口或者 telnet 输出等
	fmt_puts_t puts;

	//app可用参数，爱怎么用就怎么用
	void *  apparg;       

	//命令行相关的参数
	uint8_t edit;       //当前命令行编辑位置
	uint8_t buftail;    //当前命令行输入结尾
	char    buf[COMMANDLINE_MAX_LEN]; //命令行内存
}
shellinput_t;




/* Public variables ---------------------------------------------------------*/

extern const char DEFAULT_INPUTSIGN[];
extern char  shell_input_sign[];


/* Public function prototypes 对外可用接口 -----------------------------------*/

//注册命令，这个函数一般不直接调用，用宏 shell_register_command() 间接调用
void _shell_register(char * , cmd_fn_t func , struct shell_cmd * );

//默认命令行输入端
void cmdline_gets(struct shell_input * ,char * ,uint32_t );

//解析命令行
int  cmdline_param(char * str,int * argv,uint32_t maxread);

int  cmdline_strtok(char * str ,char ** argv ,uint32_t maxread);


void shell_confirm(struct shell_input * shell ,char * info ,cmd_fn_t yestodo);

void shell_init(char * sign,fmt_puts_t puts);

#endif


