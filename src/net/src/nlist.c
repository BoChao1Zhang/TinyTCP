//
// Created by 32101 on 25-1-16.
//
#include "nlist.h"

void nlist_init(nlist_t *list) {
    list->first = list->last = (nlist_node_t *) 0;
    list->count = 0;
}


//考虑哪些节点存在，哪些节点不存在（注意不存在的情况）
void nlist_insert(nlist_t *list, nlist_node_t *pre, nlist_node_t *node) {
    if (nlist_is_empty(list) || !pre) {
        nlist_insert_first(list,node);
        return;
    }

    node->next = pre->next;
    node->pre = pre;
    if (pre->next) {
        pre->next->pre = node;
    }
    pre->next = node;


    if (list->last == pre) {
        list->last = node;
    }

    list->count ++;
}

void nlist_insert_first(nlist_t *list, nlist_node_t *node) {
    node->pre = (nlist_node_t *) 0;
    node->next = list->first;


    if (nlist_is_empty(list)) {
        list->first = list->last = node;
    } else {
        nlist_node_t *p = list->first;
        p->pre = node;
        list->first = node;
    }

    list->count++;
}

void nlist_insert_last(nlist_t *list, nlist_node_t *node) {
    node->pre = list->last;
    node->next = (nlist_node_t *)0;

    if (nlist_is_empty(list)) {
        list->first = list->last = node;
    } else {
        nlist_node_t* p = list->last;
        p->next = node;
        list->last = node;
    }

    list->count++;
}

nlist_node_t *nlist_remove(nlist_t *list, nlist_node_t *node) {
    if (node == list->first) {
        list->first = node->next;
    }
    if (node == list->last) {
        list->last = node->pre;
    }
    if (node->pre) {
        node->pre->next = node->next;
    }
    if (node->next) {
        node->next->pre = node->pre;
    }

    node->pre = node->next = (nlist_node_t *) 0;
    list->count--;

    return node;
}

nlist_node_t * nlist_remove_first(nlist_t *list) {
    nlist_node_t *first = nlist_first(list);
    if (first) {
        nlist_remove(list,first);
    }
    return first;
}

nlist_node_t * nlist_remove_last(nlist_t *list) {
    nlist_node_t *last = nlist_last(list);
    if (last) {
        nlist_remove(list,last);
    }

    return last;
}
