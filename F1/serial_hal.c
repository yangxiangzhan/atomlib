/**
  ******************************************************************************
  * @file           serial_hal.c
  * @author         goodmorning
  * @brief          串口控制台底层硬件实现。
  ******************************************************************************
  *
  * COPYRIGHT(c) 2018 GoodMorning
  *
  ******************************************************************************
  */
/* Includes ---------------------------------------------------*/
#include <string.h>
#include "stm32f1xx.h"
#include "stm32f1xx_hal.h"
#include "stm32f1xx_ll_bus.h"
#include "stm32f1xx_ll_gpio.h"
#include "stm32f1xx_ll_usart.h"
#include "stm32f1xx_ll_dma.h"
#include "serial_hal.h"
#include "shell.h"




//---------------------HAL层相关--------------------------
// 如果要对硬件进行移植修改，修改下列宏，并提供引脚初始化程序

#define     USART_BAUDRATE           115200   //波特率
#define     USART_DMA_CLOCK_INIT()   LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_DMA1)//跟 DMAx 对应


#define     xUSART   1 //引用串口号

#if        (xUSART == 1) //串口1对应DMA

	#define xDMA     1  //对应DMA
	#define xDMATxCH 4  //发送对应 DMA 通道号
	#define xDMARxCH 5  //接收对应 DMA 通道号
	
#elif (xUSART == 3) //串口3对应DMA

//	#define     RemapPartial_USART
	#define xDMA     1
	#define xDMATxCH 2
	#define xDMARxCH 3

#endif

//---------------------HAL层宏替换--------------------------
#define USART_X(x)	                   USART##x
#define USART_x(x)	                   USART_X(x)
	 
#define USART_IRQn(x)                  USART##x##_IRQn
#define USARTx_IRQn(x)                 USART_IRQn(x)
	 
#define USART_IRQHandler(x)            USART##x##_IRQHandler
#define USARTx_IRQHandler(x)           USART_IRQHandler(x)

#define DMA_X(x)	                   DMA##x
#define DMA_x(x)	                   DMA_X(x)
	 
#define DMA_Channel(x)                 LL_DMA_CHANNEL_##x
#define DMA_Channelx(x)                DMA_Channel(x)	 

#define DMA_Channel_IRQn(x,y)          DMA##x##_Channel##y##_IRQn
#define DMAx_Channely_IRQn(x,y)        DMA_Channel_IRQn(x,y)

#define DMA_Channel_IRQHandler(x,y)    DMA##x##_Channel##y##_IRQHandler
#define DMAx_Channely_IRQHandler(x,y)  DMA_Channel_IRQHandler(x,y)
	 
#define DMA_ClearFlag_TC(x,y)          LL_DMA_ClearFlag_TC##y(DMA##x)
#define DMAx_ClearFlag_TCy(x,y)        DMA_ClearFlag_TC(x,y)
	 
#define DMA_IsActiveFlag_TC(x,y)       LL_DMA_IsActiveFlag_TC##y(DMA##x)
#define DMAx_IsActiveFlag_TCy(x,y)     DMA_IsActiveFlag_TC(x,y)

#define DMAx                    DMA_x(xDMA)     //串口所在 dma 总线
#define USARTx                  USART_x(xUSART)   //引用串口
#define USARTIRQn               USARTx_IRQn(xUSART) //中断号
#define USART_IRQ_HANDLER       USARTx_IRQHandler(xUSART) //中断函数名

#define DMA_TX_CHx              DMA_Channelx(xDMATxCH)    //串口发送 dma 通道
#define DMA_TX_IRQn             DMAx_Channely_IRQn(xDMA,xDMATxCH)
#define DMA_TX_IRQ_HANDLER      DMAx_Channely_IRQHandler(xDMA,xDMATxCH) //中断函数名
#define DMA_TX_CLEAR_FLAG()     DMAx_ClearFlag_TCy(xDMA,xDMATxCH)
#define DMA_TX_COMPLETE_FLAG()  DMAx_IsActiveFlag_TCy(xDMA,xDMATxCH)

#define DMA_RX_CHx              DMA_Channelx(xDMARxCH)
#define DMA_RX_IRQn             DMAx_Channely_IRQn(xDMA,xDMARxCH)
#define DMA_RX_IRQ_HANDLER      DMAx_Channely_IRQHandler(xDMA,xDMARxCH) //中断函数名
#define DMA_RX_CLEAR_FLAG()     DMAx_ClearFlag_TCy(xDMA,xDMARxCH)
#define DMA_RX_COMPLETE_FLAG()  DMAx_IsActiveFlag_TCy(xDMA,xDMARxCH)

//---------------------------------------------------------

#define HAL_RX_PACKET_SIZE 4     //硬件接收到的缓冲队列，以数据包为单位
#define HAL_RX_BUF_SIZE    (1024*2+1)  //硬件接收缓冲区
#define HAL_TX_BUF_SIZE    1024  //硬件发送缓冲区

static struct _serial_tx
{
	uint16_t pkttail ;
	uint16_t pktsize ;
	char buf[HAL_TX_BUF_SIZE];
}
serial_tx = {0};


static struct _serial_rx
{
	uint16_t pkttail;
	uint16_t pktmax;
	char buf[HAL_RX_BUF_SIZE];
}
serial_rx = {0};


static struct _serial_queue
{
	uint16_t tail ;
	uint16_t head ;
	
	uint16_t  pktlen[HAL_RX_PACKET_SIZE];
	char    * pktbuf[HAL_RX_PACKET_SIZE];
}
serial_rxpkt_queue  = {0};




#if   (xUSART == 1)
static void usart_gpio_init(void)
{
	LL_GPIO_InitTypeDef GPIO_InitStruct;
	/* Peripheral clock enable */
	LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_USART1);
  
		/**USART1 GPIO Configuration
		PA9   ------> USART1_TX
		PA10   ------> USART1_RX
		*/
  GPIO_InitStruct.Pin = LL_GPIO_PIN_9;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
	GPIO_InitStruct.Pull = LL_GPIO_PULL_UP;
  LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = LL_GPIO_PIN_10;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = LL_GPIO_PULL_UP;
  LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

}

#elif (xUSART == 3)

static void usart_gpio_init(void)
{
	LL_GPIO_InitTypeDef GPIO_InitStruct;
	/* Peripheral clock enable */
	LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_USART3);
	
	#ifdef 	RemapPartial_USART
	
		/**USART3 GPIO Configuration  
		PC10   ------> USART3_TX
		PC11   ------> USART3_RX 
		*/
		GPIO_InitStruct.Pin = LL_GPIO_PIN_10;
		GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
		GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
		GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
		LL_GPIO_Init(GPIOC, &GPIO_InitStruct);
		
			
		GPIO_InitStruct.Pin = LL_GPIO_PIN_11;
		GPIO_InitStruct.Mode = LL_GPIO_MODE_INPUT;
		GPIO_InitStruct.Pull = LL_GPIO_PULL_UP;
		GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
		LL_GPIO_Init(GPIOC, &GPIO_InitStruct);

		LL_GPIO_AF_RemapPartial_USART3();
	#else

		/**USART3 GPIO Configuration  
		PB10   ------> USART3_TX
		PB11   ------> USART3_RX 
		*/
		GPIO_InitStruct.Pin = LL_GPIO_PIN_10;
		GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
		GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
		GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
		LL_GPIO_Init(GPIOB, &GPIO_InitStruct);

			
		GPIO_InitStruct.Pin = LL_GPIO_PIN_11;
		GPIO_InitStruct.Mode = LL_GPIO_MODE_INPUT;
		GPIO_InitStruct.Pull = LL_GPIO_PULL_UP;
		GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
		LL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	#endif
}


#endif


/** 
	* @brief usart_dma_init 控制台 DMA 初始化
	* @param void
	* @return NULL
*/
static void usart_dma_init(void)
{
	USART_DMA_CLOCK_INIT();

	/* USART_RX Init */  /* USART_RX Init */
	LL_DMA_SetDataTransferDirection(DMAx, DMA_RX_CHx, LL_DMA_DIRECTION_PERIPH_TO_MEMORY);
	LL_DMA_SetChannelPriorityLevel(DMAx, DMA_RX_CHx, LL_DMA_PRIORITY_MEDIUM);
	LL_DMA_SetMode(DMAx, DMA_RX_CHx, LL_DMA_MODE_NORMAL);
	LL_DMA_SetPeriphIncMode(DMAx, DMA_RX_CHx, LL_DMA_PERIPH_NOINCREMENT);
	LL_DMA_SetMemoryIncMode(DMAx, DMA_RX_CHx, LL_DMA_MEMORY_INCREMENT);
	LL_DMA_SetPeriphSize(DMAx, DMA_RX_CHx, LL_DMA_PDATAALIGN_BYTE);
	LL_DMA_SetMemorySize(DMAx, DMA_RX_CHx, LL_DMA_MDATAALIGN_BYTE);
	LL_DMA_SetPeriphAddress(DMAx,DMA_RX_CHx,LL_USART_DMA_GetRegAddr(USARTx)); 

	/* USART_TX Init */
	LL_DMA_SetDataTransferDirection(DMAx, DMA_TX_CHx, LL_DMA_DIRECTION_MEMORY_TO_PERIPH);
	LL_DMA_SetChannelPriorityLevel(DMAx, DMA_TX_CHx, LL_DMA_PRIORITY_MEDIUM);
	LL_DMA_SetMode(DMAx, DMA_TX_CHx, LL_DMA_MODE_NORMAL);
	LL_DMA_SetPeriphIncMode(DMAx, DMA_TX_CHx, LL_DMA_PERIPH_NOINCREMENT);
	LL_DMA_SetMemoryIncMode(DMAx, DMA_TX_CHx, LL_DMA_MEMORY_INCREMENT);
	LL_DMA_SetPeriphSize(DMAx, DMA_TX_CHx, LL_DMA_PDATAALIGN_BYTE);
	LL_DMA_SetMemorySize(DMAx, DMA_TX_CHx, LL_DMA_MDATAALIGN_BYTE);
	LL_DMA_SetPeriphAddress(DMAx,DMA_TX_CHx,LL_USART_DMA_GetRegAddr(USARTx));

	LL_DMA_DisableChannel(DMAx,DMA_TX_CHx);//发送暂不使能
	LL_DMA_DisableChannel(DMAx,DMA_RX_CHx);//发送暂不使能

	DMA_TX_CLEAR_FLAG();
	DMA_RX_CLEAR_FLAG();
	
	  /* DMA interrupt init 中断*/
	NVIC_SetPriority(DMA_TX_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),6, 0));
	NVIC_EnableIRQ(DMA_TX_IRQn);
	  
	NVIC_SetPriority(DMA_RX_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),6, 0));
	NVIC_EnableIRQ(DMA_RX_IRQn);
	
	LL_DMA_EnableIT_TC(DMAx,DMA_TX_CHx);
	LL_DMA_EnableIT_TC(DMAx,DMA_RX_CHx);
}




/** 
	* @brief usart_base_init 控制台串口参数初始化
	* @param void
	* @return NULL
*/
static void usart_base_init(void)
{
	LL_USART_InitTypeDef USART_InitStruct;

	USART_InitStruct.BaudRate = USART_BAUDRATE;
	USART_InitStruct.DataWidth = LL_USART_DATAWIDTH_8B;
	USART_InitStruct.StopBits = LL_USART_STOPBITS_1;
	USART_InitStruct.Parity = LL_USART_PARITY_NONE;
	USART_InitStruct.TransferDirection = LL_USART_DIRECTION_TX_RX;
	USART_InitStruct.HardwareFlowControl = LL_USART_HWCONTROL_NONE;

	LL_USART_Init(USARTx, &USART_InitStruct);
	LL_USART_ConfigAsyncMode(USARTx);

	NVIC_SetPriority(USARTIRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),7, 0));
	NVIC_EnableIRQ(USARTIRQn);

	LL_USART_DisableIT_RXNE(USARTx);
	LL_USART_DisableIT_PE(USARTx);
	LL_USART_EnableIT_IDLE(USARTx);

	LL_USART_EnableDMAReq_RX(USARTx);
	LL_USART_EnableDMAReq_TX(USARTx);

	LL_USART_Enable(USARTx);
}



/**
	* @brief    设置 console 硬件接收缓存区，同时会清除接收标志位
	* @param    空
	* @return   
*/
static inline void serial_dma_next_recv( uint32_t memory_addr ,uint16_t dma_max_len)
{
	LL_DMA_DisableIT_TC(DMAx,DMA_RX_CHx);
	LL_DMA_DisableChannel(DMAx,DMA_RX_CHx);//发送暂不使能
	
	DMA_RX_CLEAR_FLAG();
	
	LL_DMA_SetMemoryAddress(DMAx,DMA_RX_CHx,memory_addr);
	LL_DMA_SetDataLength(DMAx,DMA_RX_CHx,dma_max_len);

	LL_DMA_EnableIT_TC(DMAx,DMA_RX_CHx);
	LL_DMA_EnableChannel(DMAx,DMA_RX_CHx);//new
}


/**
  * @brief    console 启动发送当前包
  * @param    空
  * @retval   空
  */
static inline void serial_send_pkt(void)
{
	uint32_t pkt_size = serial_tx.pktsize ;
	uint32_t pkt_head  = serial_tx.pkttail - pkt_size ;
	
	serial_tx.pktsize = 0;
	
	LL_DMA_SetMemoryAddress(DMAx,DMA_TX_CHx,(uint32_t)(&serial_tx.buf[pkt_head]));
	LL_DMA_SetDataLength(DMAx,DMA_TX_CHx,pkt_size);
	LL_DMA_EnableChannel(DMAx,DMA_TX_CHx);
}



/**
  * @brief    serial_rxpkt_max_len 设置硬件接收最大包
  * @param    空
  * @retval   空
  */
void serial_recv_reset(uint16_t pktmax)
{
	serial_rx.pktmax = pktmax;
	serial_rx.pkttail = 0;

	serial_rxpkt_queue.tail = 0;
	serial_rxpkt_queue.head = 0;

	serial_dma_next_recv((uint32_t)(&serial_rx.buf[0]),pktmax);
}


int serial_busy(void)
{
	return (LL_DMA_IsEnabledChannel(DMAx,DMA_TX_CHx));
}



/**
	* @brief    serial_rxpkt_queue_in console 串口接收数据包队列入列
	* @param    
	* @return   空
*/
static inline void serial_rxpkt_queue_in(char * pkt ,uint16_t len)
{
	serial_rxpkt_queue.tail = (serial_rxpkt_queue.tail + 1) % HAL_RX_PACKET_SIZE;
	serial_rxpkt_queue.pktbuf[serial_rxpkt_queue.tail] = pkt;
	serial_rxpkt_queue.pktlen[serial_rxpkt_queue.tail] = len;
}


/**
	* @brief    serial_rxpkt_queue_out console 串口队列出队
	* @param    rxpktqueue
	* @return   空
*/
int serial_rxpkt_queue_out(char ** data,uint16_t * len)
{
	if (serial_rxpkt_queue.tail != serial_rxpkt_queue.head)
	{
		serial_rxpkt_queue.head = (serial_rxpkt_queue.head + 1) % HAL_RX_PACKET_SIZE;
		*data = serial_rxpkt_queue.pktbuf[serial_rxpkt_queue.head];
		*len  = serial_rxpkt_queue.pktlen[serial_rxpkt_queue.head];
		return 1;
	}
	else
	{
		*len = 0;
		return 0;
	}
}


/**
	* @brief    hal_usart_puts console 硬件层输出
	* @param    空
	* @return   空
*/
void serial_puts(char * const buf,uint16_t len)
{
	uint32_t pkttail = serial_tx.pkttail;   //先获取当前尾部地址
	
	if (pkttail < HAL_TX_BUF_SIZE && len)	//当前发送缓存有空间
	{
		uint32_t remain  = HAL_TX_BUF_SIZE - pkttail;
		uint32_t pktsize = (remain > len) ? len : remain;
		
		memcpy(&serial_tx.buf[pkttail] , buf , pktsize);//把数据包拷到缓存区中
		
		pkttail += pktsize;
		
		serial_tx.pkttail = pkttail;  //更新尾部
		serial_tx.pktsize += pktsize;//设置当前包大小

		//开始发送
		if (!LL_DMA_IsEnabledChannel(DMAx,DMA_TX_CHx))//
			serial_send_pkt();
	}
}



//------------------------------以下为一些中断处理------------------------------
//#include "cmsis_os.h"//用了freertos 打开

#ifdef _CMSIS_OS_H
	extern osSemaphoreId osSerialRxSemHandle;
#endif


/**
	* @brief    DMA_TX_IRQ_HANDLER console 串口发送一包数据完成中断
	* @param    空
	* @return   空
*/
void DMA_TX_IRQ_HANDLER(void) 
{
//	LL_USART_ClearFlag_TC(USARTx); //清除空闲中断
	LL_DMA_DisableChannel(DMAx,DMA_TX_CHx);
	DMA_TX_CLEAR_FLAG();

	if (serial_tx.pktsize == 0) //发送完此包后无数据，复位缓冲区
		serial_tx.pkttail = 0;
	else                        //还有数据则继续发送
		serial_send_pkt(); 	
}


/**
	* @brief    DMA_RX_IRQ_HANDLER console 串口接收满中断
	* @param    空
	* @return   空
*/
void DMA_RX_IRQ_HANDLER(void) 
{
	DMA_RX_CLEAR_FLAG();
	
	serial_rxpkt_queue_in(&serial_rx.buf[serial_rx.pkttail],serial_rx.pktmax); //把当前包地址和大小送入缓冲队列
	
	serial_rx.pkttail += serial_rx.pktmax ; //更新缓冲地址
	
	if (serial_rx.pkttail + serial_rx.pktmax > HAL_RX_BUF_SIZE) //如果剩余空间不足以缓存最大包长度，从 0 开始
		serial_rx.pkttail = 0;
	
	serial_dma_next_recv((uint32_t)(&serial_rx.buf[serial_rx.pkttail]),serial_rx.pktmax);//设置缓冲地址和最大包长度

	#ifdef _CMSIS_OS_H	
		osSemaphoreRelease(osSerialRxSemHandle);// 释放信号量
	#endif

}



/**
	* @brief    USART_IRQ_HANDLER 串口中断函数，只有空闲中断
	* @param    空
	* @return   空
*/
void USART_IRQ_HANDLER(void) 
{
	uint16_t pkt_len ;
	
	LL_USART_ClearFlag_IDLE(USARTx); //清除空闲中断
	
	pkt_len = serial_rx.pktmax - LL_DMA_GetDataLength(DMAx,DMA_RX_CHx);//得到当前包的长度
	
	if (pkt_len)
	{
		serial_rxpkt_queue_in(&(serial_rx.buf[serial_rx.pkttail]),pkt_len); //把当前包送入缓冲队列，交由应用层处理
	
		serial_rx.pkttail += pkt_len ;	 //更新缓冲地址
		if (serial_rx.pkttail + serial_rx.pktmax > HAL_RX_BUF_SIZE)//如果剩余空间不足以缓存最大包长度，从 0 开始
			serial_rx.pkttail = 0;

		serial_dma_next_recv((uint32_t)&(serial_rx.buf[serial_rx.pkttail]),serial_rx.pktmax);//设置缓冲地址和最大包长度

		#ifdef _CMSIS_OS_H
			osSemaphoreRelease(osSerialRxSemHandle);// 释放信号量
		#endif
	}
}


//------------------------------华丽的分割线------------------------------
/**
	* @brief    hal_serial_init console 硬件层初始化
	* @param    空
	* @return   空
*/
void hal_serial_init(void)
{
	//引脚初始化
	usart_gpio_init();
	usart_base_init();
	usart_dma_init();

	serial_tx.pkttail = 0;
	serial_tx.pktsize = 0;
	
	serial_recv_reset(COMMANDLINE_MAX_LEN);
}


void hal_serial_deinit(void)
{
	NVIC_DisableIRQ(DMA_TX_IRQn);
	NVIC_DisableIRQ(DMA_RX_IRQn);
	NVIC_DisableIRQ(USARTIRQn);
	
	LL_DMA_DisableIT_TC(DMAx,DMA_TX_CHx);
	LL_DMA_DisableIT_TC(DMAx,DMA_RX_CHx);	
	
	LL_USART_DisableDMAReq_RX(USARTx);
	LL_USART_DisableDMAReq_TX(USARTx);

	LL_USART_Disable(USARTx);	
}



