/**
  ******************************************************************************
  * @file           heaplib.h
  * @author         ��ô��
  * @brief          �ڴ����ͷ�ļ������� heaplib.c Դ�� freertos �� heap_4.c 
  *                 ��ͷ�ļ��ṩ heap_4.c ����ĺ궨�壬ʹ��������� freertos
  *                 �����ʹ�á������޲���ϵͳ����Ҫ���ڴ����ĵط���
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


//��Ӻ�֧�֣�ʹ�������� freertos
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

