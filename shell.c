/**
  ******************************************************************************
  * @file           shell.c
  * @author         ��ô��
  * @brief          shell ���������
  *                 ֧��  TAB �����ȫ���������Ҽ�ͷ ��BACKSPACE��ɾ
  * ʹ�ò��裺
  *    0.��ʼ��Ӳ�����֡�
  *    1.��дӲ����Ӧ��void puts(char * buf , uint16_t len) ���ͺ�����
  *    2.shell_init(sign,puts) ��ʼ�������־��Ĭ�������
  *    3.�½�һ��  shellinput_t shellx , ��ʼ����� SHELL_INPUT_INIT(&shellx,puts);
  *    4.���յ�һ�����ݺ󣬵��� shell_input(shellx,buf,len)
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
#include <stdio.h>
#include "shell.h"
#include "kernel.h"

/* Private types ------------------------------------------------------------*/

union uncmd
{
	struct // ����ŷ�Ϊ�����������  
	{
		uint32_t CRC2      : 8;
		uint32_t CRC1      : 8;//��ʮ��λΪ���� crc У����
		uint32_t Sum       : 5;//�����ַ����ܺ�
		uint32_t Len       : 5;//�����ַ��ĳ��ȣ�5 bit ��������Ȳ��ܳ���31���ַ�
		uint32_t FirstChar : 6;//�����ַ��ĵ�һ���ַ�
	}part;

	uint32_t ID;//�ɴ˺ϲ�Ϊ 32 λ��������
};

/* Private macro ------------------------------------------------------------*/

/* Private variables ------------------------------------------------------------*/

static const  uint8_t F_CRC8_Table[256] = {//����,��λ���� x^8+x^5+x^4+1
	0x00, 0x31, 0x62, 0x53, 0xc4, 0xf5, 0xa6, 0x97, 0xb9, 0x88, 0xdb, 0xea, 0x7d, 0x4c, 0x1f, 0x2e,
	0x43, 0x72, 0x21, 0x10, 0x87, 0xb6, 0xe5, 0xd4, 0xfa, 0xcb, 0x98, 0xa9, 0x3e, 0x0f, 0x5c, 0x6d,
	0x86, 0xb7, 0xe4, 0xd5, 0x42, 0x73, 0x20, 0x11, 0x3f, 0x0e, 0x5d, 0x6c, 0xfb, 0xca, 0x99, 0xa8,
	0xc5, 0xf4, 0xa7, 0x96, 0x01, 0x30, 0x63, 0x52, 0x7c, 0x4d, 0x1e, 0x2f, 0xb8, 0x89, 0xda, 0xeb,
	0x3d, 0x0c, 0x5f, 0x6e, 0xf9, 0xc8, 0x9b, 0xaa, 0x84, 0xb5, 0xe6, 0xd7, 0x40, 0x71, 0x22, 0x13,
	0x7e, 0x4f, 0x1c, 0x2d, 0xba, 0x8b, 0xd8, 0xe9, 0xc7, 0xf6, 0xa5, 0x94, 0x03, 0x32, 0x61, 0x50,
	0xbb, 0x8a, 0xd9, 0xe8, 0x7f, 0x4e, 0x1d, 0x2c, 0x02, 0x33, 0x60, 0x51, 0xc6, 0xf7, 0xa4, 0x95,
	0xf8, 0xc9, 0x9a, 0xab, 0x3c, 0x0d, 0x5e, 0x6f, 0x41, 0x70, 0x23, 0x12, 0x85, 0xb4, 0xe7, 0xd6,
	0x7a, 0x4b, 0x18, 0x29, 0xbe, 0x8f, 0xdc, 0xed, 0xc3, 0xf2, 0xa1, 0x90, 0x07, 0x36, 0x65, 0x54,
	0x39, 0x08, 0x5b, 0x6a, 0xfd, 0xcc, 0x9f, 0xae, 0x80, 0xb1, 0xe2, 0xd3, 0x44, 0x75, 0x26, 0x17,
	0xfc, 0xcd, 0x9e, 0xaf, 0x38, 0x09, 0x5a, 0x6b, 0x45, 0x74, 0x27, 0x16, 0x81, 0xb0, 0xe3, 0xd2,
	0xbf, 0x8e, 0xdd, 0xec, 0x7b, 0x4a, 0x19, 0x28, 0x06, 0x37, 0x64, 0x55, 0xc2, 0xf3, 0xa0, 0x91,
	0x47, 0x76, 0x25, 0x14, 0x83, 0xb2, 0xe1, 0xd0, 0xfe, 0xcf, 0x9c, 0xad, 0x3a, 0x0b, 0x58, 0x69,
	0x04, 0x35, 0x66, 0x57, 0xc0, 0xf1, 0xa2, 0x93, 0xbd, 0x8c, 0xdf, 0xee, 0x79, 0x48, 0x1b, 0x2a,
	0xc1, 0xf0, 0xa3, 0x92, 0x05, 0x34, 0x67, 0x56, 0x78, 0x49, 0x1a, 0x2b, 0xbc, 0x8d, 0xde, 0xef,
	0x82, 0xb3, 0xe0, 0xd1, 0x46, 0x77, 0x24, 0x15, 0x3b, 0x0a, 0x59, 0x68, 0xff, 0xce, 0x9d, 0xac
};

static const  uint8_t B_CRC8_Table[256] = {//����,��λ���� x^8+x^5+x^4+1
	0x00, 0x5e, 0xbc, 0xe2, 0x61, 0x3f, 0xdd, 0x83, 0xc2, 0x9c, 0x7e, 0x20, 0xa3, 0xfd, 0x1f, 0x41,
	0x9d, 0xc3, 0x21, 0x7f, 0xfc, 0xa2, 0x40, 0x1e, 0x5f, 0x01, 0xe3, 0xbd, 0x3e, 0x60, 0x82, 0xdc,
	0x23, 0x7d, 0x9f, 0xc1, 0x42, 0x1c, 0xfe, 0xa0, 0xe1, 0xbf, 0x5d, 0x03, 0x80, 0xde, 0x3c, 0x62,
	0xbe, 0xe0, 0x02, 0x5c, 0xdf, 0x81, 0x63, 0x3d, 0x7c, 0x22, 0xc0, 0x9e, 0x1d, 0x43, 0xa1, 0xff,
	0x46, 0x18, 0xfa, 0xa4, 0x27, 0x79, 0x9b, 0xc5, 0x84, 0xda, 0x38, 0x66, 0xe5, 0xbb, 0x59, 0x07,
	0xdb, 0x85, 0x67, 0x39, 0xba, 0xe4, 0x06, 0x58, 0x19, 0x47, 0xa5, 0xfb, 0x78, 0x26, 0xc4, 0x9a,
	0x65, 0x3b, 0xd9, 0x87, 0x04, 0x5a, 0xb8, 0xe6, 0xa7, 0xf9, 0x1b, 0x45, 0xc6, 0x98, 0x7a, 0x24,
	0xf8, 0xa6, 0x44, 0x1a, 0x99, 0xc7, 0x25, 0x7b, 0x3a, 0x64, 0x86, 0xd8, 0x5b, 0x05, 0xe7, 0xb9,
	0x8c, 0xd2, 0x30, 0x6e, 0xed, 0xb3, 0x51, 0x0f, 0x4e, 0x10, 0xf2, 0xac, 0x2f, 0x71, 0x93, 0xcd,
	0x11, 0x4f, 0xad, 0xf3, 0x70, 0x2e, 0xcc, 0x92, 0xd3, 0x8d, 0x6f, 0x31, 0xb2, 0xec, 0x0e, 0x50,
	0xaf, 0xf1, 0x13, 0x4d, 0xce, 0x90, 0x72, 0x2c, 0x6d, 0x33, 0xd1, 0x8f, 0x0c, 0x52, 0xb0, 0xee,
	0x32, 0x6c, 0x8e, 0xd0, 0x53, 0x0d, 0xef, 0xb1, 0xf0, 0xae, 0x4c, 0x12, 0x91, 0xcf, 0x2d, 0x73,
	0xca, 0x94, 0x76, 0x28, 0xab, 0xf5, 0x17, 0x49, 0x08, 0x56, 0xb4, 0xea, 0x69, 0x37, 0xd5, 0x8b,
	0x57, 0x09, 0xeb, 0xb5, 0x36, 0x68, 0x8a, 0xd4, 0x95, 0xcb, 0x29, 0x77, 0xf4, 0xaa, 0x48, 0x16,
	0xe9, 0xb7, 0x55, 0x0b, 0x88, 0xd6, 0x34, 0x6a, 0x2b, 0x75, 0x97, 0xc9, 0x4a, 0x14, 0xf6, 0xa8,
	0x74, 0x2a, 0xc8, 0x96, 0x15, 0x4b, 0xa9, 0xf7, 0xb6, 0xe8, 0x0a, 0x54, 0xd7, 0x89, 0x6b, 0x35
};

#if (COMMANDLINE_MAX_RECORD) //�����������ʷ��¼
	static struct _shell_record
	{
		char  buf[COMMANDLINE_MAX_RECORD][COMMANDLINE_MAX_LEN];
		uint8_t read;
		uint8_t write;
	}
	shell_history = {0};
#endif //#if (COMMANDLINE_MAX_RECORD) //�����������ʷ��¼

#ifdef USE_AVL_TREE
	static struct avl_root shell_root = {.avl_node = NULL};//����ƥ���ƽ����������� 
#else
	static struct shell_list shell_cmd_list = {.next = NULL};
#endif

/* Global variables ------------------------------------------------------------*/

const char DEFAULT_INPUTSIGN[] = "shell >";
char   shell_input_sign[128] = "shell >";

/* Private function prototypes -----------------------------------------------*/

static void   shell_getchar     (struct shell_input * input , char ch);
static void   shell_backspace   (struct shell_input * input) ;
static void   shell_tab         (struct shell_input * input) ;

#if (COMMANDLINE_MAX_RECORD)//�����������ʷ��¼
	static char * shell_record(struct shell_input * input);
	static void   shell_show_history(struct shell_input * input,uint8_t LastOrNext);
#else
#	define shell_record(x)
#	define shell_show_history(x,y)
#endif //#if (COMMANDLINE_MAX_RECORD)//�����������ʷ��¼

/* Gorgeous Split-line -----------------------------------------------*/


#ifdef USE_AVL_TREE //avl tree ������ѯϵͳ

/**
	* @brief    shell_search_cmd 
	*           ���������ң����� id ���ҵ���Ӧ�Ŀ��ƿ�
	* @param    cmdindex        �����
	* @return   �ɹ� id �Ŷ�Ӧ�Ŀ��ƿ�
*/
static struct shell_cmd *shell_search_cmd(int cmdindex)
{
    struct avl_node *node = shell_root.avl_node;

    while (node) 
	{
		struct shell_cmd * command = container_of(node, struct shell_cmd, cmd_node);

		if (cmdindex < command->ID)
		    node = node->avl_left;
		else 
		if (cmdindex > command->ID)
		    node = node->avl_right;
  		else 
			return command;
    }
    
    return NULL;
}



/**
	* @brief    shell_insert_cmd 
	*           ����������
	* @param    newcmd   ��������ƿ�
	* @return   �ɹ����� 0
*/
static int shell_insert_cmd(struct shell_cmd * newcmd)
{
	struct avl_node **tmp = &shell_root.avl_node;
 	struct avl_node *parent = NULL;
	
	/* Figure out where to put new node */
	while (*tmp)
	{
		struct shell_cmd *this = container_of(*tmp, struct shell_cmd, cmd_node);

		parent = *tmp;
		if (newcmd->ID < this->ID)
			tmp = &((*tmp)->avl_left);
		else 
		if (newcmd->ID > this->ID)
			tmp = &((*tmp)->avl_right);
		else
			return 1;
	}

	/* Add new node and rebalance tree. */
	avl_insert(&shell_root,&newcmd->cmd_node,parent,tmp);
	return 0;
}



/** 
	* @brief shell_tab ���� table ������
	* @param input
	*
	* @return NULL
*/
static void shell_tab(struct shell_input * input)
{
	char  *  str = input->buf;
	uint32_t strlen = input->buftail;
	uint32_t index;
	uint32_t end;
	
	struct avl_node  * node = shell_root.avl_node;	
	struct shell_cmd * shellcmd ;
	struct shell_cmd * match[10];    //ƥ�䵽��������
	uint32_t           match_cnt = 0;//ƥ�䵽������Ÿ���
	
	for ( ; *str == ' ' ; ++str,--strlen) ; //��ʱ�������ո���Ҫ����
	
	if (*str == 0 || strlen == 0 || node == NULL) //û��������Ϣ����
		return ;
	
	index = ((uint32_t)(*str)<<26) | (strlen << 21) ;//��������ĸ��ͬ���ҳ��Ȳ�С�� strlen �ĵ�
	end = (uint32_t)(*str + 1)<<26 ;                 //��һ����ĸ��ͷ�ĵ�

	while ( node )//index һ�������ڣ����ҽ������ shell_cmd ��� index ���ô���Ϊ��ʼƥ���
	{	
		shellcmd = avl_entry(node,struct shell_cmd, cmd_node);	
		node = (index < shellcmd->ID) ? node->avl_left : node->avl_right;
	}

	//����ĸ�Ĵ�С�������������Ĵ�С����������ĸ�ĵ㲻��Ҫ����ƥ��
    for (node = &shellcmd->cmd_node ; shellcmd->ID < end && node; node = avl_next(node))
	{
		shellcmd = avl_entry(node,struct shell_cmd, cmd_node);
		
		if (memcmp(shellcmd->name, str, strlen) == 0) //�Ա�������ַ��������ƥ�䵽��ͬ��
		{
			match[match_cnt] = shellcmd; //��ƥ�䵽�����������������
			if (++match_cnt > 10)        //����ʮ����ͬ����
				return ;    
		}
	}

	if (!match_cnt) //���û���������������ַ���������
		return ; 
	
	if (input->edit != input->buftail) //����༭λ�ò���ĩ�ˣ��Ȱѹ���Ƶ�ĩ��
	{
		printl(&input->buf[input->edit],input->buftail - input->edit);
		input->edit = input->buftail;
	}

	if (1 == match_cnt)  //���ֻ�ҵ���һ�����������ǰ������ַ�����ֱ�Ӳ�ȫ�������ӡ
	{
		for(char * ptr = match[0]->name + strlen ; *ptr ;++ptr) //��ӡʣ����ַ�
			shell_getchar(input,*ptr);
	}
	else   //�����ֹһ�����������ǰ������ַ�������ӡ������ͬ�ַ��������б�����ȫ�ַ������ֱ���������ֵ�
	{
		for(uint32_t i = 0;i < match_cnt; ++i) //�����к��������ַ����������б��ӡ����
			printk("\r\n\t%s",match[i]->name); 
		
		printk("\r\n%s%s",shell_input_sign,input->buf); //���´�ӡ�����־����������ַ���
		
		while(1)  //��ȫ�����ÿ������������ַ���ȫ����ӡ
		{
			for (uint32_t i = 1 ; i < match_cnt ; ++i )
			{
				if (match[0]->name[strlen] != match[i]->name[strlen])
					return  ; //�ַ���һ��������
			}
			shell_getchar(input,match[0]->name[strlen++]);  //����ͬ���ַ���ȫ�����뻺����
		}
	}
}


/********************************************************************
	* @author   ��ô��
	* @brief    shell_list_cmd 
	*           ��ʾ����ע���˵�����
	* @param    arg       �������������
	* @return   NULL
*/
static void shell_list_cmd(void * arg)
{
	uint32_t firstchar = 0;
	struct shell_cmd * cmd;
	struct avl_node  * node ;
	
	for (node = avl_first(&shell_root); node; node = avl_next(node))//���������
	{
		cmd = avl_entry(node,struct shell_cmd, cmd_node);
		if (firstchar != (cmd->ID & 0xfc000000))
		{
			firstchar = cmd->ID & 0xfc000000;
			printk("\r\n(%c)------",((firstchar>>26)|0x40));
		}
		printk("\r\n\t%s", cmd->name);
	}
	
	printk("\r\n\r\n%s",shell_input_sign);
}


#else  //#ifdef USE_AVL_TREE  ����������ѯϵͳ

/**
	* @brief    shell_search_cmd 
	*           ���������ң����� id ���ҵ���Ӧ�Ŀ��ƿ�
	* @param    cmdindex        �����
	* @return   �ɹ� id �Ŷ�Ӧ�Ŀ��ƿ�
*/
static struct shell_cmd *shell_search_cmd(int cmdindex)
{
	for (struct shell_list * node = shell_cmd_list.next; node ; node = node->next )
	{
		struct shell_cmd  * cmd = container_of(node, struct shell_cmd, cmd_node);
		if (cmd->ID > cmdindex)
			return NULL;
		else
		if (cmd->ID == cmdindex)
			return cmd;
	}
    
    return NULL;
}

/**
	* @brief    shell_insert_cmd 
	*           ����������
	* @param    newcmd   ��������ƿ�
	* @return   �ɹ����� 0
*/
static int shell_insert_cmd(struct shell_cmd * newcmd)
{
	struct shell_list * prev = &shell_cmd_list;
	struct shell_list * node ;

	for (node = prev->next; node ; prev = node,node = node->next)
	{
		struct shell_cmd * cmd = container_of(node, struct shell_cmd, cmd_node);
		if ( cmd->ID > newcmd->ID )
			break;
		else
		if (cmd->ID == newcmd->ID )
			return 1;
	}

	prev->next = &newcmd->cmd_node ;
	prev = prev->next;
	prev->next = node;

	return 0;
}


/** 
	* @brief shell_tab ���� table ������
	* @param input
	*
	* @return NULL
*/
static void shell_tab(struct shell_input * input)
{
	char  *  str = input->buf;
	uint32_t strlen = input->buftail;
	uint32_t index;
	uint32_t end;
	
	struct shell_list * node = shell_cmd_list.next;
	struct shell_cmd * shellcmd ;
	struct shell_cmd * match[10];    //ƥ�䵽��������
	uint32_t           match_cnt = 0;//ƥ�䵽������Ÿ���
	
	for ( ; *str == ' ' ; ++str,--strlen) ; //��ʱ�������ո���Ҫ����
	
	if (*str == 0 || strlen == 0) //û��������Ϣ����
		return ;
	
	index = ((uint32_t)(*str)<<26) | (strlen << 21) ;//��������ĸ��ͬ���ҳ��Ȳ�С�� strlen �ĵ�
	end = (uint32_t)(*str + 1)<<26 ;                 //��һ����ĸ��ͷ�ĵ�
	
	for ( ; node ; node = node->next )//�ҵ��� index ��ĵ㣬�ô���Ϊ��ʼƥ���
	{
		shellcmd = container_of(node, struct shell_cmd, cmd_node);
		if ( shellcmd->ID > index )
			break;
	}
	
	//����ĸ�Ĵ�С�������������Ĵ�С����������ĸ�ĵ㲻��Ҫ����ƥ��
	for ( ; node && shellcmd->ID < end ; node = node->next )
	{
		shellcmd = container_of(node,struct shell_cmd, cmd_node);
		
		if (memcmp(shellcmd->name, str, strlen) == 0) //�Ա�������ַ��������ƥ�䵽��ͬ��
		{
			match[match_cnt] = shellcmd; //��ƥ�䵽�����������������
			if (++match_cnt > 10)        //����ʮ����ͬ����
				return ;    
		}
	}

	if (!match_cnt) //���û���������������ַ���������
		return ; 
	
	if (input->edit != input->buftail) //����༭λ�ò���ĩ�ˣ��Ȱѹ���Ƶ�ĩ��
	{
		printl(&input->buf[input->edit],input->buftail - input->edit);
		input->edit = input->buftail;
	}

	if (1 == match_cnt)  //���ֻ�ҵ���һ�����������ǰ������ַ�����ֱ�Ӳ�ȫ�������ӡ
	{
		for(char * ptr = match[0]->name + strlen ; *ptr ;++ptr) //��ӡʣ����ַ�
			shell_getchar(input,*ptr);
	}
	else   //�����ֹһ�����������ǰ������ַ�������ӡ������ͬ�ַ��������б�����ȫ�ַ������ֱ���������ֵ�
	{
		for(uint32_t i = 0;i < match_cnt; ++i) //�����к��������ַ����������б��ӡ����
			printk("\r\n\t%s",match[i]->name); 
		
		printk("\r\n%s%s",shell_input_sign,input->buf); //���´�ӡ�����־����������ַ���
		
		while(1)  //��ȫ�����ÿ������������ַ���ȫ����ӡ
		{
			for (uint32_t i = 1 ; i < match_cnt ; ++i )
			{
				if (match[0]->name[strlen] != match[i]->name[strlen])
					return  ; //�ַ���һ��������
			}
			shell_getchar(input,match[0]->name[strlen++]);  //����ͬ���ַ���ȫ�����뻺����
		}
	}
}


/********************************************************************
	* @author   ��ô��
	* @brief    shell_list_cmd 
	*           ��ʾ����ע���˵�����
	* @param    arg       �������������
	* @return   NULL
*/
static void shell_list_cmd(void * arg)
{
	uint32_t firstchar = 0;
	struct shell_cmd * cmd;
	struct shell_list * node = shell_cmd_list.next;
	
	for ( ; node; node = node->next)//���������
	{
		cmd = container_of(node,struct shell_cmd, cmd_node);
		if (firstchar != (cmd->ID & 0xfc000000))
		{
			firstchar = cmd->ID & 0xfc000000;
			printk("\r\n(%c)------",((firstchar>>26)|0x40));
		}
		printk("\r\n\t%s", cmd->name);
	}
	
	printk("\r\n\r\n%s",shell_input_sign);
}

#endif //#ifdef USE_AVL_TREE




/**
	* @author   ��ô��
	* @brief    shell_backspace 
	*           ����̨���� ���� ������
	* @param    void
	* @return   void
*/
static void shell_backspace(struct shell_input * input)
{
	char printbuf[COMMANDLINE_MAX_LEN*2]={0};//��ת�ڴ�
	
	if (input->edit)//�����ǰ��ӡ�����������ݣ�����һ����λ
	{
		char * print = &printbuf[1];
		char * printend = print + (input->buftail - input->edit) + 1;
		char * edit = &input->buf[input->edit--] ;
		char * tail = &input->buf[input->buftail--];
		
		//����������Ҽ�ͷʱ����Ҫ���ַ����������ƴ���������������
		//�� abUcd ��ɾ��U����Ҫ����cd������ӡ���� '\b' ʹ���ص� ab ��
		for (char * cp = edit - 1 ; edit < tail ; *cp++ = *edit++)
		{
			*print++ = *edit;
			*printend++ = '\b';
		}

		printbuf[0] = '\b';
		*print = ' ';       //�������һ���ַ���ʾ
		*printend++ = '\b'; //������

		input->buf[input->buftail] = 0;  //ĩ������ַ���������
		printl(printbuf,printend-printbuf);
	}
}


#if (COMMANDLINE_MAX_RECORD) //�����������ʷ��¼
/**
	* @author   ��ô��
	* @brief    shell_record 
	*           ��¼�˴����е��������
	* @param    
	* @return   ���ؼ�¼��ַ
*/
static char * shell_record(struct shell_input * input)
{	
	char *  history = &shell_history.buf[shell_history.write][0];
	
	shell_history.write = (shell_history.write + 1) % COMMANDLINE_MAX_RECORD;
	shell_history.read = shell_history.write;
	
	memcpy(history,input->buf,input->buftail);
	history[input->buftail] = 0;
	
	return history;
}


/**
	* @author   ��ô��
	* @brief    shell_show_history 
	*           �����¼�ͷ����ʾ���������������˴�ֻ��¼������ε�����
	* @param    void
	* @return   void
*/
static void shell_show_history(struct shell_input * input,uint8_t LastOrNext)
{
	uint32_t len;
	char *history;
	
	printk("\33[2K\r%s",shell_input_sign);//"\33[2K\r" ��ʾ�����ǰ��

	if (!LastOrNext) //�ϼ�ͷ����һ������
	{
		uint32_t next = (!shell_history.read) ? (COMMANDLINE_MAX_RECORD - 1) : (shell_history.read - 1);
		if (next == shell_history.write)
		{
			input->buf[0] = 0;
			input->buftail = 0 ;
			input->edit = 0 ;
			shell_history.read = shell_history.write;
			return ;
		}
		else
			shell_history.read = next;
	}
	else //�¼�ͷ
	{	
		uint32_t next = (shell_history.read + 1) % COMMANDLINE_MAX_RECORD;

		if (shell_history.read == shell_history.write || next == shell_history.write)
		{
			input->buf[0] = 0;
			input->buftail = 0 ;
			input->edit = 0 ;
			shell_history.read = shell_history.write;
			return ;
		}
		else
			shell_history.read = next;
	}
	
	history = &shell_history.buf[shell_history.read][0];
	len = strlen(history);
	if (len)
	{
		printl(history,len);
		memcpy(input->buf,history,len);
		input->buf[len] = 0;
		input->buftail = len ;
		input->edit = len ;
	}
	else
	{
		input->buf[0] = 0;
		input->buftail = 0 ;
		input->edit = 0 ;
	}
}
#endif //#if (COMMANDLINE_MAX_RECORD) //�����������ʷ��¼


/**
	* @author   ��ô��
	* @brief    shell_parse
	*           �����н�������
	* @param
	* @return
*/
static void shell_parse(struct shell_input * input)
{
	uint32_t len = 0;
	uint32_t sum = 0;
	uint32_t fcrc8 = 0;
	uint32_t bcrc8 = 0;
	union uncmd unCmd ;
	struct shell_cmd * cmdmatch;

	char * str = input->buf;

	for ( ; ' ' == *str ; ++str) ;	// Shave off any leading spaces

	if (0 == *str)
		goto PARSE_END;
	
	for (unCmd.part.FirstChar = *str; (*str) && (*str != ' ') ; ++str ,++len)
	{
		sum += *str;
		fcrc8 = F_CRC8_Table[fcrc8^*str];
		bcrc8 = B_CRC8_Table[bcrc8^*str];
	}

	unCmd.part.Len = len;
	unCmd.part.Sum = sum;
	unCmd.part.CRC1 = fcrc8;
	unCmd.part.CRC2 = bcrc8;

	cmdmatch = shell_search_cmd(unCmd.ID);//ƥ�������
	if (cmdmatch != NULL)
		cmdmatch->Func(input->buf);
	else
		printk("\r\n\tno reply:%s\r\n",input->buf);

PARSE_END:
	input->buftail = 0;//��յ�ǰ����������
	input->edit = 0;
	return ;
}


/**
	* @author   ��ô��
	* @brief    shell_getchar 
	*           �����м�¼����һ���ַ�
	* @param    
	* @return   
*/
static void shell_getchar(struct shell_input * input , char ascii)
{
	char printbuf[COMMANDLINE_MAX_LEN*2]={0};//��ת�ڴ�
	
	if (input->buftail + 1 >= COMMANDLINE_MAX_LEN)
		return ;

	char *tail = &input->buf[input->buftail++];
	char *edit = &input->buf[input->edit++];
	char *print = printbuf + (tail - edit);
	char *printend = print + 1;

	//����������Ҽ�ͷʱ����Ҫ���ַ����������ƴ���������������
	//�� abcd ����bc����U����Ҫ����cd������ӡ���� '\b' ʹ���ص� abU ��
	for (char *cp = tail - 1; cp >= edit ; *tail-- = *cp--)
	{
		*print-- = *cp;
		*printend++ = '\b';
	}

	*print = ascii; 
	*edit = ascii;  //�����ַ�
	input->buf[input->buftail] = 0;      //ĩ������ַ���������
	printl(printbuf,printend - printbuf);//���Դ�ӡ
}

/**
	* @brief _shell_clean_screen ����̨����
	* @param    arg       �������������
	* @return NULL
*/
static void shell_clean_screen(void * arg)
{
	printk("\033[2J\033[%d;%dH%s",0,0,shell_input_sign);
	return ;
}


/**
	* @brief shell_debug_stream ��ȡ debug ��Ϣ
	* @param    arg       �������������
	* @return void
*/
static void shell_debug_stream(void * arg)
{
	static const char openmsg[]  = "\r\n\tget debug information\r\n\r\n";
	static const char closemsg[] = "\r\n\tclose debug information stream\r\n\r\n";

	char * msg ;
	uint32_t len;
	
	int option;
	int argc ;

	argc = cmdline_param(arg,&option,1);
	
	if ((argc > 0) && (option == 0))  //�رյ�����Ϣ��ӡ��������ʾ������Ϣ
	{
		default_puts = NULL;       //Ĭ����Ϣ�����Ϊ�գ�������ӡ������Ϣ
		msg = (char*)closemsg;
		len = sizeof(closemsg) - 1;
	}
	else
	{
		default_puts = current_puts; //���õ�ǰ����Ϊ��Ϣ�����
		msg = (char*)openmsg;
		len = sizeof(openmsg) - 1;
	}

	current_puts(msg,len);
}

/**
	* @author   ��ô��
	* @brief    _shell_register 
	*           ע��һ������źͶ�Ӧ������� ��ǰ׺Ϊ '_' ��ʾ������ֱ�ӵ��ô˺���
	* @param    cmd_name    ������
	* @param    cmd_func        ��������Ӧ��ִ�к���
	* @param    newcmd      ������ƿ��Ӧ��ָ��
	* @return   void
*/
void _shell_register(char * cmd_name, cmd_fn_t cmd_func,struct shell_cmd * newcmd)
{
	char * str = cmd_name;
	union uncmd unCmd ;

	uint32_t clen;
	uint32_t fcrc8 = 0;
	uint32_t bcrc8 = 0;
	uint32_t sum = 0;

	for (clen = 0; *str ; ++clen,++str)
	{
		sum += *str;
		fcrc8 = F_CRC8_Table[fcrc8^*str];
		bcrc8 = B_CRC8_Table[bcrc8^*str];
	}

	unCmd.part.CRC1 = fcrc8;
	unCmd.part.CRC2 = bcrc8;
	unCmd.part.Len = clen;
	unCmd.part.Sum = sum;
	unCmd.part.FirstChar = *cmd_name;
	
	newcmd->ID = unCmd.ID;   //����������
	newcmd->name = cmd_name;
	newcmd->Func = cmd_func;

	shell_insert_cmd(newcmd);//�������������˽ڵ�

	return ;
}

/**
	* @author   ��ô��
	* @brief    cmdline_strtok    for getopt();
	*     �� "a b c d" ��ʽ����ȡΪ char*argv[] = {"a","b","c","d"};�Թ�getopt()����
	* @param    str     �����ַ���������������������ָ��
	* @param    argv    ����ת���󻺴��ַ
	* @param    maxread ����ȡ��
	* @return   argc    �����������
*/
int cmdline_strtok(char * str ,char ** argv ,uint32_t maxread)
{
	int argc = 0;

	for ( ; ' ' == *str ; ++str) ; //�����ո�
	
	for ( ; *str && argc < maxread; ++argc,++argv )//�ַ���Ϊ ��\0' ��ʱ��
	{
		for (*argv = str ; ' ' != *str && *str ; ++str);//�����ǿ��ַ�
		
		for ( ; ' ' == *str; *str++ = '\0');//ÿ���������ַ����������������ո�		
	}
	
	return argc;
}



/**
	* @brief    cmdline_param
	*           ת����ȡ����ź��������������ַ���תΪ����
	* @param    str     �����ַ���������������������ָ��
	* @param    argv    ����ת���󻺴��ַ
	* @param    maxread ����ȡ��
	* @return   ���ݸ���
		* @retval   >= 0         �������������������
		* @retval   PARAMETER_ERROR(-2)  �������������������
		* @retval   PARAMETER_HELP(-1)   ���������� ? ��
*/
int cmdline_param(char * str,int * argv,uint32_t maxread)
{
	uint32_t argc;
	uint32_t value;

	for ( ; ' ' == *str        ; ++str);//�����ո�
	for ( ; ' ' != *str && *str; ++str);//������һ������
	for ( ; ' ' == *str        ; ++str);//�����ո�

	if (*str == 0)
		return 0;//����������û�и������ַ����룬����0

	if (*str == '?')
		return PARAMETER_HELP;//��������������ʺţ�����help

	for (argc = 0; *str && argc < maxread; ++argc , ++argv)//�ַ���Ϊ ��\0' ��ʱ��
	{
		*argv = 0;
		
		if ('0' == str[0] && 'x' == str[1]) //ʮ������ת��
		{
			for ( str += 2; ; ++str ) 
			{
				if ( (value = *str - '0') < 10 ) // value �ȸ�ֵ�����ж� 
					*argv = (*argv << 4)|value;
				else
				if ( (value = *str - 'A') < 6 || (value = *str - 'a') < 6)
					*argv = (*argv << 4) + value + 10;
				else
					break;
			}
		}
		else  //ѭ�����ַ���תΪ���֣�ֱ���ַ���Ϊ 0 - 9
		{
			uint32_t minus = ('-' == *str);//������ת��
			if (minus)
				++str;

			for (value = *str - '0'; value < 10 ; value = *(++str) - '0')
				*argv = (*argv * 10 + value);
			
			if (minus)
				*argv = -(*argv);
		}

		if ('\0' != *str && ' ' != *str)//������� 0 - 9 ���Ҳ��ǿո����Ǵ����ַ�
			return PARAMETER_ERROR;

		for ( ; ' ' == *str ; ++str);//�����ո�,�����ж���һ������
	}

	return argc;
}


/**
	* @author   ��ô��
	* @brief    shell_input 
	*           Ӳ���Ͻ��յ������ݵ������еĴ���
	* @param    ptr     Ӳ���������յ������ݻ�������ַ
	* @param    len     Ӳ���������յ������ݳ���
	* @return   void
*/
void cmdline_gets(struct shell_input * input,char * ptr,uint32_t len)
{
	for ( ; len && *ptr; --len,++ptr)
	{
		switch (*ptr) //�ж��ַ��Ƿ�Ϊ�����ַ�
		{
			case KEYCODE_NEWLINE: //���� \r
				break;
			case KEYCODE_ENTER:
				printk("\r\n");
				if (input->buftail)
				{
					shell_record(input);//��¼��ǰ�����������������
					shell_parse(input);
				}
				else
					printk("%s",shell_input_sign);
				break;
			
			case KEYCODE_ESC :
				if (ptr[1] == '[')
				{
					switch(ptr[2])
					{
						case 'A'://�ϼ�ͷ
							shell_show_history(input,0);
							break;

						case 'B'://�¼�ͷ
							shell_show_history(input,1);
							break;

						case 'C'://�Ҽ�ͷinput->buftail &&
							if ( input->buftail != input->edit)
								printl(&input->buf[input->edit++],1);
							break;

						case 'D'://���ͷ
							if (input->edit)
							{
								--input->edit;
								printl("\b",1);
							}
							break;
						default:;
					}
					
					len -= 2;
					ptr += 2;//��ͷ��3���ֽ��ַ�
				}
				break;

			case KEYCODE_CTRL_C:
				input->edit = 0;
				input->buftail = 0;
				printk("^C");
				break;
		
			case KEYCODE_BACKSPACE : 
			case 0x7f: //for putty
				shell_backspace(input); 
				break;
		
			case KEYCODE_TAB: 
				shell_tab(input); 
				break;
		
			default: // ��ͨ�ַ�
				shell_getchar(input,*ptr); //��ǰ����������;
		}
	}
	return ;
}

/**
	* @brief    _confirm_gets
	*           ��������Ϣȷ�ϣ�������� y/Y ��ִ������
	* @param
	* @return   void
*/
static void _confirm_gets(struct shell_input * shell ,char * buf , uint32_t len)
{
	char * option = shell->buf[COMMANDLINE_MAX_LEN-1];

	if (0 == *option) //������ [Y/y/N/n] ������������Ч
	{
		if ('Y' == *buf || 'y' == *buf || 'N' == *buf || 'n' == *buf)
		{
			*option = *buf;
			printl(buf,1);
		}
	}
	else
	if (*buf == KEYCODE_BACKSPACE) //���˼�
	{
		printl("\b \b",3);
		*option = 0;
	}
	else
	if ('\r' == *buf || '\n' == *buf) //���س�ȷ��
	{
		cmd_fn_t yestodo = (cmd_fn_t)shell->apparg;
 		char opt = *option ; 
		
		*option = 0 ;  //shell->buf[COMMANDLINE_MAX_LEN-1] = 0;
		shell->gets   = cmdline_gets;//���ݻع�Ϊ������ģʽ
		shell->apparg = NULL;

		printl("\r\n",2);

		if ( 'Y' == opt || 'y' == opt) //ȷ������ iap
			yestodo(shell->buf);
		else
			printk("cancel this operation\r\n");
	}
}

/**
	* @brief    shell_confirm
	*           ��������Ϣȷ�ϣ�������� y/Y ��ִ������
	* @param    shell  : ���뽻��
	* @param    info   : ѡ����Ϣ
	* @param    yestodo: ���� y/Y ������ִ�е�����
	* @return   void
*/
void shell_confirm(struct shell_input * shell ,char * info ,cmd_fn_t yestodo)
{
	printk(info);
	printk(" [Y/N] ");
	shell->gets = _confirm_gets;//������������ȡ�� shell_option
	shell->apparg = yestodo;
	shell->buf[COMMANDLINE_MAX_LEN-1] = 0;
}



/**
	* @author   ��ô��
	* @brief    shell_init 
	*           shell ��ʼ��
	* @param    sign : shell �����־���� shell >
	* @param    puts : shell Ĭ���������Ӵ��������
	* @return   NULL
*/
void shell_init(char * sign,fmt_puts_t puts)
{
//	SHELL_INPUT_SIHN(sign);
	strcpy(shell_input_sign,sign);
	current_puts = puts ;
	default_puts = puts ;
	
	//ע���������
	shell_register_command("cmd-list",shell_list_cmd);
	shell_register_command("clear",shell_clean_screen);
	shell_register_command("debug-info",shell_debug_stream);//
}


