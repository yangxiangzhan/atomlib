#ifndef __IAP_BASE_H__
#define __IAP_BASE_H__



#define UPDATE_PACKAGE_AT_STM32_ADRR 0x08060000 //telnet/���� ���������λ��



#define IAP_ADDR 0x08000000
#define APP_ADDR 0x08010000
#define SYSCFG_INFO_SIZE FLASH_PAGE_SIZE


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

#define APP_CORRECT()     (0x20000000 == ((*(uint32_t *)(APP_ADDR))&0x2FFE0000))


//------------------------------���� IAP ���------------------------------

int iap_erase_flash(uint32_t eraseaddr,uint32_t erasesize);

int iap_write_flash(uint32_t flash_addr,uint32_t flash_value);

int iap_unlock_flash(void);

int iap_lock_flash(void);


uint32_t syscfg_addr(void);

void syscfg_erase(void);

void syscfg_write(char * info , uint32_t len);


//------------------------------����̨����------------------------------

void shell_reboot_command(void * arg);



#endif
