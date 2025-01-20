//
// Created by 32101 on 25-1-20.
//

#ifndef PKTBUF_H
#define PKTBUF_H

#include "nlist.h"
#include "net_cfg.h"
#include "net_err.h"
#include <stdint.h>


typedef struct _pktblk_t {
    nlist_node_t node;
    int size;
    uint8_t *data;
    uint8_t payload[PKTBUF_BLK_SIZE];
} pktblk_t;

typedef struct _pktbuf_t {
    int total_size;
    nlist_t blk_list;
    nlist_node_t node;
} pktbuf_t;

//pkt buffer
net_err_t pktbuf_init();
pktbuf_t *pktbuf_alloc(int size);
void pktbuf_free(pktbuf_t *pktbuf);
net_err_t pktbuf_add_header(pktbuf_t* buf,int size,int cont);
net_err_t pktbuf_remove_header(pktbuf_t*buf,int size);
net_err_t pktbuf_resize(pktbuf_t* buf,int size);

//pkt block
pktblk_t *pktbuf_blk_next(pktblk_t *pktblk);

pktblk_t *pktbuf_first_blk(pktbuf_t *buf);

inline int cur_blk_tail_free(pktblk_t *blk) {
    return (blk->payload + PKTBUF_BLK_SIZE) - (blk->data + blk->size);
}
void pktblk_free_list(pktblk_t* first_blk);

#endif //PKTBUF_H
