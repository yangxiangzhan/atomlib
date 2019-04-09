/**
******************************************************************************
* @file           avltree.c
* @author         ��ô��
* @brief          avltree file. ƽ���������ʵ��
******************************************************************************
*
* COPYRIGHT(c) 2018 GoodMorning
*
******************************************************************************
*/
/* Includes ---------------------------------------------------*/
#include <stdio.h>
#include <stdint.h>
#include "avltree.h"

/* Private types ------------------------------------------------------------*/

/* Private macro ------------------------------------------------------------*/

//#define DEBUG_AVL_OPERATION //���Խӿ�

#ifdef DEBUG_AVL_OPERATION
#	define AVL_DEBUG(...) printf(__VA_ARGS__)
#else
#	define AVL_DEBUG(...)
#endif

#define   __left_hand_delete_track_back(node,root)   __right_hand_insert_track_back(node,root) 
#define   __right_hand_delete_track_back(node,root)  __left_hand_insert_track_back(node,root) 

/* Private variables --------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

static void __avl_rotate_right(struct avl_node *node, struct avl_root *root);
static void __avl_rotate_left(struct avl_node *node, struct avl_root *root);

static int __avl_balance_right(struct avl_node *node, struct avl_root *root);
static int __avl_balance_left(struct avl_node *node, struct avl_root *root);

static int __left_hand_insert_track_back(struct avl_node *node, struct avl_root *root);
static int __right_hand_insert_track_back(struct avl_node *node, struct avl_root *root);

/* Gorgeous Split-line -----------------------------------------------*/

/**
	* @brief    __avl_rotate_right
	*           �� node �ڵ���е���������
	* @param    node        �������ڵ�
	* @param    root        ��������
	* @return   
	*/
static void __avl_rotate_right(struct avl_node *node, struct avl_root *root)
{
	//����*nodeΪ���Ķ�����������ת��������֮��nodeָ���µ��������
	//����ת����֮ǰ���������ĸ����
	struct avl_node *left = node->avl_left;
	struct avl_node *parent = avl_parent(node);

	if ((node->avl_left = left->avl_right)) //�ȸ�ֵ��Ȼ���ж�ֵ node->avl_left 
		avl_set_parent(left->avl_right, node);

	left->avl_right = node;

	avl_set_parent(left, parent);

	if (NULL == parent)
		root->avl_node = left;
	else
	if (node == parent->avl_right)
		parent->avl_right = left;
	else
		parent->avl_left = left;

	avl_set_parent(node, left);
}


/**
	* @brief    __avl_rotate_left
	*           �� node �ڵ���е���������
	* @param    node        �������ڵ�
	* @param    root        ��������
	* @return
	*/
static void __avl_rotate_left(struct avl_node *node, struct avl_root *root)
{
	struct avl_node *right = node->avl_right;
	struct avl_node *parent = avl_parent(node);

	if ( (node->avl_right = right->avl_left) )  //�ȸ�ֵ��Ȼ���ж�ֵ node->avl_right 
		avl_set_parent(right->avl_left, node);

	right->avl_left = node;

	avl_set_parent(right, parent);

	if ( NULL == parent)
		root->avl_node = right;
	else
	if (node == parent->avl_left)
		parent->avl_left = right;
	else
		parent->avl_right = right;

	avl_set_parent(node, right);
}


/**
	* @brief    __avl_balance_left
	*           �� node �ڵ������������������� �� node �ڵ������ƽ�⴦������ƽ������
	* @param    node        �������ڵ�
	* @param    root        ��������
	* @return   ����ԭ node ����λ�ã�����ƽ�⴦��ʹ���ĸ߶Ƚ����˷��� -1�����򷵻�0
	*/
static int __avl_balance_left(struct avl_node *node, struct avl_root *root)
{
	int retl = 0;
	struct avl_node * left_child = node->avl_left;
	struct avl_node * left_right_child;

	if(left_child)
	{
		int left_child_scale = avl_scale(left_child);
		left_right_child = left_child->avl_right;
		
#ifdef USE_DELETE_NODE
		if (AVL_BALANCED == left_child_scale)
		{
			struct avl_node * left_left_child = left_child->avl_left;
			int left_right_child_scale = avl_scale(left_right_child);
			__avl_rotate_left(node->avl_left, root); //��*node������������ƽ�⴦��
			__avl_rotate_right(node, root);          //��*node����ƽ�⴦��
			avl_set_tilt_left(left_child);
			avl_set_tilt_left(left_right_child);
			
			AVL_DEBUG("L_R\r\n");

			if (left_right_child_scale == AVL_BALANCED)
				avl_set_balanced(node);
			else 
			if (left_right_child_scale == AVL_TILT_LEFT)
				avl_set_tilt_right(node);
			else {
				int left_left_child_scale = avl_scale(left_left_child) ;
				avl_set_balanced(node);
				AVL_DEBUG("->"); //�ݹ�
				retl = __avl_balance_left(left_child, root);//���������ݹ�,����Ҫ���ݵݹ�������ƽ������
				if ((retl < 0)||(left_left_child_scale != AVL_BALANCED))
					avl_set_balanced(left_right_child);
			}
		}
		else
#endif
		if (AVL_TILT_LEFT == left_child_scale)
		{
			__avl_rotate_right(node,root);
			avl_set_balanced(node);
			avl_set_balanced(left_child);
			retl = -1;//�߶ȱ����
			AVL_DEBUG("R\r\n");
		}
		else
		{
			__avl_rotate_left(node->avl_left,root);  //��*node������������ƽ�⴦��
			__avl_rotate_right(node,root);      //��*node����ƽ�⴦��
			
			switch(avl_scale(left_right_child))
			{
				case AVL_BALANCED:
					avl_set_balanced(node);
					avl_set_balanced(left_child);
					break;
				case AVL_TILT_RIGHT:
					avl_set_balanced(node);
					avl_set_tilt_left(left_child);
					break;
				case AVL_TILT_LEFT:
					avl_set_balanced(left_child);
					avl_set_tilt_right(node);
					break;
			}
			
			avl_set_balanced(left_right_child);
			retl = -1;//�߶ȱ��
			
			AVL_DEBUG("L_R\r\n");
		}
	}
	return retl;
}



/**
	* @brief    __avl_balance_right
	*           �� node �ڵ������������������� �� node �ڵ������ƽ�⴦������ƽ������
	* @param    node        �������ڵ�
	* @param    root        ��������
	* @return   ����ԭ node ��������λ�ã�����ƽ�⴦��ʹ���ĸ߶Ƚ����˷��� -1�����򷵻�0
	*/
static int __avl_balance_right(struct avl_node *node, struct avl_root *root)
{
	int ret = 0;
	struct avl_node * right_child = node->avl_right;
	struct avl_node * right_left_child = right_child->avl_left;

	if(right_child) {

		int right_child_scale = avl_scale(right_child);
		
#ifdef USE_DELETE_NODE
		if (AVL_BALANCED == right_child_scale)  {
			int right_left_child_scale = avl_scale(right_left_child);
			__avl_rotate_right(node->avl_right,root);
			__avl_rotate_left(node,root);
			avl_set_tilt_right(right_left_child);
			avl_set_tilt_right(right_child);
			
			AVL_DEBUG("R_L\r\n");
			
			if (right_left_child_scale == AVL_BALANCED)
				avl_set_balanced(node);
			else 
			if (right_left_child_scale == AVL_TILT_RIGHT)
				avl_set_tilt_left(node);
			else {
				struct avl_node * right_right_child = right_child->avl_right;
				int right_right_child_scale = avl_scale(right_right_child);
				avl_set_balanced(node);
				AVL_DEBUG("->");
				ret = __avl_balance_right(right_child, root);//��ݹ�һ��
				if ((ret < 0)||(right_right_child_scale != AVL_BALANCED)) 
					avl_set_balanced(right_left_child);
			}	
		}
		else
#endif
		if (AVL_TILT_RIGHT == right_child_scale)
		{
			AVL_DEBUG("L\r\n");
			__avl_rotate_left(node, root);
			avl_set_balanced(node);
			avl_set_balanced(right_child);
			ret = -1;//�߶ȱ����//
		}
		else {
			__avl_rotate_right(node->avl_right,root);
			__avl_rotate_left(node,root);

			AVL_DEBUG("R_L\r\n");
			switch(avl_scale(right_left_child)) {//��ת��Ҫ����ƽ������
			
				case AVL_TILT_LEFT:
					avl_set_tilt_right(right_child);
					avl_set_balanced(node);
					break;
				case AVL_BALANCED:
					avl_set_balanced(node);
					avl_set_balanced(right_child);
					break;
				case AVL_TILT_RIGHT:
					avl_set_balanced(right_child);
					avl_set_tilt_left(node);
					break;
			}
			
			avl_set_balanced(right_left_child);
			ret = -1;//
		}
	}
	return ret;
}



/**
	* @brief    __left_hand_insert_track_back
	*           �� node �ڵ�������������˲��룬����ƽ�����ӣ���ʧ������ƽ�⴦��
	* @param    node        �������ڵ�
	* @param    root        ��������
	* @return   ����ԭ node ��������λ�ã�����ƽ�⴦��ʹ���ĸ߶Ƚ����˷��� -1�����򷵻�0
	*/
static int __left_hand_insert_track_back(struct avl_node *node, struct avl_root *root)
{
	switch(avl_scale(node)) {
		case AVL_BALANCED  :
			avl_set_tilt_left(node);
			return 0;
			
		case AVL_TILT_RIGHT:
			avl_set_balanced(node);
			return -1;
			
		case AVL_TILT_LEFT :
			return __avl_balance_left(node,root);
	}
	
	return 0;
}



/**
	* @brief    __left_hand_insert_track_back
	*           �� node �ڵ�������������˲��룬����ƽ�����ӣ���ʧ������ƽ�⴦��
	* @param    node        �������ڵ�
	* @param    root        ��������
	* @return   ����ԭ node ��������λ�ã����벢ƽ���߶Ƚ����˷��� -1�����򷵻�0
*/
static int __right_hand_insert_track_back(struct avl_node *node, struct avl_root *root)
{
	switch(avl_scale(node)) {
		case AVL_BALANCED:
			avl_set_tilt_right(node);//���ڵ�����
			return 0;                //�� node Ϊ�������߶ȱ��ı䣬��δʧ��
			
		case AVL_TILT_LEFT:
			avl_set_balanced(node);//���ڵ�ƽ��
			return -1;             //�� node Ϊ�������߶Ȳ��ı�
			
		case AVL_TILT_RIGHT://�� node Ϊ��������ʧ�⣬��ƽ�⴦��
			return __avl_balance_right(node,root);//
	}
	
	return 0;
}


/**
	* @brief    avl_insert avl ������
	* @param    root        ��������
	* @param    insertnode  ����Ҫ����Ķ������ڵ�
	* @param    parent      ����Ҫ����Ķ������ڵ�ĸ��ڵ�
	* @param    avl_link    ����Ҫ����Ķ������ڵ��ڸ��ڵ��ĸ�Ҷ
	* @return   void
*/
void avl_insert(
	struct avl_root *root,
	struct avl_node * insertnode,
	struct avl_node * parent,
	struct avl_node ** avl_link)
{
	int taller = 1;

	struct avl_node *gparent = NULL;

	uint8_t parent_gparent_path = 0;
	uint8_t backtrack_path = 0;

	insertnode->avl_parent = (unsigned long)parent;
	insertnode->avl_left = insertnode->avl_right = NULL;

	*avl_link = insertnode;

	if (root->avl_node == insertnode) 
		return;

	if (AVL_BALANCED != avl_scale(parent)) {
		avl_set_balanced(parent);//���ڵ�ƽ��
		return;//��û���߷���
	}

	backtrack_path = (insertnode == parent->avl_left) ? AVL_TILT_LEFT : AVL_TILT_RIGHT;

	//�������ˣ���Ҫ����ƽ��
	while (taller && parent) {
		//����ƽ����̻�ı����Ľṹ���ȼ�¼�游�ڵ�Ͷ�Ӧ�Ļ���·������
		if ( (gparent = avl_parent(parent)) )//�ȸ�ֵ���ж�
			parent_gparent_path = (parent == gparent->avl_right) ? AVL_TILT_RIGHT : AVL_TILT_LEFT;

		if (backtrack_path == AVL_TILT_RIGHT)
			taller += __right_hand_insert_track_back(parent, root);
		else
			taller += __left_hand_insert_track_back(parent, root);

		backtrack_path = parent_gparent_path; //����
		parent = gparent;
	}
}

#ifdef USE_DELETE_NODE
void avl_delete(struct avl_root *root, struct avl_node * node)
{
	struct avl_node *child, *parent;
	struct avl_node *gparent = NULL;
	int lower = 1;

	uint8_t parent_gparent_path = 0;
	uint8_t backtrack_path = 0;

	if (!node->avl_left) { //�����ɾ�ڵ㲻����������
	
		parent = avl_parent(node);

		if ((child = node->avl_right)) //�����������븸�ڵ���
			avl_set_parent(child, parent);

		if (parent) {
			if (parent->avl_left == node) {
				parent->avl_left = child;
				backtrack_path = AVL_TILT_LEFT;
			} 
			else {
				parent->avl_right = child;
				backtrack_path = AVL_TILT_RIGHT;
			}
		} 
		else { //if (NULL == parent) 
			root->avl_node = child;
			if (child) 
				avl_set_balanced(child);
			return;
		}
	}
	else 
	if (!node->avl_right) {//�����ɾ�ڵ������������������������
	
		child = node->avl_left;

		parent = avl_parent(node);

		avl_set_parent(child, parent);

		if (parent) {
			if (parent->avl_left == node) {
				parent->avl_left = child;
				backtrack_path = AVL_TILT_LEFT;
			} 
			else {
				parent->avl_right = child;
				backtrack_path = AVL_TILT_RIGHT;
			}
		} 
		else { //if (NULL == parent) 
			root->avl_node = child;
			if (child) 
				avl_set_balanced(child);
			return;
		}
	}
	else {//��ɾ�ڵ㼴������������Ҳ����������
	
		struct avl_node *old = node, *left;

		node = node->avl_right;

		while ((left = node->avl_left) != NULL) //�ҵ�����������С�������
			node = left;

		if (avl_parent(old)){ //�ɵ����ڸ��ڵ�
			if (avl_parent(old)->avl_left == old)
				avl_parent(old)->avl_left = node;
			else
				avl_parent(old)->avl_right = node;
		}
		else {
			root->avl_node = node;
		}
		
		child = node->avl_right;  //��С���������ҽڵ�
		parent = avl_parent(node);//��С�������ĸ��ڵ�

		if (parent == old) {//Ҫɾ���Ľڵ��������û���������������(1)û���ҽڵ�,(2)����һ���ҽڵ�  
			backtrack_path = AVL_TILT_RIGHT;
			parent = node;
			node->avl_parent = old->avl_parent;
			node->avl_left = old->avl_left;
			avl_set_parent(old->avl_left, node);
		} 
		else { //Ҫɾ���Ľڵ���������������� 
			backtrack_path = AVL_TILT_LEFT;
			parent->avl_left = child;          //��С���������ҽڵ�ĸ��ڵ�
			node->avl_right = old->avl_right;
			avl_set_parent(old->avl_right, node);

			node->avl_parent = old->avl_parent;
			node->avl_left = old->avl_left;
			avl_set_parent(old->avl_left, node);
			if (child) 
				avl_set_parent(child, parent); //Ҫɾ���Ľڵ����������������ĩβ������ҽڵ�
		}
	}

	//�����ˣ���Ҫ����ƽ�⣬ֱ�����ݵ����ڵ�
	while (lower && parent) {
		if ( (gparent = avl_parent(parent)) )//(parent && (gparent = avl_parent(parent)))//�ȸ�ֵ���ж�
			parent_gparent_path = (parent == gparent->avl_right) ? AVL_TILT_RIGHT : AVL_TILT_LEFT;

		if (backtrack_path == AVL_TILT_RIGHT) //�������ݵ�����ı����Ľṹ�������ȼ�¼ gparent �ͻ���·�� 
			lower = __right_hand_delete_track_back(parent, root);
		else
			lower = __left_hand_delete_track_back(parent, root);

		backtrack_path = parent_gparent_path;
		parent = gparent;
	}
}

#endif

/*
 * This function returns the first node (in sort oright_left_childer) of the tree.
 */
struct avl_node *avl_first(const struct avl_root *root)
{
    struct avl_node    *n;

    n = root->avl_node;
    if (!n)
        return NULL;
    while (n->avl_left)
        n = n->avl_left;
    return n;
}

struct avl_node *avl_last(const struct avl_root *root)
{
    struct avl_node    *n;

    n = root->avl_node;
    if (!n)
        return NULL;
    while (n->avl_right)
        n = n->avl_right;
    return n;
}

struct avl_node *avl_next(const struct avl_node *node)
{
    struct avl_node *parent;

    if (avl_parent(node) == node)
        return NULL;

    /* If we have a right-hand child, go down and then left as far
       as we can. */
    if (node->avl_right) {
        node = node->avl_right; 
        while (node->avl_left)
            node=node->avl_left;
        return (struct avl_node *)node;
    }

    /* No right-hand children.  Everything down and left is
       smaller than us, so any 'next' node must be in the general
       direction of our parent. Go up the tree; any time the
       ancestor is a right-hand child of its parent, keep going
       up. First time it's a left-hand child of its parent, said
       parent is our 'next' node. */
    while ((parent = avl_parent(node)) && node == parent->avl_right) //�ȸ�ֵ��Ȼ���ж�ֵ parent
        node = parent;

    return parent;
}

struct avl_node *avl_prev(const struct avl_node *node)
{
    struct avl_node *parent;

    if (avl_parent(node) == node)
        return NULL;

    /* If we have a left-hand child, go down and then right as far
       as we can. */
    if (node->avl_left) {
        node = node->avl_left; 
        while (node->avl_right)
            node=node->avl_right;
        return (struct avl_node *)node;
    }

    /* No left-hand children. Go up till we find an ancestor which
       is a right-hand child of its parent */
    while ((parent = avl_parent(node)) && node == parent->avl_left)//�ȸ�ֵ��Ȼ���ж�ֵ parent
        node = parent;

    return parent;
}

void avl_replace_node(struct avl_node *victim, struct avl_node *new,
             struct avl_root *root)
{
    struct avl_node *parent = avl_parent(victim);

    /* Set the surrounding nodes to point to the replacement */
    if (parent) {
        if (victim == parent->avl_left)
            parent->avl_left = new;
        else
            parent->avl_right = new;
    } else {
        root->avl_node = new;
    }
    if (victim->avl_left)
        avl_set_parent(victim->avl_left, new);
    if (victim->avl_right)
        avl_set_parent(victim->avl_right, new);

    /* Copy the pointers/colour from the victim to the replacement */
    *new = *victim;
}
