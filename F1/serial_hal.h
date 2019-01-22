#ifndef __CONSOLE_HAL_H__
#define __CONSOLE_HAL_H__
#ifdef __cplusplus
 extern "C" {
#endif


#define HAL_RX_PACKET_SIZE 4     //Ӳ�����յ��Ļ�����У������ݰ�Ϊ��λ
#define HAL_RX_BUF_SIZE    (512*2+1)  //Ӳ�����ջ��������������̫С����Ϊ�漰�� iap
#define HAL_TX_BUF_SIZE    512  //Ӳ�����ͻ�����


void serial_puts(char * buf,uint16_t len);

int  serial_rxpkt_queue_out(char ** data,uint16_t * len);

void serial_recv_reset(uint16_t MaxLen);

int  serial_busy(void);

void hal_serial_init(void);

void hal_serial_deinit(void);

	 
#ifdef __cplusplus
}
#endif
#endif /* __CONSOLE_HAL_H__ */

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
