/**
  ******************************************************************************
  * @file           iap_hal.c
  * @author         ��ô��
  * @brief          serial_console file
                    ������������Ӳ�����󲿷�
  ******************************************************************************
  *
  * COPYRIGHT(c) 2018 GoodMorning
  *
  ******************************************************************************
  */
/* Includes ---------------------------------------------------*/


#include "stm32l1xx.h"
#include "stm32l1xx_hal.h"
#include "stm32l1xx_hal_flash_ex.h"
#include "iap_hal.h"


//------------------------------���� IAP ���------------------------------




/**
	* @brief    iap_erase_flash console ���� flash ĳ����ַ��������
	* @param    ��������ĵ�ַ
	* @return   ��HAL_OK����0 Ϊ�������������
*/
int iap_erase_flash(uint32_t eraseaddr,uint32_t erasesize)
{
	uint32_t SectorError;
    FLASH_EraseInitTypeDef FlashEraseInit;
	HAL_StatusTypeDef HAL_Status;

    /* Fill EraseInit structure************************************************/
	FlashEraseInit.TypeErase   = FLASH_TYPEERASE_PAGES;
	FlashEraseInit.PageAddress = eraseaddr;
	FlashEraseInit.NbPages     = erasesize ;


	HAL_Status = HAL_FLASHEx_Erase(&FlashEraseInit,&SectorError);

	return HAL_Status;
}



/**
	* @brief    vUsartHal_IAP_Write console д flash
	* @param    ��
	* @return   ��HAL_OK����0 Ϊ�������������
*/
int iap_write_flash(uint32_t FlashAddr,uint32_t FlashData)
{
	return HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD,FlashAddr,FlashData);
//	FLASH_Program_HalfWord(FlashAddr,FlashData);
}



/**
	* @brief    iap_lock_flash console ���� flash
	* @param    ��
	* @return   ��HAL_OK����0 Ϊ�������������
*/
int iap_lock_flash(void)
{
	return HAL_FLASH_Lock();// 0 Ϊ����
}



/**
	* @brief    iap_unlock_flash console ���� flash
	* @param    ��
	* @return   ��HAL_OK����0 Ϊ�������������
*/
int iap_unlock_flash(void)
{
	return HAL_FLASH_Unlock();
}



/**
	* @brief    vSystemReboot Ӳ������
	* @param    ��
	* @return  
*/
void shell_reboot_command(void * arg)
{
	NVIC_SystemReset();
}



/**
	* @brief    vShell_JumpToAppCmd console ���ڷ���һ����������ж�
	* @param    ��
	* @return   ��
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





