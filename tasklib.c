/**
  ******************************************************************************
  * @file            OS.c
  * @author         ��ô��
  * @brief           OS file .��Э��ģ��һ���򵥵Ĳ���ϵͳ
  ******************************************************************************
  *
  * COPYRIGHT(c) 2018 GoodMorning
  *
  ******************************************************************************
  */
/* Includes ---------------------------------------------------*/
#include <stdio.h>

#include "listlib.h"  //from linux kernel
#include "tasklib.h"


// ϵͳ����һ������
#define OS_call(task) \
		do{if(TASK_EXITED == (task)->func((task)->arg)) list_del_init(&((task)->list_node));}while(0)
	

struct list_head       OS_scheduler_list = {&OS_scheduler_list,&OS_scheduler_list};//ϵͳ�����������
struct protothread *   OS_current_task = NULL;//ϵͳ��ǰ���е�����
volatile unsigned long OS_current_time = 0;     //ϵͳʱ�䣬ms



volatile unsigned char nevents = 0; //δ�����¼�����
volatile unsigned char fevent = 0;  //�¼������±�
struct protothread *events[OS_CONF_NUMEVENTS]; //�¼�����


/** 
	* @brief OS_create_task :ע��һ��  task
	* @param TaskFunc      : Task ��Ӧ��ִ�к���ָ��
	* @param task          : Task ��Ӧ��������ƽṹ��
	* @param pTaskName     : Task ��
	*
	* @return NULL
*/
void OS_task_create(ros_task_t *task,const char * name, int (*start_rtn)(void*), void *arg)
{
	static unsigned short IDcnt = 1U;
	
	if ( task->init != TASK_IS_INITIALIZED)
	{
		INIT_LIST_HEAD(&task->list_node);
		task->init = TASK_IS_INITIALIZED;
	}

	if (list_empty(&task->list_node))//��������������У����ظ�ע��
	{
		list_add_tail(&task->list_node, &OS_scheduler_list);//�����������ĩ��
		task->lc   = 0;
		task->dly  = 0;
		task->post = 0;
		task->func = start_rtn;//����ִ�к���
		task->arg  = arg;
		
		#ifdef  OS_USE_ID_AND_NAME
			task->name = name;	 //������
			task->ID = IDcnt;
		#endif
		
		++IDcnt;
	}
}



/** 
	* @brief run task
	* @param NULL
	*
	* @return NULL
*/
void OS_scheduler(void)
{
	struct list_head * SchedulerListThisNode;
	struct list_head * SchedulerListNextNode;
	
	list_for_each_safe(SchedulerListThisNode,SchedulerListNextNode,&OS_scheduler_list)
	{
		//��ȡ��ǰ����ڵ��Ӧ��������ƿ�ָ��
		OS_current_task = list_entry(SchedulerListThisNode,struct protothread,list_node); 
		
		if (OS_current_task->dly)//�������ڵ�����ִ�У�����������ʱ�䱻���ᣬֱ������ʱ���
		{
			if (OS_current_time - OS_current_task->time < OS_current_task->dly) 
				continue; //��Ȼ��� continue Ҳ������ post �¼�������ô����Ҫ����һ��
			else
				OS_current_task->dly = 0;
		}
			
		OS_call(OS_current_task);//ִ�������е�����

		if (nevents) //���� post �¼�
		{
			OS_current_task = events[fevent];
			fevent = (fevent + 1) & OS_NUMEVENTS_MASK;
			nevents--;
			
			OS_current_task->post = 0;//������¼��� post ��־����˲ſ��Է���post
			if (OS_current_task->init == TASK_IS_INITIALIZED)
				OS_call(OS_current_task);
		}
	}
}


