#ifndef __KERNEL_H__
#define __KERNEL_H__

//------------------------------------------------------------------
#ifndef offsetof //#include <stddef.h>
	#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)// ��ȡ"MEMBER��Ա"��"�ṹ��TYPE"�е�λ��ƫ��
#endif

#ifndef container_of
	#ifndef __GNUC__
		// ����"�ṹ��(type)����"�е�"���Ա����(member)��ָ��(ptr)"����ȡָ�������ṹ�������ָ��
		#define container_of(ptr, type, member)  ((type*)((char*)ptr - offsetof(type, member)))
		// �˺궨��ԭ��Ϊ GNU C ��д�����£���Щ������ֻ֧�� ANSI C /C99 �ģ������������޸�
	#else
		#define container_of(ptr, type, member) ({          \
			const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
			(type *)( (char *)__mptr - offsetof(type,member) );})
	#endif
#endif
//------------------------------------------------------------------


#endif

