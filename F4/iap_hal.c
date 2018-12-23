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

#define SYSCFG_INFO_SIZE (0x10000)
#define SYSCFG_ADDR_MAX  (APP_ADDR-4)
#define SYSCFG_ADDR_MIN  (APP_ADDR-SYSCFG_INFO_SIZE)


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




/**
	* @brief    syscfg_addr ��ȡ���µ� syscfg �ļ��ĵ�ַ
	* @param    ��
	* @return   syscfg ��ַ
*/
uint32_t syscfg_addr(void)
{
	uint32_t cfgindex = *(uint32_t*)"cfg:";
	uint32_t *sche = (uint32_t *)SYSCFG_ADDR_MAX;
	uint32_t *schs = (uint32_t *)SYSCFG_ADDR_MIN ;
	uint32_t *sch ;

	if (*schs == 0xffffffff)//���cfg ҳ�������ݣ���������
	{
		iap_unlock_flash();
		iap_write_flash(SYSCFG_ADDR_MIN ,cfgindex);
		iap_write_flash(SYSCFG_ADDR_MIN + 4 ,(*(uint32_t*)" \r\n"));//���������
		iap_lock_flash();
		return (SYSCFG_ADDR_MIN);
	}

	while(sche - schs > 1) //���ַ����ҵ����µ� syscfg ��β
	{
		sch = schs + (sche - schs) / 2;
		if (*sch == 0xffffffff)
			sche = sch;
		else
			schs = sch;
	}

	//���ֽڶ����ڴ棬�����ҵ� cfg: ��ͷ
	for (sch = schs ; sch >= (uint32_t*)SYSCFG_ADDR_MIN ; --sch)
	{
		if (cfgindex == (*sch)) //�ҵ��˷��ش˵�ַ
			return (uint32_t)sch;
	}

	return 0;
}


/**
	* @brief    erase_syscfg ���� syscfg ��ַ������������
	* @param    ��
	* @return   void
*/
void syscfg_erase(void)
{
	iap_unlock_flash();
	iap_erase_flash(SYSCFG_ADDR_MIN ,SYSCFG_INFO_SIZE);
	iap_lock_flash();
}



/**
	* @brief    write_syscfg д�� syscfg ����
	* @param    ��
	* @return  
*/
void syscfg_write(char * info , uint32_t len)
{
	uint32_t *data = (uint32_t *)info;
	uint32_t *sche = (uint32_t *)SYSCFG_ADDR_MAX;
	uint32_t *schs = (uint32_t *)SYSCFG_ADDR_MIN;
	uint32_t *sch  = NULL;

	if( *schs == 0xffffffff) //���������δ�� config ��Ϣ
		goto WRITE_SYSCONFIG;

	while( sche - schs > 1)//���ַ����ҵ����µ� syscfg ��β
	{
		sch = schs + (sche - schs) / 2;
		if ( *sch == 0xffffffff)
			sche = sch;
		else
			schs = sch;
	}

	if ( (uint32_t)sche + len > SYSCFG_ADDR_MAX)//ʣ��ռ䲻����д��һ������
	{
		syscfg_erase();       //��� syscfg ����
		sch = (uint32_t *)SYSCFG_ADDR_MIN;//��ͷ��ʼд
	}
	else
		sch = sche;

WRITE_SYSCONFIG: //д�� syscfg ����

	iap_unlock_flash();

	for (uint32_t i = 0 ; i < (len/4 + 1); ++i)
	{
		iap_write_flash((uint32_t)sch , *data);
		sch++;
		data++;
	}

	iap_lock_flash();
}





