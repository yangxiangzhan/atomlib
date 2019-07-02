#ifndef _SERIAL_SET_H_
#define _SERIAL_SET_H_


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
#define DMAx                    DMA_x(DMAn)     //串口所在 dma 总线
#define USARTx                  USART_x(USARTn)   //引用串口
#define USART_IRQn              USARTx_IRQNUM(USARTn) //中断号
#define USART_IRQ_HANDLER       USARTx_IRQFN(USARTn) //中断函数名

#define DMA_TX_CHx              DMA_Channelx(DMATxCHn)    //串口发送 dma 通道
#define DMA_TX_STRMx            DMA_STRMx(DMATxSTRMn)
#define DMA_TX_IRQn             DMAx_STRMy_IRQNUM(DMAn,DMATxSTRMn)
#define DMA_TX_IRQ_HANDLER      DMAx_STRMy_IRQFN(DMAn,DMATxSTRMn) //中断函数名
#define DMA_TX_CLEAR_FLAG()     DMAx_STRMy_TCCLEAR(DMAn,DMATxSTRMn)
#define DMA_TX_COMPLETE_FLAG()  DMAx_STRMy_TCFLAG(DMAn,DMATxSTRMn)

#define DMA_RX_CHx              DMA_Channelx(DMARxCHn)
#define DMA_RX_STRMx            DMA_STRMx(DMARxSTRMn)
#define DMA_RX_IRQn             DMAx_STRMy_IRQNUM(DMAn,DMARxSTRMn)
#define DMA_RX_IRQ_HANDLER      DMAx_STRMy_IRQFN(DMAn,DMARxSTRMn) //中断函数名
#define DMA_RX_CLEAR_FLAG()     DMAx_STRMy_TCCLEAR(DMAn,DMARxSTRMn)
#define DMA_RX_COMPLETE_FLAG()  DMAx_STRMy_TCFLAG(DMAn,DMARxSTRMn)


#define _USART_FN_(d,x)         usart##x##d
#define _USART_FN(d,x)          _USART_FN_(d,x)
#define USART_INIT_FN           _USART_FN(init,USARTn)
#define USART_DEINIT_FN         _USART_FN(deinit,USARTn)


#define _MALLOC_RXBUF_(x)       rxbuf##x
#define _MALLOC_RXBUF(x)        _MALLOC_RXBUF_(x)
#define TTYSxRXBUF              _MALLOC_RXBUF(USARTn)

#define _MALLOC_TXBUF_(x)       txbuf##x
#define _MALLOC_TXBUF(x)        _MALLOC_TXBUF_(x)
#define TTYSxTXBUF              _MALLOC_TXBUF(USARTn)

#define _SERIAL_X_(x)           ttyS##x
#define SEIRAL_X(x)             _SERIAL_X_(x)
#define TTYSx                   SEIRAL_X(USARTn)


#define _TTY_PARITY_(x)         LL_USART_PARITY_##x
#define TTY_PARITY(x)           _TTY_PARITY_(x)


#define _TTY_STOPBIT_(x)        LL_USART_STOPBITS_##x
#define TTY_STOPBIT(x)          _TTY_STOPBIT_(x)



#define DMA_NEXT_RECV(maddr,max)\
do{\
	LL_DMA_DisableIT_TC(DMAx,DMA_RX_STRMx);\
	LL_DMA_DisableStream(DMAx,DMA_RX_STRMx);\
	while(LL_DMA_IsEnabledStream(DMAx,DMA_RX_STRMx));\
	DMA_RX_CLEAR_FLAG();\
	LL_DMA_SetDataLength(DMAx,DMA_RX_STRMx,(max));\
	LL_DMA_SetMemoryAddress(DMAx,DMA_RX_STRMx,(maddr));\
	LL_DMA_EnableIT_TC(DMAx,DMA_RX_STRMx);\
	LL_DMA_EnableStream(DMAx,DMA_RX_STRMx);\
}while(0)


// 用循环直接对内存进行拷贝会比调用 memcpy 效率要高。
#define MEMCPY(_to,_from,size)\
do{\
	char *to=(char*)_to,*from=(char*)_from;\
	for (int i = 0 ; i < size ; ++i)\
		*to++ = *from++ ;\
}while(0)






#endif
