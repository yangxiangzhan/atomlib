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
#include <string.h>
#include "stm32f4xx.h"
#include "stm32f4xx_hal.h"


#include "lfs.h"
#include "shell.h"
#include "flash_fs.h"
#include "heaplib.h"

#define FLASH_ADDR_BASE  0x08080000
#define FLASH_ADDR_END   0x08200000

#define FLASH_BLOCK_SIZE 0x20000

#define FLASH_SPECIAL_ADDR  0x08100000
#define FLASH_SPECIAL_BLOCK (FLASH_SPECIAL_ADDR - FLASH_ADDR_BASE)/(FLASH_BLOCK_SIZE)


/*
 * 使用动态内存时打开此宏定义。
 * 需要修改 "lfs_util.h" 里的动态内存操作函数
*/ 
#define LFS_MALLOC 


/* Private variables ------------------------------------------------------------*/


#ifndef LFS_MALLOC
static uint8_t	lfs_read_buf[256] = {0};
static uint8_t	lfs_prog_buf[256] = {0};
static uint8_t	lfs_lookahead_buf[256] = {0};
static uint8_t	lfs_file_buf[256] = {0};
#endif


int flash_read(const struct lfs_config *c, lfs_block_t block,
        lfs_off_t off, void *buffer, lfs_size_t size);

int flash_prog(const struct lfs_config *c, lfs_block_t block,
            lfs_off_t off, const void *buffer, lfs_size_t size);

int flash_erase(const struct lfs_config *c, lfs_block_t block);

static int block_sync(const struct lfs_config *c);


static const struct lfs_config flash_lfs_cfg =
{
	.read  = flash_read,
	.prog  = flash_prog,
	.erase = flash_erase,
	.sync  = block_sync,

	.block_size  = FLASH_BLOCK_SIZE,
	.block_count = (FLASH_ADDR_END - FLASH_ADDR_BASE)/FLASH_BLOCK_SIZE,

#ifndef LFS_MALLOC
	.prog_size   = sizeof(lfs_prog_buf),
	.read_size   = sizeof(lfs_read_buf),
	.lookahead   = sizeof(lfs_lookahead_buf),

	.file_buffer      = lfs_file_buf,
	.lookahead_buffer = lfs_lookahead_buf,
	.prog_buffer      = lfs_prog_buf,
	.read_buffer      = lfs_read_buf,
#else
	.prog_size   = 1024 * 2,
	.read_size   = 1024 * 2,
	.lookahead   = 2048,

	.file_buffer      = NULL,
	.lookahead_buffer = NULL,
	.prog_buffer      = NULL,
	.read_buffer      = NULL,
#endif
};


/* Public variables ------------------------------------------------------------*/

lfs_t g_flash_lfs;


/* Private function prototypes -----------------------------------------------*/



/* Gorgeous Split-line -----------------------------------------------*/


int flash_read(const struct lfs_config *c, lfs_block_t block,
        lfs_off_t off, void *buffer, lfs_size_t size)
{
	uint32_t addroffset = block * c->block_size + off + FLASH_ADDR_BASE;
	char * addr = (char*)addroffset;
	char * buf = (char*)buffer;
	
	for (uint32_t i = 0; i < size ; ++i)
		*buf++ = *addr++;
	
	return 0;
}

int flash_prog(const struct lfs_config *c, lfs_block_t block,
            lfs_off_t off, const void *buffer, lfs_size_t size)
{
	uint32_t addr = block * c->block_size + off + FLASH_ADDR_BASE;
	char * buf = (char*)buffer;
	
	HAL_FLASH_Unlock();
	
	for (uint32_t i = 0; i < size ; ++i)
		HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE,addr++,*buf++);
		
	HAL_FLASH_Lock();
	
	return 0;
}


int flash_erase(const struct lfs_config *c, lfs_block_t block)
{
	uint32_t SectorError;
    FLASH_EraseInitTypeDef FlashEraseInit;

	FlashEraseInit.TypeErase    = FLASH_TYPEERASE_SECTORS;       //擦除类型，扇区擦除 
	FlashEraseInit.VoltageRange = FLASH_VOLTAGE_RANGE_3;      //电压范围，VCC=2.7~3.6V之间!!

	if (block == FLASH_SPECIAL_BLOCK)
	{
		FlashEraseInit.Sector       = 12; //扇区
		FlashEraseInit.NbSectors    = 5;  //擦除 0x20000
	}
	else
	if (block > FLASH_SPECIAL_BLOCK)
	{
		FlashEraseInit.Sector = 16 + block - FLASH_SPECIAL_BLOCK; //扇区
		FlashEraseInit.NbSectors = 1;  //一次只擦除一个扇区
	}
	else
	{
		FlashEraseInit.Sector = 12 - (FLASH_SPECIAL_BLOCK - block); //扇区
		FlashEraseInit.NbSectors = 1;  //一次只擦除一个扇区
	}

	HAL_FLASH_Unlock();

	HAL_FLASHEx_Erase(&FlashEraseInit,&SectorError);
	
	HAL_FLASH_Lock();

	return 0;
}

static int block_sync(const struct lfs_config *c)
{
	return 0;
}






void shell_test_flashfs(void*arg)
{
	lfs_file_t lfs_demo_file;
	
	/*// mount the filesystem
	int err = lfs_mount(&g_flash_lfs, &spi_lfs_cfg);

	// reformat if we can't mount the filesystem
	// this should only happen on the first boot
	if (err) {
		lfs_format(&g_flash_lfs, &spi_lfs_cfg);
		lfs_mount(&g_flash_lfs, &spi_lfs_cfg);
	}
	*/

	// read current count
	uint32_t boot_count = 0;
	lfs_file_open(&g_flash_lfs, &lfs_demo_file, "boot_count", LFS_O_RDWR | LFS_O_CREAT);
	lfs_file_read(&g_flash_lfs, &lfs_demo_file, &boot_count, sizeof(boot_count));

	// update boot count
	boot_count += 1;
	lfs_file_rewind(&g_flash_lfs, &lfs_demo_file);
	lfs_file_write(&g_flash_lfs, &lfs_demo_file, &boot_count, sizeof(boot_count));

	// remember the storage is not updated until the file is closed successfully
	lfs_file_close(&g_flash_lfs, &lfs_demo_file);

	// release any resources we were using
	//lfs_unmount(&g_flash_lfs);

	// print the boot count
	printk("boot_count: %d\n", boot_count);
}



void flash_touch_file(void * arg)
{

	lfs_file_t lfs_demo_file;
	static const char ip_cfg[] = "\r\nthis is a demo file\r\n";

	int argc ;
	char * argv[4];

	argc = shell_option_suport((char*)arg,argv);

	if (argc == 2)
	{
		lfs_file_open(&g_flash_lfs, &lfs_demo_file ,argv[1], LFS_O_RDWR | LFS_O_CREAT);
		lfs_file_write(&g_flash_lfs, &lfs_demo_file, argv[1],strlen(argv[1]));
		lfs_file_write(&g_flash_lfs, &lfs_demo_file, ip_cfg, sizeof(ip_cfg)-1);
		lfs_file_close(&g_flash_lfs, &lfs_demo_file);
		printk("write file done\r\n");
	}
	else
		printk("need file name\r\n");
}


void flash_read_file(void * arg)
{
	char * buf;

	lfs_file_t lfs_demo_file;
	int argc ;
	char * argv[4];

	argc = shell_option_suport((char*)arg,argv);

	if (2 == argc)
	{
		if ( 0 != lfs_file_open(&g_flash_lfs, &lfs_demo_file ,argv[1], LFS_O_RDONLY))
		{
			printk("cannot open file\r\n");
			return ;
		}

		buf = pvPortMalloc(lfs_demo_file.size);
		
		lfs_file_read(&g_flash_lfs, &lfs_demo_file,buf,lfs_demo_file.size);

		printk("file size : %d\r\ndata:\r\n%s",lfs_demo_file.size,buf);

		lfs_file_close(&g_flash_lfs, &lfs_demo_file);

		vPortFree(buf);
	}
}


void flash_list_file(void*arg)
{
	lfs_dir_t        root_dir;
	struct lfs_info  dir_info;

	lfs_dir_open(&g_flash_lfs, &root_dir, "/");

	while (lfs_dir_read(&g_flash_lfs,&root_dir,&dir_info))
	{
		if (LFS_TYPE_REG == dir_info.type)
			printk("\t%s\r\n",dir_info.name);
		else
			color_printk(purple,"\t%s\r\n",dir_info.name);
	}

	lfs_dir_close(&g_flash_lfs,&root_dir);
}


void flash_fs_init(void)
{
	// mount the filesystem
	int err = lfs_mount(&g_flash_lfs, &flash_lfs_cfg);

	// reformat if we can't mount the filesystem
	// this should only happen on the first boot
	if (err) 
	{
		printk("little file system not found,rebuild ");
		lfs_format(&g_flash_lfs, &flash_lfs_cfg);
		lfs_mount(&g_flash_lfs, &flash_lfs_cfg);
		printk("done\r\n");
	}
	else
		printk("little file system mount:STM32 flash\r\n");

	
	shell_register_command("lfs-flash-test",shell_test_flashfs);
	shell_register_command("lfs-flash-touch",flash_touch_file);
	shell_register_command("lfs-flash-cat",flash_read_file);
	shell_register_command("lfs-flash-ls",flash_list_file);
	
}


