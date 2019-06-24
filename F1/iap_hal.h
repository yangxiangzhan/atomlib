#ifndef __IAP_BASE_H__
#define __IAP_BASE_H__



#define UPDATE_PACKAGE_AT_STM32_ADRR 0x08060000 //telnet/串口 升级包存放位置



#define IAP_ADDR 0x08000000
#define APP_ADDR 0x08010000
#define SYSCFG_INFO_SIZE FLASH_PAGE_SIZE


// stm32f429bit6 ram起始地址为 0x20000000 ，大小 0x30000
// stm32f103vet6 ram起始地址为 0x20000000 ，大小 0x10000
// 直接用 ram 区高字段的地址作为进 iap 模式的标志位
// 在 app 中，先对标志位进行置位提示，然后跳转到 iap 
#define IAP_FLAG_ADDR       0x2002FF00 

#define IAP_UPDATE_CMD_FLAG 0x1234ABCD

#define IAP_SUCCESS_FLAG    0x54329876

#define IAP_GET_UPDATE_CMD() (*(__IO uint32_t *) IAP_FLAG_ADDR == IAP_UPDATE_CMD_FLAG)

#define IAP_SET_UPDATE_CMD() (*(__IO uint32_t *) IAP_FLAG_ADDR = IAP_UPDATE_CMD_FLAG)

#define IAP_CHECK_UPDATE_FLAG() (*(__IO uint32_t *) IAP_FLAG_ADDR == IAP_SUCCESS_FLAG)

#define IAP_SET_UPDATE_FLAG()   (*(__IO uint32_t *) IAP_FLAG_ADDR = IAP_SUCCESS_FLAG)

#define IAP_CLEAR_UPDATE_FLAG() (*(__IO uint32_t *) IAP_FLAG_ADDR = 0)

#define APP_CORRECT()     (0x20000000 == ((*(uint32_t *)(APP_ADDR))&0x2FFE0000))


//------------------------------串口 IAP 相关------------------------------

int iap_erase_flash(uint32_t eraseaddr,uint32_t erasesize);

int iap_write_flash(uint32_t flash_addr,uint32_t flash_value);

int iap_unlock_flash(void);

int iap_lock_flash(void);


uint32_t syscfg_addr(void);

void syscfg_erase(void);

void syscfg_write(char * info , uint32_t len);


//------------------------------控制台命令------------------------------

void shell_reboot_command(void * arg);



#endif
