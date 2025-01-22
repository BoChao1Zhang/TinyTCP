//
// Created by 32101 on 25-1-20.
//

#include "pktbuf.h"
#include "dbg.h"
#include "mblock.h"
#include "nlocker.h"

static nlocker_t locker;
static mblock_t pktblk_mblock;
static pktblk_t block_buffer[PKTBUF_BLK_CNT];
static mblock_t pktbuf_mblock;
static pktbuf_t pktbuf_buffer[PKTBUF_BUF_CNT];


#if DBG_DISP_ENABLED(DBG_BUF)

static int total_blk_remain(pktbuf_t *buf) {
   return buf->total_size - buf->pos;
}

static int curr_blk_remain(pktbuf_t *buf) {
   pktblk_t *blk = buf->curr_blk;
     if (!blk) {
         return 0;
     }

    return (int)(blk->data + blk->size - buf->blk_offset);
}

static void display_check_buf(pktbuf_t *buf) {
    if (!buf) {
        dbg_error(DBG_BUF, "invalid buf, buffer=0\n");
        return;
    }

    plat_printf("check buf %p:size:%d\n", buf, buf->total_size);
    pktblk_t *cur;
    int index = 0, total_size = 0;
    for (cur = pktbuf_first_blk(buf); cur; cur = pktbuf_blk_next(cur)) {
        if ((cur->data < cur->payload) || (cur->data > cur->payload + PKTBUF_BLK_SIZE)) {
            dbg_error(DBG_BUF, "bad blk data pointer: %p", cur->data);
        }

        plat_printf("%d: ", index++);

        int pre_size = (int)(cur->data - cur->payload);
        plat_printf("pre: %d B, ", pre_size);

        int used_size = cur->size;
        plat_printf(" used: %d B", used_size);

        int free_size = cur_blk_tail_free(cur);
        plat_printf("free: %d B\n", free_size);

        int blk_total = pre_size + used_size + free_size;
        if (blk_total != PKTBUF_BLK_SIZE) {
            dbg_error(DBG_BUF, "bad block size： %d != %d\n", blk_total, PKTBUF_BLK_SIZE);
        }

        total_size += used_size;
    }

    if (total_size != buf->total_size) {
        dbg_error(DBG_BUF, "bad buf size: %d!= %d", total_size, buf->total_size);
    }
}
#else
#define display_check_buf(buf)
#endif


net_err_t pktbuf_init() {
    dbg_info(DBG_BUF, "pktbuf init\n");

    nlocker_init(&locker, NLOKCER_THREAD);
    mblock_init(&pktblk_mblock, block_buffer, sizeof(pktblk_t), PKTBUF_BLK_CNT, NLOKCER_THREAD);
    mblock_init(&pktbuf_mblock, pktbuf_buffer, sizeof(pktbuf_t), PKTBUF_BUF_CNT, NLOKCER_THREAD);


    dbg_info(DBG_BUF, "pktbuf init ok\n");
    return NET_ERR_OK;
}

static pktblk_t *pktblk_alloc(void) {
    pktblk_t *pktblk = mblock_alloc(&pktblk_mblock, -1);
    if (pktblk) {
        pktblk->size = 0;
        pktblk->data = (uint8_t *) 0;
        nlist_node_init(&pktblk->node);
    }
    return pktblk;
}

static pktblk_t *pktbuf_first_blk(pktbuf_t *buf) {
    nlist_node_t *first = nlist_first(&buf->blk_list);
    return nlist_entry(first, pktblk_t, node);
}

pktblk_t *pktbuf_last_blk(pktbuf_t *buf) {
    nlist_node_t *last = nlist_last(&buf->blk_list);
    return nlist_entry(last, pktblk_t, node);
}

static void pktblk_free(pktblk_t *block) {
    mblock_free(&pktblk_mblock, block);
}

void pktblk_free_list(pktblk_t *first_blk) {
    while (first_blk) {
        pktblk_t *next = pktbuf_blk_next(first_blk);
        pktblk_free(first_blk);
        first_blk = next;
    }
}

static pktblk_t *pktblk_alloc_list(int size, int add_front) {
    pktblk_t *first_block = (pktblk_t *) 0;
    pktblk_t *pre_blk = (pktblk_t *) 0;
    while (size) {
        pktblk_t *pktblk = pktblk_alloc();
        if (!pktblk) {
            dbg_error(DBG_BUF, "no free pktblk\n");

            pktblk_free_list(first_block);
            return (pktblk_t *) 0;
        }

        int cur_size = size > PKTBUF_BLK_SIZE ? PKTBUF_BLK_SIZE : size;

        if (add_front) {
            pktblk->size = cur_size;
            pktblk->data = pktblk->payload + PKTBUF_BLK_SIZE - cur_size;
            if (first_block) {
                nlist_node_set_next(&pktblk->node, &first_block->node);
            }
            first_block = pktblk;
        } else {
            if (!first_block) {
                first_block = pktblk;
            }
            pktblk->size = cur_size;
            pktblk->data = pktblk->payload;
            if (pre_blk) {
                nlist_node_set_next(&pre_blk->node, &pktblk->node);
            }
        }
        size -= cur_size;
        pre_blk = pktblk;
    }

    return first_block;
}

pktblk_t *pktbuf_blk_next(pktblk_t *pktblk) {
    nlist_node_t *next = nlist_node_next(&pktblk->node);
    return nlist_entry(next, pktblk_t, node);
}

static void pktbuf_insert_blk_list(pktbuf_t *buf, pktblk_t *first_blk, int add_last) {
    if (add_last) {
        while (first_blk) {
            pktblk_t *next_blk = pktbuf_blk_next(first_blk);

            nlist_insert_last(&buf->blk_list, &first_blk->node);
            buf->total_size += first_blk->size;
            first_blk = next_blk;
        }
    } else {
        pktblk_t *pre = (pktblk_t *) 0;
        while (first_blk) {
            pktblk_t *next = pktbuf_blk_next(first_blk);
            if (pre) {
                nlist_insert(&buf->blk_list, &pre->node, &first_blk->node);
            } else {
                nlist_insert_first(&buf->blk_list, &first_blk->node);
            }
            buf->total_size += first_blk->size;
            pre = first_blk;
            first_blk = next;
        };
    }
}

pktbuf_t *pktbuf_alloc(int size) {
    pktbuf_t *pktbuf = (pktbuf_t *) mblock_alloc(&pktbuf_mblock, -1);

    if (!pktbuf) {
        dbg_error(DBG_BUF, "no free buf\n");
        return (pktbuf_t *) 0;
    }

    pktbuf->total_size = 0;
    nlist_init(&pktbuf->blk_list);
    nlist_node_init(&pktbuf->node);

    if (size) {
        pktblk_t *block = pktblk_alloc_list(size, 1);
        if (!block) {
            mblock_free(&pktbuf_mblock, pktbuf);
            return (pktbuf_t *) 0;
        }

        pktbuf_insert_blk_list(pktbuf, block, 1);
    }
    pktbuf_rest_acc(pktbuf);
    display_check_buf(pktbuf);

    return pktbuf;
}


net_err_t pktbuf_add_header(pktbuf_t *buf, int size, int cont) {
    pktblk_t *block = pktbuf_first_blk(buf);


    int resv_size = (int) (block->data - block->payload);
    if (size <= resv_size) {
        block->size += size;
        block->data -= size;
        buf->total_size += size;

        display_check_buf(buf);
        return NET_ERR_OK;
    }

    if (cont) {
        if (size > PKTBUF_BLK_SIZE) {
            dbg_error(DBG_BUF, "size to big %d > %d", PKTBUF_BLK_SIZE, size);
            return NET_ERR_SIZE;
        }

        block = pktblk_alloc_list(size, 1);
        if (!block) {
            dbg_error(DBG_BUF, "no buffer (size %d)", size);
            return NET_ERR_NONE;
        }
    } else {
        block->data = block->payload;
        block->size += resv_size;
        buf->total_size += resv_size;
        size -= resv_size;

        block = pktblk_alloc_list(size, 1);
        if (!block) {
            dbg_error(DBG_BUF, "no buffer size:%d", size);
            return NET_ERR_NONE;
        }
    }

    pktbuf_insert_blk_list(buf, block, 0);
    display_check_buf(buf);

    return NET_ERR_OK;
}


net_err_t pktbuf_remove_header(pktbuf_t *buf, int size) {
    pktblk_t *block = pktbuf_first_blk(buf);
    while (size) {
        pktblk_t *next_blk = pktbuf_blk_next(block);
        if (size < block->size) {
            block->data += size;
            block->size -= size;
            buf->total_size -= size;
            break;
        }

        int cur_size = block->size;
        nlist_remove_first(&buf->blk_list);
        pktblk_free(block);

        size -= cur_size;
        buf->total_size -= cur_size;

        block = next_blk;
    }

    display_check_buf(buf);
    return NET_ERR_OK;
}

net_err_t pktbuf_resize(pktbuf_t *buf, int to_size) {
    if (to_size == buf->total_size) {
        return NET_ERR_OK;
    }

    if (buf->total_size == 0) {
        pktblk_t *blk = pktblk_alloc_list(to_size, 0);
        if (!blk) {
            dbg_error(DBG_BUF, "no block\n");
            return NET_ERR_MEM;
        }
    } else if (to_size == 0) {
        pktblk_free_list(pktbuf_first_blk(buf));
        buf->total_size = 0;
        nlist_init(&buf->blk_list);
    } else if (to_size >= buf->total_size) {
        pktblk_t *tail_blk = pktbuf_last_blk(buf);

        int inc_size = to_size - buf->total_size;
        int remain_size = cur_blk_tail_free(tail_blk);
        if (remain_size >= inc_size) {
            tail_blk->size += inc_size;
            buf->total_size += inc_size;
        } else {
            pktblk_t *new_blks = pktblk_alloc_list(inc_size - remain_size, 0);
            if (!new_blks) {
                dbg_error(DBG_BUF, "no block\n");
                return NET_ERR_MEM;
            }
            tail_blk->size += remain_size;
            buf->total_size += remain_size;
            pktbuf_insert_blk_list(buf, new_blks, 1);
        }
    } else {
        // 缩减尾部，整体变短
        int total_size = 0;

        // 遍历到达需要保留的最后一个缓存块
        pktblk_t* tail_blk;
        for (tail_blk = pktbuf_first_blk(buf); tail_blk; tail_blk = pktbuf_blk_next(tail_blk)) {
            total_size += tail_blk->size;
            if (total_size >= to_size) {
                break;
            }
        }

        if (tail_blk == (pktblk_t*)0) {
            return NET_ERR_SIZE;
        }

        // 减掉后续所有块链中的容量
        pktblk_t * curr_blk = pktbuf_blk_next(tail_blk);
        total_size = 0;
        while (curr_blk) {
            // 先取后续的结点
            pktblk_t * next_blk = pktbuf_blk_next(curr_blk);

            // 删除当前block
            nlist_remove(&buf->blk_list, &curr_blk->node);
            pktblk_free(curr_blk);

            total_size += curr_blk->size;
            curr_blk = next_blk;
        }

        // 调整tail_blk的大小
        tail_blk->size -= buf->total_size - total_size - to_size;
        buf->total_size = to_size;
    }

    display_check_buf(buf);
    return NET_ERR_OK;
}

net_err_t pktbuf_join(pktbuf_t *dest, pktbuf_t *src) {
    pktblk_t* first;
    while ((first = pktbuf_first_blk(src))) {
        nlist_remove_first(&src->blk_list);
        pktbuf_insert_blk_list(dest, first, 1);
    }
    pktbuf_free(src);
    display_check_buf(dest);
    return NET_ERR_OK;
}

net_err_t pktbuf_set_cont(pktbuf_t *buf, int size) {
    if (size > buf->total_size) {
        dbg_error(DBG_BUF, "size to big %d > %d\n", buf->total_size, size);
        return NET_ERR_SIZE;
    }

    if (size >PKTBUF_BLK_SIZE) {
        dbg_error(DBG_BUF, "size to big %d > %d\n", PKTBUF_BLK_SIZE, size);
        return NET_ERR_SIZE;
    }
    pktblk_t * first = pktbuf_first_blk(buf);
    if (size <first->size) {
        display_check_buf(buf);
        return NET_ERR_OK;
    }

    uint8_t* dest = first->payload;
    for (int i = 0;i<first->size;i++) {
        *dest++ = first->data[i];
    }
    first->data = first->payload;
    int remain_size = size - first->size;
    pktblk_t* curr = pktbuf_blk_next(first);
    while (remain_size) {
        int curr_size = curr->size > remain_size ? remain_size : curr->size;
        plat_memcpy(dest, curr->data, curr_size);
        dest += curr_size;
        first->size += curr_size;
        curr->size -= curr_size;
        curr->data += curr_size;

        remain_size -= curr_size;


        if (curr->size == 0) {
            pktblk_t* next = pktbuf_blk_next(curr);
            nlist_remove(&buf->blk_list, &curr->node);
            pktblk_free(curr);
            curr = next;
        }
    }
    display_check_buf(buf);
    return NET_ERR_OK;
}

net_err_t pktbuf_rest_acc(pktbuf_t *buf) {
    if (buf) {
        buf->pos = 0;
        buf->curr_blk = pktbuf_first_blk(buf);
        buf->blk_offset = buf->curr_blk? buf->curr_blk->data : 0;
    }

    return NET_ERR_OK;
}

//至多往前移动一个单位
static void move_forward(pktbuf_t* buf,int size) {
    buf->pos += size;
    buf->blk_offset += size;
    pktblk_t *curr = buf->curr_blk;
    if (buf->blk_offset >= curr->data + curr->size) {
        buf->curr_blk = pktbuf_blk_next(curr);
        if (buf->curr_blk) {
            buf->blk_offset = buf->curr_blk->data;
        } else {
            buf->blk_offset = (uint8_t *)0;
        }

    }
}


net_err_t pktbuf_write(pktbuf_t *buf, const uint8_t *data, int size) {
    if (!data || !size) {
        return NET_ERR_PARAM;
    }
    int remain_size = total_blk_remain(buf);
    if (remain_size < size) {
        dbg_error(DBG_BUF, "no space %d < %d\n", remain_size, size);
        return NET_ERR_SIZE;
    }

    while (size) {
        int curr_remain_size = curr_blk_remain(buf);
        int curr_copy = size > curr_remain_size ? curr_remain_size : size;

        plat_memcpy(buf->blk_offset, data, curr_copy);

        data += curr_copy;
        size -= curr_copy;

        move_forward(buf,curr_copy);
    }
    return NET_ERR_OK;
}

net_err_t pktbuf_read(pktbuf_t *buf, uint8_t *data, int size) {
    if (!data || !size) {
        return NET_ERR_PARAM;
    }
    int remain_size = total_blk_remain(buf);
    if (remain_size < size) {
        dbg_error(DBG_BUF, "no space to read %d < %d\n", remain_size, size);
        return NET_ERR_SIZE;
    }

    while (size) {
        int curr_remain_size = curr_blk_remain(buf);
        int curr_copy = size > curr_remain_size ? curr_remain_size : size;

        plat_memcpy(data, buf->blk_offset, curr_copy);

        data += curr_copy;
        size -= curr_copy;

        move_forward(buf,curr_copy);
    }

    return NET_ERR_OK;
}

net_err_t pktbuf_seek(pktbuf_t *buf, int offset) {
    if (offset == buf->pos) {
        return NET_ERR_OK;
    }
    if (offset < 0 || offset > buf->total_size) {
        dbg_error(DBG_BUF, "offset error %d\n", offset);
        return NET_ERR_SIZE;
    }
    int move_bytes;
    if (offset < buf->pos) {
        pktbuf_rest_acc(buf);
        move_bytes = offset;
    } else {
        move_bytes = offset - buf->pos;
    }

    while (move_bytes) {
        int curr_remain_size = curr_blk_remain(buf);
        int curr_move = move_bytes > curr_remain_size? curr_remain_size : move_bytes;
         move_forward(buf,curr_move);
        move_bytes -= curr_move;
    }

    return NET_ERR_OK;
}

net_err_t pktbuf_copy(pktbuf_t *dest, pktbuf_t *src, int size) {
    int dest_remain_size = total_blk_remain(dest);
    int src_remain_size = total_blk_remain(src);
    if (dest_remain_size < size || src_remain_size < size) {
        dbg_error(DBG_BUF, "no space to copy %d < %d\n", dest_remain_size, size);
        return NET_ERR_SIZE;
    }

    while (size) {
        int src_curr_remain_size = curr_blk_remain(src);
        int dest_curr_remain_size = curr_blk_remain(dest);
        int curr_copy = src_curr_remain_size > dest_curr_remain_size ? dest_curr_remain_size : src_curr_remain_size;
        curr_copy = size > curr_copy? curr_copy : size;
        plat_memcpy(dest->blk_offset, src->blk_offset, curr_copy);
        move_forward(dest,curr_copy);
        move_forward(src,curr_copy);
        size -= curr_copy;
    }
    return NET_ERR_OK;
}

net_err_t pktbuf_fill(pktbuf_t *buf, uint8_t v, int size) {

}


void pktbuf_free(pktbuf_t *pktbuf) {
    pktblk_free_list(pktbuf_first_blk(pktbuf));
    mblock_free(&pktbuf_mblock, pktbuf);
}
