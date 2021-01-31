#ifndef _B_TREE_
#define _B_TREE_

#include "common.h"
#include "treetest_fs.h"

#include <linux/mempool.h>

struct testkey_value_pair{
	long long key;
	void* data;
};

struct testbtree_node{
    struct key_value_pair[(TREE_TEST_BLOCK_SIZE - sizeof(void*)) / (sizeof(struct key_value_pair) + sizeof(void*))];
    void *child;
};

#define TEST_BTREE_M ((TREE_TEST_BLOCK_SIZE - sizeof(void*)) / (sizeof(struct key_value_pair) + sizeof(void*)))
#define TEST_BTREE_P (TEST_BTREE_M + 1)

struct test_btree_head{
    mempool_t* mempool;
    struct testbtree_node* root;
};

#endif
