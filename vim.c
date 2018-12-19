/**
  ******************************************************************************
  * @file           vim.c
  * @author         ��ô��
  * @brief          �ı��༭��
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
	uint16_t  filerows;//�ļ����м���
	uint16_t  Rows;//�ļ����������λ��
	uint16_t  Cols;//�ļ����������λ��
	uint16_t  tail;//�ļ�β��λ��
	char *edit;    //��ǰ�༭λ��
	char *filename;//�ļ�����
	char  option;
	char  state;
	char  editbuf[VIM_MAX_EDIT]; //�ļ��༭�ڴ�
	char  printbuf[VIM_MAX_EDIT];//�ļ���ӡ�ڴ�
};


static const char clearline[] = "\33[2K\r\n";
static const char waiting_title[] = "'i'(Edit) or ':'(quit/save)";
static const char editing_title[] = "Editing...press ESC to quit";


/*
  * @author   ��ô��
  * @brief    vim_scan_file 
  *           ɨ�赱ǰ�ļ��༭
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

	//�����Ļ������ӡ�ı�����
	printk("\033[2J\033[%d;%dH",1,1);
	printk("\t%s\r\n%s",waiting_title,vim->editbuf);
	printk("\033[%d;%dH",2,1);
}


/**
	* @brief    vim_backspace
	*           �༭����ɾһ���ַ�
	* @param    vim  �༭�ڴ�
	* @return   void
*/
static void vim_backspace(struct vim_edit_buf * vim)
{
	char * print = vim->printbuf;
	char * tail ;//tail ����λ�����ַ�����������λ��
	char * edit;
	
	if (vim->editbuf == vim->edit)
		return ;
	
	if (*(vim->edit-1) != '\n') //�����ǰ�༭λ�ò����п�ͷ
	{
		char * copy = vim->edit;
		edit = vim->edit;
		tail = &vim->editbuf[vim->tail--];
		
		//�ҵ���ǰ�н���λ�ã������Ƶ���ӡ�ڴ���
		for (*print++ = '\b' ; *copy != '\r' && *copy && *copy != '\n' ; *print++ = *copy++);
		
		*print++ = ' '; //���ǵ�ǰ�����һ���ַ���ʾ
		*print++ = '\b'; //������
		
		//��������˸���
		for (uint32_t cnt = copy - vim->edit ; cnt-- ;*print++ = '\b' );
		
		//�� abUcd ��ɾ��U����Ҫ����cd������ӡ���� '\b' ʹ���ص� ab ��
		for (copy = edit - 1 ; edit < tail ; *copy++ = *edit++);
		
		vim->editbuf[vim->tail] = 0;//ĩ������ַ���������
		
		--vim->Cols;
		--vim->edit;
		printl(vim->printbuf,print - vim->printbuf);
	}/**/
	else //��ǰ�༭λ�����п�ͷ������һ��
	{
		char * prevlines ; //��ǰ�༭����һ�еĿ�ͷ
		char * prevlinee ; //��ǰ�༭����һ�еĽ�β
		edit = vim->edit ; //��ǰ�༭λ��
		
		if (vim->Rows > 2) //��ǰ�༭�в��ǵڶ���
			for (prevlines = edit-1 ; *(prevlines-1) != '\n' ; --prevlines);//�ص���һ�п�ͷ
		else
			prevlines = vim->editbuf; //�ص���һ�п�ͷ

		//�ҵ���һ�н�β������֪����һ�У�����Ҫ�ж� \0
		for (prevlinee = prevlines ; *prevlinee != '\r' && *prevlinee != '\n' ; ++prevlinee);

		//����̨��ձ༭���������Լ��������е�������ʾ
		for(uint32_t i = vim->Rows ; i <= vim->filerows ; ++i )
			printl((char*)clearline , sizeof(clearline)-1);
		
		tail = &vim->editbuf[vim->tail];     //ԭ�༭����β
		memcpy(prevlinee, edit ,tail - edit);//�ѵ�ǰ�༭�����ݿ�����һ�н�β
		
		vim->Rows -= 1;
		vim->filerows -= 1;
		vim->edit  = prevlinee ;             //���µ�ǰ�༭λ��Ϊ��һ�н�β
		vim->Cols  = prevlinee-prevlines+1;  //���µ�ǰ��λ��Ϊ��һ�н�β
		vim->tail -= (vim->edit - prevlinee);//���±༭β������Ӧ '\r\n'
		vim->editbuf[vim->tail] = 0; //ĩ������ַ���������
		
		printk("\033[%d;%dH",vim->Rows+1,vim->Cols);//���ص��༭��λ��
		printl(vim->edit,strlen(vim->edit));        //��ӡʣ������
		printk("\033[%d;%dH",vim->Rows+1,vim->Cols);//����ٴλص��༭��λ��
	}
}


/**
	* @brief    vim_insert_char
	*           �༭������һ���ַ�
	* @param    vim  �༭�ڴ�
	* @param    ascii �������ַ�
	* @return   void
*/
static void vim_insert_char(struct vim_edit_buf * vim,char ascii)
{
	char * print = vim->printbuf ;
	char *tail ;//tail ����λ�����ַ�����������λ��
	char *copy ;
	
	if (vim->tail + 1 >= VIM_MAX_EDIT)
		return ;

	if (ascii == '\t')
		return ;

	if (ascii == '\n' || ascii == '\r')//���뻻�е����
	{
		//��մ����Լ�������������
		for(uint32_t i = vim->Rows ; i <= vim->filerows ; ++i )
			printl((char*)clearline,sizeof(clearline)-1);
		
		printk("\033[%d;%dH",vim->Rows+1,1);//���ص���ǰ�༭�п�ͷ

		if (vim->Rows > 1)
			for (copy=vim->edit ; *(copy-1) != '\n' ; --copy);
		else
			copy = vim->editbuf;
		
		printl(copy,vim->edit-copy); //��ӡ����ǰ�༭λ��

		tail = &vim->editbuf[vim->tail] + 1;//�µ�β��

		// ��ǰ�༭λ�ú��������ֽڿռ�
		for (copy = tail-2 ; copy >= vim->edit ; *tail-- = *copy--) ;
		
		*vim->edit++ = '\r';
		*vim->edit++ = '\n';
		vim->tail += 2;
		vim->Rows += 1;
		vim->Cols  = 1;
		vim->filerows += 1;
		vim->editbuf[vim->tail] = 0; //ĩ������ַ���������
		
		tail = &vim->editbuf[vim->tail] - 1;
		copy = (vim->edit - 2);
		
		printl(copy,tail-copy); //��ӡ����ǰ�༭λ��
		printk("\033[%d;%dH",vim->Rows+1,1);
	}
	else
	{
		tail = &vim->editbuf[vim->tail++];//tail ����λ�����ַ�����������λ��
		copy = vim->edit;

		//������Ҫ��ӡ���ַ������ҵ���ǰ�н���λ��
		for (*print++ = ascii; *copy != '\r' && *copy != '\n' && *copy ; *print++ = *copy++);
		
		//���㵱ǰ����Ҫ���˵Ĺ����
		for (uint32_t cnt = copy - vim->edit ; cnt-- ; *print++ = '\b');
		
		//�� abcd ����bc����U����Ҫ����cd������ӡ���� '\b' ʹ���ص� abU ��
		for (copy = tail - 1; copy >= vim->edit ; *tail-- = *copy--);

		*vim->edit = ascii;  //�����ַ�
		++vim->edit;
		++vim->Cols;
		vim->editbuf[vim->tail] = 0;      //ĩ������ַ���������
		printl(vim->printbuf,print - vim->printbuf);
	}
	
}

	

/**
	* @brief    up_arrow_response
	*           �༭����Ӧ�ϼ�ͷ
	* @param    vim  �༭�ڴ�
	* @return   void
*/
static void up_arrow_response(struct vim_edit_buf * editing)
{				
	if (editing->Rows > 1)  //����༭����λ��Ϊ��һ�У��򲻽�����Ӧ
	{
		char   * ptr = editing->edit; //��ǰ�༭λ��
		uint32_t cnt;
		
		if ((--editing->Rows) > 1)
		{
			while(*ptr-- != '\n'); //�ص���һ�н�β��
			if (*ptr != '\n')
			{
				while(*--ptr != '\n');//��ǰ�еĿ�ͷ
				++ptr;
			}
		}
		else //��������ǵڶ��У���ص���һ������λ��
		{
			ptr = editing->editbuf;
		}

		for (cnt = 1;cnt < editing->Cols ;++cnt)//���㵱ǰ�е���λ��
		{
			if (*ptr == '\r' || *ptr == '\n')   //��ǰ�б���һ�ж̣���û�а취�ص���һ�����ڵ���λ��
				break;
			else
				++ptr;
		}
		
		editing->Cols = cnt;
		editing->edit = ptr; //���µ�ǰ�༭λ��
		printk("\033[%d;%dH",editing->Rows + 1,editing->Cols); //��ӡ���λ��
	}
}



/**
	* @brief    down_arrow_response
	*           �༭����Ӧ�¼�ͷ
	* @param    vim  �༭�ڴ�
	* @return   void
*/
static void down_arrow_response(struct vim_edit_buf * editing)
{
	if (editing->Rows < editing->filerows) //�����ǰ�в����ı����һ�У���Ӧ�¼�ͷ
	{	
		char   * ptr = editing->edit;
		uint32_t cnt;
		
		while( *ptr++ != '\n'  );//��һ�п�ͷ��
		
		for (cnt = 1; cnt < editing->Cols ;++cnt)
		{
			if ( *ptr == '\r' || *ptr == '\0' || *ptr == '\n')//����һ�ж�ʱ���޷��ƶ������ԭ������λ��
				break;
			else
				++ptr;
		}

		printl(editing->edit,ptr - editing->edit);
		++editing->Rows;    //��������
		editing->Cols = cnt;//��������
		editing->edit = ptr;//���µ�ǰ�༭λ��
	}
}


/**
	* @brief    rignt_arrow_response
	*           �༭����Ӧ�Ҽ�ͷ
	* @param    vim  �༭�ڴ�
	* @return   void
*/
static void rignt_arrow_response(struct vim_edit_buf * editing)
{
	char * edit = editing->edit;
	if (*edit == '\r' || *edit == '\0' || *edit == '\n')//��ĩβ
		return ;
	
	++editing->Cols;
	++editing->edit;
	printl(edit,1);
}


/**
	* @brief    left_arrow_response
	*           �༭����Ӧ���ͷ
	* @param    vim  �༭�ڴ�
	* @return   void
*/
static void left_arrow_response(struct vim_edit_buf * editing)
{
	if (editing->edit == editing->editbuf || editing->Cols < 2)//�п�ͷ
		return;
	
	--editing->edit;
	--editing->Cols;
	printl("\b",1);
}


/**
	* @brief    arrow_response
	*           �༭����Ӧ��ͷ
	* @param    vim  �༭�ڴ�
	* @return   void
*/
static void arrow_response(struct vim_edit_buf * editing , char arrow)
{
	switch( arrow )
	{
		case 'A': //�ϼ�ͷ
			up_arrow_response(editing);
			break;

		case 'B'://�¼�ͷ
			down_arrow_response(editing);
			break;

		case 'C'://�Ҽ�ͷ
			rignt_arrow_response(editing);
			break;

		case 'D'://���ͷ
			left_arrow_response(editing);
			break;
	}
}



/**
	* @brief    vim_edit
	*           ������������
	* @param    vim  �༭�ڴ�
	* @return   void
*/
static void vim_gets(struct shell_input * shell ,char * buf , uint32_t len)
{
	struct vim_edit_buf * this_edit = (struct vim_edit_buf *)shell->apparg ;

	switch(this_edit->state)//״̬��
	{
		case VIM_READ_ONLY:
			if (*buf == 'i')//�������� 'i'
			{
				this_edit->state = VIM_EDITING;//����༭ģʽ
				printk("\033[%d;%dH\033[2K\r",1,1);//���ص���һ�в���յ�һ��
				printk("\t%s",editing_title);      //��ӡ��ʾ��Ϣ
				printk("\033[%d;%dH",this_edit->Rows + 1,this_edit->Cols);//�ָ����λ��
			}
			else
			if (*buf == ':')
			{
				this_edit->option = 0;
				this_edit->state = VIM_COMMAND;
				printk("\033[%d;%dH\033[2K\r(w/q):",1,1);//���ص���һ�в���յ�һ��
			}
			else
			if ( buf[0] == '\033' && len > 1 && buf[1] == '[' )
			{
				arrow_response(this_edit,buf[2]);
			}
			break;
			
		case VIM_EDITING: //�༭����
			if (*buf == '\033')
			{
				if ( len > 1 && buf[1] == '[' )
					arrow_response(this_edit,buf[2]);//����Ǽ�ͷ����
				else
				{
					this_edit->state = VIM_READ_ONLY; //����ǵ����� Esc ���ص�ֻ��ģʽ
					printk("\033[%d;%dH\033[2K\r",1,1);//���ص���һ�в���յ�һ��
					printk("\t%s",waiting_title);      //��ӡ��ʾ��Ϣ
					printk("\033[%d;%dH",this_edit->Rows + 1,this_edit->Cols);//�ָ����λ��
				}
			}
			else
			if (*buf == KEYCODE_BACKSPACE) 
				vim_backspace(this_edit);       //backspace
			else
				vim_insert_char(this_edit,*buf);//�����ַ�
			break;
			
		case VIM_COMMAND:
			if (*buf == KEYCODE_BACKSPACE) //���˼�
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
				this_edit->state = VIM_READ_ONLY;             //����ֻ��ģʽ
				printk("\033[2K\r\t%s\r\n",waiting_title);//��ӡ��ʾ��Ϣ

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
	
	argc = shell_option_suport((char*)arg,argv);//��ȡ·����Ϣ

	if (2 == argc) //���������·��
	{
		//���Դ��ļ�
		if ( 0 != lfs_file_open(&g_spi_lfs, &lfs_demo_file ,argv[1], LFS_O_RDONLY))
		{
			printk("cannot open file:%s\r\n",argv[1]);
			return ;
		}

		//�ж��ļ���С��̫����ļ����ܱ༭
		if (VIM_MAX_EDIT < lfs_demo_file.size)
		{
			printk("\tnot enough ram to edit this file.\r\n");
			return ;
		}

		this_shell = container_of(arg, struct shell_input, buf); //��ȡ��ǰ shell_input
		this_shell->apparg = MALLOC(sizeof(struct vim_edit_buf));//����༭�ڴ�

		if (!this_shell->apparg) //�������ʧ��
			return ;
		
		this_shell->gets = vim_gets; //�ض�������������

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



