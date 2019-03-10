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
  *    3.�½�һ��  shellinput_t shellx , ��ʼ����� shell_input_init(&shellx,puts,...);
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

union uncmd {

	struct {// ����ŷ�Ϊ�����������  
		uint32_t CRC2      : 8;
		uint32_t CRC1      : 8;//��ʮ��λΪ���� crc У����
		uint32_t Sum       : 5;//�����ַ����ܺ�
		uint32_t Len       : 5;//�����ַ��ĳ��ȣ�5 bit ��������Ȳ��ܳ���31���ַ�
		uint32_t FirstChar : 6;//�����ַ��ĵ�һ���ַ�
	}part;

	uint32_t ID;//�ɴ˺ϲ�Ϊ 32 λ��������
};

/* Private macro ------------------------------------------------------------*/

/* Private variables --------------------------------------------------------*/

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

static cmd_root_t shellcmdroot = {0};

/* Global variables ------------------------------------------------------------*/

char DEFAULT_INPUTSIGN[COMMANDLINE_MAX_LEN] = "minishell >";

/* Private function prototypes -----------------------------------------------*/
static void   shell_getchar     (struct shell_input * shellin , char ch);
static void   shell_backspace   (struct shell_input * shellin) ;
static void   shell_tab         (struct shell_input * shellin) ;
       void   shell_confirm     (struct shell_input * shellin ,char * info ,cmd_fn_t yestodo);

#if (COMMANDLINE_MAX_RECORD)//�����������ʷ��¼
	static char * shell_record(struct shell_input * shellin);
	static void   shell_show_history(struct shell_input * shellin,uint8_t LastOrNext);
#else
#	define shell_record(x)
#	define shell_show_history(x,y)
#endif //#if (COMMANDLINE_MAX_RECORD)//�����������ʷ��¼

/* Gorgeous Split-line -----------------------------------------------*/

#ifdef USE_AVL_TREE //avl tree ������ѯϵͳ

/**
  * @brief    ���������ң����� id ���ҵ���Ӧ�Ŀ��ƿ�
  * @param    cmdindex  �����
  * @param    root      �����������
  * @return   �ɹ� id �Ŷ�Ӧ�Ŀ��ƿ�
*/
static struct shellcommand *shell_search_cmd(cmd_root_t * root , int cmdindex)
{
	cmd_entry_t *node = root->avl_node;

	while (node) {
		struct shellcommand * command = container_of(node, struct shellcommand, node);

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
  * @brief    ����������
  * @param    newcmd   ��������ƿ�
  * @return   �ɹ����� 0
*/
static int shell_insert_cmd(cmd_root_t * root , struct shellcommand * newcmd)
{
	cmd_entry_t **tmp = &root->avl_node;
 	cmd_entry_t *parent = NULL;
	
	/* Figure out where to put new node */
	while (*tmp) {
		struct shellcommand *this = container_of(*tmp, struct shellcommand, node);

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
	avl_insert(root,&newcmd->node,parent,tmp);
	return 0;
}


/** 
  * @brief  ���� table ������
  * @param input
	*
  * @return NULL
*/
static void shell_tab(struct shell_input * shellin)
{
	char  *  str = shellin->cmdline;
	uint32_t strlen = shellin->tail;
	uint32_t index , end;
	
	cmd_entry_t  * node = shellcmdroot.avl_node;	
	struct shellcommand * shellcmd ;
	struct shellcommand * match[10];    //ƥ�䵽��������
	uint32_t           match_cnt = 0;//ƥ�䵽������Ÿ���
	
	for ( ; *str == ' ' ; ++str,--strlen) ; //��ʱ�������ո���Ҫ����
	
	if (*str == 0 || strlen == 0 || node == NULL) //û��������Ϣ����
		return ;
	
	index = ((uint32_t)(*str)<<26) | (strlen << 21) ;//��������ĸ��ͬ���ҳ��Ȳ�С�� strlen �ĵ�
	end = (uint32_t)(*str + 1)<<26 ;                 //��һ����ĸ��ͷ�ĵ�

	while ( node ){//index һ�������ڣ����ҽ������ shell_cmd ��� index ���ô���Ϊ��ʼƥ��� 
		shellcmd = avl_entry(node,struct shellcommand, node);	
		node = (index < shellcmd->ID) ? node->avl_left : node->avl_right;
	}

	//����ĸ�Ĵ�С�������������Ĵ�С����������ĸ�ĵ㲻��Ҫ����ƥ��
    for (node = &shellcmd->node ; shellcmd->ID < end && node; node = avl_next(node)) {
		shellcmd = avl_entry(node,struct shellcommand, node);
		
		if (memcmp(shellcmd->name, str, strlen) == 0){ //�Ա�������ַ��������ƥ�䵽��ͬ�� 
			match[match_cnt] = shellcmd; //��ƥ�䵽�����������������
			if (++match_cnt > 10)        //����ʮ����ͬ����
				return ;    
		}
	}

	if (!match_cnt) //���û���������������ַ���������
		return ; 
	
	if (shellin->edit != shellin->tail) {//����༭λ�ò���ĩ�ˣ��Ȱѹ���Ƶ�ĩ�� 
		printl(&shellin->cmdline[shellin->edit],shellin->tail - shellin->edit);
		shellin->edit = shellin->tail;
	}

	if (1 == match_cnt){  //���ֻ�ҵ���һ�����������ǰ������ַ�����ֱ�Ӳ�ȫ�������ӡ 
		for(char * ptr = match[0]->name + strlen ; *ptr ;++ptr) //��ӡʣ����ַ�
			shell_getchar(shellin,*ptr);
		shell_getchar(shellin,' ');
	}else {  //�����ֹһ�����������ǰ������ַ�������ӡ������ͬ�ַ��������б�����ȫ�ַ������ֱ���������ֵ�

		for(uint32_t i = 0;i < match_cnt; ++i) //�����к��������ַ����������б��ӡ����
			printk("\r\n\t%s",match[i]->name); 
		
		printk("\r\n%s%s",shellin->sign,shellin->cmdline); //���´�ӡ�����־����������ַ���
		
		for ( ; ; ) { //��ȫ�����ÿ������������ַ���ȫ����ӡ
			for (uint32_t i = 1 ; i < match_cnt ; ++i ) {
				if (match[0]->name[strlen] != match[i]->name[strlen])
					return  ; //�ַ���һ��������
			}
			shell_getchar(shellin,match[0]->name[strlen++]);  //����ͬ���ַ���ȫ�����뻺����
		}
	}
}


/**
  * @author   ��ô��
  * @brief    ��ʾ����ע���˵�����
  * @param    arg       �������������
  * @return   NULL
*/
static void shell_list_cmd(void * arg)
{
	struct shell_input * shellin = container_of(arg, struct shell_input, cmdline);
	struct shellcommand * cmd;
	uint32_t firstchar = 0;
	cmd_entry_t  * node ;
	
	for (node = avl_first(&shellcmdroot); node; node = avl_next(node)) { //���������
		cmd = avl_entry(node,struct shellcommand, node);
		if (firstchar != (cmd->ID & 0xfc000000)) {
			firstchar = cmd->ID & 0xfc000000;
			printk("\r\n(%c)------",((firstchar>>26)|0x40));
		}
		printk("\r\n\t%s", cmd->name);
	}
	
	printk("\r\n\r\n%s",shellin->sign);
}

#else  //#ifdef USE_AVL_TREE  ����������ѯϵͳ

/**
  * @brief    ���������ң����� id ���ҵ���Ӧ�Ŀ��ƿ�
  * @param    cmdindex        �����
  * @return   �ɹ� id �Ŷ�Ӧ�Ŀ��ƿ�
*/
static struct shellcommand *shell_search_cmd(cmd_root_t * root , int cmdindex)
{
	for (cmd_entry_t * node = root->next; node ; node = node->next ) {
		struct shellcommand  * cmd = container_of(node, struct shellcommand, node);
		if (cmd->ID > cmdindex)
			return NULL;
		else
		if (cmd->ID == cmdindex)
			return cmd;
	}
  
	return NULL;
}

/**
  * @brief    ����������
  * @param    newcmd   ��������ƿ�
  * @return   �ɹ����� 0
*/
static int shell_insert_cmd(cmd_root_t * root , struct shellcommand * newcmd)
{
	cmd_entry_t * prev = root;
	cmd_entry_t * node ;

	for (node = prev->next; node ; prev = node,node = node->next) {
		struct shellcommand * cmd = container_of(node, struct shellcommand, node);
		if ( cmd->ID > newcmd->ID )
			break;
		else
		if (cmd->ID == newcmd->ID )
			return 1;
	}

	prev->next = &newcmd->node ;
	prev = prev->next;
	prev->next = node;
	return 0;
}

	
/** 
  * @brief ���� table ������
  * @param input
	*
  * @return NULL
*/
static void shell_tab(struct shell_input * shellin)
{
	char  *  str = shellin->cmdline;
	uint32_t strlen = shellin->tail;
	
	cmd_entry_t * node = shellcmdroot.next;
	struct shellcommand * shellcmd ;
	struct shellcommand * match[10];    //ƥ�䵽��������
	uint32_t match_cnt = 0;//ƥ�䵽������Ÿ���
	uint32_t index, end;
	
	for ( ; *str == ' ' ; ++str,--strlen) ; //��ʱ�������ո���Ҫ����
	
	if (*str == 0 || strlen == 0) //û��������Ϣ����
		return ;
	
	index = ((uint32_t)(*str)<<26) | (strlen << 21) ;//��������ĸ��ͬ���ҳ��Ȳ�С�� strlen �ĵ�
	end = (uint32_t)(*str + 1)<<26 ;                 //��һ����ĸ��ͷ�ĵ�
	
	for ( ; node ; node = node->next ) { //�ҵ��� index ��ĵ㣬�ô���Ϊ��ʼƥ���
		shellcmd = container_of(node, struct shellcommand, node);
		if ( shellcmd->ID > index )
			break;
	}
	
	//����ĸ�Ĵ�С�������������Ĵ�С����������ĸ�ĵ㲻��Ҫ����ƥ��
	for ( ; node && shellcmd->ID < end ; node = node->next ) {
		shellcmd = container_of(node,struct shellcommand, node); 
		if (memcmp(shellcmd->name, str, strlen) == 0) {//�Ա�������ַ��������ƥ�䵽��ͬ�� 
			match[match_cnt] = shellcmd; //��ƥ�䵽�����������������
			if (++match_cnt > 10)        //����ʮ����ͬ����
				return ;    
		}
	}

	if (!match_cnt) //���û���������������ַ���������
		return ; 
	
	if (shellin->edit != shellin->tail){ //����༭λ�ò���ĩ�ˣ��Ȱѹ���Ƶ�ĩ�� 
		printl(&shellin->cmdline[shellin->edit],shellin->tail - shellin->edit);
		shellin->edit = shellin->tail;
	}

	if (1 == match_cnt){  //���ֻ�ҵ���һ�����������ǰ������ַ�����ֱ�Ӳ�ȫ�������ӡ
		for(char * ptr = match[0]->name + strlen ; *ptr ;++ptr) //��ӡʣ����ַ�
			shell_getchar(shellin,*ptr);
		shell_getchar(shellin,' ');
	}else {  //�����ֹһ�����������ǰ������ַ�������ӡ������ͬ�ַ��������б�����ȫ�ַ������ֱ���������ֵ�
	
		for(uint32_t i = 0;i < match_cnt; ++i) //�����к��������ַ����������б��ӡ����
			printk("\r\n\t%s",match[i]->name); 
		
		printk("\r\n%s%s",shellin->sign,shellin->cmdline); //���´�ӡ�����־����������ַ���
		
		for ( ; ; ) { //��ȫ�����ÿ������������ַ���ȫ����ӡ
			for (uint32_t i = 1 ; i < match_cnt ; ++i ) {
				if (match[0]->name[strlen] != match[i]->name[strlen])
					return  ; //�ַ���һ��������
			}
			shell_getchar(shellin,match[0]->name[strlen++]);  //����ͬ���ַ���ȫ�����뻺����
		}
	}
}

/**
  * @author   ��ô��
  * @brief    ��ʾ����ע���˵�����
  * @param    arg       �������������
  * @return   NULL
*/
static void shell_list_cmd(void * arg)
{
	uint32_t firstchar = 0;
	struct shellcommand * cmd;
	cmd_entry_t * node = shellcmdroot.next;
	struct shell_input * shellin = container_of(arg, struct shell_input, cmdline);
	
	for ( ; node; node = node->next){//���������
		cmd = container_of(node,struct shellcommand, node);
		if (firstchar != (cmd->ID & 0xfc000000)) {
			firstchar = cmd->ID & 0xfc000000;
			printk("\r\n(%c)------",((firstchar>>26)|0x40));
		}
		printk("\r\n\t%s", cmd->name);
	}
	
	printk("\r\n\r\n%s",shellin->sign);
}

#endif //#ifdef USE_AVL_TREE


#if (COMMANDLINE_MAX_RECORD) //�����������ʷ��¼

/**
  * @author   ��ô��
  * @brief    ��¼�˴����е��������
  * @param    
  * @return   ���ؼ�¼��ַ
*/
static char * shell_record(struct shell_input * shellin)
{	
	char *  history = &shellin->history[shellin->htywrt][0];
	
	shellin->htywrt  = (shellin->htywrt + 1) % COMMANDLINE_MAX_RECORD;
	shellin->htyread = shellin->htywrt;

	memcpy(history,shellin->cmdline,shellin->tail);
	history[shellin->tail] = 0;
	
	return history;
}


/**
  * @author   ��ô��
  * @brief    �����¼�ͷ����ʾ���������������˴�ֻ��¼������ε�����
  * @param    void
  * @return   void
*/
static void shell_show_history(struct shell_input * shellin,uint8_t LastOrNext)
{
	uint32_t len = 0;
	
	printk("\33[2K\r%s",shellin->sign);//"\33[2K\r" ��ʾ�����ǰ��

	if (!LastOrNext) //�ϼ�ͷ����һ������
		shellin->htyread = (!shellin->htyread) ? (COMMANDLINE_MAX_RECORD - 1) : (shellin->htyread - 1);
	else       //�¼�ͷ
	if (shellin->htyread != shellin->htywrt)
		shellin->htyread = (shellin->htyread + 1) % COMMANDLINE_MAX_RECORD;

	if (shellin->htyread != shellin->htywrt){ //����ʷ��¼�����������ڴ� 
		for (char * history = &shellin->history[shellin->htyread][0]; *history ; ++len)
			shellin->cmdline[len] = *history++;
	}
	
	shellin->cmdline[len] = 0; //��ӽ�����
	shellin->tail = len ;
	shellin->edit = len ;

	if (len)
		printl(shellin->cmdline,len); //��ӡ����������
}

#endif //#if (COMMANDLINE_MAX_RECORD) //�����������ʷ��¼

/**
  * @author   ��ô��
  * @brief    ����̨���� ���� ������
  * @param    void
  * @return   void
*/
static void shell_backspace(struct shell_input * shellin)
{
	if (!shellin->edit)//�����ǰ��ӡ�����������ݣ�����һ����λ
		return ;

	char   printbuf[COMMANDLINE_MAX_LEN*2]={0};//��ת�ڴ�
	char * print = &printbuf[1];//����� printbuf ����Ϊȫ�ֱ������п������Ե����⣬����Ϊ���ٶ�
	char * printend = print + (shellin->tail - shellin->edit) + 1;
	char * edit = &shellin->cmdline[shellin->edit--] ;
	char * tail = &shellin->cmdline[shellin->tail--];

	//����������Ҽ�ͷʱ����Ҫ���ַ����������ƴ���������������
	//�� abUcd ��ɾ��U����Ҫ����cd������ӡ���� '\b' ʹ���ص� ab ��
	for (char * cp = edit - 1 ; edit < tail ; *cp++ = *edit++) {
		*print++ = *edit;
		*printend++ = '\b';
	}

	printbuf[0] = '\b';
	*print = ' ';       //�������һ���ַ���ʾ
	*printend++ = '\b'; //������

	shellin->cmdline[shellin->tail] = 0;  //ĩ������ַ���������
	printl(printbuf,printend-printbuf);
}

/**
  * @author   ��ô��
  * @brief    �����м�¼����һ���ַ�
  * @param    
  * @return   
*/
static void shell_getchar(struct shell_input * shellin , char ascii)
{
	if (shellin->tail + 1 >= COMMANDLINE_MAX_LEN)
		return ;

	if (shellin->tail == shellin->edit) {
		shellin->cmdline[shellin->edit++] = ascii;
		shellin->cmdline[++shellin->tail] = 0;
		printl(&ascii,1);
	}else {//��ʵ else ��֧��ȫ���Դ��� tail == edit ����������� printbuf ѹջ̫�� 
		char  printbuf[COMMANDLINE_MAX_LEN*2]={0};//��ת�ڴ�
		char *tail = &shellin->cmdline[shellin->tail++];
		char *edit = &shellin->cmdline[shellin->edit++];
		char *print = printbuf + (tail - edit);
		char *printend = print + 1;

		//����������Ҽ�ͷʱ����Ҫ���ַ����������ƴ���������������
		//�� abcd ����bc����U����Ҫ����cd������ӡ���� '\b' ʹ���ص� abU ��
		for (char *cp = tail - 1; cp >= edit ; *tail-- = *cp--) {
			*print-- = *cp;
			*printend++ = '\b';
		}

		*print = ascii; 
		*edit = ascii;  //�����ַ�
		shellin->cmdline[shellin->tail] = 0; //ĩ������ַ���������
		printl(printbuf,printend - printbuf);//���Դ�ӡ
	}
}


/**
  * @author   ��ô��
  * @brief    �����н�������
  * @param
  * @return
*/
static void shell_parse(cmd_root_t * cmdroot , struct shell_input * shellin)
{
	union uncmd unCmd ;
	uint32_t len = 0;
	uint32_t sum = 0;
	uint32_t fcrc8 = 0;
	uint32_t bcrc8 = 0;
	char  *  str = shellin->cmdline;
	struct shellcommand * cmdmatch;

	for ( ; ' ' == *str ; ++str) ;	// Shave off any leading spaces

	if (0 == *str)
		goto PARSE_END;
	
	for (unCmd.part.FirstChar = *str; (*str) && (*str != ' ') ; ++str ,++len) {
		sum += *str;
		fcrc8 = F_CRC8_Table[fcrc8^*str];
		bcrc8 = B_CRC8_Table[bcrc8^*str];
	}

	unCmd.part.Len = len;
	unCmd.part.Sum = sum;
	unCmd.part.CRC1 = fcrc8;
	unCmd.part.CRC2 = bcrc8;

	cmdmatch = shell_search_cmd(cmdroot,unCmd.ID);//ƥ�������
	if (cmdmatch != NULL) {
		if (cmdmatch->fnaddr & FUNC_CONFIRM) {// �������Ҫȷ�ϵ����� 
			cmd_fn_t func = (cmd_fn_t)(cmdmatch->fnaddr & (~FUNC_CONFIRM)) ;//��ȡ����ָ��
			shellcfm_t * confirm = container_of(cmdmatch, struct shellconfirm, cmd);//��ȡȷ����Ϣ
			shell_confirm(shellin,confirm->prompt,func);//����ȷ��������
		}else {
			cmd_fn_t func = (cmd_fn_t)cmdmatch->fnaddr ;//��ͨ���� 
			func(shellin->cmdline);
		}
	}else {
		printk("\r\n\tno reply:%s\r\n",shellin->cmdline);
	}
	
PARSE_END:
	shellin->tail = 0;//��յ�ǰ����������
	shellin->edit = 0;
	return ;
}

/**
  * @brief    ����̨����
  * @param    arg       �������������
  * @return NULL
*/
static void shell_clean_screen(void * arg)
{
	struct shell_input * shellin = container_of(arg, struct shell_input, cmdline);
	printk("\033[2J\033[%d;%dH%s",0,0,shellin->sign);
	return ;
}


/**
  * @brief  ��ȡ debug ��Ϣ
  * @param    arg       �������������
  * @return void
*/
static void shell_debug_stream(void * arg)
{
	int option;
	int argc = cmdline_param(arg,&option,1);
	
	if ((argc > 0) && (option == 0)) { //�رյ�����Ϣ��ӡ��������ʾ������Ϣ 
		static const char closemsg[] = "\r\n\tclose debug information stream\r\n\r\n";
		current_puts((char*)closemsg,sizeof(closemsg) - 1);
		default_puts = NULL;       //Ĭ����Ϣ�����Ϊ�գ�������ӡ������Ϣ
	}else {
		static const char openmsg[]  = "\r\n\tget debug information\r\n\r\n";
		current_puts((char*)openmsg,sizeof(openmsg) - 1);
		default_puts = current_puts; //���õ�ǰ����Ϊ��Ϣ�����
	}
}

/**
  * @author   ��ô��
  * @brief    ע��һ������źͶ�Ӧ������� ��ǰ׺Ϊ '_' ��ʾ������ֱ�ӵ��ô˺���
  * @param    cmd_name    ������
  * @param    cmd_func        ��������Ӧ��ִ�к���
  * @param    newcmd      ������ƿ��Ӧ��ָ��
  * @return   void
*/
void _shell_register(struct shellcommand * newcmd,char * cmd_name, cmd_fn_t cmd_func,uint32_t comfirm)
{
	char * str = cmd_name;
	union uncmd unCmd ;

	uint32_t clen;
	uint32_t fcrc8 = 0;
	uint32_t bcrc8 = 0;
	uint32_t sum = 0;

	for (clen = 0; *str ; ++clen,++str) {
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
	newcmd->fnaddr = (uint32_t)cmd_func;

	shell_insert_cmd(&shellcmdroot,newcmd);//�������������˽ڵ�
	
	if (NEED_CONFIRM == comfirm) //����Ǵ�ȷ��ѡ�������
		newcmd->fnaddr |= FUNC_CONFIRM ;
	return ;
}

/**
  * @author   ��ô��
  * @brief    �� "a b c d" ��ʽ����ȡΪ char*argv[] = {"a","b","c","d"};�Թ�getopt()����
  * @param    str     �����ַ���������������������ָ��
  * @param    argv    ����ת���󻺴��ַ
  * @param    maxread ����ȡ��
  * @return   argc    ���ն�ȡ�����������
*/
int cmdline_strtok(char * str ,char ** argv ,uint32_t maxread)
{
	int argc = 0;

	for ( ; ' ' == *str ; ++str) ; //�����ո�
	
	for ( ; *str && argc < maxread; ++argc,++argv ) { //�ַ���Ϊ ��\0' ��ʱ��
	
		for (*argv = str ; ' ' != *str && *str ; ++str);//��¼���������Ȼ�������ǿ��ַ�
		
		for ( ; ' ' == *str; *str++ = '\0');//ÿ���������ַ����������������ո�		
	}
	
	return argc;
}


/**
  * @brief    ת����ȡ����ź��������������ַ���תΪ����
  * @param    str     �����ַ���������������������ָ��
  * @param    argv    ����ת���󻺴��ַ
  * @param    maxread ����ȡ��
  * @return   ���ݸ���
	  * @retval   >= 0         ��ȡ�������������������
	  * @retval   PARAMETER_ERROR(-2)  �������������������
	  * @retval   PARAMETER_HELP(-1)   ���������� ? ��
*/
int cmdline_param(char * str,int * argv,uint32_t maxread)
{
	uint32_t argc , value;

	for ( ; ' ' == *str        ; ++str);//�����ո�
	for ( ; ' ' != *str && *str; ++str);//������һ������
	for ( ; ' ' == *str        ; ++str);//�����ո�

	if (*str == '?')
		return PARAMETER_HELP;//��������������ʺţ�����help

	for (argc = 0; *str && argc < maxread; ++argc , ++argv) { //�ַ���Ϊ ��\0' ��ʱ��
	
		*argv = 0;
		
		if ('0' == str[0] && 'x' == str[1]) { //"0x" ��ͷ��ʮ������ת��
			for ( str += 2 ;  ; ++str )  {
				if ( (value = *str - '0') < 10 ) // value �ȸ�ֵ�����ж� 
					*argv = (*argv << 4)|value;
				else
				if ( (value = *str - 'A') < 6 || (value = *str - 'a') < 6)
					*argv = (*argv << 4) + value + 10;
				else
					break;
			}
		}else  { //ѭ�����ַ���תΪ���֣�ֱ���ַ���Ϊ 0 - 9
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
  * @brief    ��ӭҳ
  * @param    recv  Ӳ���������յ������ݻ�������ַ
  * @param    len     Ӳ���������յ������ݳ���
  * @return   void
*/
void welcome_gets(struct shell_input * shellin,char * recv,uint32_t len)
{
	static const char minishelllogo[]= //��ӡһ����ӭҳlogo
"\r\n\r\n\
	 __  __  ____  _   _  ____\r\n\
	|  \\/  ||_  _||  \\| ||_  _|COPYRIGHT(c):\r\n\
	| |\\/| | _||_ | |\\\\ | _||_ GoodMorning\r\n\
	|_|  |_||____||_| \\_||____|2019/01\r\n\
	 ____  _   _  ____  _     _\r\n\
	/ ___|| |_| || ___|| |   | |\r\n\
	\\___ \\|  _  || __| | |__ | |__\r\n\
	|____/|_| |_||____||____||____|\r\n\r\n";
	printl((char*)minishelllogo,sizeof(minishelllogo)-1);
	shellin->gets = cmdline_gets;
	cmdline_gets(shellin,recv,len);
	return ;
}




/**
  * @author   ��ô��
  * @brief    Ӳ���Ͻ��յ������ݵ������еĴ���
  * @param    recv  Ӳ���������յ������ݻ�������ַ
  * @param    len     Ӳ���������յ������ݳ���
  * @return   void
*/
void cmdline_gets(struct shell_input * shellin,char * recv,uint32_t len)
{
	uint32_t state = 0 ;

	for (char * end = recv + len ; recv < end ; ++recv) {
		if (0 == state) {
			if (*recv > 0x1F && *recv < 0x7f)//��ͨ�ַ�,��ǰ����������;
				shell_getchar(shellin,*recv);
			else
			switch (*recv) {//�ж��ַ��Ƿ�Ϊ�����ַ�
				case KEYCODE_ENTER:
					if (shellin->tail){
						printk("\r\n");
						shell_record(shellin);//��¼��ǰ�����������������
						shell_parse(&shellcmdroot ,shellin);
					}else{
						printk("\r\n%s",shellin->sign);
					}
					break;
				case KEYCODE_ESC :
					state = 1;
					break;
				case KEYCODE_CTRL_C:
					shellin->edit = 0;
					shellin->tail = 0;
					printk("^C\r\n%s",shellin->sign);
					break;
				case KEYCODE_BACKSPACE :
				case 0x7f: //for putty
					shell_backspace(shellin);
					break;
				case KEYCODE_TAB:
					shell_tab(shellin);
					break;
				default: ;//��ǰ����������;
			}
		}else //
		if (1 == state){ //�ж��Ƿ��Ǽ�ͷ����
			state = (*recv == '[') ? 2 : 0 ;
		}else{
		// if (2 == state) //��Ӧ��ͷ����
			switch(*recv){  
				case 'A'://�ϼ�ͷ
					shell_show_history(shellin,0);
					break;
				case 'B'://�¼�ͷ
					shell_show_history(shellin,1);
					break;
				case 'C'://�Ҽ�ͷ  input->tail &&
					if ( shellin->tail != shellin->edit)
						printl(&shellin->cmdline[shellin->edit++],1);
					break;
				case 'D'://���ͷ
					if (shellin->edit){
						--shellin->edit;
						printl("\b",1);
					}
					break;
				default:;
			} //switch ��ͷ����
		} // if (2 == state) //��Ӧ��ͷ����
	} //for ( ; len && *recv; --len,++recv)
	return ;
}

/**
  * @brief    ��������Ϣȷ�ϣ�������� y/Y ��ִ������
  * @param
  * @return   void
*/
static void confirm_gets(struct shell_input * shellin ,char * buf , uint32_t len)
{
	char * option = &shellin->cmdline[COMMANDLINE_MAX_LEN-1];

	if (0 == *option) { //������ [Y/y/N/n] ������������Ч
		if ('Y' == *buf || 'y' == *buf || 'N' == *buf || 'n' == *buf) {
			*option = *buf;
			printl(buf,1);
		}
	}else
	if (KEYCODE_BACKSPACE == *buf) { //���˼�
		printl("\b \b",3);
		*option = 0;
	}else
	if ('\r' == *buf || '\n' == *buf) {//���س�ȷ��
		cmd_fn_t yestodo = (cmd_fn_t)shellin->apparg;
 		char opt = *option ; 
		
		*option = 0 ;  //shellin->cmdline[COMMANDLINE_MAX_LEN-1] = 0;
		shellin->gets   = cmdline_gets;//���ݻع�Ϊ������ģʽ
		shellin->apparg = NULL;

		printl("\r\n",2);

		if ( 'Y' == opt || 'y' == opt) //ȷ������ iap
			yestodo(shellin->cmdline);
		else
			printk("cancel this operation\r\n");
	}
}

/**
  * @brief    ��������Ϣȷ�ϣ�������� y/Y ��ִ������
  * @param    shell  : ���뽻��
  * @param    info   : ѡ����Ϣ
  * @param    yestodo: ���� y/Y ������ִ�е�����
  * @return   void
*/
void shell_confirm(struct shell_input * shellin ,char * info ,cmd_fn_t yestodo)
{
	printk("%s [Y/N] ",info);
	shellin->gets = confirm_gets;//������������ȡ�� shell_option
	shellin->apparg = yestodo;
	shellin->cmdline[COMMANDLINE_MAX_LEN-1] = 0;
}

/**
  * @author   ��ô��
  * @brief    ��ʼ��һ�� shell ������Ĭ������Ϊ cmdline_gets
  * @param    shellin   : ��Ҫ��ʼ���� shell ���� 
  * @param    shellputs : shell ��Ӧ�������Ӵ��������
  * @param    ...       : �� gets �� sign �ض��壬��׷�� MODIFY_SIGN,"shell>>"
  * @return   NULL
*/
void shell_input_init(struct shell_input * shellin , fmt_puts_t shellputs,...)
{
	char * shellsign = DEFAULT_INPUTSIGN;
	shellgets_t shellgets = welcome_gets;
	
	va_list ap;
	va_start(ap, shellputs); //��������¶��� 

	for (uint32_t arg = va_arg(ap, uint32_t) ; MODIFY_MASK == (arg & (~0x0f)) ; arg = va_arg(ap, uint32_t) ) {
		if (MODIFY_SIGN == arg) //����ض��嵱ǰ�����������־
			shellsign = va_arg(ap, char*);
		else
		if (MODIFY_GETS == arg) //����ض��嵱ǰ��������������
			shellgets = (shellgets_t)va_arg(ap, void*);
	}

	va_end(ap);

	shellin->tail = 0;
	shellin->edit = 0;
	shellin->puts = shellputs;
	shellin->gets = shellgets;
	shellin->htywrt  = 0;
	shellin->htyread = 0;
	shellin->apparg  = NULL;
	strcpy(shellin->sign, shellsign);
}


/**
  * @author   ��ô��
  * @brief    shell ��ʼ��,ע�Ἰ�����������������ʼ����
  * @param    puts : printf,printk,printl ��Ĭ���������Ӵ��������
  * @return   void
*/
void shell_init(char * defaultsign ,fmt_puts_t puts)
{
	strcpy(DEFAULT_INPUTSIGN,defaultsign);

	current_puts = puts ;
	default_puts = puts ;
	
	//ע��һЩ��������
	shell_register_command("cmd-list",shell_list_cmd);
	shell_register_command("clear",shell_clean_screen);
	shell_register_command("debug-info",shell_debug_stream);
}


