/**
  ******************************************************************************
  * @file           shell.c
  * @author         古么宁
  * @brief          shell 命令解释器
  *                 支持  TAB 键命令补全，上下左右箭头 ，BACKSPACE回删
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
/* Includes ---------------------------------------------------*/
#include <string.h>
#include <stdarg.h>
#include <stdint.h> //定义了很多数据类型
#include <stdio.h>
#include "shell.h"
#include "containerof.h"

/* Private types ------------------------------------------------------------*/

union uncmd {

	struct {// 命令号分为以下五个部分  
		uint32_t CRC2      : 8;
		uint32_t CRC1      : 8;//低十六位为两个 crc 校验码
		uint32_t Sum       : 5;//命令字符的总和
		uint32_t Len       : 5;//命令字符的长度，5 bit ，即命令长度不能超过31个字符
		uint32_t FirstChar : 6;//命令字符的第一个字符
	}part;

	uint32_t ID;//由此合并为 32 位的命令码
};

/* Private macro ------------------------------------------------------------*/

/* Private variables --------------------------------------------------------*/

static const  uint8_t F_CRC8_Table[256] = {//正序,高位先行 x^8+x^5+x^4+1
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

static const  uint8_t B_CRC8_Table[256] = {//反序,低位先行 x^8+x^5+x^4+1
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

#if (COMMANDLINE_MAX_RECORD)//如果定义了历史纪录
	static char * shell_record(struct shell_input * shellin);
	static void   shell_show_history(struct shell_input * shellin,uint8_t LastOrNext);
#else
#	define shell_record(x)
#	define shell_show_history(x,y)
#endif //#if (COMMANDLINE_MAX_RECORD)//如果定义了历史纪录

/* Gorgeous Split-line -----------------------------------------------*/

#ifdef USE_AVL_TREE //avl tree 建立查询系统

/**
  * @brief    命令树查找，根据 id 号找到对应的控制块
  * @param    cmdindex  命令号
  * @param    root      命令二叉树根
  * @return   成功 id 号对应的控制块
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
  * @brief    命令树插入
  * @param    newcmd   新命令控制块
  * @return   成功返回 0
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
  * @brief  输入 table 键处理
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
	struct shellcommand * match[10];    //匹配到的命令行
	uint32_t match_cnt = 0;//匹配到的命令号个数
	
	for ( ; *str == ' ' ; ++str,--strlen) ; //有时候会输入空格，需要跳过
	
	if (*str == 0 || strlen == 0 || node == NULL) //没有输入信息返回
		return ;
	
	index = ((uint32_t)(*str)<<26) | (strlen << 21) ;//查找首字母相同并且长度不小于 strlen 的点
	end = (uint32_t)(*str + 1)<<26 ;                 //下一个字母开头的点

	while ( node ){//index 一定不存在，查找结束后的 shell_cmd 最靠近 index ，用此作为起始匹配点 
		shellcmd = container_of(node,struct shellcommand, node);	
		node = (index < shellcmd->ID) ? node->avl_left : node->avl_right;
	}

	//首字母的大小决定命令索引的大小，超过首字母的点不需要继续匹配
    for (node = &shellcmd->node ; shellcmd->ID < end && node; node = avl_next(node)) {
		shellcmd = container_of(node,struct shellcommand, node);
		
		if (memcmp(shellcmd->name, str, strlen) == 0){ //对比输入的字符串，如果匹配到相同的 
			match[match_cnt] = shellcmd; //把匹配到的命令号索引记下来
			if (++match_cnt > 10)        //超过十条相同返回
				return ;    
		}
	}

	if (!match_cnt) //如果没有命令包含输入的字符串，返回
		return ; 
	
	if (shellin->edit != shellin->tail) {//如果编辑位置不是末端，先把光标移到末端 
		printl(&shellin->cmdline[shellin->edit],shellin->tail - shellin->edit);
		shellin->edit = shellin->tail;
	}

	if (1 == match_cnt){  //如果只找到了一条命令包含当前输入的字符串，直接补全命令，并打印 
		for(char * ptr = match[0]->name + strlen ; *ptr ;++ptr) //打印剩余的字符
			shell_getchar(shellin,*ptr);
		shell_getchar(shellin,' ');
	}else {  //如果不止一条命令包含当前输入的字符串，打印含有相同字符的命令列表，并补全字符串输出直到命令区分点

		for(uint32_t i = 0;i < match_cnt; ++i) //把所有含有输入字符串的命令列表打印出来
			printk("\r\n\t%s",match[i]->name); 
		
		printk("\r\n%s%s",shellin->sign,shellin->cmdline); //重新打印输入标志和已输入的字符串
		
		for ( ; ; ) { //补全命令，把每条命令都包含的字符补全并打印
			for (uint32_t i = 1 ; i < match_cnt ; ++i ) {
				if (match[0]->name[strlen] != match[i]->name[strlen])
					return  ; //字符不一样，返回
			}
			shell_getchar(shellin,match[0]->name[strlen++]);  //把相同的字符补全到输入缓冲中
		}
	}
}


/**
  * @author   古么宁
  * @brief    显示所有注册了的命令
  * @param    arg       命令后所跟参数
  * @return   NULL
*/
static void shell_list_cmd(void * arg)
{
	struct shell_input * shellin = container_of(arg, struct shell_input, cmdline);
	struct shellcommand * cmd;
	uint32_t firstchar = 0;
	cmd_entry_t  * node ;
	
	for (node = avl_first(&shellcmdroot); node; node = avl_next(node)) { //遍历红黑树
		cmd = container_of(node,struct shellcommand, node);
		if (firstchar != (cmd->ID & 0xfc000000)) {
			firstchar = cmd->ID & 0xfc000000;
			printk("\r\n(%c)------",((firstchar>>26)|0x40));
		}
		printk("\r\n\t%s", cmd->name);
	}
	
	printk("\r\n\r\n%s",shellin->sign);
}

#else  //#ifdef USE_AVL_TREE  单链表建立查询系统

/**
  * @brief    命令树查找，根据 id 号找到对应的控制块
  * @param    cmdindex        命令号
  * @return   成功 id 号对应的控制块
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
  * @brief    命令树插入
  * @param    newcmd   新命令控制块
  * @return   成功返回 0
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
  * @brief 输入 table 键处理
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
	struct shellcommand * match[10];    //匹配到的命令行
	uint32_t match_cnt = 0;//匹配到的命令号个数
	uint32_t index, end;
	
	for ( ; *str == ' ' ; ++str,--strlen) ; //有时候会输入空格，需要跳过
	
	if (*str == 0 || strlen == 0) //没有输入信息返回
		return ;
	
	index = ((uint32_t)(*str)<<26) | (strlen << 21) ;//查找首字母相同并且长度不小于 strlen 的点
	end = (uint32_t)(*str + 1)<<26 ;                 //下一个字母开头的点
	
	for ( ; node ; node = node->next ) { //找到比 index 大的点，用此作为起始匹配点
		shellcmd = container_of(node, struct shellcommand, node);
		if ( shellcmd->ID > index )
			break;
	}
	
	//首字母的大小决定命令索引的大小，超过首字母的点不需要继续匹配
	for ( ; node && shellcmd->ID < end ; node = node->next ) {
		shellcmd = container_of(node,struct shellcommand, node); 
		if (memcmp(shellcmd->name, str, strlen) == 0) {//对比输入的字符串，如果匹配到相同的 
			match[match_cnt] = shellcmd; //把匹配到的命令号索引记下来
			if (++match_cnt > 10)        //超过十条相同返回
				return ;    
		}
	}

	if (!match_cnt) //如果没有命令包含输入的字符串，返回
		return ; 
	
	if (shellin->edit != shellin->tail){ //如果编辑位置不是末端，先把光标移到末端 
		printl(&shellin->cmdline[shellin->edit],shellin->tail - shellin->edit);
		shellin->edit = shellin->tail;
	}

	if (1 == match_cnt){  //如果只找到了一条命令包含当前输入的字符串，直接补全命令，并打印
		for(char * ptr = match[0]->name + strlen ; *ptr ;++ptr) //打印剩余的字符
			shell_getchar(shellin,*ptr);
		shell_getchar(shellin,' ');
	}
	else {  //如果不止一条命令包含当前输入的字符串，打印含有相同字符的命令列表，并补全字符串输出直到命令区分点
		for(uint32_t i = 0;i < match_cnt; ++i) //把所有含有输入字符串的命令列表打印出来
			printk("\r\n\t%s",match[i]->name); 
		
		printk("\r\n%s%s",shellin->sign,shellin->cmdline); //重新打印输入标志和已输入的字符串
		
		for ( ; ; ) { //补全命令，把每条命令都包含的字符补全并打印
			for (uint32_t i = 1 ; i < match_cnt ; ++i ) {
				if (match[0]->name[strlen] != match[i]->name[strlen])
					return  ; //字符不一样，返回
			}
			shell_getchar(shellin,match[0]->name[strlen++]);  //把相同的字符补全到输入缓冲中
		}
	}
}

/**
  * @author   古么宁
  * @brief    显示所有注册了的命令
  * @param    arg       命令后所跟参数
  * @return   NULL
*/
static void shell_list_cmd(void * arg)
{
	uint32_t firstchar = 0;
	struct shellcommand * cmd;
	cmd_entry_t * node = shellcmdroot.next;
	struct shell_input * shellin = container_of(arg, struct shell_input, cmdline);
	
	for ( ; node; node = node->next){//遍历红黑树
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


#if (COMMANDLINE_MAX_RECORD) //如果定义了历史纪录

/**
  * @author   古么宁
  * @brief    记录此次运行的命令及参数
  * @param    
  * @return   返回记录地址
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
  * @author   古么宁
  * @brief    按上下箭头键显示以往输入过的命令，此处只记录最近几次的命令
  * @param    void
  * @return   void
*/
static void shell_show_history(struct shell_input * shellin,uint8_t LastOrNext)
{
	uint32_t len = 0;
	
	printk("\33[2K\r%s",shellin->sign);//"\33[2K\r" 表示清除当前行

	if (!LastOrNext) //上箭头，上一条命令
		shellin->htyread = (!shellin->htyread) ? (COMMANDLINE_MAX_RECORD - 1) : (shellin->htyread - 1);
	else       //下箭头
	if (shellin->htyread != shellin->htywrt)
		shellin->htyread = (shellin->htyread + 1) % COMMANDLINE_MAX_RECORD;

	if (shellin->htyread != shellin->htywrt){ //把历史记录考到命令行内存 
		for (char * history = &shellin->history[shellin->htyread][0]; *history ; ++len)
			shellin->cmdline[len] = *history++;
	}
	
	shellin->cmdline[len] = 0; //添加结束符
	shellin->tail = len ;
	shellin->edit = len ;

	if (len)
		printl(shellin->cmdline,len); //打印命令行内容
}

#endif //#if (COMMANDLINE_MAX_RECORD) //如果定义了历史纪录

/**
  * @author   古么宁
  * @brief    控制台输入 回退 键处理
  * @param    void
  * @return   void
*/
static void shell_backspace(struct shell_input * shellin)
{
	if (!shellin->edit)//如果当前打印行有输入内容，回退一个键位
		return ;

	char   printbuf[COMMANDLINE_MAX_LEN*2]={0};//中转内存
	char * print = &printbuf[1];//这里的 printbuf 定义为全局变量，有可重入性等问题，但是为了速度
	char * printend = print + (shellin->tail - shellin->edit) + 1;
	char * edit = &shellin->cmdline[shellin->edit--] ;
	char * tail = &shellin->cmdline[shellin->tail--];

	//当输入过左右箭头时，需要作字符串插入左移处理，并作反馈回显
	//如 abUcd 中删除U，需要左移cd，并打印两个 '\b' 使光标回到 ab 处
	for (char * cp = edit - 1 ; edit < tail ; *cp++ = *edit++) {
		*print++ = *edit;
		*printend++ = '\b';
	}

	printbuf[0] = '\b';
	*print = ' ';       //覆盖最后一个字符显示
	*printend++ = '\b'; //光标回显

	shellin->cmdline[shellin->tail] = 0;  //末端添加字符串结束符
	printl(printbuf,printend-printbuf);
}

/**
  * @author   古么宁
  * @brief    命令行记录输入一个字符
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
	}
	else {//其实 else 分支完全可以处理 tail == edit 的情况，但是 printbuf 压栈太久 
		char  printbuf[COMMANDLINE_MAX_LEN*2]={0};//中转内存
		char *tail = &shellin->cmdline[shellin->tail++];
		char *edit = &shellin->cmdline[shellin->edit++];
		char *print = printbuf + (tail - edit);
		char *printend = print + 1;

		//当输入过左右箭头时，需要作字符串插入右移处理，并作反馈回显
		//如 abcd 中在bc插入U，需要右移cd，并打印两个 '\b' 使光标回到 abU 处
		for (char *cp = tail - 1; cp >= edit ; *tail-- = *cp--) {
			*print-- = *cp;
			*printend++ = '\b';
		}

		*print = ascii; 
		*edit = ascii;  //插入字符
		shellin->cmdline[shellin->tail] = 0; //末端添加字符串结束符
		printl(printbuf,printend - printbuf);//回显打印
	}
}


/**
  * @author   古么宁
  * @brief    命令行解析输入
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

	cmdmatch = shell_search_cmd(cmdroot,unCmd.ID);//匹配命令号
	if (cmdmatch != NULL) {
		if (cmdmatch->fnaddr & FUNC_CONFIRM) {// 如果是需要确认的命令 
			cmd_fn_t func = (cmd_fn_t)(cmdmatch->fnaddr & (~FUNC_CONFIRM)) ;//提取函数指针
			shellcfm_t * confirm = container_of(cmdmatch, struct shellconfirm, cmd);//提取确认信息
			shell_confirm(shellin,confirm->prompt,func);//进入确认命令行
		}else {
			cmd_fn_t func = (cmd_fn_t)cmdmatch->fnaddr ;//普通命令 
			func(shellin->cmdline);
		}
	}else {
		printk("\r\n\tno reply:%s\r\n",shellin->cmdline);
	}
	
PARSE_END:
	shellin->tail = 0;//清空当前命令行输入
	shellin->edit = 0;
	return ;
}

/**
  * @brief    控制台清屏
  * @param    arg       命令后所跟参数
  * @return NULL
*/
static void shell_clean_screen(void * arg)
{
	struct shell_input * shellin = container_of(arg, struct shell_input, cmdline);
	printk("\033[2J\033[%d;%dH%s",0,0,shellin->sign);
	return ;
}


/**
  * @brief  获取 debug 信息
  * @param    arg       命令后所跟参数
  * @return void
*/
static void shell_debug_stream(void * arg)
{
	int option;
	int argc = cmdline_param(arg,&option,1);
	
	if ((argc > 0) && (option == 0)) { //关闭调试信息打印流，仅显示交互信息 
		static const char closemsg[] = "\r\n\tclose debug information stream\r\n\r\n";
		current_puts((char*)closemsg,sizeof(closemsg) - 1);
		default_puts = NULL;       //默认信息流输出为空，将不打印调试信息
	}else {
		static const char openmsg[]  = "\r\n\tget debug information\r\n\r\n";
		current_puts((char*)openmsg,sizeof(openmsg) - 1);
		default_puts = current_puts; //设置当前交互为信息流输出
	}
}

/**
  * @author   古么宁
  * @brief    注册一个命令号和对应的命令函数 ，前缀为 '_' 表示不建议直接调用此函数
  * @param    cmd_name    命令名
  * @param    cmd_func        命令名对应的执行函数
  * @param    newcmd      命令控制块对应的指针
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
	
	newcmd->ID = unCmd.ID;   //生成命令码
	newcmd->name = cmd_name;
	newcmd->fnaddr = (uint32_t)cmd_func;

	shell_insert_cmd(&shellcmdroot,newcmd);//命令二叉树插入此节点
	
	if (NEED_CONFIRM == comfirm) //如果是带确认选项的命令
		newcmd->fnaddr |= FUNC_CONFIRM ;
	return ;
}

/**
  * @author   古么宁
  * @brief    把 "a b c d" 格式化提取为 char*argv[] = {"a","b","c","d"};以供getopt()解析
  * @param    str     命令字符串后面所跟参数缓冲区指针
  * @param    argv    数据转换后缓存地址
  * @param    maxread 最大读取数
  * @return   argc    最终读取参数个数输出
*/
int cmdline_strtok(char * str ,char ** argv ,uint32_t maxread)
{
	int argc = 0;

	for ( ; ' ' == *str ; ++str) ; //跳过空格
	
	for ( ; *str && argc < maxread; ++argc,++argv ) { //字符不为 ‘\0' 的时候
	
		for (*argv = str ; ' ' != *str && *str ; ++str);//记录这个参数，然后跳过非空字符
		
		for ( ; ' ' == *str; *str++ = '\0');//每个参数加字符串结束符，跳过空格		
	}
	
	return argc;
}


/**
  * @brief    转换获取命令号后面的输入参数，字符串转为整数
  * @param    str     命令字符串后面所跟参数缓冲区指针
  * @param    argv    数据转换后缓存地址
  * @param    maxread 最大读取数
  * @return   数据个数
	  * @retval   >= 0         读取命令后面所跟参数个数
	  * @retval   PARAMETER_ERROR(-2)  命令后面所跟参数有误
	  * @retval   PARAMETER_HELP(-1)   命令后面跟了 ? 号
*/
int cmdline_param(char * str,int * argv,uint32_t maxread)
{
	uint32_t argc , value;

	for ( ; ' ' == *str        ; ++str);//跳过空格
	for ( ; ' ' != *str && *str; ++str);//跳过第一个参数
	for ( ; ' ' == *str        ; ++str);//跳过空格

	if (*str == '?')
		return PARAMETER_HELP;//如果命令后面的是问号，返回help

	for (argc = 0; *str && argc < maxread; ++argc , ++argv) { //字符不为 ‘\0' 的时候
	
		*argv = 0;
		
		if ('0' == str[0] && 'x' == str[1]) { //"0x" 开头，十六进制转换
			for ( str += 2 ;  ; ++str )  {
				if ( (value = *str - '0') < 10 ) // value 先赋值，后判断 
					*argv = (*argv << 4)|value;
				else
				if ( (value = *str - 'A') < 6 || (value = *str - 'a') < 6)
					*argv = (*argv << 4) + value + 10;
				else
					break;
			}
		}else  { //循环把字符串转为数字，直到字符不为 0 - 9
			uint32_t minus = ('-' == *str);//正负数转换
			if (minus)
				++str;

			for (value = *str - '0'; value < 10 ; value = *(++str) - '0')
				*argv = (*argv * 10 + value);
			
			if (minus)
				*argv = -(*argv);
		}

		if ('\0' != *str && ' ' != *str)//如果不是 0 - 9 而且不是空格，则是错误字符
			return PARAMETER_ERROR;

		for ( ; ' ' == *str ; ++str);//跳过空格,继续判断下一个参数
	}

	return argc;
}

/**
  * @author   古么宁
  * @brief    欢迎页
  * @param    recv  硬件层所接收到的数据缓冲区地址
  * @param    len     硬件层所接收到的数据长度
  * @return   void
*/
void welcome_gets(struct shell_input * shellin,char * recv,uint32_t len)
{
	static const char minishelllogo[]= //打印一个欢迎页logo
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
  * @author   古么宁
  * @brief    硬件上接收到的数据到命令行的传输
  * @param    recv  硬件层所接收到的数据缓冲区地址
  * @param    len     硬件层所接收到的数据长度
  * @return   void
*/
void cmdline_gets(struct shell_input * shellin,char * recv,uint32_t len)
{
	uint32_t state = 0 ;

	for (char * end = recv + len ; recv < end ; ++recv) {
		if (0 == state) {
			if (*recv > 0x1F && *recv < 0x7f)//普通字符,当前命令行输入;
				shell_getchar(shellin,*recv);
			else
			switch (*recv) {//判断字符是否为特殊字符
				case KEYCODE_ENTER:
					if (shellin->tail){
						printk("\r\n");
						shell_record(shellin);//记录当前输入的命令和命令参数
						shell_parse(&shellcmdroot ,shellin);
					}
					else{
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
				default: ;//当前命令行输入;
			}
		}
		else //
		if (1 == state){ //判断是否是箭头内容
			state = (*recv == '[') ? 2 : 0 ;
		}
		else{// if (2 == state) //响应箭头内容
			switch(*recv){  
				case 'A'://上箭头
					shell_show_history(shellin,0);
					break;
				case 'B'://下箭头
					shell_show_history(shellin,1);
					break;
				case 'C'://右箭头  input->tail &&
					if ( shellin->tail != shellin->edit)
						printl(&shellin->cmdline[shellin->edit++],1);
					break;
				case 'D'://左箭头
					if (shellin->edit){
						--shellin->edit;
						printl("\b",1);
					}
					break;
				default:;
			} //switch 箭头内容
		} // if (2 == state) //响应箭头内容
	} //for ( ; len && *recv; --len,++recv)
	return ;
}

/**
  * @brief    命令行信息确认，如果输入 y/Y 则执行命令
  * @param
  * @return   void
*/
static void confirm_gets(struct shell_input * shellin ,char * buf , uint32_t len)
{
	char * option = &shellin->cmdline[COMMANDLINE_MAX_LEN-1];

	if (0 == *option) { //先输入 [Y/y/N/n] ，其他按键无效
		if ('Y' == *buf || 'y' == *buf || 'N' == *buf || 'n' == *buf) {
			*option = *buf;
			printl(buf,1);
		}
	}
	else
	if (KEYCODE_BACKSPACE == *buf) { //回退键
		printl("\b \b",3);
		*option = 0;
	}
	else
	if ('\r' == *buf || '\n' == *buf) {//按回车确定
		cmd_fn_t yestodo = (cmd_fn_t)shellin->apparg;
 		char opt = *option ; 
		
		*option = 0 ;  //shellin->cmdline[COMMANDLINE_MAX_LEN-1] = 0;
		shellin->gets   = cmdline_gets;//数据回归为命令行模式
		shellin->apparg = NULL;

		printl("\r\n",2);

		if ( 'Y' == opt || 'y' == opt) //确定更新 iap
			yestodo(shellin->cmdline);
		else
			printk("cancel this operation\r\n");
	}
}

/**
  * @brief    命令行信息确认，如果输入 y/Y 则执行命令
  * @param    shell  : 输入交互
  * @param    info   : 选项信息
  * @param    yestodo: 输入 y/Y 后所需执行的命令
  * @return   void
*/
void shell_confirm(struct shell_input * shellin ,char * info ,cmd_fn_t yestodo)
{
	printk("%s [Y/N] ",info);
	shellin->gets = confirm_gets;//串口数据流获取至 shell_option
	shellin->apparg = yestodo;
	shellin->cmdline[COMMANDLINE_MAX_LEN-1] = 0;
}

/**
  * @author   古么宁
  * @brief    初始化一个 shell 交互，默认输入为 cmdline_gets
  * @param    shellin   : 需要初始化的 shell 交互 
  * @param    shellputs : shell 对应输出，如从串口输出。
  * @param    ...       : 对 gets 和 sign 重定义，如追加 MODIFY_SIGN,"shell>>"
  * @return   NULL
*/
void shell_input_init(struct shell_input * shellin , fmt_puts_t shellputs,...)
{
	char * shellsign = DEFAULT_INPUTSIGN;
	shellgets_t shellgets = welcome_gets;
	
	va_list ap;
	va_start(ap, shellputs); //检测有无新定义 

	for (uint32_t arg = va_arg(ap, uint32_t) ; MODIFY_MASK == (arg & (~0x0f)) ; arg = va_arg(ap, uint32_t) ) {
		if (MODIFY_SIGN == arg) //如果重定义当前交互的输入标志
			shellsign = va_arg(ap, char*);
		else
		if (MODIFY_GETS == arg) //如果重定义当前交互的输入流向
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
  * @author   古么宁
  * @brief    shell 初始化,注册几条基本的命令。允许不初始化。
  * @param    puts : printf,printk,printl 的默认输出，如从串口输出。
  * @return   void
*/
void shell_init(char * defaultsign ,fmt_puts_t puts)
{
	strcpy(DEFAULT_INPUTSIGN,defaultsign);

	current_puts = puts ;
	default_puts = puts ;
	
	//注册一些基本命令
	shell_register_command("cmd-list",shell_list_cmd);
	shell_register_command("clear",shell_clean_screen);
	shell_register_command("debug-info",shell_debug_stream);
}


