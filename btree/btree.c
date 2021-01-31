#include <linux/module.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/string.h>

#include "lke_btree.h"

static struct kmem_cache *lke_btree_cachep;

struct lke_btree_head* lke_btree_head_alloc(void){
    struct lke_btree_head *bh;
    bh  = (struct lke_btree_head *)kmalloc(LKE_BTREE_HEAD_SIZE, GFP_KERNEL);
    if(NULL == bh){
        return NULL;
    }
    bh -> mempool = NULL;
    bh -> root = NULL;
    return bh;
}
EXPORT_SYMBOL_GPL(lke_btree_head_alloc);

void* lke_btree_node_slab_alloc(gfp_t gfp_mask, void *pool_data){
    return kmem_cache_alloc(lke_btree_cachep, gfp_mask);
}
EXPORT_SYMBOL_GPL(lke_btree_node_slab_alloc);

void lke_btree_node_slab_free(void *element, void* pool_data){
    kmem_cache_free(lke_btree_cachep, element);
}
EXPORT_SYMBOL_GPL(lke_btree_node_slab_free);

void lke_btree_head_free(struct lke_btree_head* head){
    if(NULL == head){
        return;
    }
    kfree(head);
}
EXPORT_SYMBOL_GPL(lke_btree_head_free);

static inline void _lke_btree_init(struct lke_btree_head* head){
    head -> mempool = NULL;
    head -> root = NULL;
}

int lke_btree_init(struct lke_btree_head* head){
    int result;
    if(NULL == head){
        result = -EFAULT;
        goto out;
    }
    _lke_btree_init(head);
    head -> mempool = mempool_create(0, lke_btree_node_slab_alloc, lke_btree_node_slab_free, NULL);
    if(!head -> mempool){
        result = -ENOMEM;
        goto out;
    }
    result = 0;
out:
    return result;
}
EXPORT_SYMBOL_GPL(lke_btree_init);

struct lke_btree_node* lke_btree_node_alloc(struct lke_btree_head* head, gfp_t gfp){
    struct lke_btree_node* node;
    node = mempool_alloc(head -> mempool, gfp);
    if(likely(node)){
        memset(node, 0, BLOCK_SIZE);
    }
    return node;
}
EXPORT_SYMBOL_GPL(lke_btree_node_alloc);

void lke_btree_node_free(struct lke_btree_head *head, struct lke_btree_node* element){
    if(NULL == head || NULL == head -> mempool || NULL == element ){
        // do nothing;
        goto out;
    }
    mempool_free(element, head -> mempool);
out:
    return;
}
EXPORT_SYMBOL_GPL(lke_btree_node_free);

int lke_check_btree_node_key_index(struct lke_btree_node *node, int index){
    int result;
    if( index < 0 || index >= LKE_BTREE_M_LEN ){
        result = -EINVAL;
        goto out;
    }
    result = 0;
out:
    return result;
}
EXPORT_SYMBOL_GPL(lke_check_btree_node_key_index);

int lke_check_btree_node_child_index(struct lke_btree_node *node, int index){
    int result;
    if( index < 0 || index >= LKE_BTREE_C_LEN ){
        result = -EINVAL;
        goto out;
    }
    result = 0;
out:
    return result;
}
EXPORT_SYMBOL_GPL(lke_check_btree_node_child_index);

int lke_btree_key_len(struct lke_btree_node *node){
    int len=LKE_BTREE_M_LEN;
    int i;
    for( i = 0 ; i < LKE_BTREE_M_LEN; i ++ ){
        if( 0 == node -> pair[i].key ){
            len = i;
            break;
        }
    }
    return len;
}
EXPORT_SYMBOL_GPL(lke_btree_key_len);

int lke_btree_child_len(struct lke_btree_node *node){
    int len = LKE_BTREE_C_LEN;
    int i;
    for( i = 0; i < LKE_BTREE_C_LEN; i ++ ){
        if( NULL == node -> child[i]){
            len = i;
            break;
        }
    }
    return len;
}
EXPORT_SYMBOL_GPL(lke_btree_child_len);

unsigned long lke_get_btree_node_key(struct lke_btree_node *node, int index){
    int len;
    unsigned long result;
    if( lke_check_btree_node_key_index(node, index) < 0 ){
        result = 0;
        goto out;
    }
    len = lke_btree_key_len(node);
    if( len <= index ){
        result = 0;
        goto out;
    }
    result = node -> pair[index].key;
out:
    return result;
}
EXPORT_SYMBOL_GPL(lke_get_btree_node_key);

void* lke_get_btree_node_data(struct lke_btree_node *node, int index){
    int len;
    void* result;
    if( lke_check_btree_node_key_index(node, index) < 0 ){
        result = NULL;
        goto out;
    }
    len = lke_btree_key_len(node);
    if( len <= index ){
        result =  NULL;
        goto out;
    }
    result = node -> pair[index].data;
out:
    return result;
}
EXPORT_SYMBOL_GPL(lke_get_btree_node_data);

struct lke_btree_node* lke_get_btree_node_child(struct lke_btree_node *node, int index){
    struct lke_btree_node* result;
    int len;
    if( lke_check_btree_node_child_index(node, index) < 0 ){
        result = NULL;
        goto out;
    }
    len = lke_btree_child_len(node);
    if( len <= index ){
        result =  NULL;
        goto out;
    }
    result = node -> child[index];
out:
    return result;
}
EXPORT_SYMBOL_GPL(lke_get_btree_node_child);

int lke_set_btree_node_key(struct lke_btree_node* node, int index, u64 key){
    int result;
    if( lke_check_btree_node_key_index(node, index) < 0 ){
        result = -EINVAL;
        goto out;
    }
    node -> pair[index].key = key;
    result = 0;
out:
    return result;
    
}
EXPORT_SYMBOL_GPL(lke_set_btree_node_key);

int lke_set_btree_node_data(struct lke_btree_node* node, int index, void* data){
    int result;
    if( lke_check_btree_node_key_index(node, index) < 0 ){
        result = -EINVAL;
        goto out;
    }
    node -> pair[index].data = data;
    result = 0;
out:
    return result;
}
EXPORT_SYMBOL_GPL(lke_set_btree_node_data);

int lke_set_btree_node_child(struct lke_btree_node *node, int index, struct lke_btree_node* child_node){
    int result;
    if( lke_check_btree_node_child_index(node, index) < 0 ){
        result = -EINVAL;
        goto out;
    }
    node -> child[index] = child_node;
    result = 0;
out:
    return result;
}
EXPORT_SYMBOL_GPL(lke_set_btree_node_child);

void lke_btree_node_print(struct lke_btree_node *node){
    int len;
    int i;
    char buf[1024];
    memset(buf, 0, sizeof(buf));
    len = lke_btree_key_len(node);
    printk(KERN_INFO "address:%p key_len:%d\n", node, len);
    for( i = 0; i < LKE_BTREE_M_LEN; i ++ ){
        sprintf(buf + strlen(buf), "\t%llu", node -> pair[i].key);
    }
    printk(KERN_INFO"%s.\n",buf);
    memset(buf, 0, sizeof(buf));
    for( i = 0; i < LKE_BTREE_M_LEN; i ++ ){
        sprintf(buf + strlen(buf), "\t%lu", (unsigned long)(node -> pair[i].data));
    }
    printk(KERN_INFO"%s.\n", buf);
    len = lke_btree_child_len(node);
    printk(KERN_INFO "child_len:%d\n", len);
    memset(buf, 0, sizeof(buf));
    for( i = 0; i < LKE_BTREE_C_LEN; i ++ ){
        sprintf(buf + strlen(buf), "%p\t", node -> child[i]);
    }
    printk(KERN_INFO"%s.\n",buf);
}
EXPORT_SYMBOL_GPL(lke_btree_node_print);

int lke_btree_node_is_leaf(struct lke_btree_node *node){
    return lke_btree_child_len(node) == 0;
}
EXPORT_SYMBOL_GPL(lke_btree_node_is_leaf);

int lke_btree_node_right_shift_key_with_keydatachild(struct lke_btree_node* node, int index, u64 key, void* data, struct lke_btree_node* childp){
    int i;
    int result;
    int len;
    if( 0 == key || NULL == data){
        result = -EINVAL;
        goto out;
    }
    if(lke_check_btree_node_key_index(node, index) < 0){
        result = -EINVAL;
        goto out;
    }
    len = lke_btree_key_len(node);
    if( index >= len ){
        result = -EINVAL;
        goto out;
    }
    for( i = len; i > index; i -- ){
        lke_set_btree_node_key(node, i, lke_get_btree_node_key(node, i - 1));
        lke_set_btree_node_data(node, i, lke_get_btree_node_data(node, i - 1));
        lke_set_btree_node_child(node, i + 1, lke_get_btree_node_child(node, i));
    }
    lke_set_btree_node_key(node, index, key);
    lke_set_btree_node_data(node, index, data);
    lke_set_btree_node_child(node, index + 1, childp);
    result = 0;
out:
    return result;
}
EXPORT_SYMBOL_GPL(lke_btree_node_right_shift_key_with_keydatachild);

int lke_btree_node_insert(struct lke_btree_node* node, int index, u64 key, void* data, struct lke_btree_node* leftchildp, struct lke_btree_node* rightchildp){
    int result;
    if( 0 == key || NULL == data ){
        result = -EINVAL;
        goto out;
    }
    if(lke_check_btree_node_key_index(node, index) < 0 ){
        result = -EINVAL;
        goto out;
    }
    lke_set_btree_node_key(node, index, key);
    lke_set_btree_node_data(node, index, data);
    if( NULL != leftchildp ){
        lke_set_btree_node_child(node, index, leftchildp);
    }
    if( NULL != rightchildp ){
        lke_set_btree_node_child(node, index + 1, rightchildp);
    }
    result = 0;
out:
    return result;
}
EXPORT_SYMBOL_GPL(lke_btree_node_insert);

static int _lke_btree_node_findpos(struct lke_btree_node* node, u64 key){
    int len;
    int i;
    int result = 0;
    unsigned long node_key;
    if( 0 == key ){
        result = -EINVAL;
        goto out;
    }
    len = lke_btree_key_len(node);
    for( i = 0 ; i < len; i ++ ){
        node_key = lke_get_btree_node_key(node, i);
        if( key == node_key ){
            result = i;
            goto out;
        }else if( node_key > key ){
            result = i;
            goto out;
        }
    }
    if( i == len ){
        result = len;
        goto out;
    }
out:
    return result;
}

static int _lke_btree_node_split(struct lke_btree_head* head, struct lke_btree_node* node, u64 *upkey, void **updata, struct lke_btree_node** leftchildp, struct lke_btree_node** rightchildp){
    int result;
    int i, j;
    int len = lke_btree_key_len(node);
    struct lke_btree_node* new_node = NULL;
    int mid;
    if( LKE_BTREE_M_LEN != len ){
        result = -EINVAL;
        goto out;
    }
    new_node = lke_btree_node_alloc(head, ZONE_NORMAL);
    if(NULL == new_node){
        result = -ENOMEM;
        goto out;
    }
    mid = len / 2;
    *upkey = lke_get_btree_node_key(node, mid);
    *updata = lke_get_btree_node_data(node, mid);
    for( j = 0, i = mid + 1; i < len; i ++, j++ ){
        lke_set_btree_node_key(new_node, j, lke_get_btree_node_key(node, i));
        lke_set_btree_node_data(new_node, j, lke_get_btree_node_data(node, i));
        lke_set_btree_node_child(new_node, j, lke_get_btree_node_child(node, i));
    }
    lke_set_btree_node_child(new_node, j, lke_get_btree_node_child(node, i));
    for( i = mid; i < len; i ++ ){
        lke_set_btree_node_key(node, i, 0);
        lke_set_btree_node_data(node, i, NULL);
        lke_set_btree_node_child(node, i + 1, NULL);
    }
    lke_set_btree_node_child(node, i + 1, NULL);
    *leftchildp = node;
    *rightchildp = new_node;
    result = 0;
out:
    return result;
}

static int _lke_btree_insert(struct lke_btree_head* head, struct lke_btree_node* node, u64 key, void* data, u64 *upkey, void **updata, struct lke_btree_node** leftchildp, struct lke_btree_node** rightchildp){
    int len;
    int result = 0;
    int pos;
    int is_leaf;
    int rtval;
    u64 split_key;
    void* split_data;
    struct lke_btree_node *split_leftchildp, *split_rightchildp;
    len = lke_btree_key_len(node);
    is_leaf = lke_btree_node_is_leaf(node);
    pos = _lke_btree_node_findpos(node, key);
    if( pos < 0 ){
        result = pos;
        goto out;
    }
    if( pos == len ){
        if( !is_leaf ){
            rtval = _lke_btree_insert(head, lke_get_btree_node_child(node, pos), key, data, &split_key, &split_data, &split_leftchildp, &split_rightchildp);
            if(1 == rtval){
                lke_btree_node_insert(node, pos, split_key, split_data, split_leftchildp, split_rightchildp);
            }
        }else{
            lke_set_btree_node_key(node, pos, key);
            lke_set_btree_node_data(node, pos, data);
        }
    }else if( lke_get_btree_node_key(node, pos) == key ){
        printk(KERN_INFO "key:%llu already in btree, insert failed.\n", key);
        result = -EINVAL;
        goto out;
    }else{
        if(!is_leaf){
            rtval = _lke_btree_insert(head, lke_get_btree_node_child(node, pos), key, data, &split_key, &split_data, &split_leftchildp, &split_rightchildp);
            if( 1 == rtval ){
                lke_btree_node_right_shift_key_with_keydatachild(node, pos, split_key, split_data, split_rightchildp);
                lke_btree_node_insert(node, pos, split_key, split_data, split_leftchildp, split_rightchildp);
            }
        }else{
            lke_btree_node_right_shift_key_with_keydatachild(node, pos, key, data, NULL);
        }
    }
    len = lke_btree_key_len(node);
    if( LKE_BTREE_M_LEN == len ){
        // split
        rtval = _lke_btree_node_split(head, node, &split_key, &split_data, &split_leftchildp, &split_rightchildp);
        if(rtval < 0){
            // split_fail remove key
            result = rtval;
            goto out;
        }
        *upkey = split_key;
        *updata = split_data;
        *leftchildp = split_leftchildp;
        *rightchildp = split_rightchildp;
        result = 1;
        goto out;
    }else{
        result = 0;
        goto out;
    }
out:
    return result;
}

int lke_btree_insert(struct lke_btree_head* head, u64 key, void* data){
    int result = 0;
    struct lke_btree_node* new_node = NULL;
    int rtval;
    u64 split_key;
    void* split_data;
    struct lke_btree_node *split_leftchildp, *split_rightchildp;
    if( NULL == head ){
        result = -EINVAL;
        goto out;
    }
    if( 0 == key || NULL == data ){
        result = -EINVAL;
        goto out;
    }
    if(head -> mempool == NULL){
        if((rtval = lke_btree_init(head)) < 0 ){
            result = -1;
            goto out;
        }
    }
    if( NULL == head -> root ){
        new_node = lke_btree_node_alloc(head, ZONE_NORMAL);
        if( NULL == new_node ){
            result = -ENOMEM;
            goto out;
        }
        head -> root = new_node;
    }
    
    rtval = _lke_btree_insert(head, head -> root, key, data, &split_key, &split_data, &split_leftchildp, &split_rightchildp);
    if( 1 == rtval ){
        new_node = lke_btree_node_alloc(head, ZONE_NORMAL);
        if( NULL == new_node ){
            result = -ENOMEM;
            goto out;
        }
        lke_btree_node_insert(new_node, 0, split_key, split_data, split_leftchildp, split_rightchildp);
        head -> root = new_node;
    }
    result = 0;
out:
    return result;
}
EXPORT_SYMBOL_GPL(lke_btree_insert);

static void* _lke_btree_lookup(struct lke_btree_node* node, u64 key){
    int len;
    int i;
    unsigned long node_key;
    void* result;
    if( NULL == node ){
        result = NULL;
        goto out;
    }
    len = lke_btree_key_len(node);
    for( i = 0; i < len; i ++ ){
        node_key = lke_get_btree_node_key(node, i);
        if(node_key == key){
            result = lke_get_btree_node_data(node, i);
            goto out;
        }else if(node_key > key){
            result = _lke_btree_lookup(lke_get_btree_node_child(node, i), key);
            break;
        }
    }
    if( i == len ){
        result = _lke_btree_lookup(lke_get_btree_node_child(node, len), key);
        goto out;
    }
out:
    return result;
}

void* lke_btree_lookup(struct lke_btree_head* head, u64 key){
    void* result;
    if( NULL == head -> root ){
        result = NULL;
        goto out;
    }
    result = _lke_btree_lookup(head -> root, key);
out:
    return result;
}
EXPORT_SYMBOL_GPL(lke_btree_lookup);

void _lke_btree_traversal(struct lke_btree_node *node, void (*func)(struct lke_btree_node* node, void* dataStruct), void *data){
    int len;
    int i;
    if( node == NULL ){
        goto out;
    }
    len = lke_btree_child_len(node);
    for( i = 0 ; i < len; i ++ ){
        _lke_btree_traversal(lke_get_btree_node_child(node, i), func, data);
    }
    func(node, data);
out:
    return;
}

void lke_btree_traversal(struct lke_btree_head* head, void (*func)(struct lke_btree_node* node, void* dataStruct), void *data){
    _lke_btree_traversal(head -> root, func, data);
}
EXPORT_SYMBOL_GPL(lke_btree_traversal);

void _lke_btree_value_traversal(struct lke_btree_node *node, void (*func)(void *value, void* dataStruct), void *data){
    int child_len, key_len;
    int i;
    int is_leaf = 0;
    if(NULL == node){
        goto out;
    }
    is_leaf = lke_btree_node_is_leaf(node);
    child_len = lke_btree_child_len(node);
    key_len = lke_btree_key_len(node);
    if(!is_leaf && key_len != child_len - 1){
        goto out;
    }
    for(i = 0; i < key_len; i ++ ){
        if(!is_leaf){
            _lke_btree_value_traversal(lke_get_btree_node_child(node,i), func, data);
        }
        func(lke_get_btree_node_data(node, i), data);
    }
    if(!is_leaf){
        _lke_btree_value_traversal(lke_get_btree_node_child(node,i), func, data);
    }
out:
    return;
}

void lke_btree_value_traversal(struct lke_btree_head *head, void (*func)(void *value, void* dataStruct), void *data){
    _lke_btree_value_traversal(head->root, func, data);
}
EXPORT_SYMBOL_GPL(lke_btree_value_traversal);


static int _find_biggest_key_and_data(struct lke_btree_node* node, u64* key, void** data){
    int is_leaf;
    int len;
    struct lke_btree_node* tmp_node;
    int result;
    if( NULL == node ){
        result = -1;
        goto out;
    }
    tmp_node = node;
    while(1){
        if(NULL == tmp_node){
            result = -1;
            goto out;
        }
        is_leaf = lke_btree_node_is_leaf(tmp_node);
        len = lke_btree_key_len(tmp_node);
        if(is_leaf){
            *key = lke_get_btree_node_key(tmp_node, len - 1);
            *data = lke_get_btree_node_data(tmp_node, len - 1);
            result = 0;
            break;
        }
        tmp_node = lke_get_btree_node_child(tmp_node, len);
    }
    result = 0;
out:
    return result;
}

static int lke_btree_leaf_left_shift(struct lke_btree_node* node, int index){
    int result;
    int len;
    int i;
    int is_leaf;
    if( NULL == node ){
        result = -1;
        goto out;
    }
    is_leaf = lke_btree_node_is_leaf(node);
    if(!is_leaf || lke_check_btree_node_key_index(node, index) < 0 ){
        result = -1;
        goto out;
    }
    len = lke_btree_key_len(node);
    if( index >= len ){
        result = -1;
        goto out;
    }
    for( i = index; i < len - 1; i ++ ){
        lke_set_btree_node_key(node, i, lke_get_btree_node_key(node, i + 1 ));
        lke_set_btree_node_data(node, i, lke_get_btree_node_data(node, i + 1 ));
    }
    lke_set_btree_node_key(node, i, 0);
    lke_set_btree_node_data(node, i, NULL);
    result = 0;
out:
    return result;
}

static int lke_btree_node_merge(struct lke_btree_head *head, struct lke_btree_node* node, int pos){
    int result;
    int len;
    int i, j;
    struct lke_btree_node* leftchildp, *rightchildp;
    int leftchildlen, rightchildlen;
    if( NULL == node ){
        result = -1;
        goto out;
    }
    if(lke_btree_node_is_leaf(node)){
        result = -1;
        goto out;
    }
    len = lke_btree_key_len(node);
    if( lke_check_btree_node_key_index(node, pos) < 0 || pos >= len ){
        result = -1;
        goto out;
    }
    leftchildp = lke_get_btree_node_child(node, pos);
    rightchildp = lke_get_btree_node_child(node, pos + 1);
    leftchildlen = lke_btree_key_len(leftchildp);
    rightchildlen = lke_btree_key_len(rightchildp);
    if( leftchildlen + rightchildlen + 1 >= LKE_BTREE_M_LEN ){
        result = -1;
        goto out;
    }
    lke_set_btree_node_key(leftchildp, leftchildlen, lke_get_btree_node_key(node, pos));
    lke_set_btree_node_data(leftchildp, leftchildlen, lke_get_btree_node_data(node, pos));
    for( i = 0, j = leftchildlen + 1; i < rightchildlen; i ++ , j ++){
        lke_set_btree_node_key(leftchildp, j, lke_get_btree_node_key(rightchildp, i));
        lke_set_btree_node_data(leftchildp, j, lke_get_btree_node_data(rightchildp, i));
        lke_set_btree_node_child(leftchildp, j, lke_get_btree_node_child(rightchildp, i));
    }
    lke_set_btree_node_child(leftchildp, j, lke_get_btree_node_child(rightchildp, i));
    for( i = pos; i < len - 1; i ++ ){
        lke_set_btree_node_key(node, i, lke_get_btree_node_key(node, i + 1));
        lke_set_btree_node_data(node, i, lke_get_btree_node_data(node, i + 1));
        lke_set_btree_node_child(node, i + 1, lke_get_btree_node_child(node, i + 2));
    }
    lke_set_btree_node_key(node, i, 0);
    lke_set_btree_node_data(node, i, NULL);
    lke_set_btree_node_child(node, i + 1, NULL);
    lke_btree_node_free(head, rightchildp);
    result = 0;
out:
    return result;
}

static int lke_btree_node_left_rotate(struct lke_btree_node* node, int pos){
    int result;
    int len;
    int i;
    struct lke_btree_node* leftchildp, *rightchildp;
    int leftchildlen, rightchildlen;
    if( NULL == node ){
        result = -1;
        goto out;
    }
    if(lke_btree_node_is_leaf(node)){
        result = -1;
        goto out;
    }
    len = lke_btree_key_len(node);
    if( lke_check_btree_node_key_index(node, pos) < 0 || pos >= len ){
        result = -1;
        goto out;
    }
    leftchildp = lke_get_btree_node_child(node, pos);
    rightchildp = lke_get_btree_node_child(node, pos + 1);
    leftchildlen = lke_btree_key_len(leftchildp);
    rightchildlen = lke_btree_key_len(rightchildp);
    lke_set_btree_node_key(leftchildp, leftchildlen, lke_get_btree_node_key(node, pos));
    lke_set_btree_node_data(leftchildp, leftchildlen, lke_get_btree_node_data(node, pos));
    lke_set_btree_node_child(leftchildp, leftchildlen + 1, lke_get_btree_node_child(rightchildp, 0));
    lke_set_btree_node_key(node, pos, lke_get_btree_node_key(rightchildp, 0));
    lke_set_btree_node_data(node, pos, lke_get_btree_node_data(rightchildp, 0));
    for( i = 0 ; i < rightchildlen - 1; i ++ ){
        lke_set_btree_node_key(rightchildp, i, lke_get_btree_node_key(rightchildp, i + 1));
        lke_set_btree_node_data(rightchildp, i, lke_get_btree_node_data(rightchildp, i + 1));
        lke_set_btree_node_child(rightchildp, i, lke_get_btree_node_child(rightchildp, i + 1));
    }
    lke_set_btree_node_child(rightchildp, i, lke_get_btree_node_child(rightchildp, i + 1));
    lke_set_btree_node_key(rightchildp, i, 0);
    lke_set_btree_node_data(rightchildp, i, NULL);
    lke_set_btree_node_child(rightchildp, i + 1, NULL);
    result = 0;
out:
    return result;
}

static int lke_btree_node_right_rotate(struct lke_btree_node* node, int pos){
    int result;
    int len;
    int i;
    struct lke_btree_node* leftchildp, *rightchildp;
    int leftchildlen, rightchildlen;
    if( NULL == node ){
        result = -1;
        goto out;
    }
    if(lke_btree_node_is_leaf(node)){
        result = -1;
        goto out;
    }
    len = lke_btree_key_len(node);
    if( lke_check_btree_node_key_index(node, pos) < 0 || pos >= len ){
        result = -1;
        goto out;
    }
    leftchildp = lke_get_btree_node_child(node, pos);
    rightchildp = lke_get_btree_node_child(node, pos + 1);
    leftchildlen = lke_btree_key_len(leftchildp);
    rightchildlen = lke_btree_key_len(rightchildp);
    for( i = rightchildlen; i > 0; i -- ){
        lke_set_btree_node_key(rightchildp, i, lke_get_btree_node_key(rightchildp, i - 1));
        lke_set_btree_node_data(rightchildp, i, lke_get_btree_node_data(rightchildp, i - 1));
        lke_set_btree_node_child(rightchildp, i + 1, lke_get_btree_node_child(rightchildp, i));
    }
    lke_set_btree_node_child(rightchildp, i + 1, lke_get_btree_node_child(rightchildp, i));
    lke_set_btree_node_key(rightchildp, i, lke_get_btree_node_key(node, pos));
    lke_set_btree_node_data(rightchildp, i, lke_get_btree_node_data(node, pos));
    lke_set_btree_node_child(rightchildp, i, lke_get_btree_node_child(leftchildp, leftchildlen));
    lke_set_btree_node_key(node, pos, lke_get_btree_node_key(leftchildp, leftchildlen - 1));
    lke_set_btree_node_data(node, pos, lke_get_btree_node_data(leftchildp, leftchildlen - 1));
    lke_set_btree_node_key(leftchildp, leftchildlen - 1, 0);
    lke_set_btree_node_data(leftchildp, leftchildlen - 1, NULL);
    lke_set_btree_node_child(leftchildp, leftchildlen, NULL);
    result = 0;
out:
    return result;
}

int _lke_btree_remove(struct lke_btree_head *head, struct lke_btree_node* node, u64 key){
    int result;
    int pos;
    int is_leaf;
    int rtval;
    int len;
    u64 biggest_key;
    void* biggest_data;
    int leftchildlen, rightchildlen, midchildlen;
    struct lke_btree_node* leftchildp, *rightchildp, *midchildp;
    if( NULL == node ){
        result = -1;
        goto out;
    }
    is_leaf = lke_btree_node_is_leaf(node);
    pos = _lke_btree_node_findpos(node, key);
    len = lke_btree_key_len(node);
    if( pos < 0 ){
        result = pos;
        goto out;
    }
    if( pos == len ){
        if( !is_leaf ){
            rtval = _lke_btree_remove(head, lke_get_btree_node_child(node, pos), key);
            if( rtval < 0 ){
                result = rtval;
                goto out;
            }else if( 1 == rtval ){
                leftchildp = lke_get_btree_node_child(node, pos - 1);
                leftchildlen = lke_btree_key_len(leftchildp);
                rightchildp = lke_get_btree_node_child(node, pos);
                rightchildlen = lke_btree_key_len(rightchildp);
                if( leftchildlen + rightchildlen + 1 < LKE_BTREE_M_LEN ){
                    lke_btree_node_merge(head, node, pos - 1);
                }else{
                    lke_btree_node_left_rotate(node, pos - 1);
                }
            }
        }else{
            result = -1;
            goto out;
        }
    }else if( lke_get_btree_node_key(node, pos) == key ){
        if( !is_leaf ){
            rtval = _find_biggest_key_and_data(lke_get_btree_node_child(node, pos), &biggest_key, &biggest_data);
            if( rtval < 0 ){
                result = -1;
                goto out;
            }
            lke_set_btree_node_key(node, pos, biggest_key);
            lke_set_btree_node_data(node, pos, biggest_data);
            rtval = _lke_btree_remove(head, lke_get_btree_node_child(node, pos), biggest_key);
            if( 1 == rtval){
                if( 0 == pos ){
                    leftchildp = lke_get_btree_node_child(node, pos);
                    leftchildlen = lke_btree_key_len(leftchildp);
                    rightchildp = lke_get_btree_node_child(node, pos + 1);
                    rightchildlen = lke_btree_key_len(rightchildp);
                    if( leftchildlen + rightchildlen + 1 < LKE_BTREE_M_LEN ){
                        lke_btree_node_merge(head, node, 0);
                    }else{
                        lke_btree_node_right_rotate(node, 0);
                    }
                }else{
                    midchildp = lke_get_btree_node_child(node, pos);
                    midchildlen = lke_btree_key_len(midchildp);
                    leftchildp = lke_get_btree_node_child(node, pos - 1);
                    leftchildlen = lke_btree_key_len(leftchildp);
                    rightchildp = lke_get_btree_node_child(node, pos + 1);
                    rightchildlen = lke_btree_key_len(rightchildp);
                    if( leftchildlen + midchildlen + 1 < LKE_BTREE_M_LEN ){
                        lke_btree_node_merge(head, node, pos - 1);
                    }else if( midchildlen + rightchildlen + 1 < LKE_BTREE_M_LEN ){
                        lke_btree_node_merge(head, node, pos);
                    }else{
                        lke_btree_node_right_rotate(node, pos);
                    }
                }
            }
        }else{
            lke_btree_leaf_left_shift(node, pos);
        }
    }else{
        if( !is_leaf ){
            rtval = _lke_btree_remove(head, lke_get_btree_node_child(node, pos), key);
            if( 1 == rtval){
                if( 0 == pos ){
                    leftchildp = lke_get_btree_node_child(node, pos);
                    leftchildlen = lke_btree_key_len(leftchildp);
                    rightchildp = lke_get_btree_node_child(node, pos + 1);
                    rightchildlen = lke_btree_key_len(rightchildp);
                    if( leftchildlen + rightchildlen + 1 < LKE_BTREE_M_LEN ){
                        lke_btree_node_merge(head, node, 0);
                    }else{
                        lke_btree_node_right_rotate(node, 0);
                    }
                }else{
                    midchildp = lke_get_btree_node_child(node, pos);
                    midchildlen = lke_btree_key_len(midchildp);
                    leftchildp = lke_get_btree_node_child(node, pos - 1);
                    leftchildlen = lke_btree_key_len(leftchildp);
                    rightchildp = lke_get_btree_node_child(node, pos + 1);
                    rightchildlen = lke_btree_key_len(rightchildp);
                    if( leftchildlen + midchildlen + 1 < LKE_BTREE_M_LEN ){
                        lke_btree_node_merge(head, node, pos - 1);
                    }else if( midchildlen + rightchildlen + 1 < LKE_BTREE_M_LEN ){
                        lke_btree_node_merge(head, node, pos);
                    }else{
                        lke_btree_node_right_rotate(node, pos);
                    }
                }
            }
        }else{
            result = -1;
            goto out;
        }
    }
    len = lke_btree_key_len(node);
    if( len < LKE_BTREE_M_LEN / 2 ){
        result = 1;
        goto out;
    }
    result = 0;
out:
    return result;
}

int lke_btree_remove(struct lke_btree_head* head, u64 key){
    int result;
    int rtval;
    int key_len;
    int child_len;
    struct lke_btree_node* tmp_node;
    if(key == 0){
        result = -EINVAL;
        goto out;
    }
    if( NULL == head ){
        result = -EINVAL;
        goto out;
    }
    if( NULL == head -> root ){
        result = -EINVAL;
        goto out;
    }
    rtval = _lke_btree_remove(head, head -> root, key);
    key_len = lke_btree_key_len(head -> root);
    child_len = lke_btree_child_len(head -> root );
    if(0 == key_len && 1 == child_len ){
        tmp_node = head -> root;
        head -> root = lke_get_btree_node_child(head -> root, 0);
        lke_btree_node_free(head, tmp_node);
    }else if( 0 == key_len && 0 == child_len ){
        lke_btree_node_free(head, head -> root);
        head -> root = NULL;
    }
out:
    return result;
}
EXPORT_SYMBOL_GPL(lke_btree_remove);

static void _lke_btree_destroy(struct lke_btree_head* head, struct lke_btree_node* node){
    int len;
    int i;
    struct lke_btree_node* childp;
    if( NULL == node ){
        goto out;
    }
    len = lke_btree_child_len(node);
    for( i = 0 ; i < len; i ++ ){
        childp = lke_get_btree_node_child(node, i);
        _lke_btree_destroy(head, childp);
    }
    lke_btree_node_free(head, node);
out:
    return;
}

void lke_btree_destroy(struct lke_btree_head* head){
    if(NULL == head){
        goto out;
    }
    if( NULL != head -> mempool ){
        _lke_btree_destroy(head, head -> root);
        mempool_destroy(head -> mempool);
    }
    lke_btree_head_free(head);
out:
    return;
}
EXPORT_SYMBOL_GPL(lke_btree_destroy);

static void __exit lke_btree_module_exit(void){
    kmem_cache_destroy(lke_btree_cachep);
}

static int __init lke_btree_module_init(void){
    int result;
    lke_btree_cachep = kmem_cache_create("lke_btree_node", BLOCK_SIZE, 0, SLAB_HWCACHE_ALIGN, NULL);
    printk("M:%ld %ld", LKE_BTREE_M_LEN, LKE_BTREE_C_LEN);
    if(!lke_btree_cachep){
        result = -ENOMEM;
    }else{
        result = 0;
    }
    return result;
}

module_init(lke_btree_module_init);
module_exit(lke_btree_module_exit);

MODULE_AUTHOR("LiZhang");
MODULE_LICENSE("GPL");
