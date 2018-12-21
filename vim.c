/**
  ******************************************************************************
  * @file           vim.c
  * @author         ��ô��
  * @brief          �ı��༭�������� shell.h/shell.c
  ******************************************************************************
  *
  * COPYRIGHT(c) 2018 GoodMorning
  *
  ******************************************************************************
  */
/* Includes ---------------------------------------------------*/
#include <string.h>
#include <stdint.h> //�����˺ܶ���������
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
	vim_fputs fputs; //�ļ����
	char *    fpath; //����������ı༭�ļ���
	char *    edit;  //��ǰ�༭λ��
	uint16_t  rowmax;//�ļ����м���
	uint16_t  rows;  //�ļ����������λ��
	uint16_t  cols;  //�ļ����������λ��
	uint16_t  tail;  //�ļ�β��λ��
	char  editbuf[VIM_MAX_EDIT]; //�ļ��༭�ڴ�
	char  printbuf[VIM_MAX_EDIT];//�ļ���ӡ��ת�ڴ�
	char  option;
	char  state;
};

/* Private macro ------------------------------------------------------------*/

//#define VIM_USE_HEAP  //���غ꣬�Ƿ�ʹ�ö��ڴ棬���ε���ʹ�þ�̬�ڴ�

#ifdef  VIM_USE_HEAP  //���ʹ�ö��ڴ棬��Ҫ�ṩ VIM_MALLOC �� VIM_FREE 

	#include "heaplib.h"
	#define VIM_MALLOC(bytes) MALLOC(bytes)
	#define VIM_FREE(buf)     FREE(buf)
	
#else //ʹ�þ�̬���������

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
	*           �༭����Ӧ�ϼ�ͷ
	* @param    vim  �༭�ڴ�
	* @return   void
*/
static void cursor_up(struct vim_edit_buf * vimedit)
{				
	if (vimedit->rows > 1)  //����༭����λ��Ϊ��һ�У��򲻽�����Ӧ
	{
		char   * ptr = vimedit->edit; //��ǰ�༭λ��
		uint32_t cnt;
		
		if ((--vimedit->rows) > 1)
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
			ptr = vimedit->editbuf;
		}

		for (cnt = 1;cnt < vimedit->cols ;++cnt)//���㵱ǰ�е���λ��
		{
			if (*ptr == '\r' || *ptr == '\n')   //��ǰ�б���һ�ж̣���û�а취�ص���һ�����ڵ���λ��
				break;
			else
				++ptr;
		}
		
		vimedit->cols = cnt;
		vimedit->edit = ptr; //���µ�ǰ�༭λ��
		printk("\033[%d;%dH",vimedit->rows + 1,vimedit->cols); //��ӡ���λ��
	}
}


/**
	* @brief    cursor_down
	*           �༭����Ӧ�¼�ͷ
	* @param    vim  �༭�ڴ�
	* @return   void
*/
static void cursor_down(struct vim_edit_buf * vimedit)
{
	if (vimedit->rows < vimedit->rowmax) //�����ǰ�в����ı����һ�У���Ӧ�¼�ͷ
	{	
		char   * ptr = vimedit->edit;
		uint32_t cnt;
		
		while( *ptr++ != '\n'  );//��һ�п�ͷ��
		
		for (cnt = 1; cnt < vimedit->cols ;++cnt)
		{
			if ( *ptr == '\r' || *ptr == '\0' || *ptr == '\n')//����һ�ж�ʱ���޷��ƶ������ԭ������λ��
				break;
			else
				++ptr;
		}

		printl(vimedit->edit,ptr - vimedit->edit);
		++vimedit->rows;    //��������
		vimedit->cols = cnt;//��������
		vimedit->edit = ptr;//���µ�ǰ�༭λ��
	}
}


/**
	* @brief    cursor_right
	*           �༭����Ӧ�Ҽ�ͷ
	* @param    vim  �༭�ڴ�
	* @return   void
*/
static void cursor_right(struct vim_edit_buf * vimedit)
{
	char * edit = vimedit->edit;
	if (*edit == '\r' || *edit == '\0' || *edit == '\n')//��ĩβ
		return ;
	
	++vimedit->cols;
	++vimedit->edit;
	printl(edit,1);
}


/**
	* @brief    cursor_left
	*           �༭����Ӧ���ͷ
	* @param    vim  �༭�ڴ�
	* @return   void
*/
static void cursor_left(struct vim_edit_buf * vimedit)
{
	if (vimedit->edit == vimedit->editbuf || vimedit->cols < 2)//�п�ͷ
		return;
	
	--vimedit->edit;
	--vimedit->cols;
	printl("\b",1);
}


/**
	* @brief    cursor_move
	*           �༭����Ӧ��ͷ
	* @param    vim  �༭�ڴ�
	* @return   void
*/
static void cursor_move(struct vim_edit_buf * vimedit , char arrow)
{
	switch( arrow )
	{
		case 'A': //�ϼ�ͷ
			cursor_up(vimedit);
			break;

		case 'B'://�¼�ͷ
			cursor_down(vimedit);
			break;

		case 'C'://�Ҽ�ͷ
			cursor_right(vimedit);
			break;

		case 'D'://���ͷ
			cursor_left(vimedit);
			break;
		
		default : ;
	}
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
	char * edit = vim->edit;     //��ǰ�༭λ��
	char * tail = &vim->editbuf[vim->tail];//ԭ�༭����β; 
	
	if (*(vim->edit-1) != '\n') //�����ǰ�༭λ�ò����п�ͷ
	{
		char * copy = vim->edit;
		
		//�ҵ���ǰ�н���λ�ã������Ƶ���ӡ�ڴ���
		for (*print++ = '\b' ; *copy != '\r' && *copy && *copy != '\n' ; *print++ = *copy++);
		
		*print++ = ' '; //���ǵ�ǰ�����һ���ַ���ʾ
		*print++ = '\b'; //������
		
		//��������˸���
		for (uint32_t cnt = copy - vim->edit ; cnt-- ;*print++ = '\b' );
		
		//�� abUcd ��ɾ��U����Ҫ����cd������ӡ���� '\b' ʹ���ص� ab ��
		for (copy = edit - 1 ; edit < tail ; *copy++ = *edit++);
		
		--vim->cols;
		--vim->edit;
		--vim->tail;
		vim->editbuf[vim->tail] = 0;//ĩ������ַ���������
		printl(vim->printbuf,print - vim->printbuf);
	}/**/
	else //��ǰ�༭λ�����п�ͷ������һ��
	{
		char * prevlines ; //��ǰ�༭����һ�еĿ�ͷ
		char * prevlinee ; //��ǰ�༭����һ�еĽ�β
		
		if (vim->rows > 2) //��ǰ�༭�в��ǵڶ���
			for (prevlines = edit-1 ; *(prevlines-1) != '\n' ; --prevlines);//�ص���һ�п�ͷ
		else
			prevlines = vim->editbuf; //�ص���һ�п�ͷ

		//�ҵ���һ�н�β������֪����һ�У�����Ҫ�ж� \0
		for (prevlinee = prevlines ; *prevlinee != '\r' && *prevlinee != '\n' ; ++prevlinee);

		//����̨��ձ༭���������Լ��������е�������ʾ
		for(uint32_t i = vim->rows ; i <= vim->rowmax ; ++i )
			printl((char*)clearline , sizeof(clearline)-1);
		
		memcpy(prevlinee, edit ,tail - edit);//�ѵ�ǰ�༭�����ݿ�����һ�н�β
		
		vim->rows -= 1;
		vim->rowmax -= 1;
		vim->edit  = prevlinee ;             //���µ�ǰ�༭λ��Ϊ��һ�н�β
		vim->cols  = prevlinee-prevlines+1;  //���µ�ǰ��λ��Ϊ��һ�н�β
		vim->tail -= (vim->edit - prevlinee);//���±༭β������Ӧ '\r\n'
		vim->editbuf[vim->tail] = 0; //ĩ������ַ���������
		
		printk("\033[%d;%dH",vim->rows+1,vim->cols);//���ص��༭��λ��
		printl(vim->edit,strlen(vim->edit));        //��ӡʣ������
		printk("\033[%d;%dH",vim->rows+1,vim->cols);//����ٴλص��༭��λ��
	}
}


/**
	* @brief    vim_insert_char
	*           �༭������һ����ͨ�ַ�
	* @param    vim  �༭�ڴ�
	* @param    ascii �������ַ�
	* @return   void
*/
static void vim_insert_char(struct vim_edit_buf * vim,char ascii)
{
	char * print = vim->printbuf ;
	char * tail = &vim->editbuf[vim->tail];//tail ����λ�����ַ�����������λ��
	char * copy = vim->edit;

	//������Ҫ��ӡ���ַ������ҵ���ǰ�н���λ��
	for (*print++ = ascii; *copy != '\r' && *copy != '\n' && *copy ; *print++ = *copy++);
	
	//���㵱ǰ����Ҫ���˵Ĺ����
	for (uint32_t cnt = copy - vim->edit ; cnt-- ; *print++ = '\b');
	
	//�� abcd ����bc����U����Ҫ����cd������ӡ���� '\b' ʹ���ص� abU ��
	for (copy = tail - 1; copy >= vim->edit ; *tail-- = *copy--);
	
	*vim->edit = ascii;  //�����ַ�
	++vim->edit;
	++vim->cols;
	++vim->tail;
	vim->editbuf[vim->tail] = 0; //ĩ������ַ���������
	printl(vim->printbuf,print - vim->printbuf);
}



/**
	* @brief    vim_insert_newline
	*           �༭�����뻻�з�/�س���
	* @param    vim  �༭�ڴ�
	* @param    ascii �������ַ�
	* @return   void
*/
static void vim_insert_newline(struct vim_edit_buf * vim)
{
	char * tail = &vim->editbuf[vim->tail];//tail ����λ�����ַ�����������λ��
	char * copy ;
	
	//��մ����Լ�������������
	for(uint32_t i = vim->rows ; i <= vim->rowmax ; ++i )
		printl((char*)clearline,sizeof(clearline)-1);
	
	printk("\033[%d;%dH",vim->rows+1,1);//���ص���ǰ�༭�п�ͷ

	if (vim->rows > 1)
		for (copy=vim->edit ; *(copy-1) != '\n' ; --copy);
	else
		copy = vim->editbuf;
	
	printl(copy,vim->edit-copy); //��ӡ����ǰ�༭λ��

	tail += 1;//�µ�β��

	// ��ǰ�༭λ�ú��������ֽڿռ�
	for (copy = tail-2 ; copy >= vim->edit ; *tail-- = *copy--) ;
	
	*vim->edit++ = '\r';
	*vim->edit++ = '\n';
	vim->tail += 2;
	vim->rows += 1;
	vim->cols  = 1;
	vim->rowmax += 1;
	vim->editbuf[vim->tail] = 0; //ĩ������ַ���������
	
	tail = &vim->editbuf[vim->tail] - 1;
	copy = (vim->edit - 2);
	
	printl(copy,tail-copy); //��ӡ����ǰ�༭λ��
	printk("\033[%d;%dH",vim->rows+1,1);
}



/**
	* @brief    vim_insert_newline
	*           �༭���̽����ַ�
	* @param    vim  �༭�ڴ�
	* @param    data �������ַ�
	* @return   void
*/
static void vim_edit_getchar(struct vim_edit_buf * vim,char data)
{
	if (KEYCODE_CTRL_C == data || KEYCODE_TAB == data ||
		KEYCODE_END == data || KEYCODE_HOME == data )//һЩ�ر���ַ�����
	{
		return ;
	}

	if (KEYCODE_BACKSPACE == data)
	{
		if (vim->editbuf != vim->edit)
			vim_backspace(vim);
	}
	else
	if (vim->tail < VIM_MAX_EDIT/2) //�����ַ�
	{
		if ('\r' == data || '\n' == data ) //���뻻�з�
			vim_insert_newline(vim);
		else
			vim_insert_char(vim , data);   //������ͨ�ַ�
	}
}

/**
	* @brief    vim_edit
	*           ������������
	* @param    vim  �༭�ڴ�
	* @return   void
*/
static void shell_vim_gets(struct shell_input * shell ,char * buf , uint32_t len)
{
	struct vim_edit_buf * vimedit = (struct vim_edit_buf *)shell->apparg ;

	switch(vimedit->state)//״̬��
	{
		case VIM_READ_ONLY: //�ı�ֻ������
			if (*buf == 'i')//�������� 'i'
			{
				vimedit->state = VIM_EDITING;//����༭ģʽ
				printk("\033[%d;%dH\033[2K\r",1,1);//���ص���һ�в���յ�һ��
				printk("\t%s",editing_title);      //��ӡ��ʾ��Ϣ
				printk("\033[%d;%dH",vimedit->rows + 1,vimedit->cols);//�ָ����λ��
			}
			else
			if (*buf == ':')
			{
				vimedit->option = 0;
				vimedit->state = VIM_COMMAND;
				printk("\033[%d;%dH\033[2K\r(w/q):",1,1);//���ص���һ�в���յ�һ��
			}
			else
			if ( buf[0] == '\033' && len > 1 && buf[1] == '[' )
			{
				cursor_move(vimedit,buf[2]);
			}
			break;
			
		case VIM_EDITING: //�ı��༭����
			if (*buf == '\033')
			{
				if ( len > 1 && buf[1] == '[' )
				{
					cursor_move(vimedit,buf[2]);//����Ǽ�ͷ����
				}
				else
				{
					vimedit->state = VIM_READ_ONLY; //����ǵ����� Esc ���ص�ֻ��ģʽ
					printk("\033[%d;%dH\033[2K\r",1,1);//���ص���һ�в���յ�һ��
					printk("\t%s",waiting_title);      //��ӡ��ʾ��Ϣ
					printk("\033[%d;%dH",vimedit->rows + 1,vimedit->cols);//�ָ����λ��
				}
			}
			else
			{
				vim_edit_getchar(vimedit ,* buf);
			}
			break;
			
		case VIM_COMMAND: //�ȴ�����
			if (*buf == 'q' ||*buf == 'w')
			{
				vimedit->state  = VIM_QIUT;
				vimedit->option = *buf;
				printl(buf,1);
			}
			else
			if (*buf == '\033' )//&& len == 1)
			{
				vimedit->state = VIM_READ_ONLY;  //����ֻ��ģʽ
				vimedit->edit = vimedit->editbuf;
				vimedit->cols = 1;
				vimedit->rows = 1;
				printk("\033[2K\r\t%s\r\n",waiting_title);//��ӡ��ʾ��Ϣ
			}
			break;
			
		case VIM_QIUT:   //�ȴ��˳�
			if (*buf == KEYCODE_BACKSPACE) //���˼�
			{
				vimedit->state  = VIM_COMMAND;
				printk("\b \b");
			}
			else
			if (*buf == '\r' || *buf == '\n') //�س�ȷ��
			{
				if ('w' == vimedit->option)//���뱣�棬��������ļ�
					vimedit->fputs(vimedit->fpath,vimedit->editbuf,vimedit->tail);
				VIM_FREE(vimedit);
				shell->apparg = NULL;
				shell->gets   = cmdline_gets;
				printk("\033[2J\033[%d;%dH%s",1,1,shell_input_sign);
			}
			else
			if (*buf == '\033' )//&& len == 1)
			{
				vimedit->state = VIM_READ_ONLY;  //����ֻ��ģʽ
				vimedit->edit = vimedit->editbuf;
				vimedit->cols = 1;
				vimedit->rows = 1;
				printk("\033[2K\r\t%s\r\n",waiting_title);//��ӡ��ʾ��Ϣ
			}
			break;

		default:;
	}
}



/**
	* @brief    shell_into_edit
	*           ��ǰ��������ı��༭ģʽ
	* @param    shell  ��Ҫ�����ı��༭�� shell ����
	* @param    fgets  ��ȡ�༭�ı�����Դ�Ľӿ�
	* @param    fputs  �����ı�����Դ�Ľӿ�
	* @return   void
*/
void shell_into_edit(struct shell_input * shell,vim_fgets fgets ,vim_fputs fputs)
{
	char * argv[4] = {NULL};
	struct vim_edit_buf * edit;
	
	shell->apparg = VIM_MALLOC(sizeof(struct vim_edit_buf));
	if ( NULL ==  shell->apparg )
	{
		printk("not enough memery\r\n");
		return ;
	}

	shell_option_suport(shell->buf,argv);//��ȡ·����Ϣ

	edit = (struct vim_edit_buf *)(shell->apparg);
	edit->fpath = argv[1];               //·������Ϊ�ڶ�������

	//���Դ򿪣����ʧ���򷵻�
	if (VIM_FILE_OK !=  fgets(edit->fpath,&edit->editbuf[0],&edit->tail))
	{
		VIM_FREE(shell->apparg);
		return ;
	}
	
	shell->gets = shell_vim_gets; //�ض�������������,�༭�ļ�ģʽ

	edit->fputs = fputs;
	edit->editbuf[edit->tail] = 0;
	edit->edit   = edit->editbuf;
	edit->state  = VIM_READ_ONLY;
	edit->option = 0;
	edit->cols   = 1;
	edit->rows   = 1;
	edit->rowmax = 1;

	//ɨ���ļ����м���
	for (char * editbuf = edit->editbuf ; *editbuf ; ++editbuf)
	{
		if (*editbuf == '\n')
			++edit->rowmax;
	}

	//�����Ļ������ӡ�ı�����
	printk("\033[2J\033[%d;%dH",1,1);
	printk("\t%s\r\n%s",waiting_title,edit->editbuf);
	printk("\033[%d;%dH",2,1);
}


