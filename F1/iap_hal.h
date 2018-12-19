#ifndef __IAP_BASE_H__
#define __IAP_BASE_H__


#define UPDATE_PACKAGE_AT_W25X16_ADRR 0x10000
#define IAP_RX_BUF_SIZE 256  //0x100//  W25X16 һҳ��С



#define UPDATE_PACKAGE_AT_STM32_ADRR 0x08060000 //telnet/���� ���������λ��



#define IAP_ADDR 0x08000000
#define APP_ADDR 0x08010000

// stm32f429bit6 ram��ʼ��ַΪ 0x20000000 ����С 0x30000
// stm32f103vet6 ram��ʼ��ַΪ 0x20000000 ����С 0x10000
// ֱ���� ram �����ֶεĵ�ַ��Ϊ�� iap ģʽ�ı�־λ
// �� app �У��ȶԱ�־λ������λ��ʾ��Ȼ����ת�� iap 
#define IAP_FLAG_ADDR       0x2002FF00 

#define IAP_UPDATE_CMD_FLAG 0x1234ABCD

#define IAP_SUCCESS_FLAG    0x54329876

#define IAP_GET_UPDATE_CMD() (*(__IO uint32_t *) IAP_FLAG_ADDR == IAP_UPDATE_CMD_FLAG)

#define IAP_SET_UPDATE_CMD() (*(__IO uint32_t *) IAP_FLAG_ADDR = IAP_UPDATE_CMD_FLAG)

#define IAP_CHECK_UPDATE_FLAG() (*(__IO uint32_t *) IAP_FLAG_ADDR == IAP_SUCCESS_FLAG)

#define IAP_SET_UPDATE_FLAG()   (*(__IO uint32_t *) IAP_FLAG_ADDR = IAP_SUCCESS_FLAG)

#define IAP_CLEAR_UPDATE_FLAG() (*(__IO uint32_t *) IAP_FLAG_ADDR = 0)





#define vJumpTo(where)\
do{\
	uint32_t SpInitVal = *(uint32_t *)(where##_ADDR);    \
	uint32_t JumpAddr = *(uint32_t *)(where##_ADDR + 4); \
	void (*pAppFun)(void) = (void (*)(void))JumpAddr;    \
	__set_BASEPRI(0); 	      \
	__set_FAULTMASK(0);       \
	__disable_irq();          \
	__set_MSP(SpInitVal);     \
	__set_PSP(SpInitVal);     \
	__set_CONTROL(0);         \
	(*pAppFun) ();            \
}while(0)


//------------------------------���� IAP ���------------------------------

int iap_erase_flash(uint32_t eraseaddr,uint32_t erasesize);

int iap_write_flash(uint32_t flash_addr,uint32_t flash_value);

int iap_unlock_flash(void);

int iap_lock_flash(void);

//------------------------------����̨����------------------------------
void shell_jump_command(void * arg);
void shell_reboot_command(void * arg);



#endif
