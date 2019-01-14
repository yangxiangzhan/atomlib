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
typedef uint32_t (*vim_fgets_t)(char * fpath, char * fdata,uint16_t * fsize);

/**
	* @brief    vim_fputs
	*           编辑器对 fpath 文件进行数据输出
	* @param    fpath : 文件路径，命令行所输入的路径
	* @param    fdata : 将写入 fpath 文件的数据
	* @param    fsize : 将写入 fpath 文件的数据大小
	* @return   void
*/
typedef void	 (*vim_fputs_t)(char * fpath, char * fdata,uint32_t fsize);



/* Public function prototypes 对外可用接口 -----------------------------------*/

/**
	* @brief    shell_into_edit
	*           shell 交互进入文本编辑模式
	*           已知问题：当控制台的列小于文本的列，会有问题，即某行有100字符，但控制台一行只能显示80
	* @param    shell : 交互
	* @param    fgets : 获取文本数据的入口
	* @param    fputs : 文本编辑结束的输出
	* @return   void
*/
void shell_into_edit(struct shell_input  * shell,vim_fgets_t fgets ,vim_fputs_t fputs);


#endif

