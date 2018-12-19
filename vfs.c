/**
  ******************************************************************************
  * @file           flash_fs.c
  * @author         古么宁
  * @brief          在 stm32 片上 flash 上挂载 littlefs 文件系统
  ******************************************************************************
  *
  * COPYRIGHT(c) 2018 GoodMorning
  *
  ******************************************************************************
  */
/* Includes ---------------------------------------------------*/
#include <stdio.h>
#include <stdint.h>
#include "stm32f4xx.h"
#include "stm32f4xx_hal.h"


#include "shell.h"
#include "flash_fs.h"
#include "lfs.h"


int vfs_file_open(const char *path, int flags)
{
	
}



int vfs_file_read(lfs_t *lfs, lfs_file_t *file,
        void *buffer, lfs_size_t size)








