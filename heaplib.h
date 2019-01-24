/**
  ******************************************************************************
  * @file           heaplib.h
  * @author         古么宁
  * @brief          内存管理头文件，其中 heaplib.c 源自 freertos 的 heap_4.c 
  *                 此头文件提供 heap_4.c 所需的宏定义，使其可以脱离 freertos
  *                 便可以使用。用于无操作系统但需要做内存管理的地方。
  ******************************************************************************
  *
  * COPYRIGHT(c) 2018 GoodMorning
  *
  ******************************************************************************
*/
#ifndef HEAP_LIB_H__
#define HEAP_LIB_H__



#define configTOTAL_HEAP_SIZE      ((size_t)(20 * 1024))



#define portBYTE_ALIGNMENT			8


//添加宏支持，使编译脱离 freertos
#if portBYTE_ALIGNMENT == 32
	#define portBYTE_ALIGNMENT_MASK ( 0x001f )
#endif

#if portBYTE_ALIGNMENT == 16
	#define portBYTE_ALIGNMENT_MASK ( 0x000f )
#endif

#if portBYTE_ALIGNMENT == 8
	#define portBYTE_ALIGNMENT_MASK ( 0x0007 )
#endif

#if portBYTE_ALIGNMENT == 4
	#define portBYTE_ALIGNMENT_MASK	( 0x0003 )
#endif

#if portBYTE_ALIGNMENT == 2
	#define portBYTE_ALIGNMENT_MASK	( 0x0001 )
#endif

#if portBYTE_ALIGNMENT == 1
	#define portBYTE_ALIGNMENT_MASK	( 0x0000 )
#endif


#define configASSERT(x)
#define mtCOVERAGE_TEST_MARKER()
#define vTaskSuspendAll()
#define xTaskResumeAll() 0
#define traceMALLOC(...)
#define traceFREE(...)


#define MALLOC(x) pvPortMalloc(x)
#define FREE(x)   vPortFree(x)


void *pvPortMalloc( size_t xWantedSize );

void vPortFree( void *pv );

size_t xPortGetFreeHeapSize( void );

size_t xPortGetMinimumEverFreeHeapSize( void );



#endif

