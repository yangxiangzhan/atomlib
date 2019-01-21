/**
  ******************************************************************************
  * @file           iap_hal.h
  * @author         古么宁
  * @brief          serial_console file
                    串口控制台文件。文件不直接操作硬件，依赖 serial_hal.c 和 iap_hal.c
  ******************************************************************************
  *
  * COPYRIGHT(c) 2018 GoodMorning
  *
  ******************************************************************************
  */
#ifndef __IAP_BASE_H__
#define __IAP_BASE_H__

/*
 *  把 flash 大致分为四部分
 *  IAP_ADDR         SYSCFG_ADDR    APP_ADDR  UPDATE_PACKAGE_AT_STM32_ADRR
 *  |--------------------|--------------|---------------|--------------|
 *   iap,即bootloader部分    sysconfig信息          app 部分           存储升级包部分
 *
 */



#define UPDPKG_AT_MCU_ADDR           0x08060000

#define UPDATE_PACKAGE_AT_STM32_ADRR UPDPKG_AT_MCU_ADDR //telnet/串口 升级包存放位置

#define IAP_ADDR 0x08000000
#define APP_ADDR 0x08020000


#define SYSCFG_INFO_SIZE (0x10000)
#define SYSCFG_ADDR_MAX  (APP_ADDR-4)
#define SYSCFG_ADDR_MIN  (APP_ADDR-SYSCFG_INFO_SIZE)


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

#define APP_CORRECT()     (SRAM_BASE == ((*(uint32_t *)(APP_ADDR))&0x2FFC0000))



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
