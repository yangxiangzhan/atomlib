#ifndef _SERIAL_SET_H_
#define _SERIAL_SET_H_


//---------------------HAL层宏替换--------------------------
#define USART__(x)	            USART##x
#define USART_x(x)	            USART__(x)

#define USART_IRQNUM(x)         USART##x##_IRQn
#define USARTx_IRQNUM(x)        USART_IRQNUM(x)

#define USART_IRQFN(x)          USART##x##_IRQHandler
#define USARTx_IRQFN(x)         USART_IRQFN(x)

#define DMA_X(x)	            DMA##x
#define DMA_x(x)	            DMA_X(x)

#define DMA_Channel(x)          LL_DMA_CHANNEL_##x
#define DMA_Channelx(x)         DMA_Channel(x)

#define DMA_Channel_IRQn(x,y)          DMA##x##_Channel##y##_IRQn
#define DMAx_Channely_IRQn(x,y)        DMA_Channel_IRQn(x,y)

#define DMA_Channel_IRQHandler(x,y)    DMA##x##_Channel##y##_IRQHandler
#define DMAx_Channely_IRQHandler(x,y)  DMA_Channel_IRQHandler(x,y)
	 
#define DMA_ClearFlag_TC(x,y)          LL_DMA_ClearFlag_TC##y(DMA##x)
#define DMAx_ClearFlag_TCy(x,y)        DMA_ClearFlag_TC(x,y)
	 
#define DMA_IsActiveFlag_TC(x,y)       LL_DMA_IsActiveFlag_TC##y(DMA##x)
#define DMAx_IsActiveFlag_TCy(x,y)     DMA_IsActiveFlag_TC(x,y)

#define DMA0                    ((DMA_TypeDef *)0)
#define DMAx                    DMA_x(DMAn)     //串口所在 dma 总线
#define USARTx                  USART_x(USARTn)   //引用串口
#define USART_IRQn              USARTx_IRQNUM(USARTn) //中断号
#define USART_IRQ_HANDLER       USARTx_IRQFN(USARTn) //中断函数名

#define DMA_TX_CHx              DMA_Channelx(DMATxCHn)    //串口发送 dma 通道
#define DMA_TX_IRQn             DMAx_Channely_IRQn(DMAn,DMATxCHn)
#define DMA_TX_IRQ_HANDLER      DMAx_Channely_IRQHandler(DMAn,DMATxCHn) //中断函数名
#define DMA_TX_CLEAR_FLAG()     DMAx_ClearFlag_TCy(DMAn,DMATxCHn)
#define DMA_TX_COMPLETE_FLAG()  DMAx_IsActiveFlag_TCy(DMAn,DMATxCHn)

#define DMA_RX_CHx              DMA_Channelx(DMARxCHn)
#define DMA_RX_IRQn             DMAx_Channely_IRQn(DMAn,DMARxCHn)
#define DMA_RX_IRQ_HANDLER      DMAx_Channely_IRQHandler(DMAn,DMARxCHn) //中断函数名
#define DMA_RX_CLEAR_FLAG()     DMAx_ClearFlag_TCy(DMAn,DMARxCHn)
#define DMA_RX_COMPLETE_FLAG()  DMAx_IsActiveFlag_TCy(DMAn,DMARxCHn)


#define USART_FN_(d,x)          usart##x##d
#define USART_FN(d,x)           USART_FN_(d,x)
#define USART_INIT_FN           USART_FN(init,USARTn)
#define USART_DEINIT_FN         USART_FN(deinit,USARTn)


#define MALLOC_RXBUF_(x)        rxbuf##x
#define MALLOC_RXBUF(x)         MALLOC_RXBUF_(x)
#define TTYSxRXBUF              MALLOC_RXBUF(USARTn)

#define MALLOC_TXBUF_(x)        txbuf##x
#define MALLOC_TXBUF(x)         MALLOC_TXBUF_(x)
#define TTYSxTXBUF              MALLOC_TXBUF(USARTn)

#define SERIAL_X_(x)            ttyS##x
#define SEIRAL_X(x)             SERIAL_X_(x)
#define TTYSx                   SEIRAL_X(USARTn)


/*
	LL_DMA_DisableIT_TC(DMAx,DMA_RX_CHx);\
	LL_DMA_EnableIT_TC(DMAx,DMA_RX_CHx);\
 */

#define DMA_NEXT_RECV(maddr,max)\
do{\
	LL_DMA_DisableChannel(DMAx,DMA_RX_CHx);\
	while(LL_DMA_IsEnabledChannel(DMAx,DMA_RX_CHx));\
	DMA_RX_CLEAR_FLAG();\
	LL_DMA_SetDataLength(DMAx,DMA_RX_CHx,(max));\
	LL_DMA_SetMemoryAddress(DMAx,DMA_RX_CHx,(maddr));\
	LL_DMA_EnableChannel(DMAx,DMA_RX_CHx);\
}while(0)


// 用循环直接对内存进行拷贝会比调用 memcpy 效率要高。
#define MEMCPY(_to,_from,size)\
do{\
	char *to=(char*)_to,*from=(char*)_from;\
	for (int i = 0 ; i < size ; ++i)\
		*to++ = *from++ ;\
}while(0)



#endif
