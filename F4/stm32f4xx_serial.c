/**
  ******************************************************************************
  * @file           stm32f1xx_serial.c
  * @author         古么宁
  * @brief          基于 LL 库的串口驱动具体实现。
  * 使用步骤：
  * 1> 此驱动不包含引脚驱动，因为引脚有肯能被各种重定向。所以第一步是在 stm32cubemx 中
  *    把串口对应的引脚使能为串口的输入输出，使能 usart 。注意不需要使能 usart 的 dma
  *    和中断，仅选择引脚初始化和串口初始化即可。然后选择库为 LL 库。生成工程。
  * 2> 把 stm32fxxx_serial.c 和头文件加入工程，其中 stm32fxxx_serial_set.h 和
  *    stm32fxxx_serial_func.h 仅为 stm32fxxx_serial.c 内部使用。此头文件为所有对外
  *    接口定义。
  * 3> include "stm32fxxx_serial.h" ,在此头文件下，打开所需要用的串口宏 USE_USARTx 
  *    后，调用 serial_open(&ttySx,...) ;打开串口后，便可调用 
  *    serial_write/serial_gets/serial_read 等函数进行操作。
  * 4> 有需要则在 stm32fxxx_serial.c  修改串口配置参数，比如缓存大小，是否使能 DMA 等。
  *    
  ******************************************************************************
  *
  * COPYRIGHT(c) 2018 GoodMorning
  *
  ******************************************************************************
  */
/* Includes ---------------------------------------------------*/
#include <string.h>
#include <stdint.h>

#include "stm32f4xx_ll_bus.h"  ///< LL 库必须依赖项
#include "stm32f4xx_ll_usart.h"///< LL 库必须依赖项
#include "stm32f4xx_ll_dma.h"  ///< LL 库必须依赖项

#include "stm32f4xx_serial_set.h"  ///< 这个头文件仅供此 .c 文件内部使用
#include "stm32f4xx_serial.h"

static inline void serial_send_pkt(serial_t * ttySx);

//---------------------HAL层相关--------------------------
// 如果要对硬件进行移植修改，修改下列宏

#if USE_USART1  // 如果启用串口 USARTn
	#define USARTn                  1        ///< 引用串口号
	#define DMAn                    2        ///< 对应 DMA 
	#define DMATxCHn                4        ///< 发送对应 DMA 通道号
	#define DMARxCHn                4        ///< 接收对应 DMA 通道号
	#define DMATxSTRMn              7
	#define DMARxSTRMn              2

	#define DMARxEnable             1        ///< 为 1 时启用 dma 接收
	#define DMATxEnable             1        ///< 为 1 时启用 dma 发送
	#define RX_BUF_SIZE             256      ///< 硬件接收缓冲区
	#define TX_BUF_SIZE             512      ///< 硬件发送缓冲区

	#define IRQnPRIORITY            7        ///< 串口相关中断的优先级
	#define DMAxPRIORITY            6        ///< dma 相关中断的优先级
	#define USART_DMA_CLOCK_INIT()  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_DMA2)///< 跟 DMAx 对应

	///<  这个头文件仅供此 .c 文件内部使用，此头文件展开会生成串口对应的中断函数和硬件初始化函数
	#include "stm32f4xx_serial_func.h"
#endif


#if USE_USART2  // 如果启用串口 USARTn
	#define USARTn                  2        ///< 引用串口号
	#define DMAn                    1        ///< 对应 DMA 
	#define DMATxCHn                4        ///< 发送对应 DMA 通道号
	#define DMARxCHn                4        ///< 接收对应 DMA 通道号
	#define DMATxSTRMn              5
	#define DMARxSTRMn              6

	#define DMARxEnable             1        ///< 为 1 时启用 dma 接收
	#define DMATxEnable             1        ///< 为 1 时启用 dma 发送
	#define RX_BUF_SIZE             256      ///< 硬件接收缓冲区
	#define TX_BUF_SIZE             512      ///< 硬件发送缓冲区

	#define IRQnPRIORITY            7        ///< 串口相关中断的优先级
	#define DMAxPRIORITY            6        ///< dma 相关中断的优先级
	#define USART_DMA_CLOCK_INIT()  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_DMA1)///< 跟 DMAx 对应

	///<  这个头文件仅供此 .c 文件内部使用，此头文件展开会生成串口对应的中断函数和硬件初始化函数
	#include "stm32f4xx_serial_func.h"
#endif


#if USE_USART3  // 如果启用串口 USARTn
	#define USARTn                  3        ///< 引用串口号
	#define DMAn                    1        ///< 对应 DMA 
	#define DMATxCHn                4        ///< 发送对应 DMA 通道号
	#define DMARxCHn                4        ///< 接收对应 DMA 通道号
	#define DMATxSTRMn              3
	#define DMARxSTRMn              1

	#define DMARxEnable             1        ///< 为 1 时启用 dma 接收
	#define DMATxEnable             1        ///< 为 1 时启用 dma 发送
	#define RX_BUF_SIZE             256      ///< 硬件接收缓冲区
	#define TX_BUF_SIZE             512      ///< 硬件发送缓冲区

	#define IRQnPRIORITY            7        ///< 串口相关中断的优先级
	#define DMAxPRIORITY            6        ///< dma 相关中断的优先级
	#define USART_DMA_CLOCK_INIT()  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_DMA1)///< 跟 DMAx 对应

	///<  这个头文件仅供此 .c 文件内部使用，此头文件展开会生成串口对应的中断函数和硬件初始化函数
	#include "stm32f4xx_serial_func.h"
#endif



#if USE_USART4  // 如果启用串口 USARTn
	#define USARTn                  4        ///< 引用串口号
	#define DMAn                    1        ///< 对应 DMA 
	#define DMATxCHn                4        ///< 发送对应 DMA 通道号
	#define DMARxCHn                4        ///< 接收对应 DMA 通道号
	#define DMATxSTRMn              4
	#define DMARxSTRMn              2

	#define DMARxEnable             1        ///< 为 1 时启用 dma 接收
	#define DMATxEnable             1        ///< 为 1 时启用 dma 发送
	#define RX_BUF_SIZE             256      ///< 硬件接收缓冲区
	#define TX_BUF_SIZE             512      ///< 硬件发送缓冲区

	#define IRQnPRIORITY            7        ///< 串口相关中断的优先级
	#define DMAxPRIORITY            6        ///< dma 相关中断的优先级
	#define USART_DMA_CLOCK_INIT()  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_DMA1)///< 跟 DMAx 对应

	///<  这个头文件仅供此 .c 文件内部使用，此头文件展开会生成串口对应的中断函数和硬件初始化函数
	#include "stm32f4xx_serial_func.h"
#endif


#if USE_USART5  // 如果启用串口 USARTn
	#define USARTn                  5        ///< 引用串口号
	#define DMAn                    1        ///< 对应 DMA 
	#define DMATxCHn                4        ///< 发送对应 DMA 通道号
	#define DMARxCHn                4        ///< 接收对应 DMA 通道号
	#define DMATxSTRMn              7
	#define DMARxSTRMn              0

	#define DMARxEnable             1        ///< 为 1 时启用 dma 接收
	#define DMATxEnable             1        ///< 为 1 时启用 dma 发送
	#define RX_BUF_SIZE             256      ///< 硬件接收缓冲区
	#define TX_BUF_SIZE             512      ///< 硬件发送缓冲区

	#define IRQnPRIORITY            7        ///< 串口相关中断的优先级
	#define DMAxPRIORITY            6        ///< dma 相关中断的优先级
	#define USART_DMA_CLOCK_INIT()  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_DMA1)///< 跟 DMAx 对应

	///<  这个头文件仅供此 .c 文件内部使用，此头文件展开会生成串口对应的中断函数和硬件初始化函数
	#include "stm32f4xx_serial_func.h"
#endif


#if USE_USART6  ///<  如果启用串口
	#define USARTn                  6  ///< 引用串口号
	#define DMAn                    2  ///< 对应 DMA
	#define DMATxCHn                5  ///< 发送对应 DMA 通道号
	#define DMARxCHn                5  ///< 接收对应 DMA 通道号
	#define DMATxSTRMn              6
	#define DMARxSTRMn              1

	#define DMARxEnable             1        ///< 为 1 时启用 dma 接收
	#define DMATxEnable             1        ///< 为 1 时启用 dma 发送
	#define RX_BUF_SIZE             256      ///< 硬件接收缓冲区
	#define TX_BUF_SIZE             512      ///< 硬件发送缓冲区

	#define IRQnPRIORITY            7        ///< 串口相关中断的优先级
	#define DMAxPRIORITY            6        ///< dma 相关中断的优先级
	#define USART_DMA_CLOCK_INIT()  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_DMA2)///< 跟 DMAx 对应

	///<  这个头文件仅供此 .c 文件内部使用，此头文件展开会生成串口对应的中断函数和硬件初始化函数
	#include "stm32f4xx_serial_func.h"
#endif

//------------------------------华丽的分割线------------------------------


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
	LL_DMA_EnableStream(ttySx->dma,ttySx->dma_tx);
}

/**
  * @brief    串口设备发送一包数据
  * @param    ttySx   : 串口设备
  * @param    data    : 需要发送的数据
  * @param    datalen : 需要发送的数据长度
  * @return   实际从硬件发送出去的数据长度
*/
int serial_write(serial_t *ttySx , const void * data , int datalen , int block ) 
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
			ttySx->tx_lock();                           // 修改全局变量，禁用中断。互斥
			ttySx->txtail  = pkttail + pktsize;         // 更新尾部
			ttySx->txsize += pktsize;                   // 设置当前包大小
			if (!LL_DMA_IsEnabledStream(ttySx->dma,ttySx->dma_tx))//开始发送
				serial_send_pkt(ttySx);
			ttySx->tx_unlock();
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
  * @param    block   : O_BLOCK/O_NOBLOCK 是否阻塞
  * @return   接收缓存数据包长度
*/
int serial_gets(serial_t *ttySx ,char ** databuf , int block  )
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
  * @param    ttySx   : 串口设备
  * @param    databuf : 读取数据内存
  * @param    bufsize : 读取数据内存总大小
  * @param    block   : O_BLOCK/O_NOBLOCK 是否阻塞
  * @return   接收数据包长度
*/
int  serial_read(serial_t * ttySx ,void * databuf , int bufsize , int block )
{
	int datalen ;
	char * data = NULL;
	if (!ttySx->hal)    // 未初始化的设备 ，返回 -1
		return -1;

	if (ttySx->rxread == ttySx->rxtail) { // 接收缓冲区中无接收数据
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
	if (ttySx->rxread > ttySx->rxtail) {        // 接收到数据
		data = &ttySx->rxbuf[ttySx->rxread];   // 数据缓冲区
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
		data = &ttySx->rxbuf[ttySx->rxread];   // 数据缓冲区
		datalen = ttySx->rxtail - ttySx->rxread;// 当前包数据大小
		if (datalen > bufsize)
			datalen = bufsize;
		ttySx->rxread += datalen ;         // 数据读更新
	}
	ttySx->rx_unlock();
	
	MEMCPY(databuf,data,datalen);
	return datalen ;
}


/**
  * @brief    打开串口设备，并进行初始化
  * @param    ttySx       : 串口设备
  * @param    rxblock     : 调用 read/gets 读取数据时是否阻塞至有数据接收
  * @param    txblock     : 调用 write/puts 发送数据时是否阻塞至发送结束
  * @return   接收数据包长度
*/
int serial_open(serial_t *ttySx , int speed, int bits, char event, float stop) 
{
	OS_SEM_INIT(ttySx->rxsem); 
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
	return 0;
}



