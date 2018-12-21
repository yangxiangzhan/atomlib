/**
  ******************************************************************************
  * @file           shell.h
  * @author         ��ô��
  * @brief          ���������ͷ�ļ�
  ******************************************************************************
  *
  * COPYRIGHT(c) 2018 GoodMorning
  *
  ******************************************************************************
*/
#ifndef __SHELL_H__
#define __SHELL_H__

// ����Ϊ shell �������Ļ�����
#include "ustdio.h"


/* Public macro (���к�)------------------------------------------------------------*/

//����������30��ʱ���Կ�����ƽ����������в���ƥ��
//#define USE_AVL_TREE

#ifdef USE_AVL_TREE
	#include "avltree.h"//����������avl�����в���ƥ��
#endif


#define     COMMANDLINE_MAX_RECORD    4      //����̨��¼��Ŀ��
#define     COMMANDLINE_MAX_LEN       36     //������ϲ������ַ����������¼����


#define     KEYCODE_CTRL_C            0x03
#define     KEYCODE_NEWLINE           0x0A
#define     KEYCODE_ENTER             0x0D   //���̵Ļس���
#define     KEYCODE_BACKSPACE         0x08   //���̵Ļ��˼�
#define     KEYCODE_ESC               0x1b
#define     KEYCODE_END               35
#define     KEYCODE_HOME              36
#define     KEYCODE_TAB               '\t'   //���̵�tab��

#define     KEEP_STREAM    0
#define     RELEASE_STREAM 1
/*
-----------------------------------------------------------------------
	���ú� shell_register_command(pstr,pfunc) ע������
	ע��һ������ŵ�ͬʱ���½�һ���������Ӧ�Ŀ��ƿ�
	�� shell ע��ĺ�������ͳһΪ void(*CmdFuncDef)(void * arg); 
	arg Ϊ����̨��������������Ĳ�������
-----------------------------------------------------------------------
*/
#define shell_register_command(pstr,pfunc)\
	do{\
		static struct shell_cmd newcmd = {0};\
		_shell_register(pstr,pfunc,&newcmd); \
	}while(0)


//��ʼ��һ�� shell ������Ĭ������Ϊ cmdline_gets
#define SHELL_INPUT_INIT(shellinput,shellputs) \
	do{\
		(shellinput)->gets    = cmdline_gets;\
		(shellinput)->puts	  = shellputs;\
		(shellinput)->buftail = 0;		  \
		(shellinput)->edit	  = 0;		  \
	}while(0)


//��ȡһ������ĳ���
#define SHELL_CMD_LEN(pCommand)  (((pCommand)->ID >> 21) & 0x0000001F)


/* Public types ------------------------------------------------------------*/

enum INPUT_PARAMETER
{
	PARAMETER_ERROR = -2,
	PARAMETER_HELP = -1,
};


// �����Ӧ�ĺ������ͣ�����Ϊʲô�������Ϊ void *,�Ҳ��ǵ���
typedef void (*cmd_fn_t)(void * arg);

// �������ڵ㣬����������
typedef struct shell_list
{
	struct shell_list * next;
}
shell_list_t ;

//����ṹ�壬����ע��ƥ������
typedef struct shell_cmd
{
	uint32_t	  ID;	//�����ʶ��
	char *		  name; //��¼ÿ�������ַ������ڴ��ַ
	cmd_fn_t	  Func; //��¼�����ָ��
	
	#ifdef USE_AVL_TREE
		struct avl_node cmd_node;//avl���ڵ�
	#else
		struct shell_list cmd_node;//�����ڵ�
	#endif
}
shellcmd_t;


// �����ṹ�壬���ݵ����������һ��
typedef struct shell_input
{
	//ָ������������,��ʼ��Ĭ��Ϊ cmdline_gets() ,��������
	void (*gets)(struct shell_input * , char * ,uint32_t );

	//ָ����������Ӧ������ӿڣ����ڻ��� telnet �����
	fmt_puts_t puts;

	//app���ò���������ô�þ���ô��
	void *  apparg;       

	//��������صĲ���
	uint8_t edit;       //��ǰ�����б༭λ��
	uint8_t buftail;    //��ǰ�����������β
	char    buf[COMMANDLINE_MAX_LEN]; //�������ڴ�
}
shellinput_t;


//shell ��ڶ�Ӧ���ڣ�������������Ӷ�Ӧ�ĵط����
#define shell_input(shell,buf,len) \
	do{\
		current_puts = (shell)->puts;      \
		(shell)->gets((shell),(buf),(len));\
		current_puts = default_puts;       \
	}while(0)


/* Public variables ---------------------------------------------------------*/

extern const char DEFAULT_INPUTSIGN[];
extern char  shell_input_sign[];


/* Public function prototypes ������ýӿ� -----------------------------------*/

//ע������������һ�㲻ֱ�ӵ��ã��ú� shell_register_command() ��ӵ���
void _shell_register(char * , cmd_fn_t func,struct shell_cmd * );

//Ĭ�������������
void cmdline_gets(struct shell_input * ,char * ,uint32_t );

int  shell_cmdparam(char * str,int * argv);

int  shell_option_suport(char * str ,char ** argv);

void shell_init(char * sign,fmt_puts_t puts);


#endif

