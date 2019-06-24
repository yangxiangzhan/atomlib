/**
******************************************************************************
* @file           avltree.h
* @author         古么宁
* @brief          avltree file. 平衡二叉树头文件
******************************************************************************
*
* COPYRIGHT(c) 2018 GoodMorning
*
******************************************************************************
*/
#ifndef __AVL_TREE_H__
#define __AVL_TREE_H__

/* Public macro (共有宏)------------------------------------------------------------*/

/*
	* 因为一般来说用到平衡二叉树都比较少进行节点删除，所以这部分代码是后
	* 面添加的，写的不是很直观，所以除非有必要，否则可以不开启删除功能，
	* 如果涉及到删点操作，可以考虑使用红黑树，在 linux 内核代码中有现成实现。
*/
//#define USE_DELETE_NODE


// 获取节点 r 的父节点
#define avl_parent(r) ((struct avl_node *)((r)->avl_parent & (~3)))

//------------------------------------------------------------------

#define avl_scale(r)    ((r)->avl_parent & 3) //获取 avl 节点的平衡因子，值如下
#define AVL_BALANCED    0 //节点平衡
#define AVL_TILT_RIGHT  1 //节点右边比左边高，用 0b01 表示，右倾
#define AVL_TILT_LEFT   2 //节点左边比右边高，用 0b10 表示，左倾


// 节点的平衡因子设置 ----------------------------------------------
//设置 avl 节点为平衡节点
#define avl_set_balanced(r)   do {((r)->avl_parent) &= (~3);}while(0)

//设置 avl 节点为右倾节点
#define avl_set_tilt_right(r) do {(r)->avl_parent=(((r)->avl_parent & ~3)|AVL_TILT_RIGHT);} while (0)

//设置 avl 节点为左倾节点
#define avl_set_tilt_left(r)  do {(r)->avl_parent=(((r)->avl_parent & ~3)|AVL_TILT_LEFT);} while (0)

//------------------------------------------------------------------

//#define avl_entry(ptr, type, member) container_of(ptr, type, member)

/* Public types ------------------------------------------------------------*/

struct avl_node // avl 节点
{
    unsigned long  avl_parent; 
    struct avl_node *avl_left;
    struct avl_node *avl_right;
};

struct avl_root // avl 树根
{
    struct avl_node *avl_node;
};



/* Public variables ---------------------------------------------------------*/

/* Public function prototypes 对外可用接口 -----------------------------------*/

// 设置 avl 节点的父节点为 p
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
