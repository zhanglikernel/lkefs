#ifndef _B_TREE_
#define _B_TREE_

#include <linux/mempool.h>

#include "common.h"

#ifndef BLOCK_SIZE
#define BLOCK_SIZE  512
#endif

struct lke_key_value{
	unsigned long key;
	void* data;
};

#define LKE_BTREE_M_LEN ((BLOCK_SIZE - sizeof(struct lke_btree_node*)) / (sizeof(struct lke_key_value) + sizeof(struct lke_btree_node*)))
#define LKE_BTREE_C_LEN (LKE_BTREE_M_LEN + 1)

struct lke_btree_node{
    struct lke_key_value pair[LKE_BTREE_M_LEN];
    struct lke_btree_node *child[LKE_BTREE_C_LEN];
};

struct lke_btree_head{
    mempool_t* mempool;
    struct lke_btree_node* root;
};

#define LKE_BTREE_HEAD_SIZE sizeof(struct lke_btree_head)

#endif
