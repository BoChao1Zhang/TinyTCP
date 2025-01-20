//
// Created by 32101 on 25-1-16.
//

#ifndef NLIST_H
#define NLIST_H


typedef struct _nlist_node_t {
    struct _nlist_node_t *pre;
    struct _nlist_node_t *next;
} nlist_node_t;

static inline void nlist_node_init(nlist_node_t *node) {
    node->next = node->pre = (nlist_node_t *) 0;
}

static inline nlist_node_t *nlist_node_pre(nlist_node_t *node) {
    return node->pre;
}

static inline nlist_node_t *nlist_node_next(nlist_node_t *node) {
    return node->next;
}

static inline void nlist_node_set_next(nlist_node_t *node, nlist_node_t *next) {
    node->next = next;
}

typedef struct _nlist_t {
    nlist_node_t *first;
    nlist_node_t *last;
    int count;
} nlist_t;

void nlist_init(nlist_t *list);

static inline int nlist_is_empty(nlist_t *list) {
    return list->count == 0;
}

static inline int nlist_count(nlist_t *list) {
    return list->count;
}

static inline nlist_node_t *nlist_first(nlist_t *list) {
    return list->first;
}

static inline nlist_node_t *nlist_last(nlist_t *list) {
    return list->last;
}

void nlist_insert(nlist_t* list,nlist_node_t* pre,nlist_node_t *node);
void nlist_insert_first(nlist_t *list, nlist_node_t *node);
void nlist_insert_last(nlist_t *list,nlist_node_t *node);


nlist_node_t* nlist_remove(nlist_t* list,nlist_node_t *node);
nlist_node_t* nlist_remove_first(nlist_t* list);
nlist_node_t* nlist_remove_last(nlist_t* list);

/**
 * @brief 计算父结构体中成员变量的偏移量
 *
 * 通过宏定义计算结构体中成员变量的偏移量。
 *
 * @param parent_type 父结构体的类型
 * @param node_name   成员变量的名称
 *
 * @return 成员变量在父结构体中的偏移量（以字节为单位）
 */
#define noffset_in_parent(parent_type, node_name)             \
    ((char *)(&(((parent_type *)0))->node_name))

/**
 * @brief 根据成员变量指针计算父结构体的指针
 *
 * 通过成员变量的指针和成员变量在父结构体中的偏移量，计算父结构体的指针。
 *
 * @param node        成员变量的指针
 * @param parent_type 父结构体的类型
 * @param node_name   成员变量的名称
 *

* @return 父结构体的指针
 */
#define noffset_to_parent(node, parent_type,node_name)        \
    (parent_type *)((char *)(node) - noffset_in_parent(parent_type, node_name))

/**
 * @brief 根据成员变量指针获取父结构体指针
 *
 * 通过成员变量的指针和成员变量在父结构体中的偏移量，获取父结构体的指针。如果成员变量指针为空，则返回空指针。
 *
 * @param node        成员变量的指针
 * @param parent_type 父结构体的类型
 * @param node_name   成员变量的名称
 *
 * @return 父结构体的指针，如果成员变量指针为空，则返回空指针
 */
#define nlist_entry(node,parent_type,node_name)     \
    ((node) ? noffset_to_parent((node),parent_type,node_name) : (parent_type *)0)

/**
 * @brief 遍历链表
 *
 * 遍历链表中的每个节点。
 *
 * @param node 当前节点指针
 * @param list 链表头指针
 */
#define nlist_foreach(node,list) for((node) = (list)->first;(node); (node) = (node)->next)

#endif //NLIST_H
