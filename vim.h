/**
  ******************************************************************************
  * @file           vim.h
  * @author         ��ô��
  * @brief          �ı��༭��
  ******************************************************************************
  *
  * COPYRIGHT(c) 2018 GoodMorning
  *
  ******************************************************************************
  */

#ifndef _VIM_EDIT_
#define _VIM_EDIT_


/* Public macro (���к�)---------------------------------------------------*/

//����״̬��
#define VIM_FILE_OK    0
#define VIM_FILE_ERROR 1

#define VIM_MAX_EDIT 1024


/* Public types ------------------------------------------------------------*/



/**
	* @brief    vim_fgets
	*           �༭���� fpath ��ȡ�ı���Ϣ�ӿ�
	* @param    fpath : �ļ�·�����������������·��
	* @param    fdata : �� fpath �ļ���ȡ���������
	* @param    fsize : fpath �ļ��ܴ�С��� 
	* @return   VIM_FILE_OK / VIM_FILE_ERROR
*/
typedef uint32_t (*vim_fgets)(char * fpath, char * fdata,uint16_t * fsize);

/**
	* @brief    vim_fputs
	*           �༭���� fpath �ļ������������
	* @param    fpath : �ļ�·�����������������·��
	* @param    fdata : ��д�� fpath �ļ�������
	* @param    fsize : ��д�� fpath �ļ������ݴ�С
	* @return   void
*/
typedef void	 (*vim_fputs)(char * fpath, char * fdata,uint32_t fsize);



/* Public function prototypes ������ýӿ� -----------------------------------*/


void shell_into_edit(struct shell_input  * shell,vim_fgets fgets ,vim_fputs fputs);


#endif

