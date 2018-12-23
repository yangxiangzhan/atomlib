#ifndef _ROS_TASK_H_
#define _ROS_TASK_H_

#include "listlib.h"


#ifndef NULL
	#define NULL ((void*)0)
#endif

#define OS_USE_ID_AND_NAME //�Ƿ�ʹ�� id �ź����֣���ʵ���Ǵ����� shell ������ʹ�������û�������


//--------------------------------------

enum TASK_STATUS_VALUE
{
	TASK_WAITING= 0 ,
	TASK_EXITED ,
};



typedef struct protothread  //���� Protothread ������չ�Ŀ��ƿ�
{
	volatile unsigned short lc;     //Local Continuation ,Protothread Э�̵ĺ���
	volatile unsigned short dly;    //delay/sleep
	volatile unsigned int time;     //����ʱ��㣬����ʵ�� sleep �ͳ�ʱ yield

	unsigned char post;             //post �¼���־
	unsigned char init;             //init ��־
	#define TASK_IS_INITIALIZED 0x9A //init ֵ
	
	#ifdef OS_USE_ID_AND_NAME
		unsigned short ID;          // id ��
		const char *name;           // ������
	#endif

	void * arg;
	int(*func)(void *);//��������ڵı�׼��ʽ
	
	struct list_head list_node; //���������
}
ros_task_t;


//-------------------TASK ����-------------------


// ����ʼ��������������ͷ����д����ʾһ��Ҫ�� 
#define TASK_BEGIN()    do{\
							int yield = 1;               \
							ros_task_t * task = task_self(); \
							task->time = OS_current_time;\
                            if (yield)                   \
							switch(task->lc)             \
							{ case 0: 
							
#define TASK_END()      	}\
							task_exit();\
						}while(0)
// ���������������������β������д����ʾһ��Ҫ��


//-----------------------------------------------------------------
/* TASK �ڲ����ò���,������� TASK_BEGIN() �� TASK_END() ֮��
 *  ��Ϊ��Ҫ�õ� TASK_BEGIN() ����� task �� yield ����
 * 
 * ע����������������ڵ� switch ʵ���������������ܣ���
 * char task(void * arg)
 * {
	 TASK_BEGIN();
	 task_sleep(100); //�������
	 switch(xxx)
	 {
		case 2:task_yield(); //��������Ϊ����Ļָ�������Ҫ��case��ʵ�� 
	 }
	 TASK_END();
 * }
*/
// ����ȴ�ĳ�����������������������������Ϯlinux �� pthred_cond_wait(),�����е㲻һ��	
#define task_cond_wait(cond) \
	do{(task)->lc = __LINE__;case __LINE__:if(!(cond)) return TASK_WAITING;}while(0)

						
// ����ȴ�ĳ����ʧЧ������Գ������������Լ��ӵ�
#define task_cond_while(cond) \
	do{(task)->lc = __LINE__;case __LINE__:if((cond)) return TASK_WAITING;}while(0)


// ����ȴ������߳�������������δ��������������Ϯlinux �� pthred_join()					
#define task_join(thread)  task_cond_wait(task_is_exited(thread))

// ��������һ��ʱ�䣬��λ���� ms 
#define task_sleep(x_ms)   do{(task)->dly = x_ms;task_yield();}while(0)


// �����ó� cpu
#define task_yield()  \
    do {\
      yield = 0;\
      (task)->lc = __LINE__;case __LINE__:if(!yield){return TASK_WAITING;}\
    }while(0)
	

// ����ʱ��ܳ���ѭ��������� , ��ʱ(1ms)�ó� cpu ���� post һ���¼�
#define task_timeout_yield() if (OS_current_time != task->time) do{OS_task_post(task);task_yield();}while(0)


// �����˳����񣬲�����ִ�д�����
#define task_exit()        do{task_cancel(task);return TASK_EXITED;}while(0)


	

//-------------------TASK �ⲿ���ò���-------------------
// �������񲢿�ʼ���� , ��Ϯ linux �� pthread_create(),�ڶ������������ű���
#ifdef OS_USE_ID_AND_NAME
	#define task_create(tidp,x,func,arg) OS_task_create((tidp),#func,func,arg)
#else
	#define task_create(tidp,x,func,arg) OS_task_create((tidp),NULL,func,arg)
#endif

// ɾ��һ������,��ֱ�Ӵ��б�ɾ������λ lc �ɵ�����ɾ�����п��ܲ�������������ע��ʹ��
#define task_cancel(task)     do{(task)->lc = 1;(task)->dly = 0;}while(0)


// ��ȡ�����Ƿ������� �� ����� linux �߳̿�û�е�
#define task_is_running(task)   ((task)->init == TASK_IS_INITIALIZED && (task)->lc != 1)
#define task_is_exited(task)    (!task_is_running(task))



#if 0
	#define OS_Start() OS_PSP();} void OS_Start_func(void) {
#else
	#define OS_Start() 
#endif

//------------------�򵥻�����ʵ��--------------------
typedef struct{
	volatile struct protothread * lock;
}ros_mutex_t; 


// ��ʼ��һ�� mutex 
#define task_mutex_init(mx)       (mx)->lock = NULL

// ���� mutex ��ʧ�������ֱ�������ɹ� 
#define task_mutex_lock(mx)       task_cond_while((mx)->lock);(mx)->lock = task

// ���� mutex ,���ܽ�����һ�������ϵ���
#define task_mutex_unlock(mx)     if ((mx)->lock == task)  (mx)->lock = NULL

#define task_mutex_is_locked(mx)  ((mx)->lock)
	

//------------------���ź���ʵ��--------------------
typedef struct{
	volatile int signal;
	struct protothread * wait;
}ros_semaphore_t;

// ��ʼ��һ���ź���
#define task_semaphore_init(sph)  do{(sph)->signal = 0;(sph)->wait = NULL;}while(0)

// �ͷ�һ���ź���������һ���ж�.�� post һ���¼���������Ӧ
#define task_semaphore_release(sph)\
do{\
	(sph)->signal = 1;  \
	if((sph)->wait)     \
		TASK_POST((sph)->wait);\
}while(0)

// �ȴ�һ����ֵ�ź���
#define task_semaphore_wait(sph)\
do{\
	(sph)->wait = task ;          \
	task_cond_wait((sph)->signal);\
	task_semaphore_init(sph);     \
}while(0)
	

//------------------ϵͳ��������--------------------

#define OS_CONF_NUMEVENTS 8U //�¼�������ȣ�Ϊ 2 ��ָ��
#define OS_NUMEVENTS_MASK (OS_CONF_NUMEVENTS-1)

extern volatile unsigned char nevents ; //δ�����¼�����
extern volatile unsigned char fevent ;  //�¼������±�
extern struct protothread  *  events[] ;//�¼�����

#define TASK_POST(task)\
do{\
	if ((task)->post || nevents == OS_CONF_NUMEVENTS) break;  \
	events[(fevent + nevents++) & OS_NUMEVENTS_MASK] = (task);\
}while(0)


extern struct list_head       OS_scheduler_list;
extern struct protothread *   OS_current_task;
#define task_self()           OS_current_task

extern volatile unsigned long OS_current_time;
#define OS_heartbeat()        do{++OS_current_time;}while(0)
	

void    OS_task_create(ros_task_t *task,const char * name, int (*start_rtn)(void*), void *arg);
//void    OS_task_post  (ros_task_t * task);
void    OS_scheduler(void);

#endif

