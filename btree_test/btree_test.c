#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/random.h>
#include <linux/string.h>

#include "lke_btree.h"

void test_value_func(void *value, void *data){
    printk(KERN_INFO"%d ", (int)value);
}

void test_node_func(struct lke_btree_node* node, void *data){
    char buf[1024];
    int len;
    int i;
    len = lke_btree_key_len(node);
    printk(KERN_INFO "node address:%p len:%d.\n", node, len);
    memset(buf, 0, sizeof(buf));
    for( i = 0; i < LKE_BTREE_M_LEN; i ++){
        sprintf(buf + strlen(buf), "\t%lu", lke_get_btree_node_key(node, i));
    }
    printk(KERN_INFO "%s.\n", buf);
    memset(buf, 0, sizeof(buf));
    for( i = 0; i < LKE_BTREE_M_LEN; i ++){
        sprintf(buf + strlen(buf), "\t%lu", (unsigned long)lke_get_btree_node_data(node, i));
    }
    printk(KERN_INFO "%s.\n", buf);
    len = lke_btree_child_len(node);
    memset(buf, 0, sizeof(buf));
    for( i = 0; i < len; i ++ ){
        sprintf(buf + strlen(buf), "%p\t", lke_get_btree_node_child(node, i));
    }
    printk(KERN_INFO "%s.\n", buf);
}

static int __init lke_test_init(void){
    int i;
    u64 key[22] = {39, 22, 97, 41, 53, 13, 21, 40, 30, 27, 33, 36, 35, 34, 24, 29, 26, 17, 28, 23, 31, 32};
    u64 rm_key[4] = {21, 27, 32, 40};
    void* data[22];
    struct lke_btree_head* head;
    int result;
    int rtval;
    for( i = 0; i < 22; i ++ ){
        data[i] = (void*)(key[i] + 2);
    }
    head = lke_btree_head_alloc();
    if(NULL == head){
        result = -1;
        goto out;
    }
    if((rtval = lke_btree_init(head)) < 0 ){
        result = -1;
        goto out;
    }
    for( i = 0; i < 22; i ++ ){
        lke_btree_insert(head, key[i], data[i]);
    }
    lke_btree_traversal(head, test_node_func, NULL);
    printk(KERN_INFO "======================================\n");
    for( i = 0; i < 4; i ++ ){
        lke_btree_remove(head, rm_key[i]);
    }
    lke_btree_traversal(head, test_node_func, NULL);
    lke_btree_value_traversal(head, test_value_func, NULL);
    lke_btree_destroy(head);
    result = 0;
out:
    return result;
}

static void __exit lke_test_exit(void){
    printk(KERN_INFO "quit test.\n");
}

module_init(lke_test_init);
module_exit(lke_test_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Test");
