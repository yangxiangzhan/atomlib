/**
  ******************************************************************************
  * @file           flash_fs.c
  * @author         ��ô��
  * @brief          �� stm32 Ƭ�� flash �Ϲ��� littlefs �ļ�ϵͳ
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








