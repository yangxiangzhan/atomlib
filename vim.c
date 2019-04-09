/**
  ******************************************************************************
  * @file           vim.c
  * @author         古么宁
  * @brief          文本编辑器，依赖 shell.h/shell.c
  ******************************************************************************
  *
  * COPYRIGHT(c) 2018 GoodMorning
  *
  ******************************************************************************
  */
/* Includes ---------------------------------------------------*/
#include <string.h>
#include <stdint.h> //定义了很多数据类型
#include "shell.h"
#include "vim.h"

/* Private types ------------------------------------------------------------*/
enum VIM_STATE
{
	VIM_READ_ONLY = 0,
	VIM_EDITING ,
	VIM_COMMAND ,
	VIM_QIUT
};

struct vim_edit_buf
{
	vim_fputs_t fputs; //文件输出
	char *      fpath; //命令行输入的编辑文件名
	char *      edit;  //当前编辑位置
	uint16_t    rowmax;//文件共有几行
	uint16_t    rows;  //文件光标所在行位置
	uint16_t    cols;  //文件光标所在列位置
	uint16_t    tail;  //文件尾部位置
	char  editbuf[VIM_MAX_EDIT]; //文件编辑内存 
	char  printbuf[VIM_MAX_EDIT];//文件打印中转内存
	char  option;
	char  state;
};

/* Private macro ------------------------------------------------------------*/

//#define VIM_USE_HEAP  //开关宏，是否使用堆内存，屏蔽掉则使用静态内存

#ifdef  VIM_USE_HEAP  //如果使用堆内存，需要提供 VIM_MALLOC 和 VIM_FREE 

	#include "heaplib.h"
	#define VIM_MALLOC(bytes) MALLOC(bytes)
	#define VIM_FREE(buf)     FREE(buf)
	
#else //使用静态变量的情况

	static struct vim_edit_buf vimsbuf = {0};
	#define VIM_MALLOC(bytes)  ((vimsbuf.fputs == NULL)?((void*)(&vimsbuf)):NULL)
	#define VIM_FREE(buf)      (((struct vim_edit_buf*)(buf))->fputs = NULL)

#endif


/* Private variables ------------------------------------------------------------*/
	
static const char clearline[] = "\33[2K\r\n";
static const char waiting_title[] = "'i'(Edit) or ':'(quit/save)";
static const char editing_title[] = "Editing...press ESC to quit";

/* Private function prototypes -----------------------------------------------*/

/* Gorgeous Split-line -----------------------------------------------*/


/**
	* @brief    cursor_up
	*           编辑器响应上箭头
	* @param    vim  编辑内存
	* @return   void
*/
static void cursor_up(struct vim_edit_buf * vimedit)
{				
	if (vimedit->rows > 1) { //如果编辑所在位置为第一行，则不进行响应
	
		char   * ptr = vimedit->edit; //当前编辑位置
		uint32_t cnt;
		
		if ((--vimedit->rows) > 1) {
			for ( ; *  ptr   != '\n' ; --ptr ); //回到上一行结尾处的'\n'
			for ( ; *(ptr-1) != '\n' ; --ptr ); //上一行开头
		}
		else {//如果现在是第二行，则回到第一行所在位置
			ptr = vimedit->editbuf;
		}

		for (cnt = 1;cnt < vimedit->cols ;++cnt) { //计算当前行的列位置
			if (*ptr == '\r' || *ptr == '\n')      //当前行比下一行短，则没有办法回到下一行所在的列位置
				break;
			else
				++ptr;
		}
		
		vimedit->cols = cnt;
		vimedit->edit = ptr; //更新当前编辑位置
		printk("\033[%d;%dH",vimedit->rows + 1,vimedit->cols); //打印光标位置
	}
}


/**
	* @brief    cursor_down
	*           编辑器响应下箭头
	* @param    vim  编辑内存
	* @return   void
*/
static void cursor_down(struct vim_edit_buf * vimedit)
{
	if (vimedit->rows < vimedit->rowmax){	 //如果当前行不是文本最后一行，响应下箭头
	
		char   * ptr = vimedit->edit;
		uint32_t cnt;
		
		while( *ptr++ != '\n'  );//下一行开头处
		
		for (cnt = 1; cnt < vimedit->cols ; ++cnt) {
			if ( *ptr == '\r' || *ptr == '\0' || *ptr == '\n')//比上一行短时，无法移动光标至原来的列位置
				break;
			else
				++ptr;
		}

		printl(vimedit->edit,ptr - vimedit->edit);
		++vimedit->rows;    //更新行数
		vimedit->cols = cnt;//更行列数
		vimedit->edit = ptr;//更新当前编辑位置
	}
}


/**
	* @brief    cursor_right
	*           编辑器响应右箭头
	* @param    vim  编辑内存
	* @return   void
*/
static void cursor_right(struct vim_edit_buf * vimedit)
{
	char * edit = vimedit->edit;
	if (*edit == '\r' || *edit == '\0' || *edit == '\n')//行末尾
		return ;
	
	++vimedit->cols;
	++vimedit->edit;
	printl(edit,1);
}


/**
	* @brief    cursor_left
	*           编辑器响应左箭头
	* @param    vim  编辑内存
	* @return   void
*/
static void cursor_left(struct vim_edit_buf * vimedit)
{
	if (vimedit->edit == vimedit->editbuf || vimedit->cols < 2)//行开头
		return;
	
	--vimedit->edit;
	--vimedit->cols;
	printl("\b",1);
}


/**
	* @brief    cursor_move
	*           编辑器响应箭头
	* @param    vim  编辑内存
	* @return   void
*/
static void cursor_move(struct vim_edit_buf * vimedit , char arrow)
{
	switch( arrow ) {
		case 'A':cursor_up(vimedit);break; //上箭头

		case 'B':cursor_down(vimedit);break;//下箭头

		case 'C':cursor_right(vimedit);break;//右箭头

		case 'D':cursor_left(vimedit);break;//左箭头
		
		default : return ;
	}
}


/**
	* @brief    vim_backspace
	*           编辑器回删一个字符
	* @param    vim  编辑内存
	* @return   void
*/
static void vim_backspace(struct vim_edit_buf * vim)
{
	char * print = vim->printbuf;
	char * edit = vim->edit;     //当前编辑位置
	char * tail = &vim->editbuf[vim->tail];//原编辑区结尾; 
	
	//if (*(vim->edit-1) != '\n') //如果当前编辑位置不是行开头
	if (vim->cols != 1) {  //如果当前编辑位置不是行开头
	
		char * copy = vim->edit;
		
		//找到当前行结束位置，并复制到打印内存中
		for (*print++ = '\b' ; *copy != '\r' && *copy && *copy != '\n' ; *print++ = *copy++);
		
		*print++ = ' '; //覆盖当前行最后一个字符显示
		*print++ = '\b'; //光标回显
		
		//计算光标回退个数
		for (uint32_t cnt = copy - vim->edit ; cnt-- ;*print++ = '\b' );
		
		//如 abUcd 中删除U，需要左移cd，并打印两个 '\b' 使光标回到 ab 处
		for (copy = edit - 1 ; edit < tail ; *copy++ = *edit++);
		
		--vim->cols;
		--vim->edit;
		--vim->tail;
		vim->editbuf[vim->tail] = 0;//末端添加字符串结束符
		printl(vim->printbuf,print - vim->printbuf);
	}
	else {//当前编辑位置是行开头，回退一行
	
		char * prevlines ; //当前编辑点上一行的开头
		char * prevlinee ; //当前编辑点上一行的结尾
		
		if (vim->rows > 2) //当前编辑行不是第二行
			for (prevlines = edit-1 ; *(prevlines-1) != '\n' ; --prevlines);//回到上一行开头
		else
			prevlines = vim->editbuf; //回到第一行开头

		//找到上一行结尾处，已知有下一行，不需要判断 \0
		for (prevlinee = prevlines ; *prevlinee != '\r' && *prevlinee != '\n' ; ++prevlinee);

		//控制台清空编辑点所在行以及后面所有的内容显示
		for(uint32_t i = vim->rows ; i <= vim->rowmax ; ++i )
			printl((char*)clearline , sizeof(clearline)-1);

		//把当前编辑点内容考到上一行结尾
		//memcpy(prevlinee, edit ,tail - edit);
		for (char * copy = prevlinee ; edit < tail ; *copy++ = *edit++);
		
		vim->rowmax -= 1;
		vim->rows -= 1;
		vim->tail -= (vim->edit - prevlinee);//更新编辑尾部，适应 '\r\n'
		vim->edit  = prevlinee ;             //更新当前编辑位置为上一行结尾
		vim->cols  = prevlinee-prevlines+1;  //更新当前列位置为上一行结尾
		vim->editbuf[vim->tail] = 0; //末端添加字符串结束符
		
		printk("\033[%d;%dH",vim->rows+1,vim->cols);//光标回到编辑点位置
		printl(vim->edit,strlen(vim->edit));        //打印剩余内容
		printk("\033[%d;%dH",vim->rows+1,vim->cols);//光标再次回到编辑点位置
	}
}


/**
	* @brief    vim_insert_char
	*           编辑器插入一个普通字符
	* @param    vim  编辑内存
	* @param    ascii 所插入字符
	* @return   void
*/
static void vim_insert_char(struct vim_edit_buf * vim,char ascii)
{
	char * print = vim->printbuf ;
	char * tail = &vim->editbuf[vim->tail];//tail 所处位置是字符串结束符的位置
	char * copy = vim->edit;

	//计算需要打印的字符串，找到当前行结束位置
	for (*print++ = ascii; *copy != '\r' && *copy != '\n' && *copy ; *print++ = *copy++);
	
	//计算当前行需要回退的光标数
	for (uint32_t cnt = copy - vim->edit ; cnt-- ; *print++ = '\b');
	
	//如 abcd 中在bc插入U，需要右移cd，并打印两个 '\b' 使光标回到 abU 处
	for (copy = tail - 1; copy >= vim->edit ; *tail-- = *copy--);
	
	*vim->edit = ascii;  //插入字符
	++vim->edit;
	++vim->cols;
	++vim->tail;
	vim->editbuf[vim->tail] = 0; //末端添加字符串结束符
	printl(vim->printbuf,print - vim->printbuf);
}



/**
	* @brief    vim_insert_newline
	*           编辑器插入换行符/回车符
	* @param    vim  编辑内存
	* @param    ascii 所插入字符
	* @return   void
*/
static void vim_insert_newline(struct vim_edit_buf * vim)
{
	char * tail = &vim->editbuf[vim->tail];//tail 所处位置是字符串结束符的位置
	char * copy ;
	
	//清空此行以及后面所有内容
	for(uint32_t i = vim->rows ; i <= vim->rowmax ; ++i )
		printl((char*)clearline,sizeof(clearline)-1);
	
	printk("\033[%d;%dH",vim->rows+1,1);//光标回到当前编辑行开头

	if (vim->rows > 1)
		for (copy=vim->edit ; *(copy-1) != '\n' ; --copy);
	else
		copy = vim->editbuf;
	
	printl(copy,vim->edit-copy); //打印至当前编辑位置

	tail += 1;//新的尾部

	// 当前编辑位置后移两个字节空间
	for (copy = tail-2 ; copy >= vim->edit ; *tail-- = *copy--) ;
	
	*vim->edit++ = '\r';
	*vim->edit++ = '\n';
	vim->tail += 2;
	vim->rows += 1;
	vim->cols  = 1;
	vim->rowmax += 1;
	vim->editbuf[vim->tail] = 0; //末端添加字符串结束符
	
	tail = &vim->editbuf[vim->tail] - 1;
	copy = (vim->edit - 2);
	
	printl(copy,tail-copy); //打印至当前编辑位置
	printk("\033[%d;%dH",vim->rows+1,1);
}



/**
	* @brief    vim_insert_newline
	*           编辑过程接收字符
	* @param    vim  编辑内存
	* @param    data 所接收字符
	* @return   void
*/
static void vim_edit_getchar(struct vim_edit_buf * vim,char data)
{

	if (KEYCODE_CTRL_C == data || KEYCODE_TAB == data ||
		KEYCODE_END == data || KEYCODE_HOME == data ) { //一些特别的字符过滤
		return ;
	}

	if (KEYCODE_BACKSPACE == data || 0x7f == data) {//0x7f ,for putty
		if (vim->editbuf != vim->edit)
			vim_backspace(vim);
	}
	else
	if (vim->tail < VIM_MAX_EDIT/2) {//插入字符
		if ('\r' == data || '\n' == data ) //插入换行符
			vim_insert_newline(vim);
		else
			vim_insert_char(vim , data);   //插入普通字符
	}
}

/**
	* @brief    vim_edit
	*           键盘数据流入
	* @param    vim  编辑内存
	* @return   void
*/
static void shell_vim_gets(struct shell_input * shell ,char * buf , uint32_t len)
{
	struct vim_edit_buf * vimedit = (struct vim_edit_buf *)shell->apparg ;

	switch(vimedit->state) {   //状态机
		case VIM_READ_ONLY:    //文本只读过程
			if (*buf == 'i') { //键盘输入 'i'
				vimedit->state = VIM_EDITING;//进入编辑模式
				printk("\033[%d;%dH\033[2K\r\t",1,1);//光标回到第一行并清空第一行
				printl((char*)editing_title,sizeof(editing_title)-1); //打印提示信息
				printk("\033[%d;%dH",vimedit->rows + 1,vimedit->cols);//恢复光标位置
			}
			else
			if (*buf == ':') {
				vimedit->option = 0;
				vimedit->state = VIM_COMMAND;
				printk("\033[%d;%dH\033[2K\r(w/q):",1,1);//光标回到第一行并清空第一行
			}
			else
			if ( buf[0] == '\033' && len > 1 && buf[1] == '[' ) {
				cursor_move(vimedit,buf[2]);
			}
			break;
			
		case VIM_EDITING: //文本编辑过程
			if (*buf == '\033') {
				if ( len > 1 && buf[1] == '[' ) {
					cursor_move(vimedit,buf[2]);//如果是箭头输入
				}
				else {
					vimedit->state = VIM_READ_ONLY; //如果是单按键 Esc ，回到只读模式
					printk("\033[%d;%dH\033[2K\r",1,1);//光标回到第一行并清空第一行
					printk("\t%s",waiting_title);      //打印提示信息
					printk("\033[%d;%dH",vimedit->rows + 1,vimedit->cols);//恢复光标位置
				}
			}
			else {
				vim_edit_getchar(vimedit ,* buf);
			}
			break;
			
		case VIM_COMMAND: //等待命令
			if (*buf == 'q' ||*buf == 'w') {
				vimedit->state  = VIM_QIUT;
				vimedit->option = *buf;
				printl(buf,1);
			}
			else
			if (*buf == '\033' ) { //&& len == 1) 
				vimedit->state = VIM_READ_ONLY;  //进入只读模式
				vimedit->edit = vimedit->editbuf;
				vimedit->cols = 1;
				vimedit->rows = 1;
				printk("\033[2K\r\t%s\r\n",waiting_title);//打印提示信息
			}
			break;
			
		case VIM_QIUT:   //等待退出
			if (*buf == KEYCODE_BACKSPACE || *buf == 0x7f) { //回退键
				vimedit->state  = VIM_COMMAND;
				printl("\b \b",3);
			}
			else
			if (*buf == '\r' || *buf == '\n') { //回车确认
				if ('w' == vimedit->option)//输入保存，则输出此文件
					vimedit->fputs(vimedit->fpath,vimedit->editbuf,vimedit->tail);
				VIM_FREE(vimedit);
				shell->apparg = NULL;
				shell->gets   = cmdline_gets;
				printk("\033[2J\033[%d;%dH%s",1,1,shell->sign);
			}
			else
			if (*buf == '\033' ) { //&& len == 1)
				vimedit->state = VIM_READ_ONLY;  //进入只读模式
				vimedit->edit = vimedit->editbuf;
				vimedit->cols = 1;
				vimedit->rows = 1;
				printk("\033[2K\r\t%s\r\n",waiting_title);//打印提示信息
			}
			break;

		default:;
	}
}



/**
	* @brief    shell_into_edit
	*           当前输入进入文本编辑模式
	* @param    shell  需要进行文本编辑的 shell 交互
	* @param    fgets  获取编辑文本数据源的接口
	* @param    fputs  更新文本数据源的接口
	* @return   void
*/
void shell_into_edit(struct shell_input * shell,vim_fgets_t fgets ,vim_fputs_t fputs)
{
	char * argv[2] = {NULL};
	struct vim_edit_buf * edit;
	
	shell->apparg = VIM_MALLOC(sizeof(struct vim_edit_buf));
	if ( NULL ==  shell->apparg ) {
		printk("not enough memery\r\n");
		return ;
	}

	cmdline_strtok(shell->cmdline,argv,2);//提取路径信息,路径名称为第二个参数
	
	edit = (struct vim_edit_buf *)(shell->apparg);
	edit->fpath = argv[1]; //路径名称为第二个参数

	//尝试打开，如果失败则返回
	if (VIM_FILE_OK !=  fgets(edit->fpath,&edit->editbuf[0],&edit->tail)) {
		VIM_FREE(shell->apparg);
		return ;
	}
	
	shell->gets = shell_vim_gets; //重定义数据流输入,编辑文件模式
	if (default_puts == shell->puts)
		default_puts = NULL;
	
	edit->fputs = fputs;
	edit->editbuf[edit->tail] = 0;
	edit->edit   = edit->editbuf;
	edit->state  = VIM_READ_ONLY;
	edit->option = 0;
	edit->cols   = 1;
	edit->rows   = 1;
	edit->rowmax = 1;

	//扫描文件共有几行
	for (char * editbuf = edit->editbuf ; *editbuf ; ++editbuf) {
		if (*editbuf == '\n')
			++edit->rowmax;
	}

	//清空屏幕，并打印文本内容
	printk("\033[2J\033[%d;%dH",1,1);
	printk("\t%s\r\n%s",waiting_title,edit->editbuf);
	printk("\033[%d;%dH",2,1);
}


