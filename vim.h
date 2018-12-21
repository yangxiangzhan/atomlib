/**
  ******************************************************************************
  * @file           vim.h
  * @author         古么宁
  * @brief          文本编辑器
  ******************************************************************************
  *
  * COPYRIGHT(c) 2018 GoodMorning
  *
  ******************************************************************************
  */

#ifndef _VIM_EDIT_
#define _VIM_EDIT_


/* Public macro (共有宏)---------------------------------------------------*/

//两个状态量
#define VIM_FILE_OK    0
#define VIM_FILE_ERROR 1

#define VIM_MAX_EDIT 1024


/* Public types ------------------------------------------------------------*/



/**
	* @brief    vim_fgets
	*           编辑器从 fpath 获取文本信息接口
	* @param    fpath : 文件路径，命令行所输入的路径
	* @param    fdata : 从 fpath 文件读取的数据输出
	* @param    fsize : fpath 文件总大小输出 
	* @return   VIM_FILE_OK / VIM_FILE_ERROR
*/
typedef uint32_t (*vim_fgets)(char * fpath, char * fdata,uint16_t * fsize);

/**
	* @brief    vim_fputs
	*           编辑器对 fpath 文件进行数据输出
	* @param    fpath : 文件路径，命令行所输入的路径
	* @param    fdata : 将写入 fpath 文件的数据
	* @param    fsize : 将写入 fpath 文件的数据大小
	* @return   void
*/
typedef void	 (*vim_fputs)(char * fpath, char * fdata,uint32_t fsize);



/* Public function prototypes 对外可用接口 -----------------------------------*/


void shell_into_edit(struct shell_input  * shell,vim_fgets fgets ,vim_fputs fputs);


#endif

