/**
  ******************************************************************************
  * @file           stm32f4xx_serial_func.h
  * @author         古么宁
  * @brief          串口相关函数生成头文件。
  *                 此头文件可以被多次 include 展开，仅供 serial_f4xx.c 使用
  ******************************************************************************
  *
  * COPYRIGHT(c) 2018 GoodMorning
  *
  ******************************************************************************
  */


char TTYSxRXBUF[RX_BUF_SIZE] = {0};
char TTYSxTXBUF[TX_BUF_SIZE] = {0};


static void USART_INIT_FN(uint32_t nspeed, uint32_t nbits, uint32_t nevent, uint32_t nstop)
{
	// ---------------- 结构体配置 ----------------
	TTYSx.hal    = USARTx;
	TTYSx.dma    = DMAx;

	// ---------------- 串口参数配置 ----------------
	LL_USART_InitTypeDef USART_InitStruct;

	USART_InitStruct.BaudRate     = nspeed;
	USART_InitStruct.DataWidth    = nbits;
	USART_InitStruct.Parity       = nevent;
	USART_InitStruct.StopBits     = nstop;
//	USART_InitStruct.OverSampling = LL_USART_OVERSAMPLING_16;
	USART_InitStruct.TransferDirection   = LL_USART_DIRECTION_TX_RX;
	USART_InitStruct.HardwareFlowControl = LL_USART_HWCONTROL_NONE;

	LL_USART_Init(USARTx, &USART_InitStruct);
	LL_USART_ConfigAsyncMode(USARTx);

	NVIC_SetPriority(USART_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),IRQnPRIORITY, 0));
	NVIC_EnableIRQ(USART_IRQn);


	#if (DMATxEnable) // 如果启用 dma 
		TTYSx.dma_tx = DMA_TX_CHx;
		TTYSx.flag  |= FLAG_TX_DMA ;
		LL_USART_EnableDMAReq_TX(USARTx);
	#else 
		TTYSx.dma_tx = 0;
		TTYSx.flag  &= ~FLAG_TX_DMA ;
	#endif 

	#if (DMARxEnable) // 如果启用 dma
		TTYSx.dma_rx = DMA_RX_CHx; 
		TTYSx.flag  |= FLAG_RX_DMA ;
		LL_USART_EnableDMAReq_RX(USARTx);
		LL_USART_DisableIT_RXNE(USARTx);
	#else 
		TTYSx.dma_rx = 0;
		TTYSx.flag  &= ~FLAG_RX_DMA ;
		LL_USART_EnableIT_RXNE(USARTx);
	#endif 
	
	LL_USART_DisableIT_PE(USARTx);
	LL_USART_DisableIT_TC(USARTx);
	LL_USART_EnableIT_IDLE(USARTx);
	LL_USART_Enable(USARTx);

	// ---------------- DMA 相关配置 ----------------
	#if (DMATxEnable || DMARxEnable) 
		USART_DMA_CLOCK_INIT();
	#endif 
	
	#if (DMATxEnable) // 如果启用 dma 
		LL_DMA_DisableChannel(DMAx,DMA_TX_CHx);//发送暂不使能
		while(LL_DMA_IsEnabledChannel(DMAx,DMA_TX_CHx));
		
		/* USART_TX Init */
		LL_DMA_SetDataTransferDirection(DMAx, DMA_TX_CHx, LL_DMA_DIRECTION_MEMORY_TO_PERIPH);
		LL_DMA_SetChannelPriorityLevel(DMAx, DMA_TX_CHx, LL_DMA_PRIORITY_MEDIUM);
		LL_DMA_SetMode(DMAx, DMA_TX_CHx, LL_DMA_MODE_NORMAL);
		LL_DMA_SetPeriphIncMode(DMAx, DMA_TX_CHx, LL_DMA_PERIPH_NOINCREMENT);
		LL_DMA_SetMemoryIncMode(DMAx, DMA_TX_CHx, LL_DMA_MEMORY_INCREMENT);
		LL_DMA_SetPeriphSize(DMAx, DMA_TX_CHx, LL_DMA_PDATAALIGN_BYTE);
		LL_DMA_SetMemorySize(DMAx, DMA_TX_CHx, LL_DMA_MDATAALIGN_BYTE);
		LL_DMA_SetPeriphAddress(DMAx,DMA_TX_CHx,LL_USART_DMA_GetRegAddr(USARTx));

		DMA_TX_CLEAR_FLAG();

		/* DMA interrupt init 中断*/
		NVIC_SetPriority(DMA_TX_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),DMAxPRIORITY, 0));
		NVIC_EnableIRQ(DMA_TX_IRQn);
		LL_DMA_EnableIT_TC(DMAx,DMA_TX_CHx);
	#endif 

	#if (DMARxEnable) // 如果启用 dma 
		LL_DMA_DisableChannel(DMAx,DMA_RX_CHx);//发送暂不使能
		while(LL_DMA_IsEnabledChannel(DMAx,DMA_RX_CHx));

		/* USART_RX Init */
		LL_DMA_SetDataTransferDirection(DMAx, DMA_RX_CHx, LL_DMA_DIRECTION_PERIPH_TO_MEMORY);
		LL_DMA_SetChannelPriorityLevel(DMAx, DMA_RX_CHx, LL_DMA_PRIORITY_MEDIUM);
		LL_DMA_SetMode(DMAx, DMA_RX_CHx, LL_DMA_MODE_NORMAL);
		LL_DMA_SetPeriphIncMode(DMAx, DMA_RX_CHx, LL_DMA_PERIPH_NOINCREMENT);
		LL_DMA_SetMemoryIncMode(DMAx, DMA_RX_CHx, LL_DMA_MEMORY_INCREMENT);
		LL_DMA_SetPeriphSize(DMAx, DMA_RX_CHx, LL_DMA_PDATAALIGN_BYTE);
		LL_DMA_SetMemorySize(DMAx, DMA_RX_CHx, LL_DMA_MDATAALIGN_BYTE);
		LL_DMA_SetPeriphAddress(DMAx,DMA_RX_CHx,LL_USART_DMA_GetRegAddr(USARTx)); 

		DMA_RX_CLEAR_FLAG();

		NVIC_SetPriority(DMA_RX_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),DMAxPRIORITY, 0));
		NVIC_EnableIRQ(DMA_RX_IRQn);
		LL_DMA_EnableIT_TC(DMAx,DMA_RX_CHx);
		DMA_NEXT_RECV((uint32_t)TTYSx.rxbuf,TTYSx.rxmax);
	#endif 
}


static void USART_DEINIT_FN(void)
{
	NVIC_DisableIRQ(USART_IRQn);
	LL_USART_Disable(USARTx);
	LL_USART_DisableIT_IDLE(USARTx);
	LL_USART_DisableIT_RXNE(USARTx);
	LL_USART_DisableIT_TC(USARTx);
	LL_USART_DisableDMAReq_RX(USARTx);
	LL_USART_DisableDMAReq_TX(USARTx);

	#if (DMARxEnable) // 如果启用 dma 接收
		LL_DMA_DisableIT_TC(DMAx,DMA_RX_CHx);
		NVIC_DisableIRQ(DMA_RX_IRQn);
	#endif

	#if (DMATxEnable) // 如果启用 dma 发送
		LL_DMA_DisableIT_TC(DMAx,DMA_TX_CHx);
		NVIC_DisableIRQ(DMA_TX_IRQn);
	#endif 
}

void USART_TXLOCK_FN(void)
{
	#if (DMATxEnable)
		//LL_DMA_DisableIT_TC(DMAx,DMA_TX_CHx);
		NVIC_EnableIRQ(DMA_TX_IRQn);
	#else 
		LL_USART_DisableIT_TC(USARTx);
	#endif 
}


void USART_TXUNLOCK_FN(void)
{
	#if (DMATxEnable)
		//LL_DMA_EnableIT_TC(DMAx,DMA_TX_CHx);
		NVIC_EnableIRQ(DMA_TX_IRQn);
	#else 
		LL_USART_EnableIT_TC(USARTx);
	#endif 
}


void USART_RXLOCK_FN(void)
{
	#if (DMARxEnable)
		NVIC_DisableIRQ(DMA_RX_IRQn);
		//LL_DMA_DisableIT_TC(DMAx,DMA_RX_CHx);
	#else 
		LL_USART_DisableIT_TC(USARTx);
	#endif 
	NVIC_DisableIRQ(USART_IRQn);
	//LL_USART_DisableIT_IDLE(USARTx);
}


void USART_RXUNLOCK_FN(void)
{
	#if (DMARxEnable)
		NVIC_EnableIRQ(DMA_RX_IRQn);
		//LL_DMA_EnableIT_TC(DMAx,DMA_RX_CHx);
	#else 
		//LL_USART_EnableIT_RXNE(USARTx);
	#endif 
	NVIC_EnableIRQ(USART_IRQn);
	//LL_USART_EnableIT_IDLE(USARTx);
}

serial_t TTYSx = {
	.init      = USART_INIT_FN ,
	.deinit    = USART_DEINIT_FN ,
	.tx_lock   = USART_TXLOCK_FN,
	.tx_unlock = USART_TXUNLOCK_FN,
	.rx_lock   = USART_RXLOCK_FN,
	.rx_unlock = USART_RXUNLOCK_FN,
	.rxbuf  = TTYSxRXBUF ,
	.txbuf  = TTYSxTXBUF ,
	.txmax  = TX_BUF_SIZE,
	.rxmax  = RX_BUF_SIZE/2,
};



#if (DMATxEnable)   // 如果启用了 DMA ，则生成对应的 DMA 中断函数和串口中断函数

/**
	* @brief    DMA_TX_IRQ_HANDLER console 串口发送一包数据完成中断
	* @param    空
	* @return   空
*/
void DMA_TX_IRQ_HANDLER(void)
{
	LL_DMA_DisableChannel(DMAx,DMA_TX_CHx);
	DMA_TX_CLEAR_FLAG();

	if (!TTYSx.txsize) //发送完此包后无数据，复位缓冲区
		TTYSx.txtail = 0;
	else                        //还有数据则继续发送
		serial_send_pkt(&TTYSx);
}
#endif


#if (DMARxEnable) // 如果启用 dma 接收，则开启 dma 接收中断
/**
	* @brief    DMA_RX_IRQ_HANDLER console 串口接收满中断
	* @param    空
	* @return   空
*/
void DMA_RX_IRQ_HANDLER(void)
{
	DMA_RX_CLEAR_FLAG();

	TTYSx.rxtail += TTYSx.rxmax; //更新缓冲地址

	// 如果剩余空间不足以缓存最大包长度，从 0 开始
	// rxmax 为缓存区大小的一半，超过一半的时候即回归 0
	if (TTYSx.rxtail > TTYSx.rxmax) {
		TTYSx.rxend = TTYSx.rxtail;
		TTYSx.rxtail = 0;
	}

	DMA_NEXT_RECV((uint32_t)(&TTYSx.rxbuf[TTYSx.rxtail]),TTYSx.rxmax);//设置缓冲地址和最大包长度

	if ((TTYSx.flag & FLAG_RX_BLOCK) && (TTYSx.hal))
		OS_SEM_POST(TTYSx.rxsem);// 如果是阻塞的，释放信号量
}
#endif


/**
	* @brief    USART_IRQ_HANDLER 串口中断函数，只有空闲中断
	* @param    空
	* @return   空
*/
void USART_IRQ_HANDLER(void)
{
	#if (!DMARxEnable) // 如果不启用 dma 接收，则开启接收中断
		static unsigned short rxtail = 0;
		if (LL_USART_IsActiveFlag_RXNE(USARTx)) {//接收单个字节中断
			LL_USART_ClearFlag_RXNE(USARTx); //清除中断

			TTYSx.rxbuf[rxtail++] = LL_USART_ReceiveData8(USARTx);//数据存入内存
			if (rxtail - TTYSx.rxtail >= TTYSx.rxmax) {//接收到最大包长度
				TTYSx.rxtail += TTYSx.rxmax ; //更新缓冲尾部

				//如果剩余空间不足以缓存最大包长度，从 0 开始
				if (TTYSx.rxtail > TTYSx.rxmax){
					TTYSx.rxtail = 0;
					rxtail = 0;
				}

				///if (TTYSx.rxblock && TTYSx.hal)
				if ((TTYSx.flag & FLAG_RX_BLOCK) && (TTYSx.hal))
					OS_SEM_POST(TTYSx.rxsem);// 如果是阻塞的，释放信号量
			} //if (rxtail - TTYSx.rxtail >= TTYSx.rxmax ) {//接收到最大包长度
		}
	#endif 
	
	if (LL_USART_IsActiveFlag_IDLE(USARTx)) { //空闲中断
		LL_USART_ClearFlag_IDLE(USARTx);      //清除空闲中断

		#if (!DMARxEnable)
			unsigned short pkt_len = rxtail - TTYSx.rxtail;//空闲中断长度
		#else 	
			unsigned short pkt_len = TTYSx.rxmax - LL_DMA_GetDataLength(DMAx,DMA_RX_CHx);//得到当前包的长度
		#endif 

		if (pkt_len) {
			TTYSx.rxtail += pkt_len ;	 //更新缓冲地址
			if (TTYSx.rxtail > TTYSx.rxmax){//如果剩余空间不足以缓存最大包长度，从 0 开始
				TTYSx.rxend = TTYSx.rxtail;
				TTYSx.rxtail = 0;
			}

			DMA_NEXT_RECV((uint32_t)&(TTYSx.rxbuf[TTYSx.rxtail]),TTYSx.rxmax);//设置缓冲地址和最大包长度

			if ((TTYSx.flag & FLAG_RX_BLOCK) && (TTYSx.hal))
				OS_SEM_POST(TTYSx.rxsem);// 如果是阻塞的，释放信号量 
		}
	}

	#if (!DMATxEnable) // 如果不启用 dma 发送，则开启发送中断
		if (LL_USART_IsActiveFlag_TC(USARTx)) { //发送中断
			LL_USART_ClearFlag_TC(USARTx); //清除中断
			static unsigned short txhead = 0;
			txhead = (!txhead)?(TTYSx.txtail-TTYSx.txsize+1):(1+txhead);
			if (txhead < TTYSx.txtail) { //如果未发送玩当前数据，发送下一个字节
				LL_USART_TransmitData8(USARTx,(uint8_t)(TTYSxTXBUF[txhead]));
			} 
			else {
				TTYSx.txsize = 0 ;
				TTYSx.txtail = 0 ;
				txhead = 0;
				LL_USART_DisableIT_TC(USARTx);
			}
		}
	#endif 
}



#undef USARTn 
#undef DMAn
#undef DMATxCHn 
#undef DMARxCHn
#undef USART_BAUDRATE
#undef IRQnPRIORITY
#undef DMAxPRIORITY
#undef USART_DMA_CLOCK_INIT
