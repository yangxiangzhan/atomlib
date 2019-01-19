/**
  ******************************************************************************
  * @file           shell.h
  * @author         ��ô��
  * @brief          ���������ͷ�ļ�
  * ʹ�ò��裺
  *    0.��ʼ��Ӳ�����֡�
  *    1.��дӲ����Ӧ��void puts(char * buf , uint16_t len) ���ͺ�����
  *    2.shell_init(sign,puts) ��ʼ�������־��Ĭ�������
  *    3.�½�һ��  shellinput_t shellx , ��ʼ����� shell_input_init(&shellx,puts,...);
  *    4.���յ�һ�����ݺ󣬵��� shell_input(shellx,buf,len)
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

/* ---- option (������) ---- */

//����������30��ʱ���Կ�����ƽ����������в���ƥ��
//#define USE_AVL_TREE

//������ϲ������ַ����������¼����
#define COMMANDLINE_MAX_LEN     36     

//����̨��¼��Ŀ������Ϊ 0 ʱ����¼
#define COMMANDLINE_MAX_RECORD  4

/* ---- option end ---- */


// һЩ��ֵ��
#define KEYCODE_END               35
#define KEYCODE_HOME              36
#define KEYCODE_CTRL_C            0x03
#define KEYCODE_BACKSPACE         0x08   //���̵Ļ��˼�
#define KEYCODE_TAB               '\t'   //���̵�tab��
#define KEYCODE_NEWLINE           0x0A
#define KEYCODE_ENTER             0x0D   //���̵Ļس���
#define KEYCODE_ESC               0x1b


#define MODIFY_MASK  0xABCD4320


#define NEED_CONFIRM MODIFY_MASK
#define DONT_CONFIRM 0
#define FUNC_CONFIRM (1U<<31) // stm32 flash ��ַ��Χ���λ
/*
-----------------------------------------------------------------------
	���ú� shell_register_command(pstr,pfunc) ע������
	ע��һ������ŵ�ͬʱ���½�һ���������Ӧ�Ŀ��ƿ�
	�� shell ע��ĺ�������ͳһΪ void(*cmd_fn_t)(void * arg); 
	arg Ϊ����̨��������������Ĳ�������
-----------------------------------------------------------------------
*/
#define shell_register_command(name,func)\
	do{\
		static struct shellcommand newcmd = {0};\
		_shell_register(&newcmd,name,func,DONT_CONFIRM); \
	}while(0)


//ע��һ����ѡ�������Ҫ���� [Y/N/y/n] ��ִ�ж�Ӧ������
#define shell_register_confirm(name,func,info)\
	do{\
		static struct shellconfirm confirm = {.prompt = info}; \
		_shell_register(&confirm.cmd,name,func,NEED_CONFIRM);\
	}while(0)



// ����Ϊ shell_input_init() ���ú�
#define MODIFY_SIGN (MODIFY_MASK|0x1)
#define MODIFY_GETS (MODIFY_MASK|0x2)

//��ʷ�������⣬���ݾɰ汾����
#define SHELL_INPUT_INIT(...) shell_input_init(__VA_ARGS__)


//��ȡһ������ĳ���
#define SHELL_CMD_LEN(pCommand)  (((pCommand)->ID >> 21) & 0x0000001F)


//shell ��ڶ�Ӧ���ڣ�������������Ӷ�Ӧ�ĵط����
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


// �����Ӧ�ĺ������ͣ�����Ϊʲô�������Ϊ void *,�Ҳ��ǵ���
typedef void (*cmd_fn_t)(void * arg);



#ifdef USE_AVL_TREE     // ����������avl�����в���ƥ��
	#include "avltree.h"
	typedef struct avl_node cmd_entry_t ;
	typedef struct avl_root cmd_root_t ;
#else                   // ������ڵ㣬����������
	struct slist{struct slist * next;} ;
	typedef struct slist cmd_entry_t ;
	typedef struct slist cmd_root_t ;
#endif



//����ṹ�壬����ע��ƥ������
typedef struct shellcommand
{
	cmd_entry_t   node; //������������㣬������������������������
	char *		  name; //��¼ÿ�������ַ������ڴ��ַ
	uint32_t	  ID;	//�����ʶ��
	uint32_t	  fnaddr; //��¼����� cmd_fn_t ��Ӧ���ڴ��ַ
}
shellcmd_t;


//��ȷ��ѡ�������ṹ��
typedef struct shellconfirm
{
	char * prompt ; //ȷ����ʾ��Ϣ
	struct shellcommand  cmd;//��Ӧ��������ڴ�
}
shellcfm_t ;



// �����ṹ�壬���ݵ����������һ��
typedef struct shell_input
{
	//ָ������������,��ʼ��Ĭ��Ϊ cmdline_gets() ,��������
	void (*gets)(struct shell_input * , char * ,uint32_t );

	//ָ����������Ӧ������ӿڣ����ڻ��� telnet �����
	fmt_puts_t puts;

	//app���ò���������ô�þ���ô��
	void *  apparg;

	//�������������
	char    sign[COMMANDLINE_MAX_LEN];

	//��������صĲ���
	char    cmdline[COMMANDLINE_MAX_LEN]; //�������ڴ�
	uint8_t edit;    //��ǰ�����б༭λ��
	uint8_t tail;    //��ǰ�����������β tail

	#if (COMMANDLINE_MAX_RECORD) //�����������ʷ��¼
		uint8_t htywrt;  //��ʷ��¼д
		uint8_t htyread; //��ʷ��¼��
		char    history[COMMANDLINE_MAX_RECORD][COMMANDLINE_MAX_LEN]; //��ʷ��¼�ڴ�
	#endif
}
shellinput_t;


typedef	void (*shellgets_t)(struct shell_input * , char * ,uint32_t );

/* Public variables ---------------------------------------------------------*/

extern char DEFAULT_INPUTSIGN[]; // Ĭ�Ͻ�����־


/* Public function prototypes ������ýӿ� -----------------------------------*/

//ע������������һ�㲻ֱ�ӵ��ã��ú� shell_register_command() ��ӵ���
void _shell_register(struct shellcommand * newcmd,char * cmd_name, cmd_fn_t cmd_func,uint32_t comfirm);

//Ĭ�������������
void cmdline_gets(struct shell_input * ,char * ,uint32_t );

//���������в�����ع��ܺ���
int  cmdline_param(char * str,int * argv,uint32_t maxread);
int  cmdline_strtok(char * str ,char ** argv ,uint32_t maxread);

// ��ʼ����غ���
void shell_init(char * defaultsign ,fmt_puts_t puts);
void shell_input_init(struct shell_input * shellin , fmt_puts_t shellputs,...);

#endif


