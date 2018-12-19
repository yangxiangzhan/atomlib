/**
  ******************************************************************************
  * @file           vim.c
  * @author         古么宁
  * @brief          文本编辑器
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
#include "kernel.h"

#include "heaplib.h"

#include "lfs.h"
#include "spiflash_lfs.h"


#define VIM_MAX_EDIT 1024


enum VIM_STATE
{
	VIM_READ_ONLY = 0,
	VIM_EDITING ,
	VIM_COMMAND ,
};
	

struct vim_edit_buf
{
	uint16_t  filerows;//文件共有几行
	uint16_t  Rows;//文件光标所在行位置
	uint16_t  Cols;//文件光标所在列位置
	uint16_t  tail;//文件尾部位置
	char *edit;    //当前编辑位置
	char *filename;//文件名称
	char  option;
	char  state;
	char  editbuf[VIM_MAX_EDIT]; //文件编辑内存
	char  printbuf[VIM_MAX_EDIT];//文件打印内存
};


static const char clearline[] = "\33[2K\r\n";
static const char waiting_title[] = "'i'(Edit) or ':'(quit/save)";
static const char editing_title[] = "Editing...press ESC to quit";


/*
  * @author   古么宁
  * @brief    vim_scan_file 
  *           扫描当前文件编辑
  * @param    vim
  * @return   void
*/
static void vim_scan_file(struct vim_edit_buf * vim)
{
	char *editbuf = vim->editbuf;
	vim->Cols = 1;
	vim->Rows = 1;
	vim->filerows = 1;
	vim->edit = vim->editbuf;
	vim->state = VIM_READ_ONLY;
	vim->option = 0;
		
	while(*editbuf)
	{
		if (*editbuf++ == '\n')
			++vim->filerows;
	}
	
	vim->tail = editbuf - vim->editbuf;

	//清空屏幕，并打印文本内容
	printk("\033[2J\033[%d;%dH",1,1);
	printk("\t%s\r\n%s",waiting_title,vim->editbuf);
	printk("\033[%d;%dH",2,1);
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
	char * tail ;//tail 所处位置是字符串结束符的位置
	char * edit;
	
	if (vim->editbuf == vim->edit)
		return ;
	
	if (*(vim->edit-1) != '\n') //如果当前编辑位置不是行开头
	{
		char * copy = vim->edit;
		edit = vim->edit;
		tail = &vim->editbuf[vim->tail--];
		
		//找到当前行结束位置，并复制到打印内存中
		for (*print++ = '\b' ; *copy != '\r' && *copy && *copy != '\n' ; *print++ = *copy++);
		
		*print++ = ' '; //覆盖当前行最后一个字符显示
		*print++ = '\b'; //光标回显
		
		//计算光标回退个数
		for (uint32_t cnt = copy - vim->edit ; cnt-- ;*print++ = '\b' );
		
		//如 abUcd 中删除U，需要左移cd，并打印两个 '\b' 使光标回到 ab 处
		for (copy = edit - 1 ; edit < tail ; *copy++ = *edit++);
		
		vim->editbuf[vim->tail] = 0;//末端添加字符串结束符
		
		--vim->Cols;
		--vim->edit;
		printl(vim->printbuf,print - vim->printbuf);
	}/**/
	else //当前编辑位置是行开头，回退一行
	{
		char * prevlines ; //当前编辑点上一行的开头
		char * prevlinee ; //当前编辑点上一行的结尾
		edit = vim->edit ; //当前编辑位置
		
		if (vim->Rows > 2) //当前编辑行不是第二行
			for (prevlines = edit-1 ; *(prevlines-1) != '\n' ; --prevlines);//回到上一行开头
		else
			prevlines = vim->editbuf; //回到第一行开头

		//找到上一行结尾处，已知有下一行，不需要判断 \0
		for (prevlinee = prevlines ; *prevlinee != '\r' && *prevlinee != '\n' ; ++prevlinee);

		//控制台清空编辑点所在行以及后面所有的内容显示
		for(uint32_t i = vim->Rows ; i <= vim->filerows ; ++i )
			printl((char*)clearline , sizeof(clearline)-1);
		
		tail = &vim->editbuf[vim->tail];     //原编辑区结尾
		memcpy(prevlinee, edit ,tail - edit);//把当前编辑点内容考到上一行结尾
		
		vim->Rows -= 1;
		vim->filerows -= 1;
		vim->edit  = prevlinee ;             //更新当前编辑位置为上一行结尾
		vim->Cols  = prevlinee-prevlines+1;  //更新当前列位置为上一行结尾
		vim->tail -= (vim->edit - prevlinee);//更新编辑尾部，适应 '\r\n'
		vim->editbuf[vim->tail] = 0; //末端添加字符串结束符
		
		printk("\033[%d;%dH",vim->Rows+1,vim->Cols);//光标回到编辑点位置
		printl(vim->edit,strlen(vim->edit));        //打印剩余内容
		printk("\033[%d;%dH",vim->Rows+1,vim->Cols);//光标再次回到编辑点位置
	}
}


/**
	* @brief    vim_insert_char
	*           编辑器插入一个字符
	* @param    vim  编辑内存
	* @param    ascii 所插入字符
	* @return   void
*/
static void vim_insert_char(struct vim_edit_buf * vim,char ascii)
{
	char * print = vim->printbuf ;
	char *tail ;//tail 所处位置是字符串结束符的位置
	char *copy ;
	
	if (vim->tail + 1 >= VIM_MAX_EDIT)
		return ;

	if (ascii == '\t')
		return ;

	if (ascii == '\n' || ascii == '\r')//插入换行的情况
	{
		//清空此行以及后面所有内容
		for(uint32_t i = vim->Rows ; i <= vim->filerows ; ++i )
			printl((char*)clearline,sizeof(clearline)-1);
		
		printk("\033[%d;%dH",vim->Rows+1,1);//光标回到当前编辑行开头

		if (vim->Rows > 1)
			for (copy=vim->edit ; *(copy-1) != '\n' ; --copy);
		else
			copy = vim->editbuf;
		
		printl(copy,vim->edit-copy); //打印至当前编辑位置

		tail = &vim->editbuf[vim->tail] + 1;//新的尾部

		// 当前编辑位置后移两个字节空间
		for (copy = tail-2 ; copy >= vim->edit ; *tail-- = *copy--) ;
		
		*vim->edit++ = '\r';
		*vim->edit++ = '\n';
		vim->tail += 2;
		vim->Rows += 1;
		vim->Cols  = 1;
		vim->filerows += 1;
		vim->editbuf[vim->tail] = 0; //末端添加字符串结束符
		
		tail = &vim->editbuf[vim->tail] - 1;
		copy = (vim->edit - 2);
		
		printl(copy,tail-copy); //打印至当前编辑位置
		printk("\033[%d;%dH",vim->Rows+1,1);
	}
	else
	{
		tail = &vim->editbuf[vim->tail++];//tail 所处位置是字符串结束符的位置
		copy = vim->edit;

		//计算需要打印的字符串，找到当前行结束位置
		for (*print++ = ascii; *copy != '\r' && *copy != '\n' && *copy ; *print++ = *copy++);
		
		//计算当前行需要回退的光标数
		for (uint32_t cnt = copy - vim->edit ; cnt-- ; *print++ = '\b');
		
		//如 abcd 中在bc插入U，需要右移cd，并打印两个 '\b' 使光标回到 abU 处
		for (copy = tail - 1; copy >= vim->edit ; *tail-- = *copy--);

		*vim->edit = ascii;  //插入字符
		++vim->edit;
		++vim->Cols;
		vim->editbuf[vim->tail] = 0;      //末端添加字符串结束符
		printl(vim->printbuf,print - vim->printbuf);
	}
	
}

	

/**
	* @brief    up_arrow_response
	*           编辑器响应上箭头
	* @param    vim  编辑内存
	* @return   void
*/
static void up_arrow_response(struct vim_edit_buf * editing)
{				
	if (editing->Rows > 1)  //如果编辑所在位置为第一行，则不进行响应
	{
		char   * ptr = editing->edit; //当前编辑位置
		uint32_t cnt;
		
		if ((--editing->Rows) > 1)
		{
			while(*ptr-- != '\n'); //回到上一行结尾处
			if (*ptr != '\n')
			{
				while(*--ptr != '\n');//当前行的开头
				++ptr;
			}
		}
		else //如果现在是第二行，则回到第一行所在位置
		{
			ptr = editing->editbuf;
		}

		for (cnt = 1;cnt < editing->Cols ;++cnt)//计算当前行的列位置
		{
			if (*ptr == '\r' || *ptr == '\n')   //当前行比下一行短，则没有办法回到下一行所在的列位置
				break;
			else
				++ptr;
		}
		
		editing->Cols = cnt;
		editing->edit = ptr; //更新当前编辑位置
		printk("\033[%d;%dH",editing->Rows + 1,editing->Cols); //打印光标位置
	}
}



/**
	* @brief    down_arrow_response
	*           编辑器响应下箭头
	* @param    vim  编辑内存
	* @return   void
*/
static void down_arrow_response(struct vim_edit_buf * editing)
{
	if (editing->Rows < editing->filerows) //如果当前行不是文本最后一行，响应下箭头
	{	
		char   * ptr = editing->edit;
		uint32_t cnt;
		
		while( *ptr++ != '\n'  );//下一行开头处
		
		for (cnt = 1; cnt < editing->Cols ;++cnt)
		{
			if ( *ptr == '\r' || *ptr == '\0' || *ptr == '\n')//比上一行短时，无法移动光标至原来的列位置
				break;
			else
				++ptr;
		}

		printl(editing->edit,ptr - editing->edit);
		++editing->Rows;    //更新行数
		editing->Cols = cnt;//更行列数
		editing->edit = ptr;//更新当前编辑位置
	}
}


/**
	* @brief    rignt_arrow_response
	*           编辑器响应右箭头
	* @param    vim  编辑内存
	* @return   void
*/
static void rignt_arrow_response(struct vim_edit_buf * editing)
{
	char * edit = editing->edit;
	if (*edit == '\r' || *edit == '\0' || *edit == '\n')//行末尾
		return ;
	
	++editing->Cols;
	++editing->edit;
	printl(edit,1);
}


/**
	* @brief    left_arrow_response
	*           编辑器响应左箭头
	* @param    vim  编辑内存
	* @return   void
*/
static void left_arrow_response(struct vim_edit_buf * editing)
{
	if (editing->edit == editing->editbuf || editing->Cols < 2)//行开头
		return;
	
	--editing->edit;
	--editing->Cols;
	printl("\b",1);
}


/**
	* @brief    arrow_response
	*           编辑器响应箭头
	* @param    vim  编辑内存
	* @return   void
*/
static void arrow_response(struct vim_edit_buf * editing , char arrow)
{
	switch( arrow )
	{
		case 'A': //上箭头
			up_arrow_response(editing);
			break;

		case 'B'://下箭头
			down_arrow_response(editing);
			break;

		case 'C'://右箭头
			rignt_arrow_response(editing);
			break;

		case 'D'://左箭头
			left_arrow_response(editing);
			break;
	}
}



/**
	* @brief    vim_edit
	*           键盘数据流入
	* @param    vim  编辑内存
	* @return   void
*/
static void vim_gets(struct shell_input * shell ,char * buf , uint32_t len)
{
	struct vim_edit_buf * this_edit = (struct vim_edit_buf *)shell->apparg ;

	switch(this_edit->state)//状态机
	{
		case VIM_READ_ONLY:
			if (*buf == 'i')//键盘输入 'i'
			{
				this_edit->state = VIM_EDITING;//进入编辑模式
				printk("\033[%d;%dH\033[2K\r",1,1);//光标回到第一行并清空第一行
				printk("\t%s",editing_title);      //打印提示信息
				printk("\033[%d;%dH",this_edit->Rows + 1,this_edit->Cols);//恢复光标位置
			}
			else
			if (*buf == ':')
			{
				this_edit->option = 0;
				this_edit->state = VIM_COMMAND;
				printk("\033[%d;%dH\033[2K\r(w/q):",1,1);//光标回到第一行并清空第一行
			}
			else
			if ( buf[0] == '\033' && len > 1 && buf[1] == '[' )
			{
				arrow_response(this_edit,buf[2]);
			}
			break;
			
		case VIM_EDITING: //编辑过程
			if (*buf == '\033')
			{
				if ( len > 1 && buf[1] == '[' )
					arrow_response(this_edit,buf[2]);//如果是箭头输入
				else
				{
					this_edit->state = VIM_READ_ONLY; //如果是单按键 Esc ，回到只读模式
					printk("\033[%d;%dH\033[2K\r",1,1);//光标回到第一行并清空第一行
					printk("\t%s",waiting_title);      //打印提示信息
					printk("\033[%d;%dH",this_edit->Rows + 1,this_edit->Cols);//恢复光标位置
				}
			}
			else
			if (*buf == KEYCODE_BACKSPACE) 
				vim_backspace(this_edit);       //backspace
			else
				vim_insert_char(this_edit,*buf);//插入字符
			break;
			
		case VIM_COMMAND:
			if (*buf == KEYCODE_BACKSPACE) //回退键
			{
				if (this_edit->option)
				{
					this_edit->option = 0;
					printk("\b \b");
				}
			}
			else
			if (*buf == '\r' || *buf == '\n')
			{
				if ('w' == this_edit->option)
				{
					lfs_file_t filehandle;
					lfs_file_open(&g_spi_lfs, &filehandle ,this_edit->filename, LFS_O_RDWR|LFS_O_CREAT);
					lfs_file_write(&g_spi_lfs, &filehandle,this_edit->editbuf,this_edit->tail);
					lfs_file_close(&g_spi_lfs, &filehandle);
				}
				FREE(shell->apparg);
				shell->apparg = NULL;
				shell->gets   = cmdline_gets;
				printk("\033[2J\033[%d;%dH%s",1,1,shell_input_sign);
			}
			else
			if (*buf == '\033' && len == 1)
			{
				this_edit->Cols = 1;
				this_edit->Rows = 1;
				this_edit->edit = this_edit->editbuf;
				this_edit->state = VIM_READ_ONLY;             //进入只读模式
				printk("\033[2K\r\t%s\r\n",waiting_title);//打印提示信息

			}
			else
			if (*buf == 'q' ||*buf == 'w')
			{
				if (0 == this_edit->option )
				{
					this_edit->option = *buf;
					printl(buf,1);
				}
			}
			break;

		default:;
	}
}




static void shell_vim_edit(void * arg)
{
	lfs_file_t lfs_demo_file;
	int argc ;
	char * argv[4];
	
	struct vim_edit_buf * this_edit;
	struct shell_input  * this_shell;
	
	argc = shell_option_suport((char*)arg,argv);//提取路径信息

	if (2 == argc) //如果有输入路径
	{
		//尝试打开文件
		if ( 0 != lfs_file_open(&g_spi_lfs, &lfs_demo_file ,argv[1], LFS_O_RDONLY))
		{
			printk("cannot open file:%s\r\n",argv[1]);
			return ;
		}

		//判断文件大小，太大的文件不能编辑
		if (VIM_MAX_EDIT < lfs_demo_file.size)
		{
			printk("\tnot enough ram to edit this file.\r\n");
			return ;
		}

		this_shell = container_of(arg, struct shell_input, buf); //获取当前 shell_input
		this_shell->apparg = MALLOC(sizeof(struct vim_edit_buf));//申请编辑内存

		if (!this_shell->apparg) //如果申请失败
			return ;
		
		this_shell->gets = vim_gets; //重定义数据流输入

		this_edit = (struct vim_edit_buf *)(this_shell->apparg);

		lfs_file_read(&g_spi_lfs, &lfs_demo_file,this_edit->editbuf,lfs_demo_file.size);
		this_edit->filename = argv[1];
		this_edit->editbuf[lfs_demo_file.size] = 0;
		vim_scan_file(this_edit);

		lfs_file_close(&g_spi_lfs, &lfs_demo_file);
	}
}



void vim_init(void)
{
	shell_register_command("vim", shell_vim_edit);
}



