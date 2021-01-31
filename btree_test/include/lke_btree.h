#ifndef _B_TREE_
#define _B_TREE_

#include <linux/mempool.h>

#ifndef BLOCK_SIZE
#define BLOCK_SIZE  128
#endif

struct lke_key_value{
	u64 key;
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

struct lke_btree_head* lke_btree_head_alloc(void);
int lke_btree_init(struct lke_btree_head* head);
int lke_check_btree_node_key_index(struct lke_btree_node *node, int index);
int lke_check_btree_node_child_index(struct lke_btree_node *node, int index);
int lke_btree_key_len(struct lke_btree_node *node);
int lke_btree_child_len(struct lke_btree_node *node);
unsigned long lke_get_btree_node_key(struct lke_btree_node *node, int index);
void* lke_get_btree_node_data(struct lke_btree_node *node, int index);
struct lke_btree_node* lke_get_btree_node_child(struct lke_btree_node *node, int index);
int lke_set_btree_node_key(struct lke_btree_node* node, int index, u64 key);
int lke_set_btree_node_data(struct lke_btree_node* node, int index, void* data);
int lke_set_btree_node_child(struct lke_btree_node *node, int index, struct lke_btree_node* child_node);
void lke_btree_node_print(struct lke_btree_node *node);
struct lke_btree_node* lke_btree_node_alloc(struct lke_btree_head* head, gfp_t gfp);
void lke_btree_node_free(struct lke_btree_head *head, struct lke_btree_node* element);
void lke_btree_head_free(struct lke_btree_head* head);
int lke_btree_node_is_leaf(struct lke_btree_node *node);
int lke_btree_node_right_shift_key_with_keydatachild(struct lke_btree_node* node, int index, u64 key, void* data, struct lke_btree_node* childp);
int lke_btree_insert(struct lke_btree_head* head, u64 key, void* data);
int lke_btree_node_insert(struct lke_btree_node* node, int index, u64 key, void* data, struct lke_btree_node* leftchildp, struct lke_btree_node* rightchildp);
void* lke_btree_lookup(struct lke_btree_head* head, u64 key);
void lke_btree_traversal(struct lke_btree_head* head, void (*func)(struct lke_btree_node* node, void* dataStruct), void *data);
int lke_btree_remove(struct lke_btree_head* head, u64 key);
void lke_btree_destroy(struct lke_btree_head* head);
void lke_btree_value_traversal(struct lke_btree_head *head, void (*func)(void *value, void* dataStruct), void *data);

#endif
