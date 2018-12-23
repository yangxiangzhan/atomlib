/**
  ******************************************************************************
  * @file           iap_hal.c
  * @author         古么宁
  * @brief          serial_console file
                    在线升级功能硬件抽象部分
  ******************************************************************************
  *
  * COPYRIGHT(c) 2018 GoodMorning
  *
  ******************************************************************************
  */
/* Includes ---------------------------------------------------*/


#include "stm32f4xx.h"
#include "stm32f4xx_hal.h"
#include "iap_hal.h"


//------------------------------串口 IAP 相关------------------------------

#define SYSCFG_INFO_SIZE (0x10000)
#define SYSCFG_ADDR_MAX  (APP_ADDR-4)
#define SYSCFG_ADDR_MIN  (APP_ADDR-SYSCFG_INFO_SIZE)


/**
	* @brief    iap_erase_sector console 擦除 flash 某个扇区
	* @param    空
	* @return   （HAL_OK）既0 为正常，否则出错
*/
int iap_erase_sector(uint32_t SECTOR , uint32_t NbSectors)
{
	uint32_t SectorError;
    FLASH_EraseInitTypeDef FlashEraseInit;
	HAL_StatusTypeDef HAL_Status;
	
	FlashEraseInit.TypeErase    = FLASH_TYPEERASE_SECTORS;       //擦除类型，扇区擦除 
	FlashEraseInit.Sector       = SECTOR;                        //扇区
	FlashEraseInit.NbSectors    = NbSectors;                     //一次只擦除一个扇区
	FlashEraseInit.VoltageRange = FLASH_VOLTAGE_RANGE_3;      //电压范围，VCC=2.7~3.6V之间!!
	
	HAL_Status = HAL_FLASHEx_Erase(&FlashEraseInit,&SectorError);
	
	return HAL_Status;
}


int iap_get_flash_sector(uint32_t flashaddr,uint32_t * sector)
{
	uint32_t sectoraddr;
	uint32_t SECTOR12 ;
	uint32_t addr ;

	if (flashaddr < 0x08000000 || flashaddr > 0x08200000)//stm32f429bi 有 2 M 空间
		return 1;

	addr = flashaddr - 0x08000000;
	if (addr > 0x100000)//12~23扇区分布与 0~11扇区分布一样
	{
		SECTOR12 = 12;
		addr -= 0x100000;
	}
	else
		SECTOR12 = 0 ;

	if ( addr < 0x10000) // 0~3 扇区每区 0x4000 大小
		sectoraddr = addr / 0x4000 ;
	else
	if (addr < 0x20000) //扇区 4 : 0x08010000 - 0x0801FFFF
		sectoraddr = 4;
	else                // 5~11 扇区每区 0x20000 大小
		sectoraddr = 5 + (addr - 0x20000) / 0x20000 ;

	*sector = sectoraddr += SECTOR12;

	return 0;
}


/**
	* @brief    iap_erase_flash console 擦除 flash 某个地址所在扇区
	* @param    所需擦除的地址
	* @return   （HAL_OK）既0 为正常，否则出错
*/
int iap_erase_flash(uint32_t eraseaddr,uint32_t erasesize)
{
	uint32_t erasesectors;
	uint32_t erasesectore;

	if (!erasesize)
		return 1;

	if (iap_get_flash_sector(eraseaddr,&erasesectors))
		return 1;

	if (iap_get_flash_sector(eraseaddr + erasesize - 1,&erasesectore))
		return 1;

	if (erasesectors > erasesectore)
		return 1;

	if (iap_erase_sector(erasesectors,erasesectore - erasesectors + 1))
		return 1;

	return 0;
}



/**
	* @brief    vUsartHal_IAP_Write console 写 flash
	* @param    空
	* @return   （HAL_OK）既0 为正常，否则出错
*/
int iap_write_flash(uint32_t FlashAddr,uint32_t FlashData)
{
	return HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD,FlashAddr,FlashData);
}



/**
	* @brief    iap_lock_flash console 上锁 flash
	* @param    空
	* @return   （HAL_OK）既0 为正常，否则出错
*/
int iap_lock_flash(void)
{
	return HAL_FLASH_Lock();// 0 为正常
}



/**
	* @brief    iap_unlock_flash console 解锁 flash
	* @param    空
	* @return   （HAL_OK）既0 为正常，否则出错
*/
int iap_unlock_flash(void)
{
	return HAL_FLASH_Unlock();
}



/**
	* @brief    vSystemReboot 硬件重启
	* @param    空
	* @return  
*/
void shell_reboot_command(void * arg)
{
	NVIC_SystemReset();
}



/**
	* @brief    vShell_JumpToAppCmd console 串口发送一包数据完成中断
	* @param    空
	* @return   空
*/
void shell_jump_command(void * arg)
{
	uint32_t UPDATE_ADDR = (SCB->VTOR == FLASH_BASE) ? (APP_ADDR):(IAP_ADDR);
	uint32_t SpInitVal = *(uint32_t *)(UPDATE_ADDR);    
	uint32_t JumpAddr = *(uint32_t *)(UPDATE_ADDR + 4); 
	void (*pAppFun)(void) = (void (*)(void))JumpAddr;    
	__set_BASEPRI(0); 	      
	__set_FAULTMASK(0);       
	__disable_irq();          
	__set_MSP(SpInitVal);     
	__set_PSP(SpInitVal);     
	__set_CONTROL(0);         
	(*pAppFun) ();            
}




/**
	* @brief    syscfg_addr 获取最新的 syscfg 文件的地址
	* @param    空
	* @return   syscfg 地址
*/
uint32_t syscfg_addr(void)
{
	uint32_t cfgindex = *(uint32_t*)"cfg:";
	uint32_t *sche = (uint32_t *)SYSCFG_ADDR_MAX;
	uint32_t *schs = (uint32_t *)SYSCFG_ADDR_MIN ;
	uint32_t *sch ;

	if (*schs == 0xffffffff)//如果cfg 页面无内容，创建内容
	{
		iap_unlock_flash();
		iap_write_flash(SYSCFG_ADDR_MIN ,cfgindex);
		iap_write_flash(SYSCFG_ADDR_MIN + 4 ,(*(uint32_t*)" \r\n"));//加入结束符
		iap_lock_flash();
		return (SYSCFG_ADDR_MIN);
	}

	while(sche - schs > 1) //二分法查找到最新的 syscfg 结尾
	{
		sch = schs + (sche - schs) / 2;
		if (*sch == 0xffffffff)
			sche = sch;
		else
			schs = sch;
	}

	//四字节对齐内存，回溯找到 cfg: 开头
	for (sch = schs ; sch >= (uint32_t*)SYSCFG_ADDR_MIN ; --sch)
	{
		if (cfgindex == (*sch)) //找到了返回此地址
			return (uint32_t)sch;
	}

	return 0;
}


/**
	* @brief    erase_syscfg 擦除 syscfg 地址区域所有数据
	* @param    空
	* @return   void
*/
void syscfg_erase(void)
{
	iap_unlock_flash();
	iap_erase_flash(SYSCFG_ADDR_MIN ,SYSCFG_INFO_SIZE);
	iap_lock_flash();
}



/**
	* @brief    write_syscfg 写入 syscfg 数据
	* @param    空
	* @return  
*/
void syscfg_write(char * info , uint32_t len)
{
	uint32_t *data = (uint32_t *)info;
	uint32_t *sche = (uint32_t *)SYSCFG_ADDR_MAX;
	uint32_t *schs = (uint32_t *)SYSCFG_ADDR_MIN;
	uint32_t *sch  = NULL;

	if( *schs == 0xffffffff) //如果区域内未有 config 信息
		goto WRITE_SYSCONFIG;

	while( sche - schs > 1)//二分法查找到最新的 syscfg 结尾
	{
		sch = schs + (sche - schs) / 2;
		if ( *sch == 0xffffffff)
			sche = sch;
		else
			schs = sch;
	}

	if ( (uint32_t)sche + len > SYSCFG_ADDR_MAX)//剩余空间不足以写这一包数据
	{
		syscfg_erase();       //清空 syscfg 区域
		sch = (uint32_t *)SYSCFG_ADDR_MIN;//从头开始写
	}
	else
		sch = sche;

WRITE_SYSCONFIG: //写入 syscfg 数据

	iap_unlock_flash();

	for (uint32_t i = 0 ; i < (len/4 + 1); ++i)
	{
		iap_write_flash((uint32_t)sch , *data);
		sch++;
		data++;
	}

	iap_lock_flash();
}





