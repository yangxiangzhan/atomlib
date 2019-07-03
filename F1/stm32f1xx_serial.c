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
#include <stdint.h>

#include "stm32f1xx_ll_bus.h"
#include "stm32f1xx_ll_usart.h"
#include "stm32f1xx_ll_dma.h"

#include "stm32f1xx_serial.h"
#include "stm32f1xx_serial_set.h"


static inline void serial_send_pkt(serial_t * ttySx);


#if USE_USART1  // 如果启用串口
	#define USARTn                  1    ///< 引用串口号
	#define DMAn                    1    ///< 对应 DMA ，设为 0 时不开启 dma 功能
	#define DMATxCHn                4    ///< 发送对应 DMA 通道号
	#define DMARxCHn                5    ///< 接收对应 DMA 通道号
	
	#define DMARxEnable             1    ///< 为 1 时启用 dma 接收
	#define DMATxEnable             1    ///< 为 1 时启用 dma 发送
	#define RX_BUF_SIZE             256  ///< 硬件接收缓冲区
	#define TX_BUF_SIZE             512  ///< 硬件发送缓冲区
	
	#define IRQnPRIORITY            7    ///< 串口相关中断的优先级
	#define DMAxPRIORITY            6    ///< dma 相关中断的优先级
	#define USART_DMA_CLOCK_INIT()  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_DMA1)//跟 DMAx 对应

	// 这个头文件仅供此 .c 文件内部使用，此头文件展开会生成串口对应的中断函数和硬件初始化函数
	#include "stm32f1xx_serial_func.h"
#endif


#if USE_USART3  // 如果启用串口
	#define USARTn                  3    ///< 引用串口号
	#define DMAn                    1    ///< 对应 DMA ，设为 0 时不开启 dma 功能
	#define DMATxCHn                2    ///< 发送对应 DMA 通道号
	#define DMARxCHn                3    ///< 接收对应 DMA 通道号
	
	#define DMARxEnable             1    ///< 为 1 时启用 dma 接收
	#define DMATxEnable             1    ///< 为 1 时启用 dma 发送
	#define RX_BUF_SIZE             256  ///< 硬件接收缓冲区
	#define TX_BUF_SIZE             512  ///< 硬件发送缓冲区
	
	#define IRQnPRIORITY            7    ///< 串口相关中断的优先级
	#define DMAxPRIORITY            6    ///< dma 相关中断的优先级
	#define USART_DMA_CLOCK_INIT()  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_DMA1)//跟 DMAx 对应

	// 这个头文件仅供此 .c 文件内部使用，此头文件展开会生成串口对应的中断函数和硬件初始化函数
	#include "stm32f1xx_serial_func.h"
#endif


/**
  * @brief    串口启动发送当前包
  * @param    ttySx : 串口设备
  * @retval   空
*/
static inline void serial_send_pkt(serial_t * ttySx)
{
	uint32_t pkt_size = ttySx->txsize ;
	uint32_t pkt_head = ttySx->txtail - pkt_size ;
	ttySx->txsize = 0;
	LL_DMA_SetMemoryAddress(ttySx->dma,ttySx->dma_tx,(uint32_t)(&ttySx->txbuf[pkt_head]));
	LL_DMA_SetDataLength(ttySx->dma,ttySx->dma_tx,pkt_size);
	LL_DMA_EnableChannel(ttySx->dma,ttySx->dma_tx);
}


/**
  * @brief    串口设备发送一包数据
  * @param    ttySx   : 串口设备
  * @param    data    : 需要发送的数据
  * @param    datalen : 需要发送的数据长度
  * @return   实际从硬件发送出去的数据长度
*/
int serial_write(serial_t *ttySx , const void * data , int datalen, int block )
{
	if (!ttySx->hal)    // 未初始化的设备 ，返回 -1
		return -1;

	if (O_NOBLOCK != block) {                  // 如果是阻塞发送，用最简单的方式发送
		uint8_t* databuf = (uint8_t *)data; 
		//while(LL_USART_IsEnabledIT_TC(ttySx->hal)) ;// 等待缓冲区全部输出
		while(ttySx->txtail) ;
		for (int i = 0 ; i < datalen ; ++i) {
			LL_USART_TransmitData8(ttySx->hal,databuf[i]);
			while(!LL_USART_IsActiveFlag_TXE(ttySx->hal));
		}
		return datalen;
	}

	int pkttail = ttySx->txtail;   //先获取当前尾部地址
	int remain  = ttySx->txmax - pkttail;
	int pktsize = (remain > datalen) ? datalen : remain;

	if (remain && datalen) {//当前发送缓存有空间
		MEMCPY(&ttySx->txbuf[pkttail] , data , pktsize);//把数据包拷到缓存区中
		if (ttySx->flag & FLAG_TX_DMA) {                // dma 模式下发送
			ttySx->txtail  = pkttail + pktsize;         // 更新尾部
			ttySx->txsize += pktsize;                   // 设置当前包大小
			if (!LL_DMA_IsEnabledChannel(ttySx->dma,ttySx->dma_tx))//开始发送
				serial_send_pkt(ttySx);
		}
		else  {
			if (!ttySx->txtail){
				LL_USART_TransmitData8(ttySx->hal,(uint8_t)(ttySx->txbuf[pkttail]));
				LL_USART_EnableIT_TC(ttySx->hal);
			}
			ttySx->txtail  = pkttail + pktsize;  // 更新尾部
			ttySx->txsize += pktsize;            // 设置当前包大小
		}
	}

	return pktsize;
}

/**
  * @brief    直接读取串口设备的接收缓存区数据
  * @param    ttySx   : 串口设备
  * @param    databuf : 返回串口设备接收缓存数据包首地址
  * @return   接收缓存数据包长度
*/
int serial_gets(serial_t *ttySx ,char ** databuf, int block )
{
	int datalen ;
	
	if (!ttySx->hal)    // 未初始化的设备 ，返回 -1
		return -1;

	if ( ttySx->rxread == ttySx->rxtail ) {
		if ( O_NOBLOCK != block ) {
			ttySx->flag |= FLAG_RX_BLOCK ;    // 标记当前设备为正在阻塞读
			OS_SEM_WAIT(ttySx->rxsem) ;       // 如果是阻塞读，等待信号量
			ttySx->flag &= ~FLAG_RX_BLOCK ;   // 清楚阻塞读标志
			if (!ttySx->hal) {                // 判断是否为退出信号
				OS_SEM_DEINIT(ttySx->rxsem);  // 退出时去初始化信号量
				return -1;                    // 返回错误
			}
		}
		else {
			return 0 ; // 非阻塞下无数据直接 return 0;
		}
	}

	ttySx->rx_lock();
	// 接收到数据
	if (ttySx->rxtail < ttySx->rxread) {        // 接收到数据，且数据包是在缓冲区最后一段
		datalen  = ttySx->rxend - ttySx->rxread ; 
		*databuf = &ttySx->rxbuf[ttySx->rxread];   // 数据缓冲区
		ttySx->rxread = 0 ;
		ttySx->rxend = 0 ;
	}
	else { 
		datalen = ttySx->rxtail - ttySx->rxread;// 当前包数据大小
		*databuf = &ttySx->rxbuf[ttySx->rxread];// 数据缓冲区
		ttySx->rxread = ttySx->rxtail ;         // 数据读更新
	}
	ttySx->rx_unlock();
	
	return datalen ;
}

/**
  * @brief    读取串口设备接收，存于 databuf 中
  * @param    ttySx       : 串口设备
  * @param    databuf     : 读取数据内存
  * @param    databufsize : 读取数据内存总大小
  * @return   接收数据包长度
*/
int  serial_read(serial_t * ttySx ,void * databuf , int bufsize, int block )
{
	int datalen , readrx = ttySx->rxread;
	
	if (!ttySx->hal)    // 未初始化的设备 ，返回 -1
		return -1;

	if (readrx == ttySx->rxtail) { // 接收缓冲区中无接收数据
		if ( O_NOBLOCK != block ) {
			ttySx->flag |= FLAG_RX_BLOCK ;    // 标记当前设备为正在阻塞读
			OS_SEM_WAIT(ttySx->rxsem) ;       // 如果是阻塞读，等待信号量
			ttySx->flag &= ~FLAG_RX_BLOCK ;   // 清楚阻塞读标志
			if (!ttySx->hal) {                // 判断是否为退出信号
				OS_SEM_DEINIT(ttySx->rxsem);  // 退出时去初始化信号量
				return -1;                    // 返回错误
			}
		}
		else {
			return 0 ; // 非阻塞下无数据直接 return 0;
		}
	}

	
	ttySx->rx_lock(); 
	if (readrx > ttySx->rxtail) {        // 接收到数据
		if (ttySx->rxend > bufsize) {
			datalen = bufsize ; 
			ttySx->rxread += bufsize ;         // 数据读更新
			ttySx->rxend  -= bufsize;
		}
		else {
			datalen  = ttySx->rxend ; 
			ttySx->rxread = 0 ;
			ttySx->rxend = 0 ;
		}
	}
	else { // 接收到数据，且数据包是在缓冲区最后一段
		datalen = ttySx->rxtail - ttySx->rxread;// 当前包数据大小 
		if (datalen > bufsize)
			datalen = bufsize;
		ttySx->rxread += datalen ;              // 数据读更新
	}
	ttySx->rx_unlock();
	
	MEMCPY(databuf,&ttySx->rxbuf[readrx],datalen);
	return datalen ;
}


/**
  * @brief    打开串口设备，并进行初始化
  * @param    ttySx       : 串口设备
  * @param    speed       : 串口波特率
  * @param    bits        : 数据位宽，一般为 8
  * @param    event       : 奇偶校验，'N'/'E'/'O'：无/偶/奇
  * @param    stop        : 停止位，0.5/1.0/1.5/2.0
  * @return   接收数据包长度
*/
int serial_open(serial_t *ttySx , int speed, int bits, char event, float stop) 
{
	OS_SEM_INIT(ttySx->rxsem);
//	ttySx->txblock = txblock ;
//	ttySx->rxblock = rxblock ; 
//	ttySx->txblock = 0 ;
//	ttySx->rxblock = 0 ; 
	ttySx->txtail = 0;
	ttySx->txsize = 0;
	ttySx->rxtail = 0;
	ttySx->rxend  = 0;
	ttySx->rxread = 0;
	ttySx->flag   = 0;

	uint32_t nevent , nbits  , nstop; 

	// 数据宽度，一般为 8
	nbits = (bits == 9) ? LL_USART_DATAWIDTH_9B : LL_USART_DATAWIDTH_8B;

	// 奇偶校验
	if (event == 'E')
		nevent = LL_USART_PARITY_EVEN ;
	else 
	if (event == 'O')
		nevent = LL_USART_PARITY_ODD ; 
	else 
		nevent = LL_USART_PARITY_NONE ;

	// 停止位
	if (stop == 2.0f)
		nstop = LL_USART_STOPBITS_2 ;
	else 
	if (stop == 1.5f)
		nstop = LL_USART_STOPBITS_1_5 ;
	else 
	if (stop == 0.5f)
		nstop = LL_USART_STOPBITS_0_5 ;
	else 
		nstop = LL_USART_STOPBITS_1 ;

	ttySx->init(speed,nbits,nevent,nstop);
	return 0;
}



/**
  * @brief    打开串口设备，并进行初始化
  * @param    ttySx       : 串口设备
  * @param    rxblock     : 调用 read/gets 读取数据时是否阻塞至有数据接收
  * @param    txblock     : 调用 write/puts 发送数据时是否阻塞至发送结束
  * @return   接收数据包长度
*/
int serial_close(serial_t *ttySx) 
{
	ttySx->deinit();
	ttySx->hal    = 0;

	if (ttySx->flag & FLAG_RX_BLOCK) // 如果有线程在进行阻塞读
		OS_SEM_POST(ttySx->rxsem);   // post 一个信号量让正在阻塞的任务退出
	else 
		OS_SEM_DEINIT(ttySx->rxsem); // 没有线程阻塞时直接删除当前信号
	
	ttySx->txtail = 0;
	ttySx->txsize = 0;
	ttySx->rxtail = 0;
	ttySx->rxend  = 0;
	ttySx->rxread = 0;
	ttySx->hal    = 0;
///	ttySx->rxblock = 0;
//	ttySx->txblock = 0;
	return 0;
}



