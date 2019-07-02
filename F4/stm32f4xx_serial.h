/**
  ******************************************************************************
  * @file           stm32f4xx_serial.h
  * @author         古么宁
  * @brief          串口驱动对外文件。
  * 使用步骤：
  * 1> 此驱动不包含引脚驱动，因为引脚有肯能被各种重定向。所以第一步是在 stm32cubemx 中
  *    把串口对应的引脚使能为串口的输入输出，使能 usart 。注意不需要使能 usart 的 dma
  *    和中断，仅选择引脚初始化和串口初始化即可。然后选择库为 LL 库。生成工程。
  * 2> 把 stm32f4xx_serial.c 和头文件加入工程，其中 stm32f4xx_serial_set.h 和
  *    stm32f4xx_serial_func.h 仅为 stm32f4xx_serial.c 内部使用，此头文件为所有对外
  *    接口定义。有需要则在 stm32f4xx_serial.c 
  *    修改串口配置参数，比如缓存大小，是否使能 DMA 等。
  * 4> include "stm32f4xx_serial.h" ,在此头文件下，打开所需要用的串口宏 USE_USARTx 
  *    后，调用 serial_open(&ttySx,...) ;
  *    打开串口后，便可调用 serial_write/serial_gets/serial_read 等函数进行操作。
  ******************************************************************************
  *
  * COPYRIGHT(c) 2018 GoodMorning
  *
  ******************************************************************************
  */
/* Includes ---------------------------------------------------*/
#ifndef _F4SERIAL_H_
#define _F4SERIAL_H_

#define SERIAL_OS    0 ///< 有无操作系统

#define USE_USART1   1 ///< 启用串口 1
#define USE_USART2   0 ///< 启用串口 2
#define USE_USART3   0 ///< 启用串口 3
#define USE_USART4   0 ///< 启用串口 4
#define USE_USART5   0 ///< 启用串口 5
#define USE_USART6   1 ///< 启用串口 6


#if SERIAL_OS  ///< 如果带操作系统，需要提供操作系统的信号量

	#include "cmsis_os.h"
	typedef osSemaphoreId serial_sem_t ;

	#define OS_SEM_POST(x)   osSemaphoreRelease((x))
	#define OS_SEM_WAIT(x)   osSemaphoreWait((x),osWaitForever)
	#define OS_SEM_DEINIT(x) vSemaphoreDelete((x))
	#define OS_SEM_INIT(x) \
	do {\
		osSemaphoreDef(Rx);\
		(x) = osSemaphoreCreate(osSemaphore(Rx), 1);\
	}while(0)


#else          ///< 无操作系统下，信号量相关被如下定义

	typedef char serial_sem_t ;
	#define OS_SEM_INIT(x)   do{(x) = 0;}while(0)
	#define OS_SEM_POST(x)   do{(x) = 1;}while(0)
	#define OS_SEM_WAIT(x)   do{for(int i=0;!(x);i++);(x)=0;}while(0)
	#define OS_SEM_DEINIT(x) OS_SEM_INIT(x)
#endif 


#define O_NOBLOCK   0
#define O_BLOCK     1
#define O_BLOCKING  0x40

#define tcdrain(x)     for (int i = 0 ; (x)->txtail ; i++)
#define serial_busy(x) ((x)->txtail != 0)


#define FLAG_TX_DMA   (1)
#define FLAG_RX_DMA   (1<<1)
#define FLAG_TX_BLOCK (1<<2)
#define FLAG_RX_BLOCK (1<<3)


typedef struct serial {
	///< 串口初始化函数
	void (*init)(uint32_t nspeed, uint32_t nbits, uint32_t nevent, uint32_t nstop) ;
	void (*deinit)(void);      ///< 串口去初始化
	void (*tx_lock)(void) ;
	void (*tx_unlock)(void) ;
	void (*rx_lock)(void) ;
	void (*rx_unlock)(void) ;
	char * const   rxbuf ;     ///< 串口接收缓冲区指针
	char * const   txbuf ;     ///< 串口发送缓冲区首地址
	const unsigned short txmax;///< 串口一包最大发送，一般为缓冲区大小
	const unsigned short rxmax;///< 串口一包最大接收，一般为缓冲区大小的一半
	void *         hal;        ///< 串口，USARTx
	void *         dma;        ///< 串口所在 dma
	unsigned int   dma_rx;     ///< 串口 dma 发送所在流
	unsigned int   dma_tx;     ///< 串口 dma 接收所在流

	volatile unsigned short txsize ;    ///< 串口当前发送包大小
	volatile unsigned short txtail ;    ///< 串口当前发送缓冲区尾部
	volatile unsigned short rxtail ;    ///< 串口当前接收缓冲区尾部
	volatile unsigned short rxread ;    ///< 串口未读数据头部
	
	// 串口在缓冲区最后一包数据包尾部。当 &rxbuf[rxtail] 后面内存不足以存放
	// 一包数据 rxmax 大小时，rxtail 会清零，此时需要记下来当前包的大小
	volatile unsigned short rxend  ;

	// 使用 serial_read 函数时是否进行阻塞
	// 阻塞：有操作系统时会等待信号量，无操作系统时会死循环
	// 非阻塞：有数据则直接返回数据长度，否则直接返回0
	//char         rxblock;

	// 使用 serial_write 函数时是否进行阻塞
	// 阻塞：直到硬件完全输出才从 serial_write 返回。
	// 非阻塞：调用 serial_write 后马上返回，数据后续发送。
	//char         txblock;

	char         flag   ;

	serial_sem_t rxsem   ;   ///< 信号量，表示缓冲区中有数据可读取 
}
serial_t;


/**
  * @brief    打开串口设备，并进行初始化
  * @param    ttySx       : 串口设备
  * @param    rxblock     : 调用 read/gets 读取数据时是否阻塞至有数据接收
  * @param    txblock     : 调用 write/puts 发送数据时是否阻塞至发送结束
  * @return   接收数据包长度
*/
int serial_open(serial_t *ttySx , int speed, int bits, char event, float stop);

/**
  * @brief    串口设备发送一包数据
  * @param    ttySx   : 串口设备
  * @param    data    : 需要发送的数据
  * @param    datalen : 需要发送的数据长度
  * @param    block   : O_BLOCK/O_NOBLOCK 是否阻塞
  * @return   实际从硬件发送出去的数据长度
*/
int serial_write(serial_t *ttySx , const void * data , int wrtlen, int block );
#define serial_puts(t,d,l,b) serial_write(t,d,l,b)


/**
  * @brief    读取串口设备接收，存于 databuf 中
  * @param    ttySx   : 串口设备
  * @param    databuf : 读取数据内存
  * @param    bufsize : 读取数据内存总大小
  * @param    block   : O_BLOCK/O_NOBLOCK 是否阻塞
  * @return   接收数据包长度
*/
int serial_read(serial_t *ttySx , void * databuf , int bufsize, int block  );

/**
  * @brief    直接读取串口设备的接收缓存区数据
  * @param    ttySx   : 串口设备
  * @param    databuf : 返回串口设备接收缓存数据包首地址
  * @param    block   : O_BLOCK/O_NOBLOCK 是否阻塞
  * @return   接收缓存数据包长度
*/
int serial_gets(serial_t *ttySx , char ** databfuf ,int block );

// 关闭设备，去初始化
int serial_close(serial_t *ttySx);


#if (USE_USART1)
	extern serial_t ttyS1;
#endif 

#if (USE_USART2)
	extern serial_t ttyS2;
#endif 

#if (USE_USART3)
	extern serial_t ttyS3;
#endif 

#if (USE_USART4)
	extern serial_t ttyS4;
#endif 

#if (USE_USART5)
	extern serial_t ttyS5;
#endif 

#if (USE_USART6)
	extern serial_t ttyS6;
#endif 

#endif
