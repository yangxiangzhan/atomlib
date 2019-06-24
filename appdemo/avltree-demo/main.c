
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


#include "avltree.h"



#define TEST_MAX  200//测试点数

#define DEBUG_INSERT      //添加过程打印


#ifdef DEBUG_INSERT
#	define debug_insert(...) printf(__VA_ARGS__)
#else
#	define debug_insert(...)
#endif

#define treefile "/treedata2.txt"



typedef struct mydata
{
	uint32_t myvalue;	//树值
	struct avl_node avlnode;//节点
}
mydata_t;


struct avl_root mydata_root = { NULL };//树根
struct mydata   mydata_buf[TEST_MAX] = {0};

uint32_t myvalue_record[TEST_MAX] = { 0 };

uint16_t allnum = 0; //目前共有几个点

/**
	* @brief    mydata_insert
	*           把 insertdata 插入 avl 树
	* @param    所需插入树的结构体指针
	* @return   成功返回0，节点已在树上返回 -1
*/
static int mydata_insert(struct mydata * insertdata)
{
	struct avl_root *root = &mydata_root;            // root of avl tree
	struct avl_node **insert_pos = &root->avl_node;//insert position
	struct avl_node *parent = NULL;

	debug_insert("## insert %d:\r\n", insertdata->myvalue);

	/* Figure out where to put new node */
	while (*insert_pos)
	{
		struct mydata * treedata = container_of(*insert_pos, struct mydata, avlnode);

		parent = *insert_pos;
		if (insertdata->myvalue < treedata->myvalue)
		{
			insert_pos = &((*insert_pos)->avl_left);
			debug_insert("-");
		}
		else if (insertdata->myvalue > treedata->myvalue)
		{
			insert_pos = &((*insert_pos)->avl_right);
			debug_insert("+");
		}
		else
			return -1;
	}

	debug_insert("\r\n");

	/* Add new node and rebalance tree. */
	avl_insert(root, &insertdata->avlnode, parent, insert_pos);

	debug_insert("\r\n\r\n");

	return 0;
}



/**
	* @brief    mydata_search
	*           根据键值对 avl 树进行搜索
	* @param    searchvalue 值
	* @return   搜索成功返回键值所在结构体指针，否则返回 NULL
*/
static struct mydata *mydata_search(uint32_t searchvalue)
{
	struct avl_root *root = &mydata_root;
	struct avl_node *node = root->avl_node;//从根节点开始搜索

	while (node) //为 NULL 即搜索结束
	{
		struct mydata *treedata = container_of(node, struct mydata, avlnode);

		if (searchvalue < treedata->myvalue)
			node = node->avl_left;
		else 
		if (searchvalue > treedata->myvalue)
			node = node->avl_right;
		else 
			return treedata;
	}

	return NULL;
}






/**
	* @brief    tree_deep
	*           递归获取二叉树高度
	* @param    二叉树节点
	* @return   二叉树高度
*/
int tree_deep(struct avl_node *node)
{
	int deep = 0;
	if (node)
	{
		int leftdeep = tree_deep(node->avl_left);
		int rightdeep = tree_deep(node->avl_right);
		deep = leftdeep >= rightdeep ? leftdeep + 1 : rightdeep + 1;
	}
	return deep;
}


/**
	* @brief    tree_deep
	*           获取第 ideep 层的第 iwidth 个节点
	        9       
		  /   \
		 5     15
		/ \   /  \
	   4   6 12  16   15 为第二层第二个节点 ，12 为第三层第三个节点	   
	* @param    二叉树根节点
	* @return   二叉树节点指针，或 NULL
*/
struct avl_node * get_tree_node(const struct avl_root *root, int ideep, int iwidth)
{
	struct avl_node *node = root->avl_node;
	int path = iwidth - 1;

	if (ideep < 2) 
		return node;

	if (iwidth >(1 << (ideep - 1))) 
		return NULL;

	for (int step = 1 << (ideep - 2); step ; step >>= 1)
	{
		node = (step & path) ? node->avl_right : node->avl_left;
		if (!node) 
			break;
	}

	return node;
}

/**
	* @brief    print_mytree
	*           打印二叉树
	* @param    二叉树根节点
	* @return   void
*/
void print_mytree(const struct avl_root *root)
{
	struct avl_node * node = root->avl_node;
	struct mydata   * myavldata;

	uint32_t spacesum;

	int spacec;
	int linec; //下划线计数
	int emptyc; //下划线计数
	int widthc;
	int widthmax;

	int mytreedeep = tree_deep(node);

	spacesum = 1 << (mytreedeep - 1);

	for (int deepc = 1; deepc <= mytreedeep; ++deepc)
	{
		widthmax = 1 << (deepc - 1);

		emptyc = (spacesum >> deepc);
		
		linec = (emptyc / 2 > 0) ? (emptyc * 3 - 4) : 0;
		
		if (linec)
			spacec = (emptyc > 0) ? (emptyc * 6 + 3) : 1; //空格数
		else
			spacec = (emptyc > 0) ? 7 : 1;
		
		
		for (widthc = 1; widthc <= widthmax; ++widthc)
		{
			if (widthc < 2) 
			{
				for (uint8_t i = 0;i < spacec / 2 ; ++i) 
					putchar(' ');
			}
			else
			{
				for (uint8_t i = 0;i < spacec ; ++i) 
					putchar(' ');
			}

			for (uint8_t i = 0;i < linec ; ++i) 
				putchar('_');
			
			if ((node = get_tree_node(root, deepc, widthc)))
			{
				myavldata = container_of(node, struct mydata, avlnode);
				switch (avl_scale(node))
				{
					case AVL_BALANCED  :printf(" %3d ", myavldata->myvalue); break;
					case AVL_TILT_LEFT :printf(".%3d ", myavldata->myvalue); break;
					case AVL_TILT_RIGHT:printf(" %3d.", myavldata->myvalue); break;
				}
			}
			else
			{
				printf(" [ ] ");
			}
			
			for (uint8_t i = 0; i < linec ; ++i) 
				putchar('_');
		}
		printf("\r\n");
	}

	printf("--------------------deep : %d--------------------\r\n", mytreedeep);
}



void build_mytree(uint16_t nodenum) 
{
	uint32_t myvalue;
	struct mydata * mydatanode;
	
	if ( allnum < nodenum )
	{
		while(allnum < nodenum) //随机添加样点直到样点数为 nodenum
		{
			uint32_t rand_value = rand() % 1000;
			myvalue_record[allnum] = rand_value;
			
			mydatanode = &mydata_buf[allnum];
			mydatanode->myvalue = rand_value;
			
			if (0 != mydata_insert(mydatanode))
				printf("this number is on the tree\r\n");
			else
				++allnum;
		}
	}
	else
	{
		printf("delete nodes:\r\n");
		while (allnum > nodenum) //随机删除样点直到样点数为 nodenum
		{
			int count = 0;
			uint32_t del_index = rand() % (--allnum);
			
			myvalue = myvalue_record[del_index];
			mydatanode = mydata_search(myvalue);
			
			printf("delete %d\r\n",myvalue);
			
			avl_delete(&mydata_root, &mydatanode->avlnode);
			
			myvalue_record[del_index] = myvalue_record[allnum];

			for (count = 0 ; count < allnum ; ++count)
			{
				if (mydata_buf[count].myvalue == myvalue)
					break;
			}
			
			if (count != allnum)
			{
				avl_replace_node(&mydata_buf[allnum].avlnode,&mydata_buf[count].avlnode,&mydata_root) ;
				mydata_buf[count].myvalue = mydata_buf[allnum].myvalue;
			}
		}		
	}
}




//暴力测试，随机添加 TEST_MAX 个样点再进行随机删除
int main(void)
{
	struct mydata * data_node;
	
	srand((int)time(0));
	
	/* 暴力测试，随机添加样点再进行随机删除
	build_mytree(100);
	build_mytree(50);
	build_mytree(150);
	build_mytree(100);*/
	build_mytree(180);
	build_mytree(140);
	build_mytree(180);
	build_mytree(20);
	
	print_mytree(&mydata_root); //打印剩下的树
	
	printf("\r\n\ttest delete:\r\n");

	while (allnum)
	{
		int myvalue , count = 0; 
		
		printf("\r\ninput a number('+':insert,'-':delete):");
		scanf("%d", &myvalue);
		
		if (myvalue < 0)
		{
			myvalue = 0 - myvalue;
			printf("delete %d\r\n",myvalue);
			data_node = mydata_search(myvalue); //搜索键值所在的二叉树节点

			if (data_node != NULL)//如果键值在树上，删除
			{
				avl_delete(&mydata_root, &data_node->avlnode);

				for (--allnum ; count < allnum ; ++count)
				{
					if (mydata_buf[count].myvalue == myvalue) 
						break;
				}
				
				if (count != allnum)
				{
					avl_replace_node(&mydata_buf[allnum].avlnode,&mydata_buf[count].avlnode,&mydata_root) ;
					mydata_buf[count].myvalue = mydata_buf[allnum].myvalue;
				}
				
				print_mytree(&mydata_root);
			}
		}
		else
		{
			struct mydata * insert_data = &mydata_buf[allnum];
			insert_data->myvalue = myvalue;
			
			if (0 != mydata_insert(insert_data))
			{
				printf("this number is on the tree\r\n");
			}
			else
			{
				printf("insert %d\r\n",myvalue);
				print_mytree(&mydata_root); //打印剩下的树
				++allnum;
			}
		}
	}
}



