/**
******************************************************************************
* @file           avltree.h
* @author         ��ô��
* @brief          avltree file. ƽ�������ͷ�ļ�
******************************************************************************
*
* COPYRIGHT(c) 2018 GoodMorning
*
******************************************************************************
*/
#ifndef __AVL_TREE_H__
#define __AVL_TREE_H__

/* Public macro (���к�)------------------------------------------------------------*/

/*
	* ��Ϊһ����˵�õ�ƽ����������Ƚ��ٽ��нڵ�ɾ���������ⲿ�ִ����Ǻ�
	* ����ӵģ�д�Ĳ��Ǻ�ֱ�ۣ����Գ����б�Ҫ��������Բ�����ɾ�����ܣ�
	* ����漰��ɾ����������Կ���ʹ�ú�������� linux �ں˴��������ֳ�ʵ�֡�
*/
//#define USE_DELETE_NODE


// ��ȡ�ڵ� r �ĸ��ڵ�
#define avl_parent(r) ((struct avl_node *)((r)->avl_parent & (~3)))

//------------------------------------------------------------------

#define avl_scale(r)    ((r)->avl_parent & 3) //��ȡ avl �ڵ��ƽ�����ӣ�ֵ����
#define AVL_BALANCED    0 //�ڵ�ƽ��
#define AVL_TILT_RIGHT  1 //�ڵ��ұ߱���߸ߣ��� 0b01 ��ʾ������
#define AVL_TILT_LEFT   2 //�ڵ���߱��ұ߸ߣ��� 0b10 ��ʾ������


// �ڵ��ƽ���������� ----------------------------------------------
//���� avl �ڵ�Ϊƽ��ڵ�
#define avl_set_balanced(r)   do {((r)->avl_parent) &= (~3);}while(0)

//���� avl �ڵ�Ϊ����ڵ�
#define avl_set_tilt_right(r) do {(r)->avl_parent=(((r)->avl_parent & ~3)|AVL_TILT_RIGHT);} while (0)

//���� avl �ڵ�Ϊ����ڵ�
#define avl_set_tilt_left(r)  do {(r)->avl_parent=(((r)->avl_parent & ~3)|AVL_TILT_LEFT);} while (0)

//------------------------------------------------------------------

//#define avl_entry(ptr, type, member) container_of(ptr, type, member)

/* Public types ------------------------------------------------------------*/

struct avl_node // avl �ڵ�
{
    unsigned long  avl_parent; 
    struct avl_node *avl_left;
    struct avl_node *avl_right;
};

struct avl_root // avl ����
{
    struct avl_node *avl_node;
};



/* Public variables ---------------------------------------------------------*/

/* Public function prototypes ������ýӿ� -----------------------------------*/

// ���� avl �ڵ�ĸ��ڵ�Ϊ p
static inline void avl_set_parent(struct avl_node *avl, struct avl_node *p)
{
    avl->avl_parent = (avl->avl_parent & 3) | (unsigned long)p;
}

void avl_insert(
               struct avl_root *root,
               struct avl_node * insertnode, 
               struct avl_node * insertparent,
               struct avl_node ** avl_link);

#ifdef USE_DELETE_NODE
void avl_delete(struct avl_root *root,struct avl_node * node);
#endif

/* Find logical next and previous nodes in a tree */
extern struct avl_node *avl_next(const struct avl_node *);
extern struct avl_node *avl_prev(const struct avl_node *);
extern struct avl_node *avl_first(const struct avl_root *);
extern struct avl_node *avl_last(const struct avl_root *);
void avl_replace_node(struct avl_node *victim, struct avl_node *new,
             struct avl_root *root);
#endif
