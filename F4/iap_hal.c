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


#include "stm32f4xx.h"
#include "stm32f4xx_hal.h"
#include "iap_hal.h"


//------------------------------���� IAP ���------------------------------


/**
	* @brief    iap_erase_sector console ���� flash ĳ������
	* @param    ��
	* @return   ��HAL_OK����0 Ϊ�������������
*/
int iap_erase_sector(uint32_t SECTOR , uint32_t NbSectors)
{
	uint32_t SectorError;
    FLASH_EraseInitTypeDef FlashEraseInit;
	HAL_StatusTypeDef HAL_Status;
	
	FlashEraseInit.TypeErase    = FLASH_TYPEERASE_SECTORS;       //�������ͣ��������� 
	FlashEraseInit.Sector       = SECTOR;                        //����
	FlashEraseInit.NbSectors    = NbSectors;                     //һ��ֻ����һ������
	FlashEraseInit.VoltageRange = FLASH_VOLTAGE_RANGE_3;      //��ѹ��Χ��VCC=2.7~3.6V֮��!!
	
	HAL_Status = HAL_FLASHEx_Erase(&FlashEraseInit,&SectorError);
	
	return HAL_Status;
}


int iap_get_flash_sector(uint32_t flashaddr,uint32_t * sector)
{
	uint32_t sectoraddr;
	uint32_t SECTOR12 ;
	uint32_t addr ;

	if (flashaddr < 0x08000000 || flashaddr > 0x08200000)//stm32f429bi �� 2 M �ռ�
		return 1;

	addr = flashaddr - 0x08000000;
	if (addr > 0x100000)//12~23�����ֲ��� 0~11�����ֲ�һ��
	{
		SECTOR12 = 12;
		addr -= 0x100000;
	}
	else
		SECTOR12 = 0 ;

	if ( addr < 0x10000) // 0~3 ����ÿ�� 0x4000 ��С
		sectoraddr = addr / 0x4000 ;
	else
	if (addr < 0x20000) //���� 4 : 0x08010000 - 0x0801FFFF
		sectoraddr = 4;
	else                // 5~11 ����ÿ�� 0x20000 ��С
		sectoraddr = 5 + (addr - 0x20000) / 0x20000 ;

	*sector = sectoraddr += SECTOR12;

	return 0;
}


/**
	* @brief    iap_erase_flash console ���� flash ĳ����ַ��������
	* @param    ��������ĵ�ַ
	* @return   ��HAL_OK����0 Ϊ�������������
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
	* @brief    vUsartHal_IAP_Write console д flash
	* @param    ��
	* @return   ��HAL_OK����0 Ϊ�������������
*/
int iap_write_flash(uint32_t FlashAddr,uint32_t FlashData)
{
	return HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD,FlashAddr,FlashData);
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




