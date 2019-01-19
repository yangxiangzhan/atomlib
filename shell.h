/**
  ******************************************************************************
  * @file           shell.h
  * @author         古么宁
  * @brief          命令解释器头文件
  * 使用步骤：
  *    0.初始化硬件部分。
  *    1.编写硬件对应的void puts(char * buf , uint16_t len) 发送函数。
  *    2.shell_init(sign,puts) 初始化输入标志和默认输出。
  *    3.新建一个  shellinput_t shellx , 初始化输出 shell_input_init(&shellx,puts,...);
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

/* ---- option (配置项) ---- */

//命令数超过30条时可以考虑用平衡二叉树进行查找匹配
//#define USE_AVL_TREE

//命令带上参数的字符串输入最长记录长度
#define COMMANDLINE_MAX_LEN     36     

//控制台记录条目数，设为 0 时不记录
#define COMMANDLINE_MAX_RECORD  4

/* ---- option end ---- */


// 一些键值：
#define KEYCODE_END               35
#define KEYCODE_HOME              36
#define KEYCODE_CTRL_C            0x03
#define KEYCODE_BACKSPACE         0x08   //键盘的回退键
#define KEYCODE_TAB               '\t'   //键盘的tab键
#define KEYCODE_NEWLINE           0x0A
#define KEYCODE_ENTER             0x0D   //键盘的回车键
#define KEYCODE_ESC               0x1b


#define MODIFY_MASK  0xABCD4320


#define NEED_CONFIRM MODIFY_MASK
#define DONT_CONFIRM 0
#define FUNC_CONFIRM (1U<<31) // stm32 flash 地址范围最高位
/*
-----------------------------------------------------------------------
	调用宏 shell_register_command(pstr,pfunc) 注册命令
	注册一个命令号的同时会新建一个与命令对应的控制块
	在 shell 注册的函数类型统一为 void(*cmd_fn_t)(void * arg); 
	arg 为控制台输入命令后所跟的参数输入
-----------------------------------------------------------------------
*/
#define shell_register_command(name,func)\
	do{\
		static struct shellcommand newcmd = {0};\
		_shell_register(&newcmd,name,func,DONT_CONFIRM); \
	}while(0)


//注册一个带选项命令，需要输入 [Y/N/y/n] 才执行对应的命令
#define shell_register_confirm(name,func,info)\
	do{\
		static struct shellconfirm confirm = {.prompt = info}; \
		_shell_register(&confirm.cmd,name,func,NEED_CONFIRM);\
	}while(0)



// 以下为 shell_input_init() 所用宏
#define MODIFY_SIGN (MODIFY_MASK|0x1)
#define MODIFY_GETS (MODIFY_MASK|0x2)

//历史遗留问题，兼容旧版本代码
#define SHELL_INPUT_INIT(...) shell_input_init(__VA_ARGS__)


//获取一个命令的长度
#define SHELL_CMD_LEN(pCommand)  (((pCommand)->ID >> 21) & 0x0000001F)


//shell 入口对应出口，从哪里输入则从对应的地方输出
#define shell_input(shellin,buf,len) \
	do{\
		current_puts = (shellin)->puts;        \
		(shellin)->gets((shellin),(buf),(len));\
		current_puts = default_puts;           \
	}while(0)


/* Public types ------------------------------------------------------------*/

enum INPUT_PARAMETER
{
	PARAMETER_ERROR = -2,
	PARAMETER_HELP = -1,
};


// 命令对应的函数类型，至于为什么输入设计为 void *,我不记得了
typedef void (*cmd_fn_t)(void * arg);



#ifdef USE_AVL_TREE     // 命令索引用avl树进行查找匹配
	#include "avltree.h"
	typedef struct avl_node cmd_entry_t ;
	typedef struct avl_root cmd_root_t ;
#else                   // 单链表节点，用来串命令
	struct slist{struct slist * next;} ;
	typedef struct slist cmd_entry_t ;
	typedef struct slist cmd_root_t ;
#endif



//命令结构体，用于注册匹配命令
typedef struct shellcommand
{
	cmd_entry_t   node; //命令索引接入点，用链表或二叉树对命令作集合
	char *		  name; //记录每条命令字符串的内存地址
	uint32_t	  ID;	//命令标识码
	uint32_t	  fnaddr; //记录命令函数 cmd_fn_t 对应的内存地址
}
shellcmd_t;


//带确认选项的命令结构体
typedef struct shellconfirm
{
	char * prompt ; //确认提示信息
	struct shellcommand  cmd;//对应的命令号内存
}
shellcfm_t ;



// 交互结构体，数据的输入输出不一定
typedef struct shell_input
{
	//指定数据流输入,初始化默认为 cmdline_gets() ,即命令行
	void (*gets)(struct shell_input * , char * ,uint32_t );

	//指定数据流对应的输出接口，串口或者 telnet 输出等
	fmt_puts_t puts;

	//app可用参数，爱怎么用就怎么用
	void *  apparg;

	//命令行输入符号
	char    sign[COMMANDLINE_MAX_LEN];

	//命令行相关的参数
	char    cmdline[COMMANDLINE_MAX_LEN]; //命令行内存
	uint8_t edit;    //当前命令行编辑位置
	uint8_t tail;    //当前命令行输入结尾 tail

	#if (COMMANDLINE_MAX_RECORD) //如果定义了历史纪录
		uint8_t htywrt;  //历史记录写
		uint8_t htyread; //历史记录读
		char    history[COMMANDLINE_MAX_RECORD][COMMANDLINE_MAX_LEN]; //历史记录内存
	#endif
}
shellinput_t;


typedef	void (*shellgets_t)(struct shell_input * , char * ,uint32_t );

/* Public variables ---------------------------------------------------------*/

extern char DEFAULT_INPUTSIGN[]; // 默认交互标志


/* Public function prototypes 对外可用接口 -----------------------------------*/

//注册命令，这个函数一般不直接调用，用宏 shell_register_command() 间接调用
void _shell_register(struct shellcommand * newcmd,char * cmd_name, cmd_fn_t cmd_func,uint32_t comfirm);

//默认命令行输入端
void cmdline_gets(struct shell_input * ,char * ,uint32_t );

//解析命令行参数相关功能函数
int  cmdline_param(char * str,int * argv,uint32_t maxread);
int  cmdline_strtok(char * str ,char ** argv ,uint32_t maxread);

// 初始化相关函数
void shell_init(char * defaultsign ,fmt_puts_t puts);
void shell_input_init(struct shell_input * shellin , fmt_puts_t shellputs,...);

#endif


