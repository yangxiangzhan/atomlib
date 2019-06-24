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
#include "stm32f4xx.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_ll_bus.h"
#include "stm32f4xx_ll_gpio.h"
#include "stm32f4xx_ll_usart.h"
#include "stm32f4xx_ll_dma.h"
#include "serial_hal.h"
#include "shell.h"



//---------------------HAL层相关--------------------------
// 如果要对硬件进行移植修改，修改下列宏，并提供引脚初始化程序
// --- 不用 dma 时，优化等级在 -O3 以上会有问题，原因未明 ---
//#include "cmsis_os.h"//用了freertos 打开

#ifdef _CMSIS_OS_H
	extern  osSemaphoreId osSerialRxSemHandle;
	#define RXPKT_SAMPH   osSerialRxSemHandle
#endif

#define USART_BAUDRATE          115200   //波特率
#define USART_DMA_CLOCK_INIT()  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_DMA1)//跟 DMAx 对应

#define xUSART                  1  //引用串口号

#if        (xUSART == 1) //串口1对应DMA
#	define xDMA                 0  //2  //对应DMA，设为 0 时不开启 dma 功能
#	define xDMATxCH             4  //发送对应 DMA 通道号
#	define xDMARxCH             4  //接收对应 DMA 通道号
#	define xDMATxStream         7
#	define xDMARxStream         2
#endif

//---------------------HAL层宏替换--------------------------
#define USART_X(x)	            USART##x
#define USART_x(x)	            USART_X(x)

#define USART_IRQNUM(x)         USART##x##_IRQn
#define USARTx_IRQNUM(x)        USART_IRQNUM(x)

#define USART_IRQFN(x)          USART##x##_IRQHandler
#define USARTx_IRQFN(x)         USART_IRQFN(x)

#define DMA_X(x)	            DMA##x
#define DMA_x(x)	            DMA_X(x)

#define DMA_Channel(x)          LL_DMA_CHANNEL_##x
#define DMA_Channelx(x)         DMA_Channel(x)

#define DMA_STRM(x)             LL_DMA_STREAM_##x
#define DMA_STRMx(x)            DMA_STRM(x)

#define DMA_STRM_IRQNUM(x,y)    DMA##x##_Stream##y##_IRQn
#define DMAx_STRMy_IRQNUM(x,y)  DMA_STRM_IRQNUM(x,y)

#define DMA_STRM_IRQFN(x,y)     DMA##x##_Stream##y##_IRQHandler
#define DMAx_STRMy_IRQFN(x,y)   DMA_STRM_IRQFN(x,y)

#define DMA_STRM_TCFLAG(x,y)    LL_DMA_IsActiveFlag_TC##y(DMA##x)
#define DMAx_STRMy_TCFLAG(x,y)  DMA_STRM_TCFLAG(x,y)

#define DMA_STRM_TCCLEAR(x,y)   LL_DMA_ClearFlag_TC##y(DMA##x)
#define DMAx_STRMy_TCCLEAR(x,y) DMA_STRM_TCCLEAR(x,y)

#define DMA0                    ((DMA_TypeDef *)0)
#define DMAx                    DMA_x(xDMA)     //串口所在 dma 总线
#define USARTx                  USART_x(xUSART)   //引用串口
#define USART_IRQn              USARTx_IRQNUM(xUSART) //中断号
#define USART_IRQ_HANDLER       USARTx_IRQFN(xUSART) //中断函数名

#define DMA_TX_CHx              DMA_Channelx(xDMATxCH)    //串口发送 dma 通道
#define DMA_TX_STRMx            DMA_STRMx(xDMATxStream)
#define DMA_TX_IRQn             DMAx_STRMy_IRQNUM(xDMA,xDMATxStream)
#define DMA_TX_IRQ_HANDLER      DMAx_STRMy_IRQFN(xDMA,xDMATxStream) //中断函数名
#define DMA_TX_CLEAR_FLAG()     DMAx_STRMy_TCCLEAR(xDMA,xDMATxStream)
#define DMA_TX_COMPLETE_FLAG()  DMAx_STRMy_TCFLAG(xDMA,xDMATxStream)

#define DMA_RX_CHx              DMA_Channelx(xDMARxCH)
#define DMA_RX_STRMx            DMA_STRMx(xDMARxStream)
#define DMA_RX_IRQn             DMAx_STRMy_IRQNUM(xDMA,xDMARxStream)
#define DMA_RX_IRQ_HANDLER      DMAx_STRMy_IRQFN(xDMA,xDMARxStream) //中断函数名
#define DMA_RX_CLEAR_FLAG()     DMAx_STRMy_TCCLEAR(xDMA,xDMARxStream)
#define DMA_RX_COMPLETE_FLAG()  DMAx_STRMy_TCFLAG(xDMA,xDMARxStream)


//---------------------------------------------------------


static struct _serialtx
{
	volatile uint16_t pkttail ;
	volatile uint16_t pktsize ;
	char buf[HAL_TX_BUF_SIZE];
}
serialtx = {0};


static struct _serial_rx
{
	volatile uint16_t pkttail;
	volatile uint16_t pktmax;
	char buf[HAL_RX_BUF_SIZE];
}
serialrx = {0};


static struct _serial_queue
{
	uint16_t tail ;
	uint16_t head ;

	uint16_t  pktlen[HAL_RX_PACKET_SIZE];
	char    * pktbuf[HAL_RX_PACKET_SIZE];
}
serialpktqueue  = {0};




static void usart_gpio_init(void)
{
	LL_GPIO_InitTypeDef GPIO_InitStruct;
	/* Peripheral clock enable */
	LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_USART1);

	#if 0
		/**USART1 GPIO Configuration
		PA9   ------> USART1_TX
		PA10   ------> USART1_RX
		*/
		GPIO_InitStruct.Pin = LL_GPIO_PIN_9;
		GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
		GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
		GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
		GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
		GPIO_InitStruct.Alternate = LL_GPIO_AF_7;
		LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

		GPIO_InitStruct.Pin = LL_GPIO_PIN_10;
		GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
		GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
		GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
		GPIO_InitStruct.Pull = LL_GPIO_PULL_UP;
		GPIO_InitStruct.Alternate = LL_GPIO_AF_7;
		LL_GPIO_Init(GPIOA, &GPIO_InitStruct);
	#else
		/**USART1 GPIO Configuration
		PB6   ------> USART1_TX
		PB7   ------> USART1_RX
		*/
		GPIO_InitStruct.Pin = LL_GPIO_PIN_6;
		GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
		GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
		GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
		GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
		GPIO_InitStruct.Alternate = LL_GPIO_AF_7;
		LL_GPIO_Init(GPIOB, &GPIO_InitStruct);

		GPIO_InitStruct.Pin = LL_GPIO_PIN_7;
		GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
		GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
		GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
		GPIO_InitStruct.Pull = LL_GPIO_PULL_UP;
		GPIO_InitStruct.Alternate = LL_GPIO_AF_7;
		LL_GPIO_Init(GPIOB, &GPIO_InitStruct);
	#endif
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

	NVIC_SetPriority(USART_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),7, 0));
	NVIC_EnableIRQ(USART_IRQn);

#if (xDMA)
	LL_USART_EnableDMAReq_RX(USARTx);
	LL_USART_EnableDMAReq_TX(USARTx);
	LL_USART_DisableIT_RXNE(USARTx);
	LL_USART_DisableIT_PE(USARTx);
#else
	LL_USART_EnableIT_RXNE(USARTx);
	LL_USART_EnableIT_TC(USARTx);
#endif

	LL_USART_EnableIT_IDLE(USARTx);
	LL_USART_Enable(USARTx);
}


#if xDMA
/**
	* @brief usart_dma_init 控制台 DMA 初始化
	* @param void
	* @return NULL
*/
static void usart_dma_init(void)
{
	USART_DMA_CLOCK_INIT();

	/* USART_RX Init */  /* USART_RX Init */
	LL_DMA_SetChannelSelection(DMAx, DMA_RX_STRMx, DMA_RX_CHx);
	LL_DMA_SetDataTransferDirection(DMAx, DMA_RX_STRMx, LL_DMA_DIRECTION_PERIPH_TO_MEMORY);
	LL_DMA_SetStreamPriorityLevel(DMAx, DMA_RX_STRMx, LL_DMA_PRIORITY_MEDIUM);
	LL_DMA_SetMode(DMAx, DMA_RX_STRMx, LL_DMA_MODE_NORMAL);
	LL_DMA_SetPeriphIncMode(DMAx, DMA_RX_STRMx, LL_DMA_PERIPH_NOINCREMENT);
	LL_DMA_SetMemoryIncMode(DMAx, DMA_RX_STRMx, LL_DMA_MEMORY_INCREMENT);
	LL_DMA_SetPeriphSize(DMAx, DMA_RX_STRMx, LL_DMA_PDATAALIGN_BYTE);
	LL_DMA_SetMemorySize(DMAx, DMA_RX_STRMx, LL_DMA_MDATAALIGN_BYTE);
	LL_DMA_SetPeriphAddress(DMAx,DMA_RX_STRMx,LL_USART_DMA_GetRegAddr(USARTx));
	LL_DMA_DisableFifoMode(DMAx, DMA_RX_STRMx);

	/* USART_TX Init */
	LL_DMA_SetChannelSelection(DMAx, DMA_TX_STRMx, DMA_TX_CHx);
	LL_DMA_SetDataTransferDirection(DMAx, DMA_TX_STRMx, LL_DMA_DIRECTION_MEMORY_TO_PERIPH);
	LL_DMA_SetStreamPriorityLevel(DMAx, DMA_TX_STRMx, LL_DMA_PRIORITY_MEDIUM);
	LL_DMA_SetMode(DMAx, DMA_TX_STRMx, LL_DMA_MODE_NORMAL);
	LL_DMA_SetPeriphIncMode(DMAx, DMA_TX_STRMx, LL_DMA_PERIPH_NOINCREMENT);
	LL_DMA_SetMemoryIncMode(DMAx, DMA_TX_STRMx, LL_DMA_MEMORY_INCREMENT);
	LL_DMA_SetPeriphSize(DMAx, DMA_TX_STRMx, LL_DMA_PDATAALIGN_BYTE);
	LL_DMA_SetMemorySize(DMAx, DMA_TX_STRMx, LL_DMA_MDATAALIGN_BYTE);
	LL_DMA_SetPeriphAddress(DMAx,DMA_TX_STRMx,LL_USART_DMA_GetRegAddr(USARTx));
	LL_DMA_DisableFifoMode(DMAx, DMA_TX_STRMx);

	LL_DMA_DisableStream(DMAx,DMA_TX_STRMx);//发送暂不使能
	LL_DMA_DisableStream(DMAx,DMA_RX_STRMx);//发送暂不使能

	DMA_TX_CLEAR_FLAG();
	DMA_RX_CLEAR_FLAG();

	  /* DMA interrupt init 中断*/
	NVIC_SetPriority(DMA_TX_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),6, 0));
	NVIC_EnableIRQ(DMA_TX_IRQn);

	NVIC_SetPriority(DMA_RX_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),6, 0));
	NVIC_EnableIRQ(DMA_RX_IRQn);

	LL_DMA_EnableIT_TC(DMAx,DMA_TX_STRMx);
	LL_DMA_EnableIT_TC(DMAx,DMA_RX_STRMx);
}

/**
	* @brief    设置 console 硬件接收缓存区，同时会清除接收标志位
	* @param    空
	* @return
*/
static inline void serial_dma_next_recv( uint32_t memory_addr ,uint16_t dma_max_len)
{
	LL_DMA_DisableIT_TC(DMAx,DMA_RX_STRMx);
	LL_DMA_DisableStream(DMAx,DMA_RX_STRMx);//发送暂不使能

	DMA_RX_CLEAR_FLAG();

	LL_DMA_SetMemoryAddress(DMAx,DMA_RX_STRMx,memory_addr);
	LL_DMA_SetDataLength(DMAx,DMA_RX_STRMx,dma_max_len);

//	LL_DMA_EnableStream(DMAx,DMA_RX_STRMx);
	LL_DMA_EnableIT_TC(DMAx,DMA_RX_STRMx);
	LL_DMA_EnableStream(DMAx,DMA_RX_STRMx);//new
}

/**
  * @brief    console 启动发送当前包
  * @param    空
  * @retval   空
  */
static inline void serial_send_pkt(void)
{
	uint32_t pkt_size = serialtx.pktsize ;
	uint32_t pkt_head  = serialtx.pkttail - pkt_size ;

	serialtx.pktsize = 0;

	LL_DMA_SetMemoryAddress(DMAx,DMA_TX_STRMx,(uint32_t)(&serialtx.buf[pkt_head]));
	LL_DMA_SetDataLength(DMAx,DMA_TX_STRMx,pkt_size);
	LL_DMA_EnableStream(DMAx,DMA_TX_STRMx);
}


/**
	* @brief    hal_usart_puts console 硬件层输出
	* @param    空
	* @return   空
*/
void serial_puts(char * const buf,uint16_t len)
{
	uint32_t pkttail = serialtx.pkttail;   //先获取当前尾部地址

	if (pkttail < HAL_TX_BUF_SIZE && len)	//当前发送缓存有空间
	{
		uint32_t remain  = HAL_TX_BUF_SIZE - pkttail;
		uint32_t pktsize = (remain > len) ? len : remain;

		memcpy(&serialtx.buf[pkttail] , buf , pktsize);//把数据包拷到缓存区中

		pkttail += pktsize;

		serialtx.pkttail = pkttail;  //更新尾部
		serialtx.pktsize += pktsize;//设置当前包大小

		if (!LL_DMA_IsEnabledStream(DMAx,DMA_TX_STRMx))//开始发送
			serial_send_pkt();
	}
}


int serial_busy(void)
{
	return (LL_DMA_IsEnabledStream(DMAx,DMA_TX_STRMx));
}

#else

#define usart_dma_init()
#define serial_dma_next_recv(x,y)
#define serial_send_pkt()

/**
	* @brief    hal_usart_puts console 硬件层输出
	* @param    空
	* @return   空
*/
void serial_puts(char * const buf,uint16_t len)
{
	volatile uint32_t pkttail = serialtx.pkttail;   //先获取当前尾部地址

	if (pkttail < HAL_TX_BUF_SIZE && len)	//当前发送缓存有空间
	{
		uint32_t remain  = HAL_TX_BUF_SIZE - pkttail;
		uint32_t pktsize = (remain > len) ? len : remain;
		char * copyto = (char *)&(serialtx.buf[pkttail]);
		char * from = (char*)buf;
		for (char * end = from + pktsize ; from < end ; *copyto++ = *from++) ;
		//memcpy(&serialtx.buf[pkttail] , buf , pktsize);//把数据包拷到缓存区中

		if (0 == serialtx.pktsize)
			LL_USART_TransmitData8(USARTx,(uint8_t)(serialtx.buf[pkttail]));

		serialtx.pkttail = pktsize + pkttail;  //更新尾部
		serialtx.pktsize += pktsize;//设置当前包大小
	}
}



int serial_busy(void)
{
	return serialtx.pktsize != 0 ;
}

#endif




/**
  * @brief    serial_rxpkt_max_len 设置硬件接收最大包
  * @param    空
  * @retval   空
  */
void serial_recv_reset(uint16_t pktmax)
{
	serialrx.pktmax = pktmax;
	serialrx.pkttail = 0;

	serialpktqueue.tail = 0;
	serialpktqueue.head = 0;

#if (xDMA)
	serial_dma_next_recv((uint32_t)(&serialrx.buf[0]),pktmax);
#endif
}




/**
	* @brief    serial_rxpkt_queue_in console 串口接收数据包队列入列
	* @param
	* @return   空
*/
static inline void serial_rxpkt_queue_in(char * pkt ,uint16_t len)
{
	serialpktqueue.tail = (serialpktqueue.tail + 1) % HAL_RX_PACKET_SIZE;
	serialpktqueue.pktbuf[serialpktqueue.tail] = pkt;
	serialpktqueue.pktlen[serialpktqueue.tail] = len;
}


/**
	* @brief    serial_rxpkt_queue_out console 串口队列出队
	* @param    rxpktqueue
	* @return   空
*/
int serial_rxpkt_queue_out(char ** data,uint16_t * len)
{
	if (serialpktqueue.tail != serialpktqueue.head)
	{
		serialpktqueue.head = (serialpktqueue.head + 1) % HAL_RX_PACKET_SIZE;
		*data = serialpktqueue.pktbuf[serialpktqueue.head];
		*len  = serialpktqueue.pktlen[serialpktqueue.head];
		return 1;
	}
	else
	{
		*len = 0;
		return 0;
	}
}



//------------------------------以下为一些中断处理------------------------------



/**
	* @brief    DMA_TX_IRQ_HANDLER console 串口发送一包数据完成中断
	* @param    空
	* @return   空
*/
void DMA_TX_IRQ_HANDLER(void)
{
	LL_DMA_DisableStream(DMAx,DMA_TX_STRMx);
	DMA_TX_CLEAR_FLAG();

	if (serialtx.pktsize == 0) //发送完此包后无数据，复位缓冲区
		serialtx.pkttail = 0;
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

	serial_rxpkt_queue_in(&serialrx.buf[serialrx.pkttail],serialrx.pktmax); //把当前包地址和大小送入缓冲队列

	serialrx.pkttail += serialrx.pktmax ; //更新缓冲地址

	if (serialrx.pkttail + serialrx.pktmax > HAL_RX_BUF_SIZE) //如果剩余空间不足以缓存最大包长度，从 0 开始
		serialrx.pkttail = 0;

	serial_dma_next_recv((uint32_t)(&serialrx.buf[serialrx.pkttail]),serialrx.pktmax);//设置缓冲地址和最大包长度

	#ifdef _CMSIS_OS_H
		osSemaphoreRelease(RXPKT_SAMPH);// 释放信号量
	#endif

}



/**
	* @brief    USART_IRQ_HANDLER 串口中断函数，只有空闲中断
	* @param    空
	* @return   空
*/
#if xDMA
void USART_IRQ_HANDLER(void)
{
	uint16_t pkt_len ;

	LL_USART_ClearFlag_IDLE(USARTx); //清除空闲中断

	pkt_len = serialrx.pktmax - LL_DMA_GetDataLength(DMAx,DMA_RX_STRMx);//得到当前包的长度

	if (pkt_len)
	{
		serial_rxpkt_queue_in(&(serialrx.buf[serialrx.pkttail]),pkt_len); //把当前包送入缓冲队列，交由应用层处理

		serialrx.pkttail += pkt_len ;	 //更新缓冲地址
		if (serialrx.pkttail + serialrx.pktmax > HAL_RX_BUF_SIZE)//如果剩余空间不足以缓存最大包长度，从 0 开始
			serialrx.pkttail = 0;

		serial_dma_next_recv((uint32_t)&(serialrx.buf[serialrx.pkttail]),serialrx.pktmax);//设置缓冲地址和最大包长度

		#ifdef _CMSIS_OS_H
			osSemaphoreRelease(RXPKT_SAMPH);// 释放信号量
		#endif
	}
}
#else //不用 dma 时的串口中断

void USART_IRQ_HANDLER(void)
{
	static volatile uint16_t rxtail = 0;
	static volatile uint16_t txhead = 0;

	if (LL_USART_IsActiveFlag_RXNE(USARTx)) {//接收单个字节中断
		LL_USART_ClearFlag_RXNE(USARTx); //清除中断

		serialrx.buf[rxtail++] = LL_USART_ReceiveData8(USARTx);//数据存入内存

		if (rxtail - serialrx.pkttail >= serialrx.pktmax ) {//接收到最大包长度
			serial_rxpkt_queue_in(&serialrx.buf[serialrx.pkttail],serialrx.pktmax); //把当前包地址和大小送入缓冲队列
			serialrx.pkttail += serialrx.pktmax ; //更新缓冲尾部

			 //如果剩余空间不足以缓存最大包长度，从 0 开始
			if (serialrx.pkttail + serialrx.pktmax > HAL_RX_BUF_SIZE){
				serialrx.pkttail = 0;
				rxtail = 0;
			}
			#ifdef _CMSIS_OS_H
				osSemaphoreRelease(RXPKT_SAMPH);// 释放信号量
			#endif
		} //if (rxtail - serialrx.pkttail >= serialrx.pktmax ) {//接收到最大包长度
	}

	if (LL_USART_IsActiveFlag_IDLE(USARTx)) { //空闲中断
		LL_USART_ClearFlag_IDLE(USARTx); //清除空闲中断

		uint16_t pkt_len = rxtail - serialrx.pkttail;//空闲中断长度
		if (pkt_len) { // 如果刚好为 pktmax 长度，在 RXNE 已处理
			serial_rxpkt_queue_in(&(serialrx.buf[serialrx.pkttail]),pkt_len); //把当前包送入缓冲队列，交由应用层处理
			serialrx.pkttail += pkt_len ;	 //更新缓冲地址

			//如果剩余空间不足以缓存最大包长度，从 0 开始
			if (serialrx.pkttail + serialrx.pktmax > HAL_RX_BUF_SIZE){
				rxtail = 0;
				serialrx.pkttail = 0;
			}
			#ifdef _CMSIS_OS_H
				osSemaphoreRelease(RXPKT_SAMPH);// 释放信号量
			#endif
		} //if (pkt_len)
	}

	if (LL_USART_IsActiveFlag_TC(USARTx)) { //发送中断
		LL_USART_ClearFlag_TC(USARTx); //清除中断
		txhead = (0==txhead)?(serialtx.pkttail-serialtx.pktsize+1):(1+txhead);
		if (txhead < serialtx.pkttail) { //如果未发送玩当前数据，发送下一个字节
			LL_USART_TransmitData8(USARTx,(uint8_t)(serialtx.buf[txhead]));
		} else {
			serialtx.pktsize = 0 ;
			serialtx.pkttail = 0 ;
			txhead = 0;
		}
	}
}
#endif

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

	serialtx.pkttail = 0;
	serialtx.pktsize = 0;

	serial_recv_reset(HAL_RX_BUF_SIZE/2);
}


void hal_serial_deinit(void)
{
	NVIC_DisableIRQ(USART_IRQn);
	LL_USART_Disable(USARTx);
	LL_USART_DisableIT_IDLE(USARTx);

#if (xDMA)
	LL_DMA_DisableIT_TC(DMAx,DMA_TX_STRMx);
	LL_DMA_DisableIT_TC(DMAx,DMA_RX_STRMx);
	LL_USART_DisableDMAReq_RX(USARTx);
	LL_USART_DisableDMAReq_TX(USARTx);
	NVIC_DisableIRQ(DMA_TX_IRQn);
	NVIC_DisableIRQ(DMA_RX_IRQn);
#else
	LL_USART_DisableIT_RXNE(USARTx);
	LL_USART_DisableIT_TC(USARTx);
#endif
}


